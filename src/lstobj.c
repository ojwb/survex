/*	File:	lstobj.c	HTO Object Lister
/*	Author:	Douglas P. Dotson
/*	Date:	07 Jun 1993
/*	Edit:	08 Jun 1993	1113	DPD
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hto.h"
#include "getopt.h"

static	void	PostTermCallBack (HTO, char *);
static	void	PostNonTermCallBack (HTO, char *);

int	Terms = 1, NonTerms = 1;
FILE	*Out;
char	Match [TAG_SIZE+1];
int	DoMatch = 0, DoValue = 0;

/*----------*/
main (int argc, char **argv)
{
    HTO		p;
    char	InFile [100];
    int		c;

    /* Set default input and output files */

    strcpy (InFile, "");
    Out = stdout;

    /* Parse the command arguments */

    while ((c = getopt (argc, argv, "f:o:ntF:v")) != EOF) {
	switch (c) {
	    case 'f':
		strcpy (InFile, optarg);
		break;
	    case 'o':
		if ((Out = fopen (optarg, "w")) == NULL) {
		    fprintf (stderr, "Unable to create %d\n", optarg);
		    exit (1);
		}
		break;
	    case 'n':		/* List only non-terminal objects */
		Terms = 0;
		break;
	    case 't':		/* List only terminal objects */
		NonTerms = 0;
		break;
	    case 'F':		/* List a specific object */
		if (strlen (optarg) <= TAG_SIZE) {
		    strcpy (Match, optarg);
		    DoMatch = 1;
		} else {
		    fprintf (stderr, "Invalid object tag %s\n", optarg);
		    exit (1);
		}
		break;
	    case 'v':		/* List value with tag for -F */
		DoValue = 1;
		break;
	}
    }

    /* Open the existing HTO file */

    p = HTO_Open (InFile);
    if (p == NULL) {
	printf ("Unable to open %s\n", InFile);
	exit (1);
    }

    /* Register CallBack functions for terminal and non-terminal objects */

    HTO_RegisterNonTermCallBack (p, HTO_NoCallBack, PostNonTermCallBack);
    HTO_RegisterTermCallBack (p, HTO_NoCallBack, PostTermCallBack);

    HTO_Process (p);

    HTO_Close (p);
    fclose (Out);
    return 0;
}

/*----------*/
static void PostNonTermCallBack (HTO p, char *Tag)
{
    if (NonTerms)
	if (DoMatch) {
	    if (strcmp (Match, Tag) == 0)
		fprintf (Out, "%s\n", Tag);
	} else	
	    fprintf (Out, "%s\n", Tag);
}

/*----------*/
static void PostTermCallBack (HTO p, char *Tag)
{
    char	Value [100];

    if (Terms) {
	if (DoMatch) {
	    if (strcmp (Match, Tag) == 0)
		fprintf (Out, "%s", Tag);
	    else
		return;
	} else
	    fprintf (Out, "%s", Tag);
	if (DoValue) {
	    HTO_GetObjectValue (p, Tag, Value);
	    fprintf (Out, ":%s", Value);
	}
	fprintf (Out, "\n");
    }
}



