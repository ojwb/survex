/* dosrot.c
 * Survex cave rotator plot & translate routines for MS-DOS
 * Copyright (C) 1993-2001,2003 Olly Betts
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
# include <config.h>
#endif

#include "caverot.h"
#include "useful.h"
#include "labels.h"

/* Cross size */
#define CS 3

/* Use macros to simplify code  */
#define drawcross(x, y)\
 BLK(cvrotgfx_moveto((x) - CS, (y) - CS);\
     cvrotgfx_lineto((x) + CS, (y) + CS);\
     cvrotgfx_moveto((x) - CS, (y) + CS);\
     cvrotgfx_lineto((x) + CS, (y) - CS);)

#ifdef NO_FUNC_PTRS

/* really plebby version (needed if fn pointers won't fit in a coord) */
# define INIT() NOP /* do nothing */
# define COND(p) ((p)->_.action != STOP)

# define DO_PLOT(p, X, Y)\
 if ((p)->_.action == DRAW) cvrotgfx_lineto((X), (Y)); else cvrotgfx_moveto((X), (Y))

#elif defined(HAVE_SETJMP_H)

/* uses function pointers and setjmp (fastest for Borland C) */
# include <setjmp.h>

# define INIT() if (!setjmp(jbEnd)) NOP; else return /* store env; return after jmp */
# define COND(p) fTrue /* never exit loop by condition failing */
# define DO_PLOT(p, X, Y) (((void(*)(int, int))((p)->_.action))((X), (Y)))

jmp_buf jbEnd; /* store for environment for exiting plot loop */

extern void Far
dosrot_stop(int X, int Y)
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
   INIT();
   while (COND(p)) {
      /* calc positions and shift right get screen co-ords */
      int X = (int)((p->X * x1 + p->Y * x2) >> fixpt) + (xcMac >> 1);
      int Y = (int)((p->X * y1 + p->Y * y2 + p->Z * y3) >> fixpt) + (ycMac >> 1);
      DO_PLOT(p, X, Y);
      p++;
   }
}

/**************************************************************************/

/* plot elevation, with view height set at zero */
void
plot_no_tilt(point Huge *p, coord x1, coord x2, coord y3, int fixpt)
{
   INIT();
   while (COND(p)) {
      /* calc positions and shift right get screen co-ords */
      int X = (int)((p->X * x1 + p->Y * x2) >> fixpt) + (xcMac >> 1);
      int Y = (int)((p->Z * y3) >> fixpt) + (ycMac >> 1);
      DO_PLOT(p, X, Y);
      p++;
   }
}

/**************************************************************************/

/* plot plan only - ie Z co-ord and view height are totally ignored */
void
plot_plan(point Huge *p, coord x1, coord x2, coord y1, coord y2, int fixpt)
{
   INIT();
   while (COND(p)) {
      /* calc positions and shift right get screen co-ords */
      int X = (int)((p->X * x1 + p->Y * x2) >> fixpt) + (xcMac >> 1);
      int Y = (int)((p->X * y1 + p->Y * y2) >> fixpt) + (ycMac >> 1);
      DO_PLOT(p, X, Y);
      p++;
   }
}

/**************************************************************************/

void
do_translate(point Huge *p, coord dX, coord dY, coord dZ)
{
   while (p->_.action != STOP) {
      p->X += dX;
      p->Y += dY;
      p->Z += dZ;
      p++;
   }
}

void
do_translate_stns(point Huge *p, coord dX, coord dY, coord dZ)
{
   while (p->_.str) {
      p->X += dX;
      p->Y += dY;
      p->Z += dZ;
      p++;
   }
}

/* general plot, used when neither of the special cases below applies */
void
splot(point Huge *p, coord x1, coord x2, coord y1, coord y2, coord y3,
      int fixpt)
{
   while (p->_.str) {
      /* calc positions and shift right get screen co-ords */
      int X = (int)((p->X * x1 + p->Y * x2) >> fixpt) + (xcMac >> 1);
      int Y = (int)((p->X * y1 + p->Y * y2 + p->Z * y3) >> fixpt) + (ycMac >> 1);
      drawcross(X, Y);
      p++;
   }
}

/* plot elevation, with view height set at zero */
void
splot_no_tilt(point Huge *p, coord x1, coord x2, coord y3, int fixpt)
{
   while (p->_.str) {
      /* calc positions and shift right get screen co-ords */
      int X = (int)((p->X * x1 + p->Y * x2) >> fixpt) + (xcMac >> 1);
      int Y = (int)((p->Z * y3) >> fixpt) + (ycMac >> 1);
      drawcross(X, Y);
      p++;
   }
}

/* plot plan only - ie Z co-ord and view height are totally ignored */
void
splot_plan(point Huge *p, coord x1, coord x2, coord y1, coord y2, int fixpt)
{
   while (p->_.str) {
      /* calc positions and shift right get screen co-ords */
      int X = (int)((p->X * x1 + p->Y * x2) >> fixpt) + (xcMac >> 1);
      int Y = (int)((p->X * y1 + p->Y * y2) >> fixpt) + (ycMac >> 1);
      drawcross(X, Y);
      p++;
   }
}

/* general plot, used when neither of the special cases below applies */
void
lplot(point Huge *p, coord x1, coord x2, coord y1, coord y2, coord y3,
      int fixpt)
{
   while (p->_.str) {
      /* calc positions and shift right get screen co-ords */
      int X = (int)((p->X * x1 + p->Y * x2) >> fixpt);
      if ((X < 0 ? -X : X) <= (xcMac >> 1)) {
	 int Y = (int)((p->X * y1 + p->Y * y2 + p->Z * y3) >> fixpt);
	 if ((Y < 0 ? -Y : Y) <= (ycMac >> 1)) {
	    if (labels_plot(p->_.str, X, Y)) {
	       outtextxy(X + (unsigned)xcMac / 2u, Y + (unsigned)ycMac / 2u,
			 p->_.str);
	    }
	 }
      }
      p++;
   }
}

/* plot elevation, with view height set at zero */
void
lplot_no_tilt(point Huge *p, coord x1, coord x2, coord y3, int fixpt)
{
   while (p->_.str) {
      /* calc positions and shift right get screen co-ords */
      int X = (int)((p->X * x1 + p->Y * x2) >> fixpt);
      if ((X < 0 ? -X : X) <= (xcMac >> 1) ) {
	 int Y = (int)((p->Z * y3) >> fixpt);
	 if ((Y < 0 ? -Y : Y) <= (ycMac >> 1) ) {
	    if (labels_plot(p->_.str, X, Y)) {
	       outtextxy(X + (unsigned)xcMac / 2u, Y + (unsigned)ycMac / 2u,
			 p->_.str);
	    }
	 }
      }
      p++;
   }
}

/* plot plan only - ie Z co-ord and view height are totally ignored */
void
lplot_plan(point Huge *p, coord x1, coord x2, coord y1, coord y2, int fixpt)
{
   while (p->_.str) {
      /* calc positions and shift right get screen co-ords */
      int X = (int)((p->X * x1 + p->Y * x2) >> fixpt);
      if ((X < 0 ? -X : X) <= (xcMac >> 1)) {
	 int Y = (int)((p->X * y1 + p->Y * y2) >> fixpt);
	 if ((Y < 0 ? -Y : Y) <= (ycMac >> 1)) {
	    if (labels_plot(p->_.str, X, Y)) {
	       outtextxy(X + (unsigned)xcMac / 2u, Y + (unsigned)ycMac / 2u,
			 p->_.str);
	    }
	 }
      }
      p++;
   }
}
