/*	File:	hto.c		Hierarchical Tagged Objects library
 *	Author:	Douglas P. Dotson
 *	Date:	31 May 1993
 *	Edit:	16 Sep 1993	2002	DPD
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*#include <malloc.h>*/
#include <ctype.h>
#include "hto.h"

/* ------------------------ Grammar --------------------------*/
/*
 *	<data-file> ::= <object-list>
 *
 *	<object> ::= ( tag : <object-list> )
 *	<object> ::= ( tag : string )
 *
 *	<object-list> ::= <object> <object-list>
*/

/* Static functions in this module */

static	void		GetToken (HTO);
static	void		SortCallBackList (HTO);
static	int		SortCmp (const void *, const void *);
static	void		Object (HTO);
static	void		Object_List (HTO);
static	void		FindCallBack (HTO, char *, CallBackPtr *, CallBackPtr *);
static	int		FindCmp (const void *, const void *);
static	void		PushCallBack (HTO, CallBackPtr);
static	CallBackPtr	PopCallBack (HTO);
static	void		PushObjectValue (HTO, char *, char *);
static	void		PopObjectValues (HTO);
static	int		ValSrchCmp (const void *, const void *);
static	int		ValSortCmp (const void *, const void *);
static	void		InitHTODesc (HTO);

/********************************************************/
/*	Functions for processing existing HTO files.	*/
/********************************************************/

/*----------*/
HTO HTO_Open (char *FileName)	/* Open an HTO file */
{
    HTO		p;

    /* Allocate an HTO descriptor */

    p = (HTO) malloc (sizeof (htodesc));
    if (p == NULL)
	return NULL;

    /* Open the file containing the objects */

    if (strlen (FileName) != 0) {
	p->HTO_File = fopen (FileName, "r");
	if (p->HTO_File == NULL) {
	    free (p);
	    return NULL;
	}
    } else
	p->HTO_File = stdin;

    /* Initialize a few things */

    InitHTODesc (p);

    return p;
}

/*----------*/
void HTO_Close (HTO p)
{

    int	i, j;

    /* If the input file was completely processed successfully then */
    /* all the object value stacks should be empty.  If anything went */
    /* wrong they might not be empty.  Since each value pushed on */
    /* a stack was allocated from the free store, release any remaining */
    /* items on the stack while releasing the value lists. */

    for (i=0 ; i<p->Num_ValStacks ; i++) {
	for (j=0 ; j<NUM_LEVELS ; j++)
	    if ((*p->ValueStackList [i].VStack) [j] != NULL)
		free ((*p->ValueStackList [i].VStack) [j]);
	free (p->ValueStackList [i].VStack);
    }

    /* Close the data file and release the HTO descriptor. */

    fclose (p->HTO_File);
    free (p);
}

/*----------*/
void HTO_Process (HTO p)
{
    /* This function traverses the HTO tree and calls the specified */
    /* callback functions along the way. */

    /* Sort the callback list so that a binary search can be used */
    /* to perform lookups. */

    SortCallBackList (p);

    /* Get the first token */

    GetToken (p);

    /* Traverse the object tree */

    Object_List (p);
}

/*----------*/
static void Object (HTO p)
{
    CallBackPtr	Pre, Post;
    ObjectTag	Tag;

    /* Get the tag */

    GetToken (p);
    strcpy (Tag, p->Token_Buffer);

    /* Call the global pre-callback function */

    (*p->GlobalPreCallBack) (p, Tag);

    /* Get the tag-specific callback functions. Push the post-callback */
    /* function address and call the pre-callback function. */

    FindCallBack (p, Tag, &Pre, &Post);
    PushCallBack (p, Post);
    (*Pre) (p, Tag);

    /* Get the separator token */

    GetToken (p);

    /* If the next token is an object, then an object list is present. */
    /* Otherwise a value is present. */

    GetToken (p);
    if (p->CurrToken == TOKEN_OBJECT_START) {	/* Non-terminal object */

	/* Increment the object set level.  This is used for maintaining */
 	/* terminal object value scopes. */

	p->NestLevel++;

	/* Call the pre-callback function for this non-terminal object */

	(*p->NonTermPreCallBack) (p, Tag);

	/* Process the object list. */

	Object_List (p);

	/* Call the tag-specific post-callback function. */

    	(*PopCallBack (p)) (p, Tag);

	/* Call the post-callback function for this non-terminal object. */

	(*p->NonTermPostCallBack) (p, Tag);

	/* Pop any terminal object values that were defined within the */
	/* scope of this non-terminal object. */

	p->NestLevel--;
	PopObjectValues (p);

    } else {	/* Terminal object */

	/* Call the pre-callback function for this terminal object. */

	(*p->TermPreCallBack) (p, Tag);

	/* Push the value of this object onto the stack for the associated tag */

        PushObjectValue (p, Tag, p->Token_Buffer);

	/* Get the object terminator token. */

	GetToken (p);

	/* Call the tag-specific post-callback function. */

    	(*PopCallBack (p)) (p, Tag);

	/* Call the post-callback function for this terminal object. */

	(*p->TermPostCallBack) (p, Tag);

    }

    /* Get past the end of object token */

    GetToken (p);

    /* Call the global post-callback function */

    (*p->GlobalPostCallBack) (p, Tag);

}

/*----------*/
static void Object_List (HTO p)	/* Process a list of objects. */
{
    while (p->CurrToken == TOKEN_OBJECT_START)
	Object (p);
}

/*----------*/
static void GetToken (HTO p)	/* Get next token skipping whitespace */
{
    int		c, i, Escaped;

    /* Skip any leading blanks */

    do {
	c = getc (p->HTO_File);
    } while (isspace (c));

    /* Clear the string buffer */

    strcpy (p->Token_Buffer, "");

    /* Classify the next character */

    switch (c) {
	case OBJECT_START:	/* Start of object token */
	    p->CurrToken = TOKEN_OBJECT_START;
	    return;
	case OBJECT_END:	/* End of object token */
	    p->CurrToken = TOKEN_OBJECT_END;
	    return;
	case SEPARATOR:		/* Tag-Data separator */
	    p->CurrToken = TOKEN_SEPARATOR;
	    return;
	case EOF:
	    p->CurrToken = TOKEN_EOF;
	    return;
	default:		/* Must be a string */
	    p->Token_Buffer [0] = c;
	    if (c == '\\')
		p->Token_Buffer [0] = getc (p->HTO_File);
	    i = 1;
	    do {
		Escaped = 0;
		c = getc (p->HTO_File);
		if (c == '\\') {
		    c = getc (p->HTO_File);
		    Escaped = 1;
		}
		if (i < TOKEN_BUFFER_SIZE)
		    p->Token_Buffer [i++] = c;
	    } while (((c != SEPARATOR) && (c != OBJECT_END)) || Escaped);
	    ungetc (c, p->HTO_File);
	    p->Token_Buffer [i-1] = '\0';
	    p->CurrToken = TOKEN_STRING;
	    return;
    }
}

/*----------*/
void HTO_RegisterTagCallBack (HTO p, char *Tag, CallBackPtr Pre, CallBackPtr Post)
{
    /* Save the address of the tag-specific pre- and post-callback functions. */

    strcpy (p->CallBackList [p->Num_CallBacks].Tag, Tag);
    p->CallBackList [p->Num_CallBacks].PreFunction = Pre;
    p->CallBackList [p->Num_CallBacks].PostFunction = Post;
    p->Num_CallBacks++;
}

/*----------*/
void HTO_RegisterGlobalCallBack (HTO p, CallBackPtr Pre, CallBackPtr Post)
{
    /* Save the address of the pre- and post-callback function. */

    p->GlobalPreCallBack = Pre;
    p->GlobalPostCallBack = Post;
}

/*----------*/
void HTO_RegisterNonTermCallBack (HTO p, CallBackPtr Pre, CallBackPtr Post)
{
    /* Save the address of the non-terminal pre- and post-callback functions. */

    p->NonTermPreCallBack = Pre;
    p->NonTermPostCallBack = Post;
}

/*----------*/
void HTO_RegisterTermCallBack (HTO p, CallBackPtr Pre, CallBackPtr Post)
{
    /* Save the address of the terminal pre- and post-callback function. */

    p->TermPreCallBack = Pre;
    p->TermPostCallBack = Post;
}

/*----------*/
static void SortCallBackList (HTO p)
{
    /* Sort the callback tag-specific callback function list on tag. */
    /* This is so a binary search can be use to increade performance */
    /* during traversal. */

    qsort (p->CallBackList, p->Num_CallBacks, sizeof (CallBackEntry), SortCmp);
}

/*----------*/
static int SortCmp (const void *p, const void *q)
{
    /* This is the compare function used in SortCallBackList. */

    return strcmp (((CallBackEntry *)p)->Tag, ((CallBackEntry *)q)->Tag);
}

/*----------*/
static void FindCallBack (HTO p, char *Tag, CallBackPtr *Pre, CallBackPtr *Post)
{
    CallBackEntry	*q;

    /* Lookup the requested tag. */

    q = (CallBackEntry *) bsearch (Tag, p->CallBackList, p->Num_CallBacks,
		sizeof (CallBackEntry), FindCmp);
    /* If the tag was found then return the addresses of the pre- and */
    /* post-callback functions. If the tag is not found, return the */
    /* address of HTO_No_CallBack. */

    if (q) {
	*Pre = q->PreFunction;
	*Post = q->PostFunction;
    } else {
	*Pre = HTO_NoCallBack;
	*Post = HTO_NoCallBack;
    }
}

/*----------*/
static int FindCmp (const void *p, const void *q)
{
    /* This is compare function used by FindCallBack . */

    return strcmp ((char *)p, ((CallBackEntry *)q)->Tag);
}

/*----------*/
void HTO_NoCallBack (HTO p, char *Tag)
{
    /* This is just a stub used when no callback function is specified */
}

/*----------*/
static void PushCallBack (HTO p, CallBackPtr f)
{
    /* Push the address of the post-callback function onto the */
    /* callback stack. */

    p->CallBackStack [p->CallBackStackPtr++] = f;
}

/*----------*/
static CallBackPtr PopCallBack (HTO p)
{
    return p->CallBackStack [--p->CallBackStackPtr];
}

/*----------*/
static void PushObjectValue (HTO p, char *Tag, char *Value)
{
    VLEntry	*Ent;
    ValStack	*v;
    char	*s;

    /* This function pushes the value of a terminal object onto its */
    /* associated stack.  If this is the first occurrance of the object */
    /* then a new stack is created and initialized. */

    /* Determine if a value stack exists for the value's tag */

    Ent = (VLEntry *) bsearch (Tag, p->ValueStackList, p->Num_ValStacks,
		sizeof (VLEntry), ValSrchCmp);

    /* If this tag has not previously had a value, create a value stack */
    /* for it. */

    if (Ent == NULL) {

	/* Make an entry in the value stack list for this tag. Allocate */
	/* memory for the stack. */

	strcpy (p->ValueStackList [p->Num_ValStacks].Tag, Tag);
	v = (ValStack *) malloc (sizeof (ValStack));
	if (v == NULL) {
	    printf ("Out of memory!\n");
	    exit (1);
	}
	p->ValueStackList [p->Num_ValStacks].VStack = v;
	Ent = &p->ValueStackList [p->Num_ValStacks];
	p->Num_ValStacks++;

	/* The stack must be set to NULL pointers because all entries */
	/* under the current one may be referenced after the value goes */
	/* out of scope. Also, values are not replicated on the stack */
	/* as new scopes are entered.  A downward search of the stack is */
	/* performed when looking up a value. */

	memset (v, 0, sizeof (ValStack));

	/* Allocate memory for value and push on stack */

	s = (char *) malloc (strlen (Value) + 1);
	strcpy (s, Value);
	(*Ent->VStack) [p->NestLevel - 1] = s;

	/* Sort the list so binary searches continue to work. */

	qsort (p->ValueStackList, p->Num_ValStacks, sizeof (VLEntry),
			ValSortCmp);

	return;

    }

    s = (char *) malloc (strlen (Value) + 1);
    strcpy (s, Value);
    (*Ent->VStack) [p->NestLevel - 1] = s;

}

/*----------*/
static void PopObjectValues (HTO p)
{
    int	i;

    /* The nesting level has been reduced by 1.  Pop the stack */
    /* for all tags defined. Since all value are placed in dynamically */
    /* allocated memory, each poped value must be released back to the */
    /* free store. */

    for (i=0 ; i<p->Num_ValStacks ; i++) {
	if ((*p->ValueStackList [i].VStack) [p->NestLevel]) {
	    free ((*p->ValueStackList [i].VStack) [p->NestLevel]);
	    (*p->ValueStackList [i].VStack) [p->NestLevel] = NULL;
	}
    }
}

/*----------*/
char *HTO_GetObjectValue (HTO p, char *Tag, char *Value)
{
    VLEntry	*Ent;
    int		i;

    /* Determine if a value stack exists for this tag.  If not, place */
    /* an empty string in the value buffer and return a pointer to the */
    /* buffer. */

    Ent = (VLEntry *) bsearch (Tag, p->ValueStackList, p->Num_ValStacks,
		sizeof (VLEntry), ValSrchCmp);

    if (Ent == NULL) {
	strcpy (Value, "");
	return Value;
    }

    /* A stack does exist for this tag.  Search from the top of the stack */
    /* down until a pointer to a value is found. Copy the value into the */
    /* value buffer and return a pointer to the buffer. */


    i = p->NestLevel - 1;
    while (i >= 0) {
	if ((*Ent->VStack) [i]) {
	    strcpy (Value, (*Ent->VStack) [i]);
	    return Value;
	}
	i--;
    }

    /* The value is now out of scope.  Place an empty string in the value */
    /* buffer and return a pointer to the buffer. */

    strcpy (Value, "");
    return NULL;
}

/*----------*/
static int ValSrchCmp (const void *p, const void *q)
{
    /* This is the compare function used by PushObjectValue and */
    /* GetObjectValue. */

    return strcmp ((char *)p, ((VLEntry *)q)->Tag);
}

/*----------*/
static int ValSortCmp (const void *p, const void *q)
{
    /* This is the compare function used by PushObjectValue. */

    return strcmp (((VLEntry *)p)->Tag, ((VLEntry *)q)->Tag);
}

/************************************************/
/*	Functions for creating new HTO files.	*/
/************************************************/

/*----------*/
HTO HTO_Create (char *FileName)
{
    HTO		p;

    /* Allocate an HTO descriptor */

    p = (HTO) malloc (sizeof (htodesc));
    if (p == NULL)
	return NULL;

    /* Create the output file */

    if (strlen (FileName) != 0) {
	p->HTO_File = fopen (FileName, "w");
	if (p->HTO_File == NULL) {
	    free (p);
	    return NULL;
	}
    } else
	p->HTO_File = stdout;


    /* Initialize a few things */

    InitHTODesc (p);

    return p;
}

/*----------*/
static void InitHTODesc (HTO p)
{
    p->Num_CallBacks = 0;
    p->CallBackStackPtr = 0;
    p->GlobalPreCallBack = HTO_NoCallBack;
    p->GlobalPostCallBack = HTO_NoCallBack;
    p->NonTermPreCallBack = HTO_NoCallBack;
    p->NonTermPostCallBack = HTO_NoCallBack;
    p->TermPreCallBack = HTO_NoCallBack;
    p->TermPostCallBack = HTO_NoCallBack;
    p->NestLevel = 0;
    p->Num_ValStacks = 0;
}

/*----------*/
void HTO_BeginObject (HTO p, char *Tag)
{
    fprintf (p->HTO_File, "%c%s%c", OBJECT_START, Tag, SEPARATOR);
    p->NestLevel++;
}

/*----------*/
void HTO_EndObject (HTO p)
{
    fputc (OBJECT_END, p->HTO_File);
    p->NestLevel--;
}

/*----------*/
void HTO_WriteValue (HTO p, char *Tag, char *Value)
{
    if (strlen (Value) == 0)
	return;

    fprintf (p->HTO_File, "%c%s%c", OBJECT_START, Tag, SEPARATOR);
    while (*Value) {
	if ((*Value == OBJECT_END) || (*Value == SEPARATOR))
	    fputc ((int) '\\', p->HTO_File);
	fputc ((int) *Value, p->HTO_File);
	Value++;
    }
    fputc (OBJECT_END, p->HTO_File);
}

/*----------*/
void HTO_PutChar (HTO p, char c)
{
    fputc ((int) c, p->HTO_File);
}

/*----------*/
void HTO_PutStr (HTO p, char *s)
{
    fputs (s, p->HTO_File);
}

/*----------*/
void HTO_NewLine (HTO p)
{
    fputc ((int) '\n', p->HTO_File);
}

/*----------*/
void HTO_Finish (HTO p)
{
    while (p->NestLevel--)
	fputc (OBJECT_END, p->HTO_File);
    fclose (p->HTO_File);
    free (p);
}
