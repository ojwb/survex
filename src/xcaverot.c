/* > xcaverot.c
 * Copyright (C) 1993-2001 Bill Purvis, Olly Betts, John Pybus, Mark Shinwell,
 * Leandro Dybal Bertoni, Andy Holtsbery, et al
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
 *
 *
 * Original license on code Copyright 1993 Bill Purvis:
 *
 * Bill Purvis DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * Bill Purvis BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#define XCAVEROT_BUTTONS

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <sys/time.h> /* for gettimeofday */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <X11/extensions/Xdbe.h> /* for double buffering support */

#include "cmdline.h"
#include "useful.h"
#include "filename.h"
#include "message.h"
#include "cvrotimg.h"

#define STOP 0
#define MOVE 1
#define DRAW 2

/* Width of each button along the top of the window */
#define BUTWIDTH 60
#define BUTHEIGHT 25

/* length of cross station marker */
#define CROSSLENGTH 3

/* Width and height of compass and elevation indicator windows */
#define FONTSPACE 20
#define INDWIDTH ((int)(indrad * 2.5))
#define INDDEPTH (INDWIDTH + FONTSPACE)

#define C_IND_ANG 25
#define E_IND_ANG 11
#define E_IND_LEN 0.8
#define E_IND_LINE 0.6
#define E_IND_PTR 0.38

/* default radius of compass and clino indicators - 20 is good for smaller
 * screens */
static double indrad = 40;

/* default font to use */
#define FONTNAME "-adobe-helvetica-medium-r-normal-*-8-*"

/* values of plan_elev */
#define PLAN 1
#define ELEVATION 2

/* number of depth colour bands */
#define NUM_DEPTH_COLOURS 10

/* nasty fixed limit - checked at least */
#define SEGMENT_GROUP_SIZE 40000

/* A group of segments to be drawn in the same colour. */
typedef struct {
   int num_segments;
   XSegment segments[SEGMENT_GROUP_SIZE];
} SegmentGroup;

static SegmentGroup segment_groups[NUM_DEPTH_COLOURS + 1];

/* use double buffering in the rendering if server supports it */
static int have_double_buffering;
static XdbeBackBuffer backbuf;
static XdbeSwapInfo backswapinfo;
static XVisualInfo *visinfo;
static XSetWindowAttributes dbwinattr;

/* scale all data by this to fit in coord data type */
/* Note: data in file in metres. 100.0 below stores to nearest cm */
static double datafactor = 100.0;

static double scale_default;

static double scale; /* factor to scale view on screen by */
static double zoomfactor = 1.2;
static double sbar; /* length of scale bar */

/* position of original click in pointer drags */
static struct {
   int x;
   int y;
} orig;

static double view_angle = 0.0;	/* bearing up page */

static double elev_angle = 0.0;
static double rot_speed; /* rotation speed degrees per second */

static int plan_elev;
static int rot = 0;
static int crossing = 0;
static int labelling = 0;
static int legs = 1;
static int surf = 0;
static int allnames = 0;

static struct timeval lastframe;

static int xoff, yoff;	/* offsets at which survey is plotted in window */
static coord x_mid;
static coord y_mid;
static coord z_mid;
static double z_col_scale;

static Display *mydisplay;
static Window mywindow;

#ifdef XCAVEROT_BUTTONS
static Window butzoom, butmooz, butload, butrot, butstep, butquit;
static Window butplan, butlabel, butcross;
#if 0 /* unused */
static Window butselect;
#endif
#endif
static Window ind_com, ind_elev, scalebar;
static GC slab_gc;
static int oldwidth, oldheight;	/* to adjust scale on window resize */
static Region label_reg;	/* used to implement non-overlapping labels */

static XFontStruct *fontinfo;	/* info on the font used in labeling */
static char hello[] = "XCaverot";

static unsigned long black, white;

static int lab_col_ind = 19;	/* Hack to get labels in sensible colour JPNP */
static int fontheight, slashheight;	/* JPNP */

static void update_rotation(void);
static void fill_segment_cache(void);

static const char *ncolors[] = { "black",
#if 0
   "BlanchedAlmond", "BlueViolet",
   "CadetBlue1", "CornflowerBlue", "DarkGoldenrod", "DarkGreen",
   "DarkKhaki", "DarkOliveGreen1", "DarkOrange", "DarkOrchid",
#else
   "Orange", "BlueViolet",
   "violet", "purple", "blue", "cyan",
   "green", "yellowgreen", "yellow", "orange", "orange red", "red",
#endif
   "DarkSalmon", "DarkSeaGreen", "DarkSlateBlue", "DarkSlateGray",
   "DarkViolet", "DeepPink", "DeepSkyBlue", "DodgerBlue",
   "ForestGreen", "GreenYellow",
#if 0
   "HotPink", "IndianRed", "LawnGreen",
#else
   "red", "green", "blue",
#endif
   "LemonChiffon", "LightBlue", "LightCoral",
   "LightGoldenrod", "LightGoldenrodYellow", "LightGray", "LightPink",
   "LightSalmon", "LightSeaGreen", "LightSkyBlue", "LightSlateBlue",
   "LightSlateGray", "LightSteelBlue", "LimeGreen",
   "MediumAquamarine", "MediumOrchid", "MediumPurple", "MediumSeaGreen",
   "MediumSlateBlue", "MediumSpringGreen", "MediumTurquoise",
      "MediumVioletRed",
   "MidnightBlue", "MistyRose", "NavajoWhite",
   "NavyBlue", "OldLace", "OliveDrab", "OrangeRed1",
   "PaleGoldenrod", "PaleGreen", "PaleTurquoise", "PaleVioletRed",
   "PapayaWhip", "PeachPuff", "PowderBlue", "RosyBrown",
   "RoyalBlue", "SandyBrown", "SeaGreen1", "SkyBlue",
   "SlateBlue", "SlateGray", "SpringGreen", "SteelBlue",
   "VioletRed", "WhiteSmoke", "aquamarine", "azure",
   "beige", "bisque", "blue", "brown",
   "burlywood", "chartreuse1", "chocolate",
   "coral", "cornsilk", "cyan", "firebrick",
   "gainsboro", "gold1", "goldenrod", "green1",
   "honeydew", "ivory", "khaki", "lavender",
   "linen", "magenta", "maroon", "moccasin",
   "orange", "orchid", "pink", "plum",
   "purple", "red1", "salmon", "seashell",
   "sienna", "snow", "tan", "thistle",
   "tomato1", "turquoise", "violet", "wheat",
   "gray12", "gray24", "gray36",
   "gray48", "gray60", "gray73", "gray82", "gray97", NULL
};

static void switch_to_plan(void);
static void switch_to_elevation(void);

static Colormap color_map;
static XColor colors[128];
static XColor exact_colors[128];
#if 0 /* unused */
static char *surveys[128];
static char surveymask[128];
static int numsurvey = 0;
#endif
static GC gcs[128];

static unsigned int drag_button = AnyButton; /* AnyButton => no drag */
static double elev_angle_orig;
static double x_mid_orig, y_mid_orig, z_mid_orig;	/* original posns b4 drag */
static double view_angle_orig;
static double scale_orig; /* saved state of scale before drag */

static double sv, cv, se, ce;

/* Create a set of colors from named colors */

static void
color_set_up(Display * display, Window window)
{
   int i;
   XGCValues vals;

   /* color_map = DefaultColormap(display,0); */

   /* get the colors from the color map for the colors named */
   for (i = 0; ncolors[i]; i++) {
      XAllocNamedColor(display, color_map, ncolors[i],
		       &exact_colors[i], &colors[i]);
      vals.foreground = colors[i].pixel;
      gcs[i] = XCreateGC(display, window, GCForeground, &vals);
   }
#if 0 /* unused */
   numsurvey = 0;
#endif
}

static void
flip_button(Display * display, Window button, GC normalgc, GC inversegc,
	    const char *string)
{
   int len = strlen(string);
   int width;
   int offset;

   XFontStruct * fs;
   fs = XQueryFont(display, XGContextFromGC(inversegc));
   width = XTextWidth(fs, string, len);
   XFreeFontInfo(NULL, fs, 1);
   offset = (BUTWIDTH - width) / 2;
   if (offset < 0) offset = 0;
   /* for old behaviour, offset = BUTWIDTH/4 */

   XClearWindow(display, button);
   XFillRectangle(display, button, normalgc, 0, 0, BUTWIDTH, BUTHEIGHT);
   XDrawImageString(display, button, inversegc, offset, 20, string, len);
   XFlush(display);
}

#if 0
/* unused */
int
findsurvey(const char *name)
{
   int i;

   for (i = 0; i < numsurvey; i++)
      if (strcmp(surveys[i], name) == 0)
	 return i;
   i = numsurvey;
   if (ncolors[i + 1])
      numsurvey++;
   surveys[i] = (char *)osmalloc(strlen(name) + 1);
   strcpy(surveys[i], name);
   surveymask[i] = 1;
   return i;
}
#endif

static point **ppLegs = NULL;
static point **ppSLegs = NULL;
static point **ppStns = NULL;

static int
load_file(const char *name, int replace)
{
   XWindowAttributes attr;
   int blocks = 0;
   if (!replace && ppLegs) {
      while (ppLegs[blocks]) blocks++;
   }
   ppLegs = osrealloc(ppLegs, (blocks + 2) * sizeof(point Huge *));
   ppSLegs = osrealloc(ppSLegs, (blocks + 2) * sizeof(point Huge *));
   ppStns = osrealloc(ppStns, (blocks + 2) * sizeof(point Huge *));

   /* load data into memory */
   if (!load_data(name, ppLegs + blocks, ppSLegs + blocks, ppStns + blocks)) {
      ppLegs[blocks] = ppSLegs[blocks] = ppStns[blocks] = NULL;
      return 0;
   }
   ppLegs[blocks + 1] = ppSLegs[blocks + 1] = ppStns[blocks + 1] = NULL;

   XGetWindowAttributes(mydisplay, mywindow, &attr);
   
   {
      int c;
      point lower, upper;
       
      reset_limits(&lower, &upper);
      
      for (c = 0; ppLegs[c]; c++) {
	 update_limits(&lower, &upper, ppLegs[c], ppStns[c]);
	 update_limits(&lower, &upper, ppSLegs[c], NULL);
      }

      scale_default = scale_to_screen(&lower, &upper,
				      attr.width, attr.height, 1.0);
   }
   scale = scale_default;
   x_mid = Xorg;
   y_mid = Yorg;
   z_mid = Zorg;

   z_col_scale = ((double)(NUM_DEPTH_COLOURS - 1)) / (2 * Zrad);

   return 1;
}

static void
process_load(Display * display, Window mainwin, Window button, GC mygc, GC egc)
{
   Window enter_window;
   XEvent enter_event;

/*  Font enter_font; */
   GC enter_gc;
   KeySym key;
   char string[128];
   char text[10];
   int count;

   flip_button(display, button, mygc, egc, "Load");
#if 0 /* unused */
   numsurvey = 0;
#endif

   /* set up input window */
   enter_window = XCreateSimpleWindow(display, mainwin,
				      340, 200, 300, 100, 3, white, black);
   enter_gc = mygc;
   XMapRaised(display, enter_window);
   XClearWindow(display, enter_window);
   XSelectInput(display, enter_window, ButtonPressMask | KeyPressMask);

   while (1) {
      /* reset everything */
      XDrawString(display, enter_window, mygc, 10, 50, "Enter file name", 15);
      XFlush(display);
      string[0] = '\0';
      key = 0;

      /* accept input from the user */
      while (1) {
	 XNextEvent(display, &enter_event);
	 if (enter_event.type == KeyPress) {
	     count = XLookupString((XKeyEvent *) & enter_event, text, 10,
				   &key, 0);
	     if (key == XK_Return) break;
	     text[count] = '\0';
	     strcat(string, text);
	     XClearWindow(display, enter_window);
	     XDrawString(display, enter_window, mygc,
			 10, 50, "Enter file name", 15);
	     XFlush(display);
	     XDrawString(display, enter_window, enter_gc,
			 100, 75, string, strlen(string));
	     XFlush(display);
	 }
      }

      if (load_file(string, 1)) break;

      strcpy(string, "File not found or not a Survex image file");
      XClearWindow(display, enter_window);
      XDrawString(display, enter_window, enter_gc,
		  10, 25, string, strlen(string));
   }

   XDestroyWindow(display, enter_window);

   flip_button(display, button, egc, mygc, "Load");
}

static void
process_plan(Display * display, Window button, GC mygc, GC egc)
{
   flip_button(display, button, mygc, egc,
	       plan_elev == PLAN ? "Plan" : "Elev");
   if (plan_elev == PLAN) {
      switch_to_elevation();
   } else {
      switch_to_plan();
   }
   flip_button(display, button, egc, mygc,
	       plan_elev == PLAN ? "Plan" : "Elev");
}

static void
process_label(Display * display, Window button, GC mygc, GC egc)
{
   flip_button(display, button, mygc, egc, labelling ? "No Label" : "Label");
   labelling = !labelling;
   flip_button(display, button, egc, mygc, labelling ? "No Label" : "Label");
}

static void
process_cross(Display * display, Window button, GC mygc, GC egc)
{
   flip_button(display, button, mygc, egc, crossing ? "No Cross" : "Cross");
   crossing = !crossing;
   flip_button(display, button, egc, mygc, crossing ? "No Cross" : "Cross");
}

static void
process_rot(Display * display, Window button, GC mygc, GC egc)
{
   flip_button(display, button, mygc, egc, rot ? "Stop" : "Rotate");
   rot = !rot;
   gettimeofday(&lastframe, NULL);
   flip_button(display, button, egc, mygc, rot ? "Stop" : "Rotate");
}

static void
process_step(Display * display, Window button, GC mygc, GC egc)
{
   flip_button(display, button, mygc, egc, "Step");
   view_angle += rot_speed / 5;
   update_rotation();
   flip_button(display, button, egc, mygc, "Step");
}

static void
draw_scalebar(int changedscale, GC scale_gc)
{
   char temp[20];
   double l, m, n, o;

   if (changedscale) {
      XWindowAttributes a;

      XGetWindowAttributes(mydisplay, scalebar, &a);

      l = (a.height - 4) / (datafactor * scale);

      m = log10(l);

      n = pow(10, floor(m));	/* Ouch did I really do all this ;-) JPNP */

      o = (m - floor(m) < log10(5) ? n : 5 * n);

      sbar = (int)o;

      XClearWindow(mydisplay, scalebar);
   }

   XDrawLine(mydisplay, scalebar, scale_gc, 13, 2, 13,
	     2 + scale * datafactor * sbar);
   if (sbar<1000)
     sprintf(temp, "%d m", (int)sbar);
   else
     sprintf(temp, "%d km", (int)sbar/1000);
#ifdef XCAVEROT_BUTTONS
   XDrawString(mydisplay, mywindow, slab_gc, 8, BUTHEIGHT+5+FONTSPACE, temp, strlen(temp));
#else
   XDrawString(mydisplay, mywindow, slab_gc, 8, FONTSPACE, temp, strlen(temp));
#endif
}

static void
process_zoom(Display * display, Window button, GC mygc, GC egc)
{
   flip_button(display, button, mygc, egc, "Zoom in");
   scale *= zoomfactor;
   flip_button(display, button, egc, mygc, "Zoom in");
}

static void
process_mooz(Display * display, Window button, GC mygc, GC egc)
{
   flip_button(display, button, mygc, egc, "Zoom out");
   scale /= zoomfactor;
   flip_button(display, button, egc, mygc, "Zoom out");
}

#if 0 /* unused */
static void
process_select(Display * display, Window window, Window button, GC mygc,
	       GC egc)
{
   Window select;
   XEvent event;
   int n, wid, ht, x, y, i;

   flip_button(display, window, button, mygc, egc, "Select");
   for (n = 1; n * n * 5 < numsurvey; n++);
   ht = n * 5 * 20;
   wid = n * 130;
   select = XCreateSimpleWindow(display, window,
				0, 30, wid, ht, 3, black, white);
   XMapRaised(display, select);

   XSelectInput(display, select, ButtonPressMask);
   for (;;) {
      XClearWindow(display, select);
      x = 0;
      y = 0;
      for (i = 0; i < numsurvey; i++) {
	 XFillRectangle(display, select, gcs[i], x, y, 10, 10);
	 XDrawRectangle(display, select, mygc, x + 12, y, 10, 10);
	 if (surveymask[i]) {
	    XDrawLine(display, select, mygc, x + 12, y, x + 21, y + 9);
	    XDrawLine(display, select, mygc, x + 12, y + 9, x + 21, y);
	 }
	 XDrawString(display, select, mygc, x + 25, y + 10,
		     surveys[i], strlen(surveys[i]));
	 /* XDrawString(display, select, mygc, x+100, y+10,
	  * ncolors[i], strlen(ncolors[i])); */
	 y += 20;
	 if (y >= ht) {
	    y = 0;
	    x += 130;
	 }
      }
      XNextEvent(display, &event);
      if (event.type == ButtonPress) {
	 XButtonEvent *e = (XButtonEvent *) & event;

	 x = e->x;
	 y = e->y;
	 /*printf("select: x=%d, y=%d\n", x, y); */
	 i = 0;
	 while (x > 130) {
	    x -= 130;
	    i += n * 5;
	    /*printf("adjust: x=%d, i=%d\n", x, i); */
	 }
	 i += (y / 20);
	 /*printf("final i=%d\n", i); */
	 if (i >= numsurvey)
	    break;
	 if (x >= 12 && x < 22)
	    surveymask[i] = 1 - surveymask[i];
      }
   }
   XDestroyWindow(display, select);
   flip_button(display, window, button, egc, mygc, "Select");
}
#endif

static void
draw_ind_elev(Display * display, GC gc, double angle)
{
   char temp[32];
   int xm, ym;
   double sa, ca;
   double r = INDWIDTH * E_IND_LINE / 2;
   double q = INDWIDTH * E_IND_PTR / 2;

   xm = ym = INDWIDTH / 2;

   sa = sin(rad(angle));
   ca = cos(rad(angle));

/*printf("%f\n",ca);*/

   XClearWindow(mydisplay, ind_elev);

   XDrawLine(display, ind_elev, gc, xm - r, ym, xm + r, ym);

   XDrawLine(display, ind_elev, gc, xm - q, ym, xm - q + 2 * q * (ca + 0.001),
	     ym - 2 * q * sa);
   XDrawLine(display, ind_elev, gc, xm - q, ym,
	     xm - q + E_IND_LEN * q * cos(rad(angle + E_IND_ANG)),
	     ym - E_IND_LEN * q * sin(rad(angle + E_IND_ANG)));
   XDrawLine(display, ind_elev, gc, xm - q, ym,
	     xm - q + E_IND_LEN * q * cos(rad(angle - E_IND_ANG)),
	     ym - E_IND_LEN * q * sin(rad(angle - E_IND_ANG)));

   sprintf(temp, "%d", -(int)angle);
   XDrawString(display, ind_elev, gc, INDWIDTH / 2 - 20, INDDEPTH - 10, temp,
	       strlen(temp));
}

static void
draw_ind_com(Display * display, GC gc, double angle)
{
   char temp[32];
   int xm, ym;
   double sa, ca;
   double rs, rc, r = indrad;

   xm = ym = INDWIDTH / 2;

   rs = r * sin(rad(C_IND_ANG));
   rc = r * cos(rad(C_IND_ANG));
   sa = -sin(rad(angle));
   ca = cos(rad(angle));

   XClearWindow(mydisplay, ind_com);

   XDrawArc(display, ind_com, gc, xm - (int)r, ym - (int)r, 2 * (int)r,
	    2 * (int)r, 0, 360 * 64);

   XDrawLine(display, ind_com, gc, xm + r * sa, ym - r * ca,
	     xm - rs * ca - rc * sa, ym - rs * sa + rc * ca);
   XDrawLine(display, ind_com, gc, xm + r * sa, ym - r * ca,
	     xm + rs * ca - rc * sa, ym + rs * sa + rc * ca);
   XDrawLine(display, ind_com, gc, xm - rs * ca - rc * sa,
	     ym - rs * sa + rc * ca, xm - r * sa / 2, ym + r * ca / 2);
   XDrawLine(display, ind_com, gc, xm + rs * ca - rc * sa,
	     ym + rs * sa + rc * ca, xm - r * sa / 2, ym + r * ca / 2);
   XDrawLine(display, ind_com, gc, xm, ym - r / 2, xm, ym - r);

   sprintf(temp, "%03d", (int)angle);
   XDrawString(display, ind_com, gc, INDWIDTH / 2 - 20, INDDEPTH - 10, temp,
	       strlen(temp));
}

static void
draw_label(Display * display, Window window, GC gc, int x, int y,
	   const char *string, int length)
{
   if (!allnames) {
      int strwidth;
      XRectangle r;

      if (x >= oldwidth || y >= oldheight || y + fontheight <= 0) return;

      strwidth = XTextWidth(fontinfo, string, length);

      if (x + strwidth <= 0) return;

      if (XRectInRegion(label_reg, x, y, strwidth, fontheight) != RectangleOut)
	 return;
      
      r.x = x;
      r.y = y;
      r.width = strwidth;
      r.height = fontheight;
      XUnionRectWithRegion(&r, label_reg, label_reg);
   }

   XDrawString(display, window, gc, x, y, string, length);
}

static int
toscreen_x(point * p)
{
   return xoff - ((p->X - x_mid) * cv + (p->Y - y_mid) * sv) * scale;
}

static int
toscreen_y(point * p)
{
   int y;

   switch (plan_elev) {
   case PLAN:
      y = (p->X - x_mid) * sv - (p->Y - y_mid) * cv;
   case ELEVATION:
      y = p->Z - z_mid;
   default:
      y = ((p->X - x_mid) * sv - (p->Y - y_mid) * cv) * se
	  + (p->Z - z_mid) * ce;
   }
   return yoff - y * scale;
}

static void
add_to_segment_cache(point **pp, int f_surface)
{
   coord x1, y1;
   x1 = y1 = 0;	/* avoid compiler warning */
   for ( ; *pp; pp++) {
      point *p;
      for (p = *pp; p->_.action != STOP; p++) {
	 switch (p->_.action) {
	  case MOVE:
	    x1 = toscreen_x(p);
	    y1 = toscreen_y(p);
	    break;
	    
	  case DRAW: {
	    SegmentGroup *group;

	    if (f_surface) {
	       /* put all surface legs in one group */
	       group = &segment_groups[NUM_DEPTH_COLOURS];
	    } else {
	       /* calculate depth band */
	       int depth = (int)((double)(-p->Z + Zorg + Zrad) * z_col_scale);
	       if (depth < 0) {
		  depth = 0;
	       } else if (depth >= NUM_DEPTH_COLOURS) {
		  depth = NUM_DEPTH_COLOURS - 1;
	       }
	       group = &segment_groups[depth];
	    }

	    if (group->num_segments < SEGMENT_GROUP_SIZE) {
	       XSegment *segment = &(group->segments[group->num_segments++]);

	       /* observe the order of the following lines before modifying */
	       segment->x1 = x1;
	       segment->y1 = y1;
	       segment->x2 = x1 = toscreen_x(p);
	       segment->y2 = y1 = toscreen_y(p);
	    } else {
	       fprintf(stderr, "ignoring some legs for now rather than overflowing fixed segment buffer\n");
	    }
		
	    break;
	  }
	 }
      }
   }
}

/* Calculate positions of all line segments and put them in the cache. */
static void
fill_segment_cache(void)
{
   int group;
   for (group = 0; group <= NUM_DEPTH_COLOURS; group++) {
      segment_groups[group].num_segments = 0;
   }

   /* since I no longer attempt to make sure that plan_elev is always
    * kept up to date by any action which might change the elev_angle
    * I make sure it's correct here [JPNP]
    */
   if (elev_angle == 0.0)
      plan_elev = ELEVATION;
   else if (elev_angle == 90.0)
      plan_elev = PLAN;
   else
      plan_elev = 0;

   sv = sin(rad(view_angle));
   cv = -cos(rad(view_angle));
   se = sin(rad(elev_angle));
   ce = cos(rad(elev_angle));

   if (ppLegs && legs) add_to_segment_cache(ppLegs, 0);
   if (ppSLegs && surf) add_to_segment_cache(ppSLegs, 1);
}

/* distance_metric() is a measure of how close we are */
#if 0
#define distance_metric(DX, DY) min(abs(DX), abs(DY))
#else
#define distance_metric(DX, DY) ((DX)*(DX) + (DY)*(DY))
#endif

#if 0
static point *
find_station(int x, int y)
{
   point **pp;
   point *p, *q = NULL;

   /* Ignore points a really long way away */
   int d_min = distance_metric(300, 300);

   if (ppStns == NULL) return NULL;

   for (pp = ppStns; *pp; pp++) {
      for (p = *pp; p->_.str != NULL; p++) {
	 int d;
	 int x1 = toscreen_x(p);
	 int y1 = toscreen_y(p);

	 d = distance_metric(x1 - x, y1 - y);
	 if (d < d_min) {
	    d_min = d;
	    q = p;
	 }
      }
   }
#if 0
   printf("near %d, %d, station %s was found\nat %d, %d\t\t%d, %d, %d\n",
	  x, y, q->_.str, toscreen_x(q), toscreen_y(q), q->X, q->Y, q->Z);
#endif
   return q;
}
#endif

static void
update_rotation(void)
{
   /* calculate amount of rotation by how long since last redraw
    * to keep a constant speed no matter how long between redraws */
   struct timeval temptime;

   gettimeofday(&temptime, NULL);

   if (rot) {
      double timefact = (temptime.tv_sec - lastframe.tv_sec);

      timefact += (temptime.tv_usec - lastframe.tv_usec) / 1000000.0;
      view_angle += rot_speed * timefact;

      if (elev_angle == 0.0)
	 plan_elev = ELEVATION;
      else if (elev_angle == 90.0)
	 plan_elev = PLAN;
      else
	 plan_elev = 0;
   }

   lastframe.tv_sec = temptime.tv_sec;
   lastframe.tv_usec = temptime.tv_usec;
}

static void
redraw_image(Display * display, Window window)
{
   XdbeSwapInfo info;

   info.swap_window = window;
   info.swap_action = XdbeBackground;

   XdbeSwapBuffers(display, &info, 1);
}

static void
redraw_image_dbe(Display * display, Window window, GC gc)
{
   /* Draw the cave into a window (strictly, the second buffer). */

   coord x1, y1, x2, y2;

   char temp[32];
   int group;

   x1 = y1 = 0;	/* avoid compiler warning */

   if (ppStns == NULL && ppLegs == NULL && ppSLegs == NULL) return;

   for (group = 0; group <= NUM_DEPTH_COLOURS; group++) {
      SegmentGroup *group_ptr = &segment_groups[group];

      XDrawSegments(display, window, gcs[3 + group], group_ptr->segments,
		    group_ptr->num_segments);
   }

   label_reg = XCreateRegion();

   if ((crossing || labelling) /*&& surveymask[p->survey] */ ) {
      point **pp;
      for (pp = ppStns; *pp; pp++) {
	 point *p;
	 for (p = *pp; p->_.str != NULL; p++) {
	    x2 = toscreen_x(p);
	    y2 = toscreen_y(p);
	    if (crossing) {
#if 0 /* + crosses */
	       XDrawLine(display, window, gcs[lab_col_ind], x2 - CROSSLENGTH, y2, x2 + CROSSLENGTH, y2);
	       XDrawLine(display, window, gcs[lab_col_ind], x2, y2 - CROSSLENGTH, x2, y2 + CROSSLENGTH);
#else /* x crosses */
	       XDrawLine(display, window, gcs[lab_col_ind],
			 x2 - CROSSLENGTH, y2 - CROSSLENGTH,
			 x2 + CROSSLENGTH, y2 + CROSSLENGTH);
	       XDrawLine(display, window, gcs[lab_col_ind],
			 x2 + CROSSLENGTH, y2 - CROSSLENGTH,
			 x2 - CROSSLENGTH, y2 + CROSSLENGTH);
#endif
	    }

	    if (labelling) {
	       char *q = p->_.str;
	       draw_label(display, window, gcs[lab_col_ind],
			  x2, y2 + slashheight, q, strlen(q));
	       /* XDrawString(display,window,gcs[lab_col_ind],x2,y2+slashheight, q, strlen(q)); */
	       /* XDrawString(display,window,gcs[p->survey],x2+10,y2, q, strlen(q)); */
	    }
	 }
      }
   }

   XDestroyRegion(label_reg);

   while (view_angle < 0.0)
      view_angle += 360.0;
   while (view_angle >= 360.0)
      view_angle -= 360.0;

   draw_ind_com(display, gc, view_angle);
   draw_ind_elev(display, gc, elev_angle);

   if (sbar < 1000)
      sprintf(temp, "%d m", (int)sbar);
   else
      sprintf(temp, "%d km", (int)sbar / 1000);
#ifdef XCAVEROT_BUTTONS
   XDrawString(mydisplay, window,  slab_gc, 8, BUTHEIGHT + 5 + FONTSPACE,
	       temp, strlen(temp));
#else
   XDrawString(mydisplay, window, slab_gc, 8, FONTSPACE, temp, strlen(temp));
#endif
}

#if 0
static int
x(void)
{
   /* XClearWindow(display, window); */
   XGetWindowAttributes(display, window, &a);
   height = a.height;
   width = a.width;
   xoff = width / 2;
   yoff = height / 2;

   XFlush(display);
   /* if we're using double buffering now is the time to swap buffers */
   if (have_double_buffering)
      XdbeSwapBuffers(display, &backswapinfo, 1);
}
#endif

#if 0
static void
process_focus(Display * display, Window window, int ix, int iy)
{
   int height, width;
   double x, y;
   XWindowAttributes a;
   point *q;

   XGetWindowAttributes(display, window, &a);
   height = a.height / 2;
   width = a.width / 2;

   x = (int)((double)(-ix + width) / scale);
   y = (int)((double)(height - iy) / scale);
   /* printf("process focus: ix=%d, iy=%d, x=%f, y=%f\n", ix, iy, (double)x, (double)y); */
   /* no distinction between PLAN or ELEVATION in the focus any more */
   /* plan_elev is no longer maintained correctly anyway JPNP 14/06/97 */
   if (plan_elev == PLAN) {
      x_mid += x * cv + y * sv;
      y_mid += x * sv - y * cv;
   } else if (plan_elev == ELEVATION) {
      z_mid += y;
      x_mid += x * cv;
      y_mid += x * sv;
   } else {
      if ((q = find_station(ix, iy)) != NULL) {
	 x = -(ix - toscreen_x(q)) / scale;
	 y = (iy - toscreen_y(q)) / scale;

	 x_mid = q->X + x * cv - y * se * cv;
	 y_mid = q->Y + x * sv + y * se * cv;
	 z_mid = q->Z + y * (sv - cv) * ce * sv;
#if 0
	 printf("x_mid, y_mid moved to %d,%d,%d\n", x_mid, y_mid, z_mid);
#endif
      }
   }

   /*printf("x_mid, y_mid moved to %f,%f\n", (double)x_mid, (double)y_mid); */
}
#endif

/* Neither of these produce the animated effect desired...
 * Use the same technique as in caverot */
static void
switch_to_plan(void)
{
   /* Switch to plan view. */
   elev_angle = 90.0;
#if 0
   while (elev_angle != 90.0) {
      elev_angle += 5.0;
      if (elev_angle >= 90.0) elev_angle = 90.0;

      update_rotation();
   }
#endif
}

static void
switch_to_elevation(void)
{
   /* Switch to elevation view. */
   elev_angle = 0.0;
#if 0
   int going_up = (elev_angle < 0.0);
   double step = going_up ? 5.0 : -5.0;

   while (elev_angle != 0.0) {
      elev_angle += step;
      if (elev_angle * step >= 0.0) elev_angle = 0.0;

      update_rotation();
   }
#endif
}

static void
mouse_moved(int mx, int my)
{
   /* get offset moved */
   int dx = mx - orig.x;
   int dy = my - orig.y;

   if (drag_button == Button3) {
      double x, y;

      x = (int)((double)(dx) / scale);
      y = (int)((double)(dy) / scale);

      if (plan_elev == PLAN) {
	 x_mid = x_mid_orig + (x * cv + y * sv);
	 y_mid = y_mid_orig + (x * sv - y * cv);
      } else {
	 z_mid = z_mid_orig + y;
	 x_mid = x_mid_orig + (x * cv);
	 y_mid = y_mid_orig + (x * sv);
      }
   } else if (drag_button == Button1) {
      double a;

      /* L-R => rotation */
      view_angle = view_angle_orig - (((double)dx) / 2.5);

      /*dy = -dy; */

      /* U-D => scaling */
      if (fabs((double)dy) <= 1.0) {
	 a = 0.0;
      } else {
	 a = log(fabs((double)dy)) / 3;
      }

      if (dy <= 0) {
	 /* mouse moved up or back to starting pt */
	 scale = scale_orig * pow(2, -a);
      } else if (dy > 0) {
	 /* mouse moved down */
	 scale = scale_orig * pow(2, a);
      }
   } else if (drag_button == Button2) {
      /* U-D => tilt */
      elev_angle = elev_angle_orig + (((double)dy) / 2.5);
      if (elev_angle > 90) elev_angle = 90;
      if (elev_angle < -90) elev_angle = -90;
   }
}

#ifdef XCAVEROT_BUTTONS
static void
draw_buttons(Display * display, GC mygc, GC egc)
{
   flip_button(display, butload, egc, mygc, "Load");
   flip_button(display, butrot, egc, mygc, rot ? "Stop" : "Rotate");
   flip_button(display, butstep, egc, mygc, "Step");
   flip_button(display, butzoom, egc, mygc, "Zoom in");
   flip_button(display, butmooz, egc, mygc, "Zoom out");
   flip_button(display, butplan, egc, mygc,
	       plan_elev == ELEVATION ? "Elev" : (plan_elev ==
						  PLAN ? "Plan" : "Tilt"));
   flip_button(display, butlabel, egc, mygc,
	       labelling ? "No Label" : "Label");
   flip_button(display, butcross, egc, mygc, crossing ? "No Cross" : "Cross");
#if 0 /* unused */
   flip_button(display, butselect, egc, mygc, "Select");
#endif
   flip_button(display, butquit, egc, mygc, "Quit");
}

#endif

static void
drag_compass(int x, int y)
{
   /* printf("x %d, y %d, ", x,y); */
   x -= INDWIDTH / 2;
   y -= INDWIDTH / 2;
   /* printf("xm %d, y %d, ", x,y); */
   view_angle = deg(atan2(x, y));
   /* snap view_angle to nearest 45 degrees if outside circle */
   if (x * x + y * y > indrad * indrad)
      view_angle = ((int)((view_angle + 360 + 22) / 45)) * 45;
   /* printf("a %f\n", view_angle); */
}

static void
drag_elevation(int x, int y)
{
   x -= INDWIDTH * (1 - E_IND_PTR) / 2;
   y -= INDWIDTH / 2;

   if (x >= 0) {
      elev_angle = deg(atan2(-y, x));
   } else {
      /* if the mouse is to the left of the elevation indicator,
       * snap to view from -90, 0 or 90 */
      if (abs(y) < INDWIDTH * E_IND_PTR / 2) {
	 elev_angle = 0.0;
      } else if (y < 0) {
	 elev_angle = 90.0;
      } else {
	 elev_angle = -90.0;
      }
   }
}

static void
set_defaults(void)
{
   XWindowAttributes a;

   scale = scale_default;
   view_angle = 0.0;
   plan_elev = PLAN;
   elev_angle = 90.0;
   rot = 0;
   rot_speed = 15;	/* rotation speed in degrees per second */

   XGetWindowAttributes(mydisplay, mywindow, &a);
   xoff = a.width / 2;
   yoff = a.height / 2;
}

static const struct option long_opts[] = {
   /* const char *name; int has_arg (0 no_argument, 1 required_*, 2 optional_*); int *flag; int val; */
   {"help", no_argument, 0, HLP_HELP},
   {"version", no_argument, 0, HLP_VERSION},
   {0, 0, 0, 0}
};

#define short_opts ""

static struct help_msg help[] = {
/*				<-- */
   {0, 0}
};

int
main(int argc, char **argv)
{
   /* Window hslide, vslide; */
   XWindowAttributes attr;

   /* XWindowChanges ch; */
   XGCValues gcval;
   GC enter_gc, scale_gc, mygc;

   int visdepth;	/* used in Double Buffer setup code */

   XSizeHints myhint;
   XGCValues gvalues;

   /* XComposeStatus cs; */
   int myscreen;
   unsigned long myforeground, mybackground, ind_fg, ind_bg;
   int done;
   int temp1, temp2;
   Font font;

   char *title;
   const char *tmp;

   msg_init(argv[0]);

   cmdline_set_syntax_message("3D_FILE...", NULL);
   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 1, -1);
   while (cmdline_getopt() != EOF) {
      /* do nothing */
   }

   set_codes(MOVE, DRAW, STOP);

   /* set up display and foreground/background */
   mydisplay = XOpenDisplay("");
   if (!mydisplay) {
      printf("Unable to open display!\n");
      exit(1);
   }

   myscreen = DefaultScreen(mydisplay);
   black = mybackground = BlackPixel(mydisplay, myscreen);
   white = myforeground = WhitePixel(mydisplay, myscreen);

   ind_bg = black;
   ind_fg = white;

   have_double_buffering = XdbeQueryExtension(mydisplay, &temp1, &temp2);

   if (have_double_buffering) {
      /* Get info about visuals supporting DBE */
      XVisualInfo vinfo;
      XdbeScreenVisualInfo *svi;
      int idx = 0;

      svi = XdbeGetVisualInfo(mydisplay, NULL, &idx);
      vinfo.visualid = svi->visinfo->visual;
      visdepth = svi->visinfo->depth;
      visinfo = XGetVisualInfo(mydisplay, VisualIDMask, &vinfo, &idx);
#if 0
      printf("visinfo->depth: %d\n", visinfo->depth);
      printf("visID, %x - %x\n", vinfo.visualid, visinfo->visualid);
#endif
   }

   /* create a window with a size such that it fits on the screen */
   myhint.x = 0;
   myhint.y = 0;
   myhint.width = WidthOfScreen(DefaultScreenOfDisplay(mydisplay))-5;
   myhint.height = HeightOfScreen(DefaultScreenOfDisplay(mydisplay))-25;
   myhint.flags = /* PPosition | */ PSize;

   if (have_double_buffering) {
      /* if double buffering is supported we cannot rely on the
       * default visual being one that is supported so we must
       * explicitly set it to one that we have been told is. */
      /* we need a new colormap for the visual since it won't necessarily
       * have one. */
      color_map = XCreateColormap(mydisplay, DefaultRootWindow(mydisplay),
				  visinfo->visual, AllocNone);
      dbwinattr.border_pixel = myforeground;
      dbwinattr.background_pixel = mybackground;
      dbwinattr.colormap = color_map;
      dbwinattr.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask | ButtonMotionMask | PointerMotionMask;
      mywindow = XCreateWindow(mydisplay, DefaultRootWindow(mydisplay),
			       myhint.x, myhint.y, myhint.width, myhint.height,
			       5, visinfo->depth, InputOutput, visinfo->visual,
			       CWBorderPixel | CWBackPixel | CWColormap | CWDontPropagate,
			       &dbwinattr);
   } else {
      color_map = DefaultColormap(mydisplay, 0);
      mywindow = XCreateSimpleWindow(mydisplay, DefaultRootWindow(mydisplay),
				     myhint.x, myhint.y,
				     myhint.width, myhint.height,
				     5, myforeground, mybackground);
   }

   XGetWindowAttributes(mydisplay, mywindow, &attr);

   /* set up double buffering on the window */
   if (have_double_buffering) {
      backbuf =
	 XdbeAllocateBackBufferName(mydisplay, mywindow, XdbeBackground);
      backswapinfo.swap_window = mywindow;
      backswapinfo.swap_action = XdbeBackground;
   }

#ifdef XCAVEROT_BUTTONS
   /* create children windows that will act as menu buttons */
   butload = XCreateSimpleWindow(mydisplay, mywindow,
				 0, 0, BUTWIDTH, BUTHEIGHT, 2, myforeground,
				 mybackground);
   butrot =
      XCreateSimpleWindow(mydisplay, mywindow, BUTWIDTH, 0, BUTWIDTH,
			  BUTHEIGHT, 2, myforeground, mybackground);
   butstep =
      XCreateSimpleWindow(mydisplay, mywindow, BUTWIDTH * 2, 0, BUTWIDTH,
			  BUTHEIGHT, 2, myforeground, mybackground);
   butzoom =
      XCreateSimpleWindow(mydisplay, mywindow, BUTWIDTH * 3, 0, BUTWIDTH,
			  BUTHEIGHT, 2, myforeground, mybackground);
   butmooz =
      XCreateSimpleWindow(mydisplay, mywindow, BUTWIDTH * 4, 0, BUTWIDTH,
			  BUTHEIGHT, 2, myforeground, mybackground);
   butplan =
      XCreateSimpleWindow(mydisplay, mywindow, BUTWIDTH * 5, 0, BUTWIDTH,
			  BUTHEIGHT, 2, myforeground, mybackground);
   butlabel =
      XCreateSimpleWindow(mydisplay, mywindow, BUTWIDTH * 6, 0, BUTWIDTH,
			  BUTHEIGHT, 2, myforeground, mybackground);
   butcross =
      XCreateSimpleWindow(mydisplay, mywindow, BUTWIDTH * 7, 0, BUTWIDTH,
			  BUTHEIGHT, 2, myforeground, mybackground);
#if 0 /* unused */
   butselect =
      XCreateSimpleWindow(mydisplay, mywindow, BUTWIDTH * 8, 0, BUTWIDTH,
			  BUTHEIGHT, 2, myforeground, mybackground);
#endif
   butquit =
      XCreateSimpleWindow(mydisplay, mywindow, BUTWIDTH * 8, 0, BUTWIDTH,
			  BUTHEIGHT, 2, myforeground, mybackground);
#endif

   /* allow indrad to be set */
   tmp = getenv("XCAVEROT_INDICATOR_RADIUS");
   if (tmp) indrad = atof(tmp);

   /* children windows to act as indicators */
   ind_com =
      XCreateSimpleWindow(mydisplay, mywindow, attr.width - INDWIDTH - 1, 0,
			  INDWIDTH, INDDEPTH, 1, ind_fg, ind_bg);
   ind_elev =
      XCreateSimpleWindow(mydisplay, mywindow, attr.width - INDWIDTH * 2 - 1,
			  0, INDWIDTH, INDDEPTH, 1, ind_fg, ind_bg);
#ifdef XCAVEROT_BUTTONS
   scalebar =
      XCreateSimpleWindow(mydisplay, mywindow, 0, BUTHEIGHT + FONTSPACE + 10,
			  23, attr.height - (BUTHEIGHT + FONTSPACE + 10) - 5,
			  0, ind_fg, ind_bg);
#else
   scalebar =
      XCreateSimpleWindow(mydisplay, mywindow, 0, FONTSPACE,
                          23, attr.height - 5, 0,
                          ind_fg, ind_bg);
#endif

   /* so we can auto adjust scale on window resize */
   oldwidth = attr.width;
   oldheight = attr.height;

   title = osmalloc(strlen(hello) + strlen(argv[optind]) + 7);
   sprintf(title, "%s - [%s]", hello, argv[optind]);

   /* load file(s) */
   while (argv[optind]) {
      if (!load_file(argv[optind], 0)) {
	 printf("File `%s' not found or not a Survex image file\n",
		argv[optind]);
	 exit(1);
      }
      optind++;
   }

   set_defaults();

   /* print program name at the top */
   XSetStandardProperties(mydisplay, mywindow, title, title,
			  None, argv + optind, argc - optind, &myhint);

   /* set up foreground/backgrounds and set up colors */
   mygc = XCreateGC(mydisplay, mywindow, 0, 0);

   gvalues.foreground = myforeground;
   gvalues.background = mybackground;
   scale_gc = XCreateGC(mydisplay, mywindow, 0, 0);
   slab_gc = XCreateGC(mydisplay, mywindow, 0, 0);
   enter_gc = XCreateGC(mydisplay, mywindow, 0, 0);

   XSetForeground(mydisplay, scale_gc, white);
   XSetForeground(mydisplay, slab_gc, white);
   gcval.line_width = 2;

   XChangeGC(mydisplay, scale_gc, GCLineWidth, &gcval);

   /* Load the font */
   tmp = getenv("XCAVEROT_FONTNAME");
   if (!tmp) tmp = FONTNAME;
   font = XLoadFont(mydisplay, tmp);
   XSetFont(mydisplay, mygc, font);

   XSetBackground(mydisplay, mygc, mybackground);
   XSetForeground(mydisplay, mygc, myforeground);
   color_set_up(mydisplay, mywindow);
   XSetFont(mydisplay, gcs[lab_col_ind], font);

   XSetForeground(mydisplay, enter_gc, black);
   XSetBackground(mydisplay, enter_gc, white);
   XChangeGC(mydisplay, enter_gc, GCLineWidth, &gcval);

   /* Get height value for font to use in label positioning JPNP 26/3/97 */
   fontinfo = XQueryFont(mydisplay, XGContextFromGC(gcs[0]));
   if (fontinfo) {
      fontheight = (fontinfo->max_bounds).ascent;
      if (fontinfo->per_char != NULL)
	 slashheight = fontinfo->per_char['\\'].ascent;
      else	/* use this guess if no individual info is available */
	 slashheight = fontheight;

      /* printf("fontheight: %d\n",fontheight); */
   } else {
      /* if something goes wrong use an arbitrary value */
      fontheight = slashheight = 10;
   }

   /* Enable the events we want */
   XSelectInput(mydisplay, mywindow,
		ButtonPressMask | KeyPressMask | KeyReleaseMask | ExposureMask |
		ButtonReleaseMask | ButtonMotionMask | StructureNotifyMask | FocusChangeMask);
   XSelectInput(mydisplay, ind_com, ButtonPressMask | ButtonMotionMask);
   XSelectInput(mydisplay, ind_elev, ButtonPressMask | ButtonMotionMask);
   XSelectInput(mydisplay, scalebar, ButtonPressMask | ButtonMotionMask);
#ifdef XCAVEROT_BUTTONS
   XSelectInput(mydisplay, butload, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);
   XSelectInput(mydisplay, butrot, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);
   XSelectInput(mydisplay, butstep, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);
   XSelectInput(mydisplay, butzoom, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);
   XSelectInput(mydisplay, butmooz, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);
   XSelectInput(mydisplay, butplan, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);
   XSelectInput(mydisplay, butlabel, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);
   XSelectInput(mydisplay, butcross, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);
#if 0 /* unused */
   XSelectInput(mydisplay, butselect, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);
#endif
   XSelectInput(mydisplay, butquit, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);
#endif
   /* map window to the screen */
   XMapRaised(mydisplay, mywindow);

   /* Map children windows to the screen */

#ifdef XCAVEROT_BUTTONS
   XMapRaised(mydisplay, butload);
   XMapRaised(mydisplay, butrot);
   XMapRaised(mydisplay, butstep);
   XMapRaised(mydisplay, butzoom);
   XMapRaised(mydisplay, butmooz);
   XMapRaised(mydisplay, butplan);
   XMapRaised(mydisplay, butlabel);
   XMapRaised(mydisplay, butcross);
#if 0 /* unused */
   XMapRaised(mydisplay, butselect);
#endif
   XMapRaised(mydisplay, butquit);
#endif
   XMapRaised(mydisplay, ind_com);
   XMapRaised(mydisplay, ind_elev);
   XMapRaised(mydisplay, scalebar);

   /* Display menu strings */

   gettimeofday(&lastframe, NULL);

#ifdef XCAVEROT_BUTTONS
   draw_buttons(mydisplay, mygc, enter_gc);
#endif

   /* Loop through until a q is pressed,
    * which will cause the application to quit */

   done = 0;
   while (done == 0) {
      int refresh_window = 0;
      update_rotation();

      if (rot == 0) goto ickybodge;
      while (XPending(mydisplay)) {
	 XEvent myevent;
	 ickybodge:
         XNextEvent(mydisplay, &myevent);
#if 0
         printf("event of type #%d, in window %x\n",myevent.type, (int)myevent.xany.window);
#endif
	 if (myevent.type == ButtonRelease) {
	    if (myevent.xbutton.button == drag_button) drag_button = AnyButton;
	 } else if (myevent.type == ButtonPress) {
	    drag_button = myevent.xbutton.button;
	    orig.x = myevent.xbutton.x;
	    orig.y = myevent.xbutton.y;
#if 0
            printf("ButtonPress: x=%d, y=%d, window=%x\n",orig.x,orig.y,(int)myevent.xbutton.subwindow);
#endif
	    if (myevent.xbutton.button == Button1) {
               if (myevent.xbutton.window == ind_com) {
                  drag_compass(myevent.xbutton.x, myevent.xbutton.y);
#ifdef XCAVEROT_BUTTONS
               } else if (myevent.xbutton.window == butzoom)
                  process_zoom(mydisplay, butzoom, mygc, enter_gc);
               else if (myevent.xbutton.window == butmooz)
		  process_mooz(mydisplay, butmooz, mygc, enter_gc);
               else if (myevent.xbutton.window == butload)
		  process_load(mydisplay, mywindow, butload, mygc, enter_gc);
               else if (myevent.xbutton.window == butrot)
		  process_rot(mydisplay, butrot, mygc, enter_gc);
               else if (myevent.xbutton.window == butstep)
		  process_step(mydisplay, butstep, mygc, enter_gc);
               else if (myevent.xbutton.window == butplan)
		  process_plan(mydisplay, butplan, mygc, enter_gc);
               else if (myevent.xbutton.window == butlabel)
		  process_label(mydisplay, butlabel, mygc, enter_gc);
               else if (myevent.xbutton.window == butcross)
		  process_cross(mydisplay, butcross, mygc, enter_gc);
#if 0 /* unused */
               else if (myevent.xbutton.window == butselect)
		  process_select(mydisplay, butselect, mygc, enter_gc);
#endif
               else if (myevent.xbutton.window == butquit) {
		  done = 1;
		  break;
#endif
	       } else if (myevent.xbutton.window == ind_elev) {
		  drag_elevation(myevent.xbutton.x, myevent.xbutton.y);
	       } else if (myevent.xbutton.window == scalebar) {
		  scale_orig = scale;
	       } else if (myevent.xbutton.window == mywindow) {
		  view_angle_orig = view_angle;
		  scale_orig = scale;
	       }
            } else if (myevent.xbutton.button == Button2) {
	       elev_angle_orig = elev_angle;
	    } else if (myevent.xbutton.button == Button3) {
	       x_mid_orig = x_mid;
	       y_mid_orig = y_mid;
	       z_mid_orig = z_mid;
	    }
	 } else if (myevent.type == MotionNotify) {
	    if (myevent.xmotion.window == ind_com) {
	       drag_compass(myevent.xmotion.x, myevent.xmotion.y);
	    } else if (myevent.xmotion.window == ind_elev) {
	       drag_elevation(myevent.xmotion.x, myevent.xmotion.y);
	    } else if (myevent.xmotion.window == scalebar) {
	       if (myevent.xmotion.state & Button1Mask)
		  scale = exp(log(scale_orig)
			      + (double)(myevent.xmotion.y - orig.y) / 250);
	       else
		  scale = exp(log(scale_orig)
			      * (1 - (double)(myevent.xmotion.y - orig.y) / 100)
			      );

	    } else if (myevent.xmotion.window == mywindow) {
	       /* drag cave about / alter rotation or scale */

	       mouse_moved(myevent.xmotion.x, myevent.xmotion.y);
	    }
	 } else if (myevent.xany.window == mywindow) {
	    switch (myevent.type) {
	     case KeyPress:
		 {
		    char text[10];
		    KeySym mykey;
		    int i;
		    XKeyEvent *key_event = (XKeyEvent *) & myevent;

		    switch (key_event->keycode) {
		     case 100:
		       x_mid -= cv * 20 / scale;
		       y_mid -= sv * 20 / scale;
		       break;

		     case 102:
		       x_mid += cv * 20 / scale;
		       y_mid += sv * 20 / scale;
		       break;

		     case 98:
		       if (plan_elev == PLAN) {
			   x_mid -= sv * 20 / scale;
			   y_mid += cv * 20 / scale;
		       } else {
			   z_mid -= 20 / scale;
		       }
		       break;

		     case 104:
		       if (plan_elev == PLAN) {
			   x_mid += sv * 20 / scale;
			   y_mid -= cv * 20 / scale;
		       } else {
			   z_mid += 20 / scale;
		       }
		       break;

		     default: {
			i = XLookupString(key_event, text, 10, &mykey, 0);
			if (i == 1)
			   switch (tolower(text[0])) {
			    case 13:	/* Return => start rotating */
			      rot = 1;
			      gettimeofday(&lastframe, NULL);
			      break;
			    case 32:	/* Space => stop rotating */
			      rot = 0;
			      break;
			    case 6:	/* Ctrl+F => toggle surFace */
			      surf = !surf;
			      break;
			    case 12:	/* Ctrl+L => toggle Legs */
			      legs = !legs;
			      break;
			    case 14:	/* Ctrl+N => toggle Names */
			      labelling = !labelling;
			      break;
			    case 19:	/* Ctrl+S => toggle Surface */
			      surf = !surf;
			      break;
			    case 24:	/* Ctrl+X => toggle crosses */
			      crossing = !crossing;
			      break;
			    case 127:	/* Delete => restore defaults */
			      set_defaults();
			      refresh_window = 1;
			      break;
			    case 'q':
			      done = 1;
			      break;
			    case 'u':	/* view from above */
			      elev_angle = 90;
			      break;
			    case 'd':	/* view from below */
			      elev_angle = -90;
			      break;
			    case 'l':	/* switch to elevation */
			      switch_to_elevation();
			      break;
			    case 'p':	/* switch to plan */
			      switch_to_plan();
			      break;
			    case 'z':	/* rotn speed up */
			      rot_speed = rot_speed * 1.2;
			      if (fabs(rot_speed) > 720)
				 rot_speed = rot_speed > 0 ? 720 : -720;
			      break;
			    case 'x':	/* rotn speed down */
			      rot_speed = rot_speed / 1.2;
			      if (fabs(rot_speed) < 0.1)
				 rot_speed = rot_speed > 0 ? 0.1 : -0.1;
			      break;
			    case '[':	/* zoom out */
			    case '{':
			      scale /= zoomfactor;
			      break;
			    case ']':	/* zoom in */
			    case '}':
			      scale *= zoomfactor;
			      break;
			    case 'r':	/* reverse dirn of rotation */
			      rot_speed = -rot_speed;
			      break;
			    case 'c':	/* rotate one step "anticlockwise" */
			      view_angle += fabs(rot_speed) / 5;
			      break;
			    case 'v':	/* rotate one step "clockwise" */
			      view_angle -= fabs(rot_speed) / 5;
			      break;
			    case '\'':	/* higher viewpoint */
			    case '@': case '"': /* alternate shifted forms */
			      elev_angle += 3.0;
			      if (elev_angle > 90.0) elev_angle = 90.0;
			      break;
			    case '/':	/* lower viewpoint */
			    case '?':
			      elev_angle -= 3.0;
			      if (elev_angle < -90.0) elev_angle = -90.0;
			      break;
			    case 'n':	/* cave north */
			      view_angle = 0.0;
			      break;
			    case 's':	/* cave south */
			      view_angle = 180.0;
			      break;
			    case 'e':	/* cave east */
			      view_angle = 90.0;
			      break;
			    case 'w':	/* cave west */
			      view_angle = 270.0;
			      break;
			    case 'o':
			      allnames = !allnames;
			      break;
			   }
		     }
		    }
		    break;
		 }
	     case ConfigureNotify:

#if 0	/* rescale to keep view in window */
	       scale *= min((double)myevent.xconfigure.width /
			    (double)oldwidth,
			    (double)myevent.xconfigure.height / (double)oldheight);
#else /* keep in mind aspect ratio to make resizes associative */
	       scale *= min(3.0 * (double)myevent.xconfigure.width,
			    4.0 * (double)myevent.xconfigure.height) /
		  min(3.0 * (double)oldwidth, 4.0 * (double)oldheight);
#endif
	       oldwidth = myevent.xconfigure.width;
	       oldheight = myevent.xconfigure.height;

	       XResizeWindow(mydisplay, scalebar, 23,
			     myevent.xconfigure.height - BUTHEIGHT - FONTSPACE -
			     5);

	       XMoveWindow(mydisplay, ind_com,
			   myevent.xconfigure.width - INDWIDTH - 1, 0);
	       XMoveWindow(mydisplay, ind_elev,
			   myevent.xconfigure.width - (2 * INDWIDTH) - 1, 0);
	       break;
	    default: {
		XWindowAttributes a;

		XGetWindowAttributes(mydisplay, mywindow, &a);
		xoff = a.width / 2;
		yoff = a.height / 2;
		refresh_window = 1;
	    }
	    }
	 }
      }

      {
	 static double old_view_angle = -1;
	 static double old_elev_angle = -1;
	 static double old_scale = -1;
	 static int old_crossing = -1;
	 static int old_labelling = -1;
	 static int old_allnames = -1;
	 static int old_legs = -1;
	 static int old_surf = -1;
	 static coord old_x_mid;
	 static coord old_y_mid;
	 static coord old_z_mid;

	 int redraw = refresh_window;

	 if (old_view_angle != view_angle ||
	     old_elev_angle != elev_angle ||
	     old_x_mid != x_mid ||
	     old_y_mid != y_mid ||
	     old_z_mid != z_mid ||
	     old_scale != scale ||
	     old_legs != legs ||
	     old_surf != surf) {

	    old_view_angle = view_angle;
	    old_elev_angle = elev_angle;
	    old_x_mid = x_mid;
	    old_y_mid = y_mid;
	    old_z_mid = z_mid;
	    old_legs = legs;
	    old_surf = surf;
	    fill_segment_cache();
	    redraw = 1;
	 }

	 if (old_crossing != crossing ||
	     old_labelling != labelling ||
	     old_allnames != allnames) {
	    old_crossing = crossing;
	    old_labelling = labelling;
	    old_allnames = allnames;
	    redraw = 1;
	 }

	 if (redraw) {
	    if (have_double_buffering) {
	       redraw_image_dbe(mydisplay, (Window) backbuf, mygc);
	       redraw_image(mydisplay, mywindow);
	    } else {
	       XClearWindow(mydisplay, mywindow);
	       redraw_image_dbe(mydisplay, mywindow, mygc);
	    }
	    draw_scalebar(old_scale != scale, scale_gc);
#ifdef XCAVEROT_BUTTONS
	    draw_buttons(mydisplay, mygc, enter_gc);
#endif
	    old_scale = scale;
	 }
      }
   }	/* while */

   /* Free up and clean up the windows created */
   XFreeGC(mydisplay, mygc);
   XDestroyWindow(mydisplay, mywindow);
   XCloseDisplay(mydisplay);
   exit(0);
}
