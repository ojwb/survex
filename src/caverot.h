/* > caverot.h
 * Data structures and #defines for cave rotator
 * Copyright (C) 1993-1995,1997 Olly Betts
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

/* these are common to all systems: */
#define BIG_MAGNIFY_FACTOR     (1.1236f) /* SHIFT-ed zoom in/out factor */
#define LITTLE_MAGNIFY_FACTOR  (1.06f)   /* standard zoom in/out factor */

#include "whichos.h"
#include "useful.h"
#include <limits.h>

#if (OS!=UNIX)
#include "cvrotgfx.h"
#endif

/* avoid problems with eg cos(10) where cos prototype is double cos(); */
#define SIN(X) (sin((double)(X)))
#define COS(X) (cos((double)(X)))
#define SQRT(X) (sqrt((double)(X)))

/* SIND(A)/COSD(A) give sine/cosine of angle A degrees */
#define SIND(X) (sin(rad((double)(X))))
#define COSD(X) (cos(rad((double)(X))))

/* machine specific stuff */
#if (OS==RISCOS)
# include "armrot.h"
#elif ((OS==MSDOS) || (OS==TOS))
# include "dosrot.h"
#elif (OS==UNIX)
# include "xrot.h"
#else
# error Operating System not known
#endif

extern float scDefault;

extern bool fAllNames;

/* Data structures */
typedef struct {
   union {
      coord action; /* plot option for this point - draw/move/endofdata */
      char *str; /* string for a label */
   } _;
   coord X, Y, Z;  /* coordinates */
} point;

typedef struct LID {
   int datatype;
   point Huge *pData;
   struct LID Huge *next;
} lid; /* List Item Data */

#include "cvrotimg.h"

/* These are the datatype-s */
#define DATA_LEGS 0
#define DATA_STNS 1

#if 0
typedef struct {
   float nView_dir;               /* direction of view */
   float nView_dir_step;          /* step size of change in view */
   float nHeight;                 /* current height of view position */
   float nScale;                  /* current scale */
   coord iXcentr, iYcentr, iZcentr; /* centre of rotation of survey */
   bool  fRotating;               /* flag for rotating or not */
} view;
#endif

/* general plot (used if neither of the special cases applies */
void plot(point Huge *pData,
          coord x1, coord x2, coord y1, coord y2, coord y3, int fixpt);
void splot(point Huge *pData,
           coord x1, coord x2, coord y1, coord y2, coord y3, int fixpt);
void lplot(point Huge *pData,
           coord x1, coord x2, coord y1, coord y2, coord y3, int fixpt);

/* plot with viewheight=0 */
void plot_no_tilt(point Huge *pData,
                  coord x1, coord x2, coord y3, int fixpt);
void splot_no_tilt(point Huge *pData,
                   coord x1, coord x2, coord y3, int fixpt);
void lplot_no_tilt(point Huge *pData,
                   coord x1, coord x2, coord y3, int fixpt);

/* plot plan */
void plot_plan(point Huge *pData,
	       coord x1, coord x2, coord y1, coord y2, int fixpt);
void splot_plan(point Huge *pData,
                coord x1, coord x2, coord y1, coord y2, int fixpt);
void lplot_plan(point Huge *pData,
                coord x1, coord x2, coord y1, coord y2, int fixpt);

/* translate whole cave */
void do_translate(lid Huge *plid, coord dX, coord dY, coord dZ);
#if (OS==RISCOS)
#define do_translate_stns do_translate
#else
void do_translate_stns(lid Huge *plid, coord dX, coord dY, coord dZ);
#endif

extern int xcMac, ycMac; /* screen size in plot units (==pixels usually) */
extern float y_stretch; /* multiplier for y to correct aspect ratio */

/* plot text x chars *right* and y *down* */
extern void (text_xy)(int x, int y, const char *s);
