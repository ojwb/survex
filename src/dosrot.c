/* > dosrot.c
 * Survex cave rotator plot & translate routines for MS-DOS
 * Also for Atari ST (unfinished)
 * Copyright (C) 1993-1997 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "caverot.h"
#include "useful.h"
#include "labels.h"

extern point Huge *pData;

/* Use macros to simplify code  */
#define drawcross(x, y)\
 BLK(cvrotgfx_moveto((x) - CS, (y) - CS); cvrotgfx_lineto((x) + CS, (y) + CS);\
     cvrotgfx_moveto((x) - CS, (y) + CS); cvrotgfx_lineto((x) + CS, (y) - CS);)

/* Cross size */
#define CS 3

#ifdef NO_FUNC_PTRS

/* really plebby version (needed if fn pointers won't fit in a coord) */
# define INIT() NOP /* do nothing */
# define COND(p) ((p)->_.action != STOP)

# define DO_PLOT(p, X, Y)\
 if ((p)->_.action == DRAW) cvrotgfx_lineto((X), (Y)); else cvrotgfx_moveto((X), (Y))

#elif defined(HAVE_SETJMP)

/* uses function pointers and setjmp (fastest for Borland C) */
# include <setjmp.h>

# define INIT() if (!setjmp(jbEnd)) NOP; else return /* store env; return after jmp */
# define COND(p) fTrue /* never exit loop by condition failing */
# define DO_PLOT(p, X, Y) (((void(*)(int, int))((p)->_.action))((X), (Y)))

jmp_buf jbEnd; /* store for environment for exiting plot loop */

extern void FAR
stop(int X, int Y)
{
   X = X; /* suppress compiler warnings */
   Y = Y;
   longjmp(jbEnd, 1); /* return to setjmp() and return 1 & so exit function */
}

#else

/* uses function pointers, but not setjmp */
# define INIT() NOP /* do nothing */
# define COND(p) ((p)->_.action != STOP)
# define DO_PLOT(p, X, Y) (((void(*)(int, int))((p)->_.action))((X), (Y)))

#endif

extern int xcMac, ycMac; /* defined in caverot.c */

/* general plot, used when neither of the special cases below applies */
void
plot(point Huge *p, coord x1, coord x2, coord y1, coord y2, coord y3,
     int fixpt)
{
   int X, Y; /* screen plot position */
   INIT();
   for ( ; COND(p); p++) {
      /* calc positions and shift right get screen co-ords */
      X = (int)((p->X * x1 + p->Y * x2) >> fixpt) + (xcMac >> 1);
      Y = (int)((p->X * y1 + p->Y * y2 + p->Z * y3) >> fixpt) + (ycMac >> 1);
      DO_PLOT(p, X, Y);
   }
}

/**************************************************************************/

/* plot elevation, with view height set at zero */
void
plot_no_tilt(point Huge *p, coord x1, coord x2, coord y3, int fixpt)
{
   int X, Y; /* screen plot position */
   INIT();
   for ( ; COND(p); p++) {
      /* calc positions and shift right get screen co-ords */
      X = (int)((p->X * x1 + p->Y * x2) >> fixpt) + (xcMac >> 1);
      Y = (int)((p->Z * y3) >> fixpt) + (ycMac >> 1);
      DO_PLOT(p, X, Y);
   }
}

/**************************************************************************/

/* plot plan only - ie Z co-ord and view height are totally ignored */
void
plot_plan(point Huge *p, coord x1, coord x2, coord y1, coord y2, int fixpt)
{
   int X, Y; /* screen plot position */
   INIT();
   for ( ; COND(p); p++) {
      /* calc positions and shift right get screen co-ords */
      X = (int)((p->X * x1 + p->Y * x2) >> fixpt) + (xcMac >> 1);
      Y = (int)((p->X * y1 + p->Y * y2) >> fixpt) + (ycMac >> 1);
      DO_PLOT(p, X, Y);
   }
}

/**************************************************************************/

void
do_translate(lid Huge *plid, coord dX, coord dY, coord dZ)
{
   point Huge *p;
   for ( ; plid; plid = plid->next) {
      p = plid->pData;
      for ( ; p->_.action != STOP; p++) {
      	 p->X += dX;
         p->Y += dY;
   	 p->Z += dZ;
      }
   }
}

void
do_translate_stns(lid Huge *plid, coord dX, coord dY, coord dZ)
{
   point Huge *p;
   for ( ; plid; plid = plid->next) {
      p = plid->pData;
      for ( ; p->_.str; p++) {
      	 p->X += dX;
         p->Y += dY;
   	 p->Z += dZ;
      }
   }
}

/* general plot, used when neither of the special cases below applies */
void
splot(point Huge *p, coord x1, coord x2, coord y1, coord y2, coord y3,
      int fixpt)
{
   int X, Y; /* screen plot position */
   for ( ; p->_.str; p++) {
      /* calc positions and shift right get screen co-ords */
      X = (int)((p->X * x1 + p->Y * x2) >> fixpt) + (xcMac >> 1);
      Y = (int)((p->X * y1 + p->Y * y2 + p->Z * y3) >> fixpt) + (ycMac >> 1);
      drawcross(X, Y);
   }
}

/* plot elevation, with view height set at zero */
void
splot_no_tilt(point Huge *p, coord x1, coord x2, coord y3, int fixpt)
{
   int X, Y; /* screen plot position */
   for ( ; p->_.str; p++) {
      /* calc positions and shift right get screen co-ords */
      X = (int)((p->X * x1 + p->Y * x2) >> fixpt) + (xcMac >> 1);
      Y = (int)((p->Z * y3) >> fixpt) + (ycMac >> 1);
      drawcross(X, Y);
   }
}

/* plot plan only - ie Z co-ord and view height are totally ignored */
void
splot_plan(point Huge *p, coord x1, coord x2, coord y1, coord y2, int fixpt)
{
   int X, Y; /* screen plot position */
   for ( ; p->_.str; p++) {
      /* calc positions and shift right get screen co-ords */
      X = (int)((p->X * x1 + p->Y * x2) >> fixpt) + (xcMac >> 1);
      Y = (int)((p->X * y1 + p->Y * y2) >> fixpt) + (ycMac >> 1);
      drawcross(X, Y);
   }
}

/* general plot, used when neither of the special cases below applies */
void
lplot(point Huge *p, coord x1, coord x2, coord y1, coord y2, coord y3,
      int fixpt)
{
   int X, Y; /* screen plot position */
   for ( ; p->_.str; p++) {
      /* calc positions and shift right get screen co-ords */
      X = (int)((p->X * x1 + p->Y * x2) >> fixpt);
      if ((X < 0 ? -X : X) > (xcMac >> 1)) continue;
      Y = (int)((p->X * y1 + p->Y * y2 + p->Z * y3) >> fixpt);
      if ((Y < 0 ? -Y : Y) > (ycMac >> 1)) continue;
      if (p->_.str) fancy_label(p->_.str, X, Y);
/*    outtextxy(X, Y, (char*)(p->Option)); */
   }
}

/* plot elevation, with view height set at zero */
void
lplot_no_tilt(point Huge *p, coord x1, coord x2, coord y3, int fixpt)
{
   int X, Y; /* screen plot position */
   for ( ; p->_.str; p++) {
      /* calc positions and shift right get screen co-ords */
      X = (int)((p->X * x1 + p->Y * x2) >> fixpt);
      if ((X < 0 ? -X : X) > (xcMac >> 1) ) continue;
      Y = (int)((p->Z * y3) >> fixpt);
      if ((Y < 0 ? -Y : Y) > (ycMac >> 1) ) continue;
      if (p->_.str) fancy_label(p->_.str, X, Y);
/*    outtextxy(X, Y, (char*)(p->Option)); */
   }
}

/* plot plan only - ie Z co-ord and view height are totally ignored */
void
lplot_plan(point Huge *p, coord x1, coord x2, coord y1, coord y2, int fixpt)
{
   int X, Y; /* screen plot position */
   for ( ; p->_.str; p++) {
      /* calc positions and shift right get screen co-ords */
      X = (int)((p->X * x1 + p->Y * x2) >> fixpt);
      if ((X < 0 ? -X : X) > (xcMac >> 1)) continue;
      Y = (int)((p->X * y1 + p->Y * y2) >> fixpt);
      if ((Y < 0 ? -Y : Y) > (ycMac >> 1)) continue;
      if (p->_.str) fancy_label((char*)(p->_.str), X, Y);
/*    outtextxy(X, Y, (char*)(p->Option)); */
   }
}
