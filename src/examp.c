/*	File:	examp.c		HTO Example Program
/*	Author:	Douglas P. Dotson
/*	Date:	26 Aug 1993
/*	Edit:	26 Aug 1993	2259	DPD
*/

/* This is a simple example of how to use the HTO access functions. */
/* It illustrates how to register callback functions for terminal */
/* tags, non-terminal tags, and specific tags.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* This hto.h file contains all the definitions necessary to use */
/* the HTO library. */

#include "hto.h"

/* Prototypes for non-terminal callback functions.  These will be */
/* called before and after a non-terminal has been visited during */
/* the traverse of the HTO. */

static	void	PreNonTermCallBack (HTO, char *);
static	void	PostNonTermCallBack (HTO, char *);

/* Prototypes for terminal callback functions.  These will be */
/* called before and after a terminal has been visited during */
/* the traverse of the HTO. Note that the value associated with */
/* a terminal is not available in the pre-callback for a terminal. */
/* The value of a terminal is only available in the post-callback */
/* function. */

static	void	PreTermCallBack (HTO, char *);
static	void	PostTermCallBack (HTO, char *);

/* Prototypes for the 'fred' tag callback functions.  These will be */
/* called before and after a 'fred' object has been visited during */
/* the traverse of the HTO. Note that the value associated with */
/* a 'fred' is not available in the pre-callback. */
/* The value of 'fred' is only available in the post-callback */
/* function. */

static	void	PreFredCallBack (HTO, char *);
static	void	PostFredCallBack (HTO, char *);

/*----------*/
main (int argc, char *argv[])
{
    HTO	p;

    /* Open the existing HTO file */

    p = HTO_Open (argv [1]);
    if (p == NULL) {
	printf ("Unable to open %s\n", argv [1]);
	exit (1);
    }

    /* Register CallBack functions for terminal and non-terminal objects */

    HTO_RegisterNonTermCallBack (p, PreNonTermCallBack, PostNonTermCallBack);
    HTO_RegisterTermCallBack (p, PreTermCallBack, PostTermCallBack);

    /* Register CallBack functions for the tag "fred" */

    HTO_RegisterTagCallBack (p, "fred", PreFredCallBack, PostFredCallBack);

    /* Traverse the HTO */

    HTO_Process (p);

    HTO_Close (p);

    return 0;
}

/*----------*/
static void PreNonTermCallBack (HTO p, char *Tag)
{
    printf ("In PreNonTermCallBack for %s\n", Tag);
}

/*----------*/
static void PostNonTermCallBack (HTO p, char *Tag)
{
    printf ("In PostNonTermCallBack for %s\n", Tag);
}

/*----------*/
static void PreTermCallBack (HTO p, char *Tag)
{
    char Value [50];

    HTO_GetObjectValue (p, Tag, Value);
    printf ("In PreTermCallBack for %s, value = %s\n", Tag, Value);
}

/*----------*/
static void PostTermCallBack (HTO p, char *Tag)
{
    char Value [50];

    HTO_GetObjectValue (p, Tag, Value);
    printf ("In PostTermCallBack for %s, value = %s\n", Tag, Value);
}

/*----------*/
static void PreFredCallBack (HTO p, char *Tag)
{
    char Value [50];

    HTO_GetObjectValue (p, Tag, Value);
    printf ("In PreFredCallBack for %s, value = %s\n", Tag, Value);
}

/*----------*/
static void PostFredCallBack (HTO p, char *Tag)
{
    char Value [50];

    HTO_GetObjectValue (p, Tag, Value);
    printf ("In PostFredCallBack for %s, value = %s\n", Tag, Value);
}

