/* > cvrotimg.c
 * Reads a .3d image file into two linked lists of blocks, suitable for use
 * by caverot.c
 * Copyright (C) 1993-1997 Olly Betts
 */

/*
1993.08.02 split from caverot.c
1993.08.08 cvrot_img.c -> cvrotimg.c (MSDOS needs <= 8 char filenames)
1993.08.11 memory claiming is less crass now
1993.08.12 removed lots of dead code and tidied
1993.08.13 cPoints -> cMac
           fettled
1993.08.14 added bug-fix to avoid realloc problems
1993.11.03 changed error routines
1993.11.05 changed error numbers so we can use same error list as survex
           extracted all messages
1993.11.17 error 0->4
1993.11.18 malloc, realloc -> xosmalloc, xosrealloc
1993.11.29 reworked to eliminate error message 4
           error 5->11; 10->24
1993.11.30 sorted out error vs fatal
1993.12.05 size_t -> OSSIZE_T
1993.12.07 use of int corrected to OSSIZE_T
1993.12.14 (W) DOS point pointers made Huge to stop data wrapping at 64K
1993.12.16 added putnl() to neaten output
           improved out of memory error (gets printed now!)
1994.03.19 added crosses
1994.03.20 added img_error()
1994.03.22 commented out crosses for a bit
1994.03.23 cross back in to test under DOS
1994.03.26 put line data and crosses into separate (linked list) blocks
1994.03.27 changed my mind a bit
1994.03.29 ordering of blocks is no longer reversed (so we don't split line
           data)
1994.04.06 added a few Huge-s for BC
           (coord)-1 is terminator for station data
1994.05.13 one use of cLegMac corrected to cStnMac
           only last stn block was terminated correctly (ok for <2048 stns)
1994.06.20 factor made static
           added int argument to warning, error and fatal
1994.08.14 added comment after line which gives warning I can't suppress
1994.10.08 osnew() added
1995.01.26 allocate a large block for labels rather than malloc each
1995.03.24 fixed explicit cast warning
1997.02.24 converted to cvrotgfx
1997.05.29 tidied ; fixed load_data return value ; moved msg printing out
           altered point structure
1997.06.03 filename is now const char *
           quick fix for xcaverot
1997.07.16 hacks for const problems
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "useful.h"
#include "error.h"
#include "filelist.h"
#include "img.h"
#include "caverot.h"
#include "cvrotimg.h"
#if (OS!=UNIX)
# include "cvrotgfx.h"
#endif

/* Allocate a large block of this size for labels rather than malloc-ing
 * each individually so avoiding malloc space overhead.
 * If not defined, use osmalloc() */
#define STRINGBLOCKSIZE 8192

#define SHRINKAGE 1 /* shrink all data by this to fit in 16 bits */

/* scale all data by this to fit in coord data type */
/* Note: data in file in metres. 100.0 below stores to nearest cm */
static float factor = (float)(100.0/SHRINKAGE);

/* add elt and return new list head */
static lid Huge *AddLid( lid Huge *plidHead, point Huge *pData,
                         int datatype) {
   lid Huge *plidNew, Huge *plid;
   plidNew = osnew(lid);
   plidNew->pData = pData;
   plidNew->datatype = datatype;
#if 0
   /* for adding to start */
   plidNew->next  = plidHead;
   return;
#endif
   plidNew->next = NULL;
   if (plidHead == NULL) return plidNew; /* first elt, so it is the head */
   plid = plidHead;
   while (plid->next)
      plid = plid->next;
   plid->next = plidNew; /* otherwise add to end and return existing head */
   return plidHead;
}

extern bool load_data( const char *fnmData, lid Huge **ppLegs, lid Huge **ppStns ) {
   float x,y,z;
   char sz[256];
   int result;
   img *pimg;
   char *p;
#ifdef STRINGBLOCKSIZE
   char *pBlock = osmalloc(STRINGBLOCKSIZE);
   int left = STRINGBLOCKSIZE;
#endif

   point Huge *pLegData; /* pointer to start of current leg data block */
   point Huge *pStnData; /* pointer to start of current station data block */
   lid Huge *plidLegHead = NULL; /* head of leg data linked list */
   lid Huge *plidStnHead = NULL; /* head of leg data linked list */

   /* 0 <= c < cMac */
   OSSIZE_T cLeg, cLegMac; /* counter and ceiling for reading in leg data */
   OSSIZE_T cStn, cStnMac; /* counter and ceiling for reading in stn data */

   /* try to open image file, and check it has correct header */
   pimg = img_open( (char*)/*!HACK!*/fnmData, NULL, NULL );
   if (pimg == NULL) fatal( img_error(), wr, (char*)/*!HACK!*/fnmData, 0 );

   cLeg = 0;
   cLegMac = 2048; /* say */
   cStn = 0;
   cStnMac = 2048;

   /* get some memory to put the data in */
   /* blocks are: (all entries are coords)
    * <opt> <x> <y> <z>
    * [ and so on ... ]
    * <opt> <x> <y> <z>
    * <STOP>
    *
    * <opt> is <LINE> or <DATA> for leg data, or -> label for station data
    */
   pLegData = osmalloc( ossizeof(point) * (cLegMac + 1) );
   pStnData = osmalloc( ossizeof(point) * (cStnMac + 1) );

   do {
      result = img_read_datum( pimg, sz, &x, &y, &z );
      switch (result) {
       case img_BAD:
         break;
       case img_MOVE:
         pLegData[cLeg]._.action = (coord)MOVE;
    	 pLegData[cLeg].X = (coord)(x * factor);
    	 pLegData[cLeg].Y = (coord)(y * factor);
	 pLegData[cLeg].Z = (coord)(z * factor);
	 cLeg++;
    	 break;
       case img_LINE:
         pLegData[cLeg]._.action = (coord)DRAW;
    	 pLegData[cLeg].X = (coord)(x * factor);
    	 pLegData[cLeg].Y = (coord)(y * factor);
	 pLegData[cLeg].Z = (coord)(z * factor);
    	 cLeg++;
    	 break;
       case img_CROSS:
         /* Use labels to position crosses too - newer format .3d files don't
     	  * have crosses in */
    	 break;
       case img_LABEL: {
         int size = strlen(sz) + 1;
#ifdef STRINGBLOCKSIZE
         if (size > STRINGBLOCKSIZE)
      	    p = osmalloc(size);
    	 else {
      	    if (size > left) {
               /* !HACK! we waste the rest of the block
                * could realloc it down */
               pBlock = osmalloc(STRINGBLOCKSIZE);
               left = STRINGBLOCKSIZE;
      	    }
      	    p = pBlock;
      	    pBlock += size;
      	    left -= size;
    	 }
#else
         p = osmalloc(size);
#endif
         strcpy( p, sz );
    	 pStnData[cStn]._.str = p;
    	 pStnData[cStn].X = (coord)(x * factor);
   	 pStnData[cStn].Y = (coord)(y * factor);
    	 pStnData[cStn].Z = (coord)(z * factor);
    	 cStn++;
    	 break;
       }
      }
      if (cLeg >= cLegMac) {
         pLegData[cLeg]._.action = (coord)STOP;
         plidLegHead = AddLid( plidLegHead, pLegData, DATA_LEGS );
         pLegData = osmalloc( ossizeof(point) * (cLegMac + 1) );
         cLeg = 0;
      }
      if (cStn >= cStnMac) {
         pStnData[cStn]._.str = NULL;
         plidStnHead = AddLid( plidStnHead, pStnData, DATA_STNS );
         pStnData = osmalloc( ossizeof(point) * (cStnMac + 1) );
         cStn = 0;
      }
   } while (result!=img_BAD && result!=img_STOP);

   img_close(pimg);

   pLegData[cLeg]._.action = (coord)STOP;
   pStnData[cStn]._.str = NULL;

   *ppLegs = AddLid( plidLegHead, pLegData, DATA_LEGS );
   *ppStns = AddLid( plidStnHead, pStnData, DATA_STNS );

   return (result != img_BAD); /* return fTrue iff image was OK */
}
