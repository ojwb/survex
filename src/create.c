/*	File:	create.c	Create a simple HTO file
/*	Author:	Douglas P. Dotson
/*	Date: 	26 Aug 1993
/*	Edit:	26 Aug 1993	2309	DPD
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hto.h"

HTO	q;

/*----------*/
main (int argc, char *argv[])
{

    /* Create the new HTO file */

    q = HTO_Create ("test.hto");
    if (q == NULL) {
	printf ("Unable to create %s\n", argv [2]);
	exit (1);
    }

    /* Create a simple hto */

    HTO_BeginObject (q, "a");
    HTO_WriteValue (q, "bob", "BobValue");
    HTO_WriteValue (q, "fred", "FredValue");
    HTO_BeginObject (q, "beth");
    HTO_WriteValue (q, "jessie", "JessieValue");
    HTO_WriteValue (q, "fred", "FredSecondValue");

    /* Finish up the HTO file.  All unterminated objects */
    /* will automatically be terminated. */

    HTO_Finish (q);

    return 0;
}
