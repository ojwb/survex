/* cvrotimg.c
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
static double factor = 100.0 / SHRINKAGE;

static coord move = 1, draw = 2, stop = 0;

extern void
set_codes(coord move_, coord draw_, coord stop_)
{
   move = move_;
   draw = draw_;
   stop = stop_;
}

static bool
add(point Huge *p, OSSIZE_T *c, OSSIZE_T tot, coord act, const img_point *pt)
{
   if (*c >= tot) return fFalse;
   p[*c]._.action = act;
   p[*c].X = (coord)(pt->x * factor);
   p[*c].Y = (coord)(pt->y * factor);
   p[*c].Z = (coord)(pt->z * factor);
   (*c)++;
   return fTrue;
}

extern bool
load_data(const char *fnmData, const char *survey,
	  point Huge **ppLegs, point Huge **ppSLegs, point Huge **ppStns)
{
   img_point mv, pt;
   int result;
   img *pimg;

   point Huge *legs;
   point Huge *slegs;
   point Huge *stns;

   OSSIZE_T c_legs = 0; /* actually the number of MOVEs and DRAWs */
   OSSIZE_T c_slegs = 0; /* actually the number of MOVEs and DRAWs */
   OSSIZE_T c_stns = 0;
   OSSIZE_T c_totlabel = 0;
   OSSIZE_T c_leg, c_sleg, c_stn;

   bool f_surf = fFalse, f_pendingmove = fFalse;

   char *p, *p_end;

   /* try to open image file, and check it has correct header */
   pimg = img_open_survey(fnmData, survey);
   if (!pimg) return fFalse;

   /* Make 2 passes - one to work out exactly how much space to allocate,
    * the second to actually read the data.  Disk caching should make this
    * fairly efficient and it means we can use every last scrap of memory
    */

   do {
      result = img_read_item(pimg, &pt);
      switch (result) {
       case img_LINE:
	 if (f_pendingmove) {
	    f_pendingmove = fFalse;
	    f_surf = (pimg->flags & img_FLAG_SURFACE);
	    if (f_surf) c_slegs += 2; else c_legs += 2;
	 } else {
	    if (f_surf) {
	       if (pimg->flags & img_FLAG_SURFACE) {
		  c_slegs++;
	       } else {
		  f_surf = fFalse;
		  c_legs += 2;
	       }
	    } else {
	       if (pimg->flags & img_FLAG_SURFACE) {
		  f_surf = fTrue;
		  c_slegs += 2;
	       } else {
		  c_legs++;
	       }
	    }
	 }
	 break;
       case img_MOVE:
	 f_pendingmove = fTrue;
	 break;
       case img_LABEL:
	 c_stns++;
	 c_totlabel += strlen(pimg->label) + 1;
	 break;
       case img_BAD:
	 img_close(pimg);
	 return fFalse;
      }
   } while (result != img_STOP);

   img_rewind(pimg);
   f_surf = fFalse;

   /* get some memory to put the data in */
   /* blocks are: (all entries are coords)
    * <opt> <x> <y> <z>
    * [ and so on ... ]
    * <opt> <x> <y> <z>
    * <STOP>
    *
    * <opt> is <LINE> or <DATA> for leg data, or -> label for station data
    */
   legs = osmalloc(ossizeof(point) * (c_legs + 1));
   stns = osmalloc(ossizeof(point) * (c_stns + 1));
   slegs = osmalloc(ossizeof(point) * (c_slegs + 1));
   p = osmalloc(c_totlabel);
   p_end = p + c_totlabel;

   c_leg = c_sleg = c_stn = 0;
#if 0
   printf("%p %p\n%d %d\n%d %d\n%d %d\n",
	  p, p_end, c_leg, c_legs, c_sleg, c_slegs, c_stn, c_stns);
#endif
   do {
      result = img_read_item(pimg, &pt);
      switch (result) {
       case img_LINE: {
	 bool fBad = fFalse;
	 if (f_pendingmove) {
	    f_pendingmove = fFalse;
	    f_surf = (pimg->flags & img_FLAG_SURFACE);
	    if (f_surf) {
	       fBad = (!add(slegs, &c_sleg, c_slegs, move, &mv) ||
		       !add(slegs, &c_sleg, c_slegs, draw, &pt));
	    } else {
	       fBad = (!add(legs, &c_leg, c_legs, move, &mv) ||
		       !add(legs, &c_leg, c_legs, draw, &pt));
	    }
	 } else {
	    if (f_surf) {
	       if (pimg->flags & img_FLAG_SURFACE) {
		  fBad = !add(slegs, &c_sleg, c_slegs, draw, &pt);
	       } else {
		  f_surf = fFalse;
		  fBad = (!add(legs, &c_leg, c_legs, move, &mv) ||
			  !add(legs, &c_leg, c_legs, draw, &pt));
	       }
	    } else {
	       if (pimg->flags & img_FLAG_SURFACE) {
		  f_surf = fTrue;
		  fBad = (!add(slegs, &c_sleg, c_slegs, move, &mv) ||
			  !add(slegs, &c_sleg, c_slegs, draw, &pt));
	       } else {
		  fBad = !add(legs, &c_leg, c_legs, draw, &pt);
	       }
	    }
	 }
	 if (fBad) return fFalse;
	 mv = pt;
	 break;
       }
       case img_MOVE:
	 f_pendingmove = fTrue;
	 mv = pt;
	 break;
       case img_LABEL:
	 if (p + strlen(pimg->label) > p_end || c_stn >= c_stns) return fFalse;
	 strcpy(p, pimg->label);
	 stns[c_stn]._.str = p;
	 p += strlen(pimg->label) + 1;
	 stns[c_stn].X = (coord)(pt.x * factor);
	 stns[c_stn].Y = (coord)(pt.y * factor);
	 stns[c_stn].Z = (coord)(pt.z * factor);
	 c_stn++;
	 break;
       case img_BAD:
	 img_close(pimg);
	 return fFalse;
      }
   } while (result != img_STOP);

   img_close(pimg);

#if 0
   printf("%p %p\n%d %d\n%d %d\n", p, p_end, c_leg, c_legs, c_stn, c_stns);
#endif
   if (p != p_end ||
       c_leg != c_legs ||
       c_sleg != c_slegs ||
       c_stn != c_stns) {
      return fFalse;
   }
   legs[c_leg]._.action = stop;
   slegs[c_sleg]._.action = stop;
   stns[c_stn]._.str = NULL;

   *ppLegs = legs;
   *ppSLegs = slegs;
   *ppStns = stns;

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

#define BIG_SCALE 1000.0

double
scale_to_screen(const point *pmin, const point *pmax, int xcMac, int ycMac,
		double y_stretch)
{
   double Radius; /* "radius" of plan */

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

   return min((double)xcMac, ycMac / fabs(y_stretch)) * .99 / (Radius * 2.0);
}
