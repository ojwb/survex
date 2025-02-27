/* hpgl.cc
 * Export from Aven as HPGL.
 */
/* Copyright (C) 1993-2024 Olly Betts
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <config.h>

#include <stdio.h>

#include "hpgl.h"
#include "export.h" // For SURF, etc
#include "useful.h"

# define HPGL_USE_UC
/*# define HPGL_USE_SR */ /* for text sized relative to page size */

# define HPGL_EOL "\003" /* terminates labelling commands: LB<string>\003 */

# ifndef HPGL_USE_UC
#  define HPGL_SO "\016" /* shift in & shift out of extended character set */
#  define HPGL_SI "\017"
# endif

# define HPGL_CROSS_SIZE 28 /* length of cross arms (in HPGL units) */

static bool fNewLines = true;

/* Check if this line intersects the current page */
/* Initialise HPGL routines. */
void HPGL::header(const char *, time_t,
		  double, double, double, double, double, double)
{
   pen = 1;

   /* INitialise; Select Pen 1;  */
   fputs("IN;SP1;"
#ifndef HPGL_USE_UC
	 "CA-1;GM0,800;" /* Char set Alternate -1; Get Memory; */
#endif
#ifdef HPGL_USE_SR
	 // SR scales characters relative to P1 and P2.
	 // (0.5,1.0) is 2/3 of the default size.
	 "SR0.5,1.0;"
#else
	 // SI scales characters to size given (in cm).
	 "SI0.125,.179;"
#endif
	 , fh);
   if (fNewLines) PUTC('\n', fh);

#ifndef HPGL_USE_UC
   /* define degree and copyright symbols */
   fputs("DL32,10,30,12,30,13,29,13,27,12,26,10,26,9,27,9,29,"
	 "10,30;DL40,0,0;", fh); /* Hope this works! Seems to for BP */
   if (fNewLines) PUTC('\n', fh);

   fputs("DL67,16,14,16,18,17,22,19,25,22,28,26,30,31,31,37,32,"
	 "43,32,49,31,53,30,58,28,61,25,63,22,64,18,64,14,63,10,"
	 "61,7,58,4,53,2,49,1,43,0,37,0,31,1,26,2,22,4,19,7,17,10,"
	 "16,14;", fh);
   if (fNewLines) PUTC('\n', fh);

   fputs("DL41,4,20,3,19,0,23,-4,24,-9,24,-14,23,-17,22,-20,19,"
	 "-21,16,-20,13,-17,10,-14,9,-9,8,-4,8,0,9,3,11,4,12;", fh);
   if (fNewLines) PUTC('\n', fh);
#endif
#if 0
   // FIXME: This was needed when printhpgl supported splitting a plot over
   // multiple pages, but is it useful now we leave that to the OS printer
   // drivers?
   int PaperWidth, PaperDepth; // In mm
   xpPageWidth = (long)(HPGL_UNITS_PER_MM * (double)PaperWidth);
   ypPageDepth = (long)(HPGL_UNITS_PER_MM * (double)PaperDepth);

   /* and set clipping (Input Window!) on plotter (left,bottom,right,top) */
   fprintf(fh, "IW%ld,%ld,%ld,%ld;", clip.x_min, clip.y_min,
	   clip.x_min + xpPageWidth, clip.y_min + ypPageDepth);
#endif
}

void
HPGL::line(const img_point *p1, const img_point *p, unsigned flags, bool fPending)
{
    // Pens 1 to 6 are apparently supported by HPGL.
    enum { PEN_LEG = 1, PEN_SPLAY = 2, PEN_SURF = 3 };
    int new_pen = PEN_LEG;
    if (flags & SURF) {
	new_pen = PEN_SURF;
    } else if (flags & SPLAYS) {
	new_pen = PEN_SPLAY;
    }
    if (new_pen != pen) {
	fprintf(fh, "SP%d;", new_pen);
	pen = new_pen;
    }
    if (fPending) {
	fprintf(fh, "PU%ld,%ld;", long(p1->x * factor), long(p1->y * factor));
    }
    fprintf(fh, "PD%ld,%ld;", long(p->x * factor), long(p->y * factor));
}

#define CS HPGL_CROSS_SIZE
#define CS2 (2 * HPGL_CROSS_SIZE)
void
HPGL::cross(const img_point *p, const wxString&, int)
{
    if (pen != 1) {
	fprintf(fh, "SP1;");
	pen = 1;
    }
    fprintf(fh, "PU%ld,%ld;", long(p->x * factor), long(p->y * factor));
    /* SM plots a symbol at each point, but it isn't very convenient here   */
    /* We can write PDPR%d,%dPR%d,%d... but the HP7475A manual doesn't say  */
    /* clearly if this will work on older plotters (such as the HP9872)     */
    fprintf(fh, "PD;PR%d,%d;PR%d,%d;PU%d,0;PD%d,%d;PU%d,%d;PA;",
	    CS, CS,  -CS2, -CS2,  CS2, /*0,*/  -CS2, CS2,  CS, -CS);
    if (fNewLines) PUTC('\n', fh);
}
#undef CS
#undef CS2

void
HPGL::label(const img_point *p, const wxString& str, int /*sflags*/, int)
{
    if (pen != 1) {
	fprintf(fh, "SP1;");
	pen = 1;
    }
    const char* s = str.utf8_str();
    /* LB is a text label, terminated with a ^C */
    fprintf(fh, "PU%ld,%ld;LB", long(p->x * factor), long(p->y * factor));
    while (*s) {
	switch (*s) {
	    case '\xB0':
#ifdef HPGL_USE_UC
		/* draw a degree sign */
		fputs(HPGL_EOL ";UC1.25,7.5,99,.25,0,.125,-.25,0,-.5,"
		      "-.125,-.25,-.25,0,-.125,.25,0,.5,.125,.25;LB", fh);
#else
		/* KLUDGE: this prints the degree sign if the plotter supports
		 * extended chars or a space if not, since we tried to redefine
		 * space.  Nifty, eh? */
		fputs(HPGL_SO " " HPGL_SI, fh);
#endif
		break;
	    case '\xA9':
#ifdef HPGL_USE_UC
		/* (C) needs two chars to look right! */
		/* This bit does the circle of the (C) symbol: */
		fputs(HPGL_EOL ";", fh);
		if (fNewLines) PUTC('\n', fh);
		fputs("UC2,3.5,99,0,1,0.125,1,0.25,.75,0.375,.75,"
		      ".5,.5,.625,.25,.75,.25,.75,0,.75,-.25,.625,-.25,"
		      ".5,-.5,.375,-.75,.25,-.75,.125,-1,0,-1,-0.125,-1,"
		      "-0.25,-.75,-0.375,-.75,-.5,-.5,-.625,-.25,-.75,-.25,"
		      "-.75,0,-.75,.25,-.625,.25,-.5,.5,-.375,.75,-.25,.75,"
		      "-.125,1;", fh);
		if (fNewLines) PUTC('\n', fh);
		/* And this bit's the c in the middle: */
		fputs("UC.5,5,99,-.125,.25,-.375,.5,-.5,.25,-.625,0,"
		      "-.625,-.25,-.375,-.25,-.375,-.75,-.125,-.75,.125,-.75,"
		      ".375,-.75,.375,-.25,.625,-.25,.625,0,.5,.25,.375,.5,"
		      ".125,.25;", fh);
		if (fNewLines) PUTC('\n', fh);
		fputs("LB", fh);
#else
		fputs(HPGL_SO "(C)" HPGL_SI, fh);
#endif
		break;
	    default:
		PUTC(*s, fh);
	}
	s++;
    }
    fputs(HPGL_EOL ";", fh);
    if (fNewLines) PUTC('\n', fh);
}

void
HPGL::footer()
{
   /* Clear clipping window;  New page.  NB PG is a no-op on the HP7475A */
   fputs("IW;PG;", fh);
   if (fNewLines) PUTC('\n', fh);
}
