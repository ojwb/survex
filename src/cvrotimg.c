/* > cvrotimg.c
 * Reads a .3d image file into two linked lists of blocks, suitable for use
 * by caverot.c
 * Copyright (C) 1993-1997 Olly Betts
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "useful.h"
#include "filename.h"
#include "message.h"
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
static lid Huge *
AddLid(lid Huge *plidHead, point Huge *pData, int datatype)
{
   lid Huge *plidNew, Huge *plid;
   plidNew = osnew(lid);
   plidNew->pData = pData;
   plidNew->datatype = datatype;
#if 0
   /* for adding to start */
   plidNew->next = plidHead;
   return;
#endif
   plidNew->next = NULL;
   if (!plidHead) return plidNew; /* first elt, so it is the head */
   plid = plidHead;
   while (plid->next) plid = plid->next;
   plid->next = plidNew; /* otherwise add to end and return existing head */
   return plidHead;
}

extern bool
load_data(const char *fnmData, lid Huge **ppLegs, lid Huge **ppStns)
{
   float x, y, z;
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
   pimg = img_open(fnmData, NULL, NULL);
   if (!pimg) fatalerror(img_error(), fnmData);

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
   pLegData = osmalloc(ossizeof(point) * (cLegMac + 1));
   pStnData = osmalloc(ossizeof(point) * (cStnMac + 1));

   do {
      result = img_read_datum(pimg, sz, &x, &y, &z);
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
         strcpy(p, sz);
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
         plidLegHead = AddLid(plidLegHead, pLegData, DATA_LEGS);
         pLegData = osmalloc(ossizeof(point) * (cLegMac + 1));
         cLeg = 0;
      }
      if (cStn >= cStnMac) {
         pStnData[cStn]._.str = NULL;
         plidStnHead = AddLid(plidStnHead, pStnData, DATA_STNS);
         pStnData = osmalloc(ossizeof(point) * (cStnMac + 1));
         cStn = 0;
      }
   } while (result != img_BAD && result != img_STOP);

   img_close(pimg);

   pLegData[cLeg]._.action = (coord)STOP;
   pStnData[cStn]._.str = NULL;

   *ppLegs = AddLid(plidLegHead, pLegData, DATA_LEGS);
   *ppStns = AddLid(plidStnHead, pStnData, DATA_STNS);

   return (result != img_BAD); /* return fTrue iff image was OK */
}
