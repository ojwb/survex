/*	xcaverot.c

  X-Windows cave viewing program

  This program is intended to be used in conjunction with the SURVEX program
  for cave survey data processing. It has been developed in conjunction with
  SURVEX version 0.20 (Beta).

  Now modified to work with SURVEX version 0.51 - should be backwards
  compatible.

  use:

        xcaverot [image file] [X-options]

  Copyright 1993 Bill Purvis

   Bill Purvis DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
   ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
   Bill Purvis BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
   ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
   ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
   SOFTWARE.

  History:

  28.06.93 wrote basic X code
  30.06.93 use of mouse to select centre-of-focus
  01.07.93 labels and crosses ?

1994.03.04 hacked around to use img.c to read .3d files [Olly]
1994.05.17 added changes from Andy Holtsbery
1994.06.01 should now be able to load image file given on command line [Olly]
1994.08.13 removed erroneous '*' & check for defined MININT and MAXINT [Olly]
1995.03.16 labels were being trimmed wrongly so prefix was displayed [Olly]
1997.01.09 checks argument 1 given before using it [Olly]

  17.04.97 adjusted way labels are drawn [JPNP]
  18.04.97 added elevation controls [JPNP]
  20.04.97 compass and elevation indicators, scale bar [JPNP}
  21.04.97 various little bits [JPNP]

  May 97 JPNP tidied up his code some and added more features:
              double buffering on Xservers which support it
	      rotation speed independant of system load
	      reinstated survey sections in different colours
	      few other small things
1997.05.28 rearranged a fair bit, so we can merge with caverot soon [Olly]
1997.06.02 mostly converted to use cvrotimg [Olly]
1998.03.04 merged two deviant versions:
>1997.06.03 tweaked to compile [Olly]
>1997.06.10 fixed gcc warning [Olly]
1998.03.21 fixed up to compile cleanly on Linux

  12.06.97 started reworking in Olly's rearrangements towards merger
             with caverot, and to use cvrotimg    [JPNP]
           survey sections now broken again.
  16.06.97 finished working in Olly's changes.
           Compiles without warning with gcc. [JPNP]
           now works out max/min XYZ vals to scale survey on load again [JPNP]
	   unfinished rotation control buttons half there but not shown [JPNP]
  17.06.97 adjusted rescaling and non-overlapping labels on window resize[JPNP]
  23.05.98 Fixes use of gettimeofday() for rotation timing. [JPNP]
  11.06.99 Adjusted to compile with survex 0.91 [JPNP]
  05.07.99 Fixed double buffer support on platforms (eg SGI) with multiple
           visuals available. However the colour allocation needs sorting
	   out properly. [JPNP]
  ??.07.99 Crap drawing code fettled on Expo '99, + other improvements. [MRS]

  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <sys/time.h>	/* for gettimeofday */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <X11/extensions/Xdbe.h>	/* for double buffering support */

#include "useful.h"
#include "img.h"
#include "filename.h"
#include "message.h"
#include "caverot.h"
#include "cvrotimg.h"

int xcMac, ycMac;
float y_stretch = 1.0;

/* Width of each button along the top of the window */
#define BUTWIDTH 60
#define BUTHEIGHT 25

/* Width and height of compass and elevation indicator windows */
#define FONTSPACE 20
#define INDWIDTH 100	/* FIXME: allow INDWIDTH to be set dynamically - 50 is nice */
#define INDDEPTH (INDWIDTH + FONTSPACE)

#define C_IND_ANG 25
#define E_IND_ANG 11
#define E_IND_LEN 0.8
#define E_IND_LINE 0.6
#define E_IND_PTR 0.38

/* radius of compass and clino indicators */
#define RADIUS ((float)INDWIDTH*.4)

/* font to use */
#define FONTNAME "-adobe-helvetica-medium-r-normal-*-8-*"
/* FIXME: was #define FONTNAME "8x13", should be configurable */

#define PIBY180 0.017453293

#if 0
typedef struct {
   short Option;
   short survey;
   coord X;
   coord Y;
   coord Z;
   /*  long padding;
    * long more_pad; */
} point;

#define MOVE 1
#define DRAW 2
#define STOP 4
#define LABEL 8
#define CROSS 16
#endif

#define PLAN 1	/* values of plan_elev */
#define ELEVATION 2

#define NUM_DEPTH_COLOURS 10	/* number of depth colour bands */

typedef struct {
   /* A group of segments to be drawn in the same colour. */

   int num_segments;	/* number of segments */
   XSegment segments[20000];	/* array of line segments, fixed limit temporary hack */
} SegmentGroup;

static SegmentGroup segment_groups[NUM_DEPTH_COLOURS];

/* use double buffering in the rendering if server supports it */
static int have_double_buffering;
static XdbeBackBuffer backbuf;
static XdbeSwapInfo backswapinfo;
static XVisualInfo *visinfo;
static XSetWindowAttributes dbwinattr;

/* scale all data by this to fit in coord data type */
/* Note: data in file in metres. 100.0 below stores to nearest cm */
static float datafactor = (float)100.0;

/* factor to scale view on screen by */
static float scale = 0.1;
static float zoomfactor = 1.2;
static float sbar;	/* length of scale bar */
static float scale_orig;	/* saved state of scale, used in drag re-scale */
static int changedscale = 1;

static struct {
   int x;
   int y;
} orig;	/* position of original click in pointer drags */

static float view_angle = 180.0;	/* viewed from this bearing */

				 /* subtract 180 for bearing up page */
static float elev_angle = 0.0;
static float rot_speed;	/* rotation speed degrees per second */

static int plan_elev;
static int rot = 0;
static int crossing = 0;
static int labelling = 0;

static struct timeval lastframe;

static int xoff, yoff;	/* offsets at which survey is plotted in window */
static coord x_mid;
static coord y_mid;
static coord z_mid;
static float z_col_scale;

static Display *mydisplay;
static Window mywindow;

#ifdef XCAVEROT_BUTTONS
static Window butzoom, butmooz, butload, butrot, butstep, butquit;
static Window butplan, butlabel, butcross, butselect;
#endif
static Window ind_com, ind_elev, scalebar, rot_but;
static GC mygc, scale_gc, slab_gc;
static int oldwidth, oldheight;	/* to adjust scale on window resize */
static Region label_reg;	/* used to implement non-overlapping labels */

static XFontStruct *fontinfo;	/* info on the font used in labeling */
static char hello[] = "XCaverot";

static unsigned long black, white;

static int lab_col_ind = 19;	/* Hack to get labels in sensible colour JPNP */
static int fontheight, slashheight;	/* JPNP */

static char *ncolors[] = { "black",
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

static Colormap color_map;
static XColor colors[128];
static XColor exact_colors[128];
static char *surveys[128];
static char surveymask[128];
static GC gcs[128];
static int numsurvey = 0;

static int dragging_about = 0;	/* whether cave is being dragged with right button */
static int drag_start_x, drag_start_y;
static float x_mid_orig, y_mid_orig, z_mid_orig;	/* original posns b4 drag */
static int rotsc_start_x, rotsc_start_y;
static int rotating_and_scaling = 0;	/* whether left button is depressed */
static float rotsc_angle;	/* rotn b4 drag w/ left button */
static float rotsc_scale;	/* scale b4 drag w/ left button */
static int drag_start_xoff, drag_start_yoff;	/* original posns b4 drag */
static float sx, cx, fx, fy, fz;

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
   numsurvey = 0;
}

static void
flip_button(Display * display, Window mainwin, Window button,
	    GC normalgc, GC inversegc, char *string)
{
   int len = strlen(string);
   int width;
   int offset;

   width = XTextWidth(XQueryFont(display, XGContextFromGC(inversegc)),
		      string, len);
   offset = (BUTWIDTH - width) / 2;
   if (offset < 0)
      offset = 0;
   /* for old behaviour, offset = BUTWIDTH/4 */

   XClearWindow(display, button);
   XFillRectangle(display, button, normalgc, 0, 0, BUTWIDTH, BUTHEIGHT);
   XDrawImageString(display, button, inversegc, offset, 20, string, len);
   XFlush(display);
}

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

static lid **ppLegs = NULL;
static lid **ppStns = NULL;

int
load_file(const char *name)
{
   ppLegs = osmalloc((1 + 1) * sizeof(lid Huge *));
   ppStns = osmalloc((1 + 1) * sizeof(lid Huge *));

   /* load data into memory */
   if (!load_data(name, ppLegs, ppStns))
      return 0;
   ppLegs[1] = NULL;
   ppStns[1] = NULL;

   scale = scale_to_screen(ppLegs, ppStns);
   x_mid = Xorg;
   y_mid = Yorg;
   z_mid = Zorg;

   z_col_scale = ((float)(NUM_DEPTH_COLOURS - 1)) / (2 * Zrad);

   return 1;
}

void
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

   flip_button(display, mainwin, button, mygc, egc, "Load");
   numsurvey = 0;

   /* set up input window */
   enter_window = XCreateSimpleWindow(display, mainwin,
				      340, 200, 300, 100, 3, black, white);
   enter_gc = mygc;
   XMapRaised(display, enter_window);
   XClearWindow(display, enter_window);
   XSelectInput(display, enter_window, ButtonPressMask | KeyPressMask);

   for (;;) {
      /* reset everything */
      XDrawString(display, enter_window, mygc, 10, 50, "Enter file name", 15);
      XFlush(display);
      string[0] = '\0';
      key = 0;

      /* accept input from the user */
      do {
	 XNextEvent(display, &enter_event);
	 count = XLookupString((XKeyEvent *) & enter_event, text, 10, &key, 0);
	 text[count] = '\0';
	 if (key != XK_Return)
	    strcat(string, text);
	 XClearWindow(display, enter_window);
	 XDrawString(display, enter_window, mygc,
		     10, 50, "Enter file name", 15);
	 XFlush(display);
	 XDrawString(display, enter_window, enter_gc,
		     100, 75, string, strlen(string));
      }
      while (key != XK_Return && enter_event.type == KeyPress);

      /* Clear everything out of the buffers and reset */
      XFlush(display);
      if (!load_file(string)) {
	 strcpy(string, "File not found or not a Survex image file");
	 break;
      }
      XClearWindow(display, enter_window);
      XDrawString(display, enter_window, enter_gc,
		  10, 25, string, strlen(string));
   }	/* End of for (;;) */

   XDestroyWindow(display, enter_window);

   flip_button(display, mainwin, button, egc, mygc, "Load");
}

void
process_plan(Display * display, Window mainwin, Window button, GC mygc, GC egc)
{
   flip_button(display, mainwin, button, mygc, egc,
	       elev_angle == 0.0 ? "Elev" : "Plan");
   if (elev_angle == 90.0)
      elev_angle = 0.0;
   else
      elev_angle = 90.0;
   flip_button(display, mainwin, button, egc, mygc,
	       elev_angle == 0.0 ? "Elev" : "Plan");
}

void
process_label(Display * display, Window mainwin, Window button,
	      GC mygc, GC egc)
{
   flip_button(display, mainwin, button, mygc, egc,
	       labelling ? "No Label" : "Label");
   labelling = !labelling;
   flip_button(display, mainwin, button, egc, mygc,
	       labelling ? "No Label" : "Label");
}

void
process_cross(Display * display, Window mainwin, Window button,
	      GC mygc, GC egc)
{
   flip_button(display, mainwin, button, mygc, egc,
	       crossing ? "No Cross" : "Cross");
   crossing = !crossing;
   flip_button(display, mainwin, button, egc, mygc,
	       crossing ? "No Cross" : "Cross");
}

void
process_rot(Display * display, Window mainwin, Window button, GC mygc, GC egc)
{
   flip_button(display, mainwin, button, mygc, egc, rot ? "Stop" : "Rotate");
   rot = !rot;
   gettimeofday(&lastframe, NULL);
   flip_button(display, mainwin, button, egc, mygc, rot ? "Stop" : "Rotate");
}

void
process_step(Display * display, Window mainwin, Window button, GC mygc, GC egc)
{
   flip_button(display, mainwin, button, mygc, egc, "Step");
   view_angle += 3.0;
   if (view_angle >= 360.0)
      view_angle = 0.0;
   flip_button(display, mainwin, button, egc, mygc, "Step");
}

static void
draw_scalebar(void)
{
   float l, m, n, o;

   if (changedscale) {

      XWindowAttributes a;

      XGetWindowAttributes(mydisplay, scalebar, &a);

      l = (float)(a.height - 4) / (datafactor * scale);

      m = log10(l);

      n = pow(10, floor(m));	/* Ouch did I really do all this ;-) JPNP */

      o = (m - floor(m) < log10(5) ? n : 5 * n);

      sbar = (int)o;

      changedscale = 0;
      XClearWindow(mydisplay, scalebar);
   }

   XDrawLine(mydisplay, scalebar, scale_gc, 13, 2, 13,
	     2 + scale * datafactor * sbar);
}

/* FIXME: Zoom In -> In / Zoom Out -> Out ??? */
void
process_zoom(Display * display, Window mainwin, Window button, GC mygc, GC egc)
{
   flip_button(display, mainwin, button, mygc, egc, "Zoom in");
   scale *= zoomfactor;
   changedscale = 1;
   flip_button(display, mainwin, button, egc, mygc, "Zoom in");
   draw_scalebar();
}

void
process_mooz(Display * display, Window mainwin, Window button, GC mygc, GC egc)
{
   flip_button(display, mainwin, button, mygc, egc, "Zoom out");
   scale /= zoomfactor;
   changedscale = 1;
   flip_button(display, mainwin, button, egc, mygc, "Zoom out");
   draw_scalebar();
}

void
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

void
draw_rot_but(GC gc)
{
   int c;
   float r = INDDEPTH * 0.75 / (4.0 * 2);
   float q = INDDEPTH * 0.55 / (8.0);

   c = INDDEPTH / 8.0;

   XDrawArc(mydisplay, rot_but, gc, c - r, c - r, 2.0 * r,
	    2.0 * r, -90 * 64, 300 * 64);

   XDrawArc(mydisplay, rot_but, gc, c - r, (INDDEPTH * .75 + c)
	    - r, 2.0 * r, 2.0 * r, -90 * 64, -300 * 64);

   XDrawLine(mydisplay, rot_but, gc, c - r, (INDDEPTH * .25 + c) + q, c,
	     (INDDEPTH * .25 + c) - q);
   XDrawLine(mydisplay, rot_but, gc, c, (INDDEPTH * .25 + c) - q, c + r,
	     (INDDEPTH * .25 + c) + q);

   XDrawLine(mydisplay, rot_but, gc, c - r, (INDDEPTH * .5 + c) - q, c,
	     (INDDEPTH * .5 + c) + q);
   XDrawLine(mydisplay, rot_but, gc, c, (INDDEPTH * .5 + c) + q, c + r,
	     (INDDEPTH * .5 + c) - q);
}

void
draw_ind_elev(Display * display, GC gc, float angle)
{
   char temp[32];
   int xm, ym;
   double sa, ca;
   double r = (double)INDWIDTH * E_IND_LINE / 2;
   double q = (double)INDWIDTH * E_IND_PTR / 2;

   xm = ym = INDWIDTH / 2;

   sa = sin(angle * PIBY180);
   ca = cos(angle * PIBY180);

/*printf("%f\n",ca);*/

   XClearWindow(mydisplay, ind_elev);

   XDrawLine(display, ind_elev, gc, xm - r, ym, xm + r, ym);

   XDrawLine(display, ind_elev, gc, xm - q, ym, xm - q + 2 * q * (ca + 0.001),
	     ym - 2 * q * sa);
   XDrawLine(display, ind_elev, gc, xm - q, ym,
	     xm - q + E_IND_LEN * q * cos((angle + E_IND_ANG) * PIBY180),
	     ym - E_IND_LEN * q * sin((angle + E_IND_ANG) * PIBY180));
   XDrawLine(display, ind_elev, gc, xm - q, ym,
	     xm - q + E_IND_LEN * q * cos((angle - E_IND_ANG) * PIBY180),
	     ym - E_IND_LEN * q * sin((angle - E_IND_ANG) * PIBY180));

   sprintf(temp, "%d", (int)angle);
   XDrawString(display, ind_elev, gc, INDWIDTH / 2 - 20, INDDEPTH - 10, temp,
	       strlen(temp));
}

void
draw_ind_com(Display * display, GC gc, float angle)
{
   char temp[32];
   int xm, ym;
   double sa, ca;
   double rs, rc, r = RADIUS;

   xm = ym = INDWIDTH / 2;

   rs = r * sin(C_IND_ANG * PIBY180);
   rc = r * cos(C_IND_ANG * PIBY180);
   sa = sin((angle + 180) * PIBY180);
   ca = cos((angle + 180) * PIBY180);

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

   sprintf(temp, "%03d", 180 - (int)angle);
   XDrawString(display, ind_com, gc, INDWIDTH / 2 - 20, INDDEPTH - 10, temp,
	       strlen(temp));
}

void
draw_label(Display * display, Window window, GC gc, int x, int y,
	   const char *string, int length)
{
   XRectangle r;
   int strwidth;
   int width = oldwidth;
   int height = oldheight;

   strwidth = XTextWidth(fontinfo, string, length);

   if (x < width && y < height && x + strwidth > 0 && y + fontheight > 0) {

      if (XRectInRegion(label_reg, x, y, strwidth, fontheight) == RectangleOut) {
	 r.x = x;
	 r.y = y;
	 r.width = strwidth;
	 r.height = fontheight;
	 XUnionRectWithRegion(&r, label_reg, label_reg);
	 XDrawString(display, window, gc, x, y, string, length);
      }
   }
}

void
setview(float angle)
{
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

   sx = sin((angle) * PIBY180);
   cx = cos((angle) * PIBY180);
   fz = cos(elev_angle * PIBY180);
   fx = sin(elev_angle * PIBY180) * sx;
   fy = sin(elev_angle * PIBY180) * -cx;
}

int
toscreen_x(point * p)
{
   return (((p->X - x_mid) * -cx + (p->Y - y_mid) * -sx) * scale) + xoff;
}

int
toscreen_y(point * p)
{
   int y;

   switch (plan_elev) {
   case PLAN:
      y = ((p->X - x_mid) * sx - (p->Y - y_mid) * cx) * scale;
   case ELEVATION:
      y = (p->Z - z_mid) * scale;
   default:
      y = ((p->X - x_mid) * fx + (p->Y - y_mid) * fy
	   + (p->Z - z_mid) * fz) * scale;
   }
   return yoff - y;
}

static void
fill_segment_cache()
{
   /* Calculate positions of all line segments and put them in the cache. */

   lid *plid;
   int group;

   coord x1, y1;

   if (ppLegs == NULL)
      return;

   x1 = y1 = 0;	/* avoid compiler warning */

   setview(view_angle);

   for (group = 0; group < NUM_DEPTH_COLOURS; group++) {
      segment_groups[group].num_segments = 0;
   }

   for (plid = ppLegs[0]; plid; plid = plid->next) {
      point *p;

      for (p = plid->pData; p->_.action != STOP; p++) {
	 switch (p->_.action) {
	 case MOVE:
	    x1 = toscreen_x(p);
	    y1 = toscreen_y(p);
	    break;

	 case DRAW:{
	    XSegment *segment;
	    SegmentGroup *group;

	    /* calculate colour */
	    int depth = (int)((float)(-p->Z + Zorg + Zrad) * z_col_scale);

	    if (depth < 0) {
	       depth = 0;
	    } else if (depth >= NUM_DEPTH_COLOURS) {
	       depth = NUM_DEPTH_COLOURS - 1;
	    }
	    /* store coordinates */
	    group = &segment_groups[depth];
	    segment = &(group->segments[group->num_segments++]);

	    /* observe the order of the following lines before modifying */
	    segment->x1 = x1;
	    segment->y1 = y1;
	    segment->x2 = x1 = toscreen_x(p);
	    segment->y2 = y1 = toscreen_y(p);

	    break;
	 }
	 }
      }
   }
}

/* distance_metric() is a measure of how close we are */
#if 0
#define distance_metric(DX, DY) min(abs(DX), abs(DY))
#else
#define distance_metric(DX, DY) ((DX)*(DX) + (DY)*(DY))
#endif

point *
find_station(int x, int y, int mask)
{
   point *p, *q = NULL;

   /* Ignore points a really long way away */
   int d_min = distance_metric(300, 300);
   lid *plid;

   if (ppStns == NULL)
      return NULL;

   setview(view_angle);	/* FIXME: needed? */

   for (plid = ppStns[0]; plid; plid = plid->next) {
      for (p = plid->pData; p->_.str != NULL; p++) {
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

static void
update_rotation()
{
   /* calculate amount of rotation by how long since last redraw
    * to keep a constant speed no matter how long between redraws */
   struct timeval temptime;

   gettimeofday(&temptime, NULL);

   if (rot) {
      double timefact = (temptime.tv_sec - lastframe.tv_sec);

      timefact += (temptime.tv_usec - lastframe.tv_usec) / 1000000.0;
      view_angle += rot_speed * timefact;

      while (view_angle > 180.0)
	 view_angle -= 360.0;
      while (view_angle <= -180.0)
	 view_angle += 360.0;

      if (elev_angle == 0.0)
	 plan_elev = ELEVATION;
      else if (elev_angle == 90.0)
	 plan_elev = PLAN;
      else
	 plan_elev = 0;

      fill_segment_cache();
   }

   lastframe.tv_sec = temptime.tv_sec;
   lastframe.tv_usec = temptime.tv_usec;
}

void
redraw_image(Display * display, Window window, GC gc)
{
   XdbeSwapInfo info;

   info.swap_window = window;
   info.swap_action = XdbeBackground;

   XdbeSwapBuffers(display, &info, 1);
}

void
redraw_image_dbe(Display * display, Window window, GC gc)
{
   /* Draw the cave into a window (strictly, the second buffer). */

   coord x1, y1, x2, y2;
   int srvy = 0;	/*FIXME: JPNP had 1 - check */
   lid *plid;

   char temp[32];
   int group;

   x1 = y1 = 0;	/* avoid compiler warning */

   if (ppStns == NULL && ppLegs == NULL)
      return;

   for (group = 0; group < NUM_DEPTH_COLOURS; group++) {
      SegmentGroup *group_ptr = &segment_groups[group];

      XDrawSegments(display, window, gcs[3 + group], group_ptr->segments,
		    group_ptr->num_segments);
   }

   label_reg = XCreateRegion();

   if ((crossing || labelling) /*&& surveymask[p->survey] */ ) {
      for (plid = ppStns[0]; plid; plid = plid->next) {
	 point *p;

	 for (p = plid->pData; p->_.str != NULL; p++) {
	    x2 = toscreen_x(p);
	    y2 = toscreen_y(p);
	    if (crossing) {
	       XDrawLine(display, window, gcs[srvy], x2 - 10, y2, x2 + 10, y2);
	       XDrawLine(display, window, gcs[srvy], x2, y2 - 10, x2, y2 + 10);
	    }

	    if (labelling) {
	       char *q;

	       q = p->_.str;
	       draw_label(display, window, gcs[lab_col_ind],
			  x2, y2 + slashheight, q, strlen(q));
	       /* XDrawString(display,window,gcs[lab_col_ind],x2,y2+slashheight, q, strlen(q)); */
	       /* XDrawString(display,window,gcs[p->survey],x2+10,y2, q, strlen(q)); */
	    }
	 }
      }
   }

   XDestroyRegion(label_reg);

   draw_ind_com(display, gc, view_angle);
   draw_ind_elev(display, gc, elev_angle);
   // FIXME:   draw_rot_but(gc);

   if (sbar < 1000)
      sprintf(temp, "%d m", (int)sbar);
   else
      sprintf(temp, "%d km", (int)sbar / 1000);
   // FIXME: add BUTHEIGHT to FONTSPACE if buttons on
   XDrawString(mydisplay, window, slab_gc, 8, FONTSPACE, temp, strlen(temp));
}

static void
perform_redraw(void)
{
   if (have_double_buffering) {
      redraw_image_dbe(mydisplay, (Window) backbuf, mygc);
      redraw_image(mydisplay, mywindow, mygc);
   } else {
      XClearWindow(mydisplay, mywindow);
      redraw_image_dbe(mydisplay, mywindow, mygc);
   }
}

#if 0
int
x()
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

void
process_focus(Display * display, Window window, int ix, int iy)
{
   int height, width;
   float x, y, sx, cx;
   XWindowAttributes a;
   point *q;

   XGetWindowAttributes(display, window, &a);
   height = a.height / 2;
   width = a.width / 2;

   x = (int)((float)(-ix + width) / scale);
   y = (int)((float)(height - iy) / scale);
   sx = sin(view_angle * PIBY180);
   cx = cos(view_angle * PIBY180);
   /* printf("process focus: ix=%d, iy=%d, x=%f, y=%f\n", ix, iy, (double)x, (double)y); */
   /* no distinction between PLAN or ELEVATION in the focus any more */
   /* plan_elev is no longer maintained correctly anyway JPNP 14/06/97 */
   if (plan_elev == PLAN) {
      x_mid += x * cx + y * sx;
      y_mid += x * sx - y * cx;
   } else if (plan_elev == ELEVATION) {
      z_mid += y;
      x_mid += x * cx;
      y_mid += x * sx;
   } else {
      if ((q = find_station(ix, iy, MOVE | DRAW /*| LABEL */ )) != NULL) {
	 float n1, n2, n3, j1, j2, j3, i1, i2, i3;

	 /*
	  * x_mid = q->X;
	  * y_mid = q->Y;
	  * z_mid = q->Z;
	  */

	 /*
	  * fz = cos(elev_angle*PIBY180);
	  */
	 n1 = sx * fz;
	 n2 = cx * fz;
	 n3 = sin(elev_angle * PIBY180);
	 /*
	  * fx = n3 * sx;
	  * fy = n3 * -cx;
	  */

	 /* add a few comments to this section, when I can remember exactly
	  * how it does work   JPNP */

	 /* do we know that sx, cx, fx. fy. fz are properly set? JPNP */
	 /* yes find_station has done so! JPNP */
	 x = -(ix - toscreen_x(q)) / scale;
	 y = (iy - toscreen_y(q)) / scale;

	 i1 = cx;
	 i2 = sx;
	 i3 = 0;

	 j1 = n2 * i3 - n3 * i2;
	 j2 = n3 * i1 - n1 * i3;
	 j3 = n1 * i2 - n2 * i1;

	 x_mid = q->X + x * i1 + y * j1;
	 y_mid = q->Y + x * i2 + y * j2;
	 z_mid = q->Z + x * i3 + y * j3;
#if 0
	 printf("vector (%f,%f,%f)\n", x * i1 + y * j1, x * i2 + y * j2,
		x * i3 + y * j3);
	 printf("x_mid, y_mid moved to %d,%d,%d\n", x_mid, y_mid, z_mid);
#endif
      }
   }

   /*printf("x_mid, y_mid moved to %f,%f\n", (double)x_mid, (double)y_mid); */
}

static void
switch_to_plan(void)
{
   /* Switch to plan view. */

   while (elev_angle != 90.0) {
      elev_angle += 5.0;
      if (elev_angle >= 90.0) elev_angle = 90.0;

      update_rotation();
      if (!rot) fill_segment_cache();
      perform_redraw();
   }

   plan_elev = PLAN;
}

static void
switch_to_elevation(void)
{
   /* Switch to elevation view. */

   int going_up = (elev_angle < 0.0);
   float step = going_up ? 5.0 : -5.0;

   while (elev_angle != 0.0) {
      elev_angle += step;
      if (elev_angle * step >= 0.0) elev_angle = 0.0;

      update_rotation();
      if (!rot) fill_segment_cache();
      perform_redraw();
   }

   plan_elev = ELEVATION;
}

/* a sensible way of navigating about the cave */

static void
press_left_button(int x, int y)
{
   rotsc_start_x = x;
   rotsc_start_y = y;
   rotating_and_scaling = 1;
   rotsc_angle = view_angle;
   rotsc_scale = scale;
}

static void
press_right_button(int x, int y)
{
   drag_start_xoff = xoff;
   drag_start_yoff = yoff;
   dragging_about = 1;
   drag_start_x = x;
   drag_start_y = y;
   x_mid_orig = x_mid;
   y_mid_orig = y_mid;
   z_mid_orig = z_mid;
}

static void
release_left_button()
{
   rotating_and_scaling = 0;
}

static void
release_right_button()
{
   dragging_about = 0;
}

static void
mouse_moved(Display * display, Window window, int mx, int my)
{
   if (dragging_about) {
      float x, y;
      int dx, dy;

      /* get offset moved */
      dx = mx - drag_start_x;
      dy = my - drag_start_y;

/*	xoff = drag_start_xoff + dx; */
/*	yoff = drag_start_yoff + dy; */

      x = (int)((float)(dx) / scale);
      y = (int)((float)(dy) / scale);

      if (plan_elev == PLAN) {
	 x_mid = x_mid_orig + (x * cx + y * sx);
	 y_mid = y_mid_orig + (x * sx - y * cx);
      } else {
	 z_mid = z_mid_orig + y;
	 x_mid = x_mid_orig + (x * cx);
	 y_mid = y_mid_orig + (x * sx);
      }
   } else if (rotating_and_scaling) {
      double a;

      int dx = mx - rotsc_start_x;
      int dy = my - rotsc_start_y;

      /* L-R => rotation */
      view_angle = rotsc_angle + (((float)dx) / 2.5);

      while (view_angle < 0.0)
	 view_angle += 360.0;
      while (view_angle >= 360.0)
	 view_angle -= 360.0;

      /*dy = -dy; */

      /* U-D => scaling */
      if (fabs((float)dy) <= 1.0) {
	 a = 0.0;
      } else {
	 a = log(fabs((float)dy)) / 3;
      }

      if (dy <= 0) {
	 /* mouse moved up or back to starting pt */
	 scale = rotsc_scale * pow(2, a);
      } else if (dy > 0) {
	 /* mouse moved down */
	 scale = rotsc_scale * pow(2, -a);
      }
   }

   fill_segment_cache();
}

#ifdef XCAVEROT_BUTTONS
// FIXME:
void
draw_buttons(Display * display, Window mainwin, GC mygc, GC egc)
{
   flip_button(display, mainwin, butload, egc, mygc, "Load");
   flip_button(display, mainwin, butrot, egc, mygc, rot ? "Stop" : "Rotate");
   flip_button(display, mainwin, butstep, egc, mygc, "Step");
   flip_button(display, mainwin, butzoom, egc, mygc, "Zoom in");
   flip_button(display, mainwin, butmooz, egc, mygc, "Zoom out");
   flip_button(display, mainwin, butplan, egc, mygc,
	       plan_elev == ELEVATION ? "Elev" : (plan_elev ==
						  PLAN ? "Plan" : "Tilt"));
   flip_button(display, mainwin, butlabel, egc, mygc,
	       labelling ? "No Label" : "Label");
   flip_button(display, mainwin, butcross, egc, mygc,
	       crossing ? "No Cross" : "Cross");
   flip_button(display, mainwin, butselect, egc, mygc, "Select");
   flip_button(display, mainwin, butquit, egc, mygc, "Quit");
}

#endif

void
drag_compass(int x, int y)
{
   /* printf("x %d, y %d, ", x,y); */
   x -= INDWIDTH / 2;
   y -= INDWIDTH / 2;
   /* printf("xm %d, y %d, ", x,y); */
   view_angle = atan2(-x, y) / PIBY180;
   /* snap view_angle to nearest 45 degrees if outside circle */
   if (x * x + y * y > RADIUS * RADIUS)
      view_angle = ((int)((view_angle + 360 + 22) / 45)) * 45 - 360;
   /* printf("a %f\n", view_angle); */
   fill_segment_cache();
}

void
drag_elevation(int x, int y)
{
   x -= INDWIDTH * (1 - E_IND_PTR) / 2;
   y -= INDWIDTH / 2;

   if (x >= 0) {
      elev_angle = atan2(-y, x) / PIBY180;
   } else {
      /* if the mouse is to the left of the elevation indicator,
       * snap to view from -90, 0 or 90 */
      if (abs(y) < INDWIDTH * E_IND_PTR / 2) {
	 elev_angle = 0.0f;
      } else if (y < 0) {
	 elev_angle = 90.0f;
      } else {
	 elev_angle = -90.0f;
      }
   }
   fill_segment_cache();
}

static void
set_defaults(void)
{
   XWindowAttributes a;

   view_angle = 180.0;
   scale = 0.01;
   changedscale = 1;
   plan_elev = PLAN;
   elev_angle = 90.0;
   rot = 0;
   rot_speed = 15;	/* rotation speed in degrees per second */

   XGetWindowAttributes(mydisplay, mywindow, &a);
   xoff = a.width / 2;
   yoff = a.height / 2;
}

int
main(int argc, char **argv)
{
   /* Window hslide, vslide; */
   XWindowAttributes attr;

   /* XWindowChanges ch; */
   XGCValues gcval;
   GC enter_gc;

   int visdepth;	/* used in Double Buffer setup code */

   int redraw = 1;
   XEvent myevent;
   XSizeHints myhint;
   XGCValues gvalues;

   /* XComposeStatus cs; */
   int myscreen;
   unsigned long myforeground, mybackground, ind_fg, ind_bg;
   int done;
   int temp1, temp2;
   Font font;

   // FIXME:  int indicators = 1;
   char *title;

   msg_init(argv[0]);

   /* check command line argument(s) */
   if (argc != 2) {
      fprintf(stderr, "Syntax: %s <.3d file>\n", argv[0]);
      return 1;
   }

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
   myhint.height = HeightOfScreen(DefaultScreenOfDisplay(mydisplay))-5;
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
      mywindow = XCreateSimpleWindow(mydisplay,
				     DefaultRootWindow(mydisplay),
				     myhint.x, myhint.y, myhint.width,
				     myhint.height, 5, myforeground,
				     mybackground);
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
   butselect =
      XCreateSimpleWindow(mydisplay, mywindow, BUTWIDTH * 8, 0, BUTWIDTH,
			  BUTHEIGHT, 2, myforeground, mybackground);
   butquit =
      XCreateSimpleWindow(mydisplay, mywindow, BUTWIDTH * 9, 0, BUTWIDTH,
			  BUTHEIGHT, 2, myforeground, mybackground);
#endif

   /* children windows to act as indicators */
   ind_com =
      XCreateSimpleWindow(mydisplay, mywindow, attr.width - INDWIDTH - 1, 0,
			  INDWIDTH, INDDEPTH, 1, ind_fg, ind_bg);
   ind_elev =
      XCreateSimpleWindow(mydisplay, mywindow, attr.width - INDWIDTH * 2 - 1,
			  0, INDWIDTH, INDDEPTH, 1, ind_fg, ind_bg);
#if 0
   scalebar =
      XCreateSimpleWindow(mydisplay, mywindow, 0, BUTHEIGHT + 5 + FONTSPACE,
			  23, attr.height - (BUTHEIGHT + FONTSPACE + 5), 0,
			  ind_fg, ind_bg);
#else
   scalebar =
      XCreateSimpleWindow(mydisplay, mywindow, 0, BUTHEIGHT, 23,
			  attr.height - (BUTHEIGHT + FONTSPACE + 5), 0, ind_fg,
			  ind_bg);
#endif

   rot_but = XCreateSimpleWindow(mydisplay, mywindow, attr.width - INDWIDTH * 2
				 - 1 - INDDEPTH / 4, 0, INDDEPTH / 4, INDDEPTH,
				 1, ind_fg, ind_bg);

/*   printf("Window ind_com %d\n", ind_com);   */

/*   printf("Window ind_elev %d\n", ind_elev);  */

   /* so we can auto adjust scale on window resize */
   oldwidth = attr.width;
   oldheight = attr.height;

   xcMac = attr.width;
   ycMac = attr.height;

   /* load file */
   load_file(argv[1]);
   title = malloc(strlen(hello) + strlen(argv[1]) + 7);
   if (!title)
      exit(1);	// FIXME
   sprintf(title, "%s - [%s]", hello, argv[1]);
   argc--;
   argv++;
#if 0
   /* try to open file if we've been given one */
   if (argv[1] && argv[1][0] != '\0' && argv[1][0] != '-') {
      /* if it loaded okay then discard the command line argument */
      if (load_file(argv[1])) {
	 argc--;
	 argv++;
      }
   }
#endif

#if 0	/* FIXME: JPNP code */
   /* set scale to fit cave in screen */
   scale = 0.6 * min((float)oldwidth / (xRad * 2 + 1),
		     min((float)oldheight / (yRad * 2 + 1),
			 (float)oldheight / (zRad * 2 + 1)));


#endif

   set_defaults();
   fill_segment_cache();

   /* print program name at the top */
   XSetStandardProperties(mydisplay, mywindow, title, title,
			  None, argv, argc, &myhint);

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
   font = XLoadFont(mydisplay, FONTNAME);
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
   XSelectInput(mydisplay, butselect, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);
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
   XMapRaised(mydisplay, butselect);
   XMapRaised(mydisplay, butquit);
#endif
   XMapRaised(mydisplay, ind_com);
   XMapRaised(mydisplay, ind_elev);
   XMapRaised(mydisplay, scalebar);
   /*  XMapRaised (mydisplay, rot_but); */

   /* Display menu strings */

   gettimeofday(&lastframe, NULL);

#ifdef XCAVEROT_BUTTONS	// FIXME: !?!
   XNextEvent(mydisplay, &myevent);
   draw_buttons(mydisplay, mywindow, mygc, enter_gc);
#endif

   /* Loop through until a q is pressed,
    * which will cause the application to quit */

   done = 0;
   while (done == 0) {
      redraw = 1;

      update_rotation();

      if (rot == 0 || XPending(mydisplay)) {
         XNextEvent(mydisplay, &myevent);
#if 0
         printf("event of type #%d, in window %x\n",myevent.type, (int)myevent.xany.window);
#endif
	 if (myevent.type == ButtonRelease) {
	    if (myevent.xbutton.button == Button1) {
	       /* left button released */
	       release_left_button();
	    } else if (myevent.xbutton.button == Button3) {
	       /* right button has been released => stop dragging cave about */
	       release_right_button();
	    }
	 } else if (myevent.type == ButtonPress) {
	    orig.x = myevent.xbutton.x;
	    orig.y = myevent.xbutton.y;
#if 0
            printf("ButtonPress: x=%d, y=%d, window=%x\n",orig.x,orig.y,(int)myevent.xbutton.subwindow);
#endif
	    if (myevent.xbutton.button == Button1) {
               if (myevent.xbutton.window == ind_com) {
                  int old_angle = (int)view_angle;
                  drag_compass(myevent.xbutton.x, myevent.xbutton.y);
                  if ((int)view_angle == old_angle) 
                     redraw = 0; 
#ifdef XCAVEROT_BUTTONS
               } else if (myevent.xbutton.window == butzoom)
                     process_zoom(mydisplay, mywindow, butzoom, mygc, enter_gc);
               else if (myevent.xbutton.window == butmooz)
                     process_mooz(mydisplay, mywindow, butmooz, mygc, enter_gc);
               else if (myevent.xbutton.window == butload)
                     process_load(mydisplay, mywindow, butload, mygc, enter_gc);
               else if (myevent.xbutton.window == butrot)
                     process_rot(mydisplay, mywindow, butrot, mygc, enter_gc);
               else if (myevent.xbutton.window == butstep)
                     process_step(mydisplay, mywindow, butstep, mygc, enter_gc);
               else if (myevent.xbutton.window == butplan)
                     process_plan(mydisplay, mywindow, butplan, mygc, enter_gc);
               else if (myevent.xbutton.window == butlabel)
                     process_label(mydisplay, mywindow, butlabel, mygc, enter_gc);
               else if (myevent.xbutton.window == butcross)
                     process_cross(mydisplay, mywindow, butcross, mygc, enter_gc);
               else if (myevent.xbutton.window == butselect)
                     process_select(mydisplay, mywindow, butselect, mygc, enter_gc);
               else if (myevent.xbutton.window == butquit) {
                     done = 1;
                     break;
#endif	       
	       } else if (myevent.xbutton.window == ind_elev) {
		  int old_elev = (int)elev_angle;
		  drag_elevation(myevent.xbutton.x, myevent.xbutton.y);
		  if (old_elev == (int)elev_angle)
		     redraw = 0;
	       } else if (myevent.xbutton.window == scalebar) {
		  scale_orig = scale;
		  draw_scalebar();
	       } else if (myevent.xbutton.window == mywindow) {
		  press_left_button(myevent.xbutton.x, myevent.xbutton.y);
		  /* process_focus(mydisplay, mywindow,
		   * myevent.xbutton.x, myevent.xbutton.y); */
	       } else if (myevent.xbutton.button == Button2) {
		  /* toggle plan/elevation */
		  if (plan_elev == PLAN) {
		     switch_to_elevation();
		  } else {
		     switch_to_plan();
		  }
	       } else if (myevent.xbutton.button == Button3) {
		  /* translate cave */
		  press_right_button(myevent.xbutton.x, myevent.xbutton.y);
	       }
	    }
	 } else if (myevent.type == MotionNotify) {
	    if (myevent.xmotion.window == ind_com) {
	       int old_angle = (int)view_angle;
	       
	       drag_compass(myevent.xmotion.x, myevent.xmotion.y);
	       if ((int)view_angle == old_angle)
		  redraw = 0;
	    } else if (myevent.xmotion.window == ind_elev) {
	       int old_elev = (int)elev_angle;
	       
	       drag_elevation(myevent.xmotion.x, myevent.xmotion.y);
	       if (old_elev == (int)elev_angle)
		  redraw = 0;
	    } else if (myevent.xmotion.window == scalebar) {
	       if (myevent.xmotion.state & Button1Mask)
		  scale = exp(log(scale_orig)
			      + (float)(myevent.xmotion.y - orig.y) / (250)
			      );
	       else
		  scale = exp(log(scale_orig)
			      * (1 - (float)(myevent.xmotion.y - orig.y) / 100)
			      );
	       
	       changedscale = 1;
	       draw_scalebar();
	    } else if (myevent.xmotion.window == mywindow) {
	       /* drag cave about / alter rotation or scale */
	       
	       mouse_moved(mydisplay, mywindow, myevent.xmotion.x,
			   myevent.xmotion.y);
	    }
	 }
	 if (myevent.xany.window == mywindow) {
	    switch (myevent.type) {
	     case KeyPress:
		 {
		    char text[10];
		    KeySym mykey;
		    int i;
		    XKeyEvent *key_event = (XKeyEvent *) & myevent;
		    
		    switch (key_event->keycode) {
		     case 100:
		       xoff += 20;
		       break;
		       
		     case 102:
		       xoff -= 20;
		       break;
		       
		     case 98:
		       yoff += 20;
		       break;
		       
		     case 104:
		       yoff -= 20;
		       break;
		       
		     default:{
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
			    case 14:	/* Ctrl+N => toggle labels (station names) */
			      labelling = !labelling;
			      break;
			    case 24:	/* Ctrl+X => toggle crosses */
			      crossing = !crossing;
			      break;
			    case 127:	/* Delete => restore defaults */
			      set_defaults();
			      break;
			    case 'q':
			      done = 1;
			      break;
			    case 'u':	/* cave up */
			      elev_angle += 3.0;
			      if (elev_angle > 90.0)
				 elev_angle = 90.0;
			      break;
			    case 'd':	/* cave down */
			      elev_angle -= 3.0;
			      if (elev_angle < -90.0)
				 elev_angle = -90.0;
			      break;
			    case 'l':	/* switch to elevation */
			      switch_to_elevation();
			      break;
			    case 'p':	/* switch to plan */
			      switch_to_plan();
			      break;
			    case 'z':	/* rotn speed down */
			      rot_speed = rot_speed / 1.5;
			      if (fabs(rot_speed) < 0.1)
				 rot_speed = rot_speed > 0 ? 0.1 : -0.1;
			      break;
			    case 'x':	/* rotn speed up */
			      rot_speed = rot_speed * 1.5;
			      if (fabs(rot_speed) > 720)
				 rot_speed = rot_speed > 0 ? 720 : -720;
			      break;
			    case '[':	/* zoom out */
			      scale /= 2.0;
			      if (scale < 0.001) {
				 scale = 0.001;
			      }
			      fill_segment_cache();
			      redraw = 1;
			      break;
			    case ']':	/* zoom in */
			      scale *= 2.0;
			      if (scale > 0.4) {
				 scale = 0.4;
			      }
			      fill_segment_cache();
			      redraw = 1;
			      break;
			    case 'r':	/* reverse dirn of rotation */
			      rot_speed = -rot_speed;
			      break;
			    case 'c':	/* rotate one step "clockwise" */
			      // FIXME: view_angle += rot_step;
			      if (view_angle >= 360.0) {
				 view_angle -= 360.0;
			      }
			      fill_segment_cache();
			      redraw = 1;
			      break;
			    case 'v':	/* rotate one step "anticlockwise" */
			      // FIXME: view_angle -= rot_step;
			      if (view_angle <= 0.0) {
				 view_angle += 360.0;
			      }
			      fill_segment_cache();
			      redraw = 1;
			      break;
			    case '\'':	/* higher viewpoint (?) */
			      z_mid += Zrad / 7.5;
			      fill_segment_cache();
			      redraw = 1;
			      break;
			    case '/':	/* lower viewpoint (?) */
			      z_mid -= Zrad / 7.5;
			      fill_segment_cache();
			      redraw = 1;
			      break;
			    case 'n':	/* cave north */
			      view_angle = 0.0;
			      fill_segment_cache();
			      redraw = 1;
			      break;
			    case 's':	/* cave south */
			      view_angle = 180.0;
			      fill_segment_cache();
			      redraw = 1;
			      break;
			    case 'e':	/* cave east */
			      view_angle = 90.0;
			      fill_segment_cache();
			      redraw = 1;
			      break;
			    case 'w':	/* cave west */
			      view_angle = 270.0;
			      fill_segment_cache();
			      redraw = 1;
			      break;
			   }
		     }
		    }
		    break;
		 }
	     case ConfigureNotify:
	       
	       changedscale = 1;
#if 0	/* rescale to keep view in window */
	       scale *= min((float)myevent.xconfigure.width /
			    (float)oldwidth,
			    (float)myevent.xconfigure.height / (float)oldheight);
#else /* keep in mind aspect ratio to make resizes associative */
	       scale *= min(3.0 * (float)myevent.xconfigure.width,
			    4.0 * (float)myevent.xconfigure.height) /
		  min(3.0 * (float)oldwidth, 4.0 * (float)oldheight);
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
	       
	       draw_scalebar();
	       break;
	    }
	 }
      }
      if (redraw) {
	 perform_redraw();
	 draw_buttons(mydisplay, mywindow, mygc, enter_gc);
      }
   }	/* while */
   
   /* Free up and clean up the windows created */
   XFreeGC(mydisplay, mygc);
   XDestroyWindow(mydisplay, mywindow);
   XCloseDisplay(mydisplay);
   exit(0);
}
