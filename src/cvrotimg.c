/* > cvrotimg.c
 * Reads a .3d image file into two linked lists of blocks, suitable for use
 * by caverot.c
 * Copyright (C) 1993-2001 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include "cvrotimg.h"
#if (OS != UNIX)
# include "cvrotgfx.h"
#endif

coord Xorg, Yorg, Zorg; /* position of centre of survey */
coord Xrad, Yrad, Zrad; /* "radii" */

#define SHRINKAGE 1 /* shrink all data by this to fit in 16 bits */

/* scale all data by this to fit in coord data type */
/* Note: data in file in metres. 100.0 below stores to nearest cm */
static float factor = (float)(100.0 / SHRINKAGE);

static coord move = 1, draw = 2, stop = 0;

extern void
set_codes(coord move_, coord draw_, coord stop_)
{
   move = move_;
   draw = draw_;
   stop = stop_;
}

extern bool
load_data(const char *fnmData, point Huge **ppLegs, point Huge **ppStns)
{
   img_point pt;
   char sz[256];
   int result;
   img *pimg;

   point Huge *pLegData;
   point Huge *pStnData;

   OSSIZE_T c_legs = 0; /* actually the number of MOVEs and DRAWs */
   OSSIZE_T c_stns = 0;
   OSSIZE_T c_totlabel = 0;
   OSSIZE_T c_leg, c_stn;
   char *p, *p_end;

   /* Make 2 passes - one to work out exactly how much space to allocate,
    * the second to actually read the data.  Disk caching should make this
    * fairly efficient and it means we can use every last scrap of memory
    */

   /* try to open image file, and check it has correct header */
   pimg = img_open(fnmData, NULL, NULL);
   if (!pimg) return fFalse;

   do {
      result = img_read_item(pimg, sz, &p);
      switch (result) {
       case img_MOVE:
       case img_LINE:
    	 c_legs++;
    	 break;
       case img_CROSS:
         /* Use labels to position crosses too - newer format .3d files don't
     	  * have crosses in */
    	 break;
       case img_LABEL:
	 c_stns++;
	 c_totlabel += strlen(sz) + 1;
	 break;
       case img_BAD:
	 img_close(pimg);
	 return fFalse;
      }
   } while (result != img_STOP);

   img_rewind(pimg);

   /* get some memory to put the data in */
   /* blocks are: (all entries are coords)
    * <opt> <x> <y> <z>
    * [ and so on ... ]
    * <opt> <x> <y> <z>
    * <STOP>
    *
    * <opt> is <LINE> or <DATA> for leg data, or -> label for station data
    */
   pLegData = osmalloc(ossizeof(point) * c_legs + 1);
   pStnData = osmalloc(ossizeof(point) * c_stns + 1);
   p = osmalloc(c_totlabel);
   p_end = p + c_totlabel;

   c_leg = c_stn = 0;
#if 0
   printf("%p %p\n%d %d\n%d %d\n", p, p_end, c_leg, c_legs, c_stn, c_stns);
#endif
   do {
      result = img_read_item(pimg, sz, &p);
      switch (result) {
       case img_MOVE:
	 if (c_leg >= c_legs) return fFalse;
         pLegData[c_leg]._.action = move;
    	 pLegData[c_leg].X = (coord)(pt.x * factor);
    	 pLegData[c_leg].Y = (coord)(pt.y * factor);
	 pLegData[c_leg].Z = (coord)(pt.z * factor);
	 c_leg++;
    	 break;
       case img_LINE:
	 if (c_leg >= c_legs) return fFalse;
         pLegData[c_leg]._.action = draw;
    	 pLegData[c_leg].X = (coord)(pt.x * factor);
    	 pLegData[c_leg].Y = (coord)(pt.y * factor);
	 pLegData[c_leg].Z = (coord)(pt.z * factor);
    	 c_leg++;
    	 break;
       case img_CROSS:
         /* Use labels to position crosses too - newer format .3d files don't
     	  * have crosses in */
    	 break;
       case img_LABEL: {
	 int size = strlen(sz) + 1;
	 if (p + size > p_end) return fFalse;
	 if (c_stn >= c_stns) return fFalse;
	 strcpy(p, sz);
    	 pStnData[c_stn]._.str = p;
    	 pStnData[c_stn].X = (coord)(pt.x * factor);
   	 pStnData[c_stn].Y = (coord)(pt.y * factor);
    	 pStnData[c_stn].Z = (coord)(pt.z * factor);
    	 c_stn++;
	 p += size;
	 break;
       }
       case img_BAD:
	 img_close(pimg);
	 return fFalse;
      }
   } while (result != img_STOP);

   img_close(pimg);

#if 0
   printf("%p %p\n%d %d\n%d %d\n", p, p_end, c_leg, c_legs, c_stn, c_stns);
#endif
   if (p != p_end) return fFalse;
   if (c_leg != c_legs) return fFalse;
   pLegData[c_leg]._.action = stop;
   if (c_stn != c_stns) return fFalse;
   pStnData[c_stn]._.str = NULL;

   *ppLegs = pLegData;
   *ppStns = pStnData;

   return fTrue; /* return fTrue iff image was OK */
}

void
reset_limits(point *pmin, point *pmax)
{
   pmin->X = pmin->Y = pmin->Z = 0x7fffffff;
   pmax->X = pmax->Y = pmax->Z = 0x80000000;
}

void
update_limits(point *pmin, point *pmax, point Huge *pLegs, point Huge *pStns)
{
   /* run through data to find max & min coords */
   if (pLegs) {
      point Huge *p;
      for (p = pLegs ; p->_.action != stop; p++) {
	 if (p->X < pmin->X) pmin->X = p->X;
	 if (p->X > pmax->X) pmax->X = p->X;
	 if (p->Y < pmin->Y) pmin->Y = p->Y;
	 if (p->Y > pmax->Y) pmax->Y = p->Y;
	 if (p->Z < pmin->Z) pmin->Z = p->Z;
	 if (p->Z > pmax->Z) pmax->Z = p->Z;
      }
   }
   if (pStns) {
      point Huge *p;
      for (p = pStns ; p->_.str; p++) {
	 if (p->X < pmin->X) pmin->X = p->X;
	 if (p->X > pmax->X) pmax->X = p->X;
	 if (p->Y < pmin->Y) pmin->Y = p->Y;
	 if (p->Y > pmax->Y) pmax->Y = p->Y;
	 if (p->Z < pmin->Z) pmin->Z = p->Z;
	 if (p->Z > pmax->Z) pmax->Z = p->Z;
      }
   }
   return;
}

#define BIG_SCALE 1e3f

float
scale_to_screen(const point *pmin, const point *pmax, int xcMac, int ycMac,
		double y_stretch)
{
   float Radius; /* "radius" of plan */

   if (pmin->X > pmax->X) {
      /* no data */
      Xorg = Yorg = Zorg = 0;
      Xrad = Yrad = Zrad = 0;
   } else {
      /* centre survey in each (spatial) dimension */
      Xorg = (pmin->X + pmax->X) / 2;
      Yorg = (pmin->Y + pmax->Y) / 2;
      Zorg = (pmin->Z + pmax->Z) / 2;
      Xrad = (pmax->X - pmin->X) / 2;
      Yrad = (pmax->Y - pmin->Y) / 2;
      Zrad = (pmax->Z - pmin->Z) / 2;
   }

   Radius = radius(Xrad, Yrad);

   if (Zrad > Radius) Radius = Zrad;

   if (Radius == 0) return (BIG_SCALE);

   return min((float)xcMac, (float)ycMac / (float)fabs(y_stretch)) * .99
	  / (Radius * 2);
}
