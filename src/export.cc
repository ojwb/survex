/* export.cc
 * Export to CAD-like formats (DXF, Sketch, SVG) and also Compass PLT.
 */

/* Copyright (C) 1994-2004 Olly Betts
 * Copyright (C) 2004 John Pybus (SVG Output code)
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

/* #define DEBUG_CAD3D */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "wx.h"
#include "mainfrm.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "debug.h"
#include "filename.h"
#include "hash.h"
#include "img.h"
#include "message.h"
#include "useful.h"

/* default values - can be overridden with --htext and --msize respectively */
#define TEXT_HEIGHT	0.6
#define MARKER_SIZE	0.8

#define GRID_SPACING	100

#define POINTS_PER_INCH	72.0
#define POINTS_PER_MM (POINTS_PER_INCH / MM_PER_INCH)

#define SQRT_2          1.41421356237309504880168872420969

#define LEGS 1
#define STNS 2
#define LABELS 4

/* default to DXF */
#define FMT_DEFAULT FMT_DXF

static FILE *fh;

/* bounds */
static double min_x, min_y, min_z, max_x, max_y, max_z;

static double text_height; /* for station labels */
static double marker_size; /* for station markers */
static double grid; /* grid spacing (or 0 for no grid) */
static double scale = 500.0;
static double factor;
static const char *unit = "mm";

static const char *survey = NULL;

static void
dxf_header(void)
{
   fprintf(fh, "0\nSECTION\n"
	       "2\nHEADER\n");
   fprintf(fh, "9\n$EXTMIN\n"); /* lower left corner of drawing */
   fprintf(fh, "10\n%#-.6f\n", min_x); /* x */
   fprintf(fh, "20\n%#-.6f\n", min_y); /* y */
   fprintf(fh, "30\n%#-.6f\n", min_z); /* min z */
   fprintf(fh, "9\n$EXTMAX\n"); /* upper right corner of drawing */
   fprintf(fh, "10\n%#-.6f\n", max_x); /* x */
   fprintf(fh, "20\n%#-.6f\n", max_y); /* y */
   fprintf(fh, "30\n%#-.6f\n", max_z); /* max z */
   fprintf(fh, "9\n$PDMODE\n70\n3\n"); /* marker style as CROSS */
   fprintf(fh, "9\n$PDSIZE\n40\n%6.2f\n", marker_size); /* marker size */
   fprintf(fh, "0\nENDSEC\n");

   fprintf(fh, "0\nSECTION\n"
	       "2\nTABLES\n");
   fprintf(fh, "0\nTABLE\n" /* Define CONTINUOUS and DASHED line types. */
	       "2\nLTYPE\n"
	       "70\n10\n"
	       "0\nLTYPE\n"
	       "2\nCONTINUOUS\n"
	       "70\n64\n"
	       "3\nContinuous\n"
	       "72\n65\n"
	       "73\n0\n"
	       "40\n0.0\n"
	       "0\nLTYPE\n"
	       "2\nDASHED\n"
	       "70\n64\n"
	       "3\nDashed\n"
	       "72\n65\n"
	       "73\n2\n"
	       "40\n2.5\n"
	       "49\n1.25\n"
	       "49\n-1.25\n"
	       "0\nENDTAB\n");
   fprintf(fh, "0\nTABLE\n"
	       "2\nLAYER\n");
   fprintf(fh, "70\n10\n"); /* max # off layers in this DXF file : 10 */
   /* First Layer: CentreLine */
   fprintf(fh, "0\nLAYER\n2\nCentreLine\n");
   fprintf(fh, "70\n64\n"); /* shows layer is referenced by entities */
   fprintf(fh, "62\n5\n"); /* color: kept the same used by SpeleoGen */
   fprintf(fh, "6\nCONTINUOUS\n"); /* linetype */
   /* Next Layer: Stations */
   fprintf(fh, "0\nLAYER\n2\nStations\n");
   fprintf(fh, "70\n64\n"); /* shows layer is referenced by entities */
   fprintf(fh, "62\n7\n"); /* color: kept the same used by SpeleoGen */
   fprintf(fh, "6\nCONTINUOUS\n"); /* linetype */
   /* Next Layer: Labels */
   fprintf(fh, "0\nLAYER\n2\nLabels\n");
   fprintf(fh, "70\n64\n"); /* shows layer is referenced by entities */
   fprintf(fh, "62\n7\n"); /* color: kept the same used by SpeleoGen */
   fprintf(fh, "6\nCONTINUOUS\n"); /* linetype */
   /* Next Layer: Surface */
   fprintf(fh, "0\nLAYER\n2\nSurface\n");
   fprintf(fh, "70\n64\n"); /* shows layer is referenced by entities */
   fprintf(fh, "62\n5\n"); /* color */
   fprintf(fh, "6\nDASHED\n"); /* linetype */
   /* Next Layer: SurfaceStations */
   fprintf(fh, "0\nLAYER\n2\nSurfaceStations\n");
   fprintf(fh, "70\n64\n"); /* shows layer is referenced by entities */
   fprintf(fh, "62\n7\n"); /* color */
   fprintf(fh, "6\nCONTINUOUS\n"); /* linetype */
   /* Next Layer: SurfaceLabels */
   fprintf(fh, "0\nLAYER\n2\nSurfaceLabels\n");
   fprintf(fh, "70\n64\n"); /* shows layer is referenced by entities */
   fprintf(fh, "62\n7\n"); /* color */
   fprintf(fh, "6\nCONTINUOUS\n"); /* linetype */
   if (grid > 0) {
      /* Next Layer: Grid */
      fprintf(fh, "0\nLAYER\n2\nGrid\n");
      fprintf(fh, "70\n64\n"); /* shows layer is referenced by entities */
      fprintf(fh, "62\n7\n"); /* color: kept the same used by SpeleoGen */
      fprintf(fh, "6\nCONTINUOUS\n"); /* linetype */
   }
   fprintf(fh, "0\nENDTAB\n"
	       "0\nENDSEC\n");

   fprintf(fh, "0\nSECTION\n"
	       "2\nENTITIES\n");

   if (grid > 0) {
      double x, y;
      x = floor(min_x / grid) * grid + grid;
      y = floor(min_y / grid) * grid + grid;
#ifdef DEBUG_CAD3D
      printf("x_min: %f  y_min: %f\n", x, y);
#endif
      while (x < max_x) {
	 /* horizontal line */
	 fprintf(fh, "0\nLINE\n");
	 fprintf(fh, "8\nGrid\n"); /* Layer */
	 fprintf(fh, "10\n%6.2f\n", x);
	 fprintf(fh, "20\n%6.2f\n", min_y);
	 fprintf(fh, "30\n0\n");
	 fprintf(fh, "11\n%6.2f\n", x);
	 fprintf(fh, "21\n%6.2f\n", max_y);
	 fprintf(fh, "31\n0\n");
	 x += grid;
      }
      while (y < max_y) {
	 /* vertical line */
	 fprintf(fh, "0\nLINE\n");
	 fprintf(fh, "8\nGrid\n"); /* Layer */
	 fprintf(fh, "10\n%6.2f\n", min_x);
	 fprintf(fh, "20\n%6.2f\n", y);
	 fprintf(fh, "30\n0\n");
	 fprintf(fh, "11\n%6.2f\n", max_x);
	 fprintf(fh, "21\n%6.2f\n", y);
	 fprintf(fh, "31\n0\n");
	 y += grid;
      }
   }
}

static void
dxf_start_pass(int layer)
{
   layer = layer;
}

static void
dxf_move(const img_point *p)
{
   p = p; /* unused */
}

static void
dxf_line(const img_point *p1, const img_point *p, bool fSurface)
{
   fprintf(fh, "0\nLINE\n");
   fprintf(fh, fSurface ? "8\nSurface\n" : "8\nCentreLine\n"); /* Layer */
   fprintf(fh, "10\n%6.2f\n", p1->x);
   fprintf(fh, "20\n%6.2f\n", p1->y);
   fprintf(fh, "30\n%6.2f\n", p1->z);
   fprintf(fh, "11\n%6.2f\n", p->x);
   fprintf(fh, "21\n%6.2f\n", p->y);
   fprintf(fh, "31\n%6.2f\n", p->z);
}

static void
dxf_label(const img_point *p, const char *s, bool fSurface)
{
   /* write station label to dxf file */
   fprintf(fh, "0\nTEXT\n");
   fprintf(fh, fSurface ? "8\nSurfaceLabels\n" : "8\nLabels\n"); /* Layer */
   fprintf(fh, "10\n%6.2f\n", p->x);
   fprintf(fh, "20\n%6.2f\n", p->y);
   fprintf(fh, "30\n%6.2f\n", p->z);
   fprintf(fh, "40\n%6.2f\n", text_height);
   fprintf(fh, "1\n%s\n", s);
}

static void
dxf_cross(const img_point *p, bool fSurface)
{
   /* write station marker to dxf file */
   fprintf(fh, "0\nPOINT\n");
   fprintf(fh, fSurface ? "8\nSurfaceStations\n" : "8\nStations\n"); /* Layer */
   fprintf(fh, "10\n%6.2f\n", p->x);
   fprintf(fh, "20\n%6.2f\n", p->y);
   fprintf(fh, "30\n%6.2f\n", p->z);
}

static void
dxf_footer(void)
{
   fprintf(fh, "000\nENDSEC\n");
   fprintf(fh, "000\nEOF\n");
}

static void
sketch_header(void)
{
   fprintf(fh, "##Sketch 1 2\n"); /* Sketch file version */
   fprintf(fh, "document()\n");
   fprintf(fh, "layout((%.3f,%.3f),0)\n",
	   (max_x - min_x) * factor, (max_y - min_y) * factor);
}

static const char *layer_names[] = {
   NULL,
   "Legs",
   "Stations",
   NULL,
   "Labels"
};

static void
sketch_start_pass(int layer)
{
   fprintf(fh, "layer('%s',1,1,0,0,(0,0,0))\n", layer_names[layer]);
}

static void
sketch_move(const img_point *p)
{
   fprintf(fh, "b()\n");
   fprintf(fh, "bs(%.3f,%.3f,%.3f)\n", p->x * factor, p->y * factor, 0.0);
}

static void
sketch_line(const img_point *p1, const img_point *p, bool fSurface)
{
   fSurface = fSurface; /* unused */
   p1 = p1; /* unused */
   fprintf(fh, "bs(%.3f,%.3f,%.3f)\n", p->x * factor, p->y * factor, 0.0);
}

static void
sketch_label(const img_point *p, const char *s, bool fSurface)
{
   fSurface = fSurface; /* unused */
   fprintf(fh, "fp((0,0,0))\n");
   fprintf(fh, "le()\n");
   fprintf(fh, "Fn('Times-Roman')\n");
   fprintf(fh, "Fs(5)\n");
   fprintf(fh, "txt('");
   while (*s) {
      int ch = *s++;
      if (ch == '\'' || ch == '\\') putc('\\', fh);
      putc(ch, fh);
   }
   fprintf(fh, "',(%.3f,%.3f))\n", p->x * factor, p->y * factor);
}

static void
sketch_cross(const img_point *p, bool fSurface)
{
   fSurface = fSurface; /* unused */
   fprintf(fh, "b()\n");
   fprintf(fh, "bs(%.3f,%.3f,%.3f)\n",
	   p->x * factor - MARKER_SIZE, p->y * factor - MARKER_SIZE, 0.0);
   fprintf(fh, "bs(%.3f,%.3f,%.3f)\n",
	   p->x * factor + MARKER_SIZE, p->y * factor + MARKER_SIZE, 0.0);
   fprintf(fh, "bn()\n");
   fprintf(fh, "bs(%.3f,%.3f,%.3f)\n",
	   p->x * factor + MARKER_SIZE, p->y * factor - MARKER_SIZE, 0.0);
   fprintf(fh, "bs(%.3f,%.3f,%.3f)\n",
	   p->x * factor - MARKER_SIZE, p->y * factor + MARKER_SIZE, 0.0);
}

static void
sketch_footer(void)
{
   fprintf(fh, "guidelayer('Guide Lines',1,0,0,1,(0,0,1))\n");
   if (grid) {
      fprintf(fh, "grid((0,0,%.3f,%.3f),1,(0,0,1),'Grid')\n",
	      grid * factor, grid * factor);
   }
}

typedef struct point {
   img_point p;
   const char *label;
   struct point *next;
} point;

#define HTAB_SIZE 0x2000

static point **htab;

static void
set_name(const img_point *p, const char *s)
{
   int hash;
   point *pt;
   union {
      char data[sizeof(int) * 3];
      int x[3];
   } u;

   u.x[0] = (int)(p->x * 100);
   u.x[1] = (int)(p->y * 100);
   u.x[2] = (int)(p->z * 100);
   hash = (hash_data(u.data, sizeof(int) * 3) & (HTAB_SIZE - 1));
   for (pt = htab[hash]; pt; pt = pt->next) {
      if (pt->p.x == p->x && pt->p.y == p->y && pt->p.z == p->z) {
	 /* already got name for these coordinates */
	 /* FIXME: what about multiple names for the same station? */
	 return;
      }
   }

   pt = osnew(point);
   pt->label = osstrdup(s);
   pt->p = *p;
   pt->next = htab[hash];
   htab[hash] = pt;

   return;
}

static const char *
find_name(const img_point *p)
{
   int hash;
   point *pt;
   union {
      char data[sizeof(int) * 3];
      int x[3];
   } u;
   SVX_ASSERT(p);

   u.x[0] = (int)(p->x * 100);
   u.x[1] = (int)(p->y * 100);
   u.x[2] = (int)(p->z * 100);
   hash = (hash_data(u.data, sizeof(int) * 3) & (HTAB_SIZE - 1));
   for (pt = htab[hash]; pt; pt = pt->next) {
      if (pt->p.x == p->x && pt->p.y == p->y && pt->p.z == p->z)
	 return pt->label;
   }
   return "?";
}

static bool to_close = 0;
static bool close_g = 0;

static void
svg_header(void)
{
   size_t i;
   htab = (point **)osmalloc(HTAB_SIZE * ossizeof(point *));
   for (i = 0; i < HTAB_SIZE; ++i) htab[i] = NULL;
   fprintf(fh, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
   fprintf(fh, "<svg width=\"%.3f%s\" height=\"%.3f%s\""
               " viewBox=\"0 0 %0.3f %0.3f\">\n",
           (max_x - min_x) * factor, unit, (max_y - min_y) * factor, unit,
           (max_x - min_x) * factor, (max_y - min_y) * factor );
   fprintf(fh, "<g transform=\"translate(%.3f %.3f)\">\n",
           min_x * -factor, max_y * factor);
   to_close = 0;
   close_g = 0;
}

static void
svg_start_pass(int layer)
{
   if (to_close) {
      fprintf(fh, "\"/>\n");
      to_close = 0;
   }
   if (close_g) {
      fprintf(fh, "</g>\n");
   }
   close_g = 1;

   fprintf(fh, "<g id=\"%s\"", layer_names[layer]);
   if (layer & LEGS)
      fprintf(fh, " style=\"stroke:black;fill:none;stroke-width:0.4\"");
   else if (layer & STNS)
      fprintf(fh, " style=\"stroke:black;fill:none;stroke-width:0.05\"");
   else if (layer & LABELS)
      fprintf(fh, " style=\"font-size:%.3f\"", text_height);
   fprintf(fh, ">\n");
}

static void
svg_move(const img_point *p)
{
   if (to_close) {
      fprintf(fh, "\"/>\n");
   }
   fprintf(fh, "<path d=\"M%.3f %.3f", p->x * factor, p->y * -factor);
   to_close = 1;
}

static void
svg_line(const img_point *p1, const img_point *p, bool fSurface)
{
   fSurface = fSurface; /* unused */
   p1 = p1; /* unused */
   fprintf(fh, "L%.3f %.3f", p->x * factor, p->y * -factor);
   to_close = 1;
}

static void
svg_label(const img_point *p, const char *s, bool fSurface)
{
   fSurface = fSurface; /* unused */
   fprintf(fh, "<text transform=\"translate(%.3f %.3f)\">",
           p->x * factor, p->y * -factor);
   fprintf(fh, s);
   fprintf(fh, "</text>\n");
   set_name(p, s);
}

static void
svg_cross(const img_point *p, bool fSurface)
{
   fSurface = fSurface; /* unused */
   fprintf(fh, "<circle id=\"%s\" cx=\"%.3f\" cy=\"%.3f\" r=\"%.3f\"/>\n",
           find_name(p), p->x * factor, p->y * -factor, MARKER_SIZE * SQRT_2);
   fprintf(fh, "<path d=\"M%.3f %.3fL%.3f %.3fM%.3f %.3fL%.3f %.3f\"/>\n",
	   p->x * factor - MARKER_SIZE, p->y * -factor - MARKER_SIZE,
	   p->x * factor + MARKER_SIZE, p->y * -factor + MARKER_SIZE,
	   p->x * factor + MARKER_SIZE, p->y * -factor - MARKER_SIZE,
	   p->x * factor - MARKER_SIZE, p->y * -factor + MARKER_SIZE );
}

static void
svg_footer(void)
{
   if (to_close) {
      fprintf(fh, "\"/>\n");
      to_close = 0;
   }
   if (close_g) {
      fprintf(fh, "</g>\n");
   }
   close_g = 1;
   fprintf(fh, "</g>\n</svg>");
}

static void
plt_header(void)
{
   size_t i;
   htab = (point **)osmalloc(HTAB_SIZE * ossizeof(point *));
   for (i = 0; i < HTAB_SIZE; ++i) htab[i] = NULL;
   /* Survex is E, N, Alt - PLT file is N, E, Alt */
   fprintf(fh, "Z %.3f %.3f %.3f %.3f %.3f %.3f\r\n",
           min_y / METRES_PER_FOOT, max_y / METRES_PER_FOOT,
           min_x / METRES_PER_FOOT, max_x / METRES_PER_FOOT,
           min_z / METRES_PER_FOOT, max_z / METRES_PER_FOOT);
   fprintf(fh, "N%s D 1 1 1 C%s\r\n", survey ? survey : "X",
	   survey ? survey : "X"); // Second string is the title
}

static void
plt_start_pass(int layer)
{
   layer = layer;
}

static void
plt_move(const img_point *p)
{
   /* Survex is E, N, Alt - PLT file is N, E, Alt */
   fprintf(fh, "M %.3f %.3f %.3f ",
	   p->y / METRES_PER_FOOT, p->x / METRES_PER_FOOT, p->z / METRES_PER_FOOT);
   /* dummy passage dimensions are required to avoid compass bug */
   fprintf(fh, "S%s P -9 -9 -9 -9\r\n", find_name(p));
}

static void
plt_line(const img_point *p1, const img_point *p, bool fSurface)
{
   fSurface = fSurface; /* unused */
   p1 = p1; /* unused */
   /* Survex is E, N, Alt - PLT file is N, E, Alt */
   fprintf(fh, "D %.3f %.3f %.3f ",
	   p->y / METRES_PER_FOOT, p->x / METRES_PER_FOOT, p->z / METRES_PER_FOOT);
   /* dummy passage dimensions are required to avoid compass bug */
   fprintf(fh, "S%s P -9 -9 -9 -9\r\n", find_name(p));
}

static void
plt_label(const img_point *p, const char *s, bool fSurface)
{
   fSurface = fSurface; /* unused */
   /* FIXME: also ctrl characters - ought to remap them, not give up */
   if (strchr(s, ' ')) {
      fprintf(stderr, "PLT format can't cope with spaces in station names\n");
      exit(1);
   }
   set_name(p, s);
}

static void
plt_cross(const img_point *p, bool fSurface)
{
   fSurface = fSurface; /* unused */
   p = p; /* unused */
}

static void
plt_footer(void)
{
   /* Survex is E, N, Alt - PLT file is N, E, Alt */
   fprintf(fh, "X %.3f %.3f %.3f %.3f %.3f %.3f\r\n",
           min_y / METRES_PER_FOOT, max_y / METRES_PER_FOOT,
           min_x / METRES_PER_FOOT, max_x / METRES_PER_FOOT,
           min_z / METRES_PER_FOOT, max_z / METRES_PER_FOOT);
   /* Yucky DOS "end of textfile" marker */
   putc('\x1a', fh);
}

static int dxf_passes[] = { LEGS|STNS|LABELS, 0 };
static int sketch_passes[] = { LEGS, STNS, LABELS, 0 };
static int plt_passes[] = { LABELS, LEGS, 0 };
static int svg_passes[] = { LEGS, LABELS, STNS, 0 };

typedef enum {
    FMT_DXF = 0, FMT_SVG, FMT_SKETCH, FMT_PLT, FMT_ENDMARKER
} export_format;
static const char *extensions[] = { "dxf", "svg", "sk", "plt" };

bool
Export(const wxString &fnm_out, const MainFrm * mainfrm,
       double pan, double tilt, bool labels, bool crosses, bool legs,
       bool surface)
{
   int fSeenMove = 0;
   img_point p, p1;
   double s = 0, c = 0;
   export_format format = FMT_DXF;
   int *pass;
   bool elevation = (tilt == 90.0);
   double elev_angle = pan;

   void (*header)(void);
   void (*start_pass)(int);
   void (*move)(const img_point *);
   void (*line)(const img_point *, const img_point *, bool);
   void (*label)(const img_point *, const char *, bool);
   void (*cross)(const img_point *, bool);
   void (*footer)(void);
   const char *mode = "w"; /* default to text output */

   /* Defaults */
   grid = 0;
   text_height = TEXT_HEIGHT;
   marker_size = MARKER_SIZE;
#if 0
   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 1, 2);
   while (1) {
      int opt = cmdline_getopt();
      if (opt == EOF) break;
      switch (opt) {
       case 'e': /* Elevation */
	 elevation = 1;
	 elev_angle = cmdline_double_arg();
	 break;
       case 'c': /* Crosses */
	 crosses = 0;
	 break;
       case 'n': /* Labels */
	 labels = 0;
	 break;
       case 'l': /* Legs */
	 legs = 0;
	 break;
       case 'g': /* Grid */
	 if (optarg) {
	    grid = cmdline_double_arg();
	 } else {
	    grid = (double)GRID_SPACING;
	 }
	 break;
       case 'r': /* Reduction factor */
	 scale = cmdline_double_arg();
	 break;
       case 't': /* Text height */
	 text_height = cmdline_double_arg();
#ifdef DEBUG_CAD3D
	 printf("Text Height: `%s' input, converted to %6.2f\n", optarg, text_height);
#endif
	 break;
       case 'm': /* Marker size */
	 marker_size = cmdline_double_arg();
#ifdef DEBUG_CAD3D
	 printf("Marker Size: `%s', converted to %6.2f\n", optarg, marker_size);
#endif
	 break;
       case 'D':
	 format = FMT_DXF;
	 break;
       case 'S':
	 format = FMT_SKETCH;
	 break;
       case 'P':
	 format = FMT_PLT;
	 break;
       case 'V':
         format = FMT_SVG;
         break;
       case 's':
	 survey = optarg;
	 break;
#ifdef DEBUG_CAD3D
       default:
	 printf("Internal Error: 'getopt' returned '%c' %d\n", opt, opt);
#endif
      }
   }
#endif

   {
      size_t i;
      size_t len = fnm_out.length();
      for (i = 0; i < FMT_ENDMARKER; ++i) {
	 size_t l = strlen(extensions[i]);
	 if (len > l + 1 && fnm_out[len - l - 1] == FNM_SEP_EXT &&
	     strcasecmp(fnm_out.c_str() + len - l, extensions[i]) == 0) {
	    format = export_format(i);
	    break;
	 }
      }
   }
   switch (format) {
    case FMT_DXF:
      header = dxf_header;
      start_pass = dxf_start_pass;
      move = dxf_move;
      line = dxf_line;
      label = dxf_label;
      cross = dxf_cross;
      footer = dxf_footer;
      pass = dxf_passes;
      break;
    case FMT_SKETCH:
      header = sketch_header;
      start_pass = sketch_start_pass;
      move = sketch_move;
      line = sketch_line;
      label = sketch_label;
      cross = sketch_cross;
      footer = sketch_footer;
      pass = sketch_passes;
      factor = POINTS_PER_MM * 1000.0 / scale;
      mode = "wb"; /* Binary file output */
      break;
    case FMT_PLT:
      header = plt_header;
      start_pass = plt_start_pass;
      move = plt_move;
      line = plt_line;
      label = plt_label;
      cross = plt_cross;
      footer = plt_footer;
      pass = plt_passes;
      mode = "wb"; /* Binary file output */
      break;
    case FMT_SVG:
      header = svg_header;
      start_pass = svg_start_pass;
      move = svg_move;
      line = svg_line;
      label = svg_label;
      cross = svg_cross;
      footer = svg_footer;
      pass = svg_passes;
      factor = 1000.0 / scale;
      mode = "wb"; /* Binary file output */
      break;
    default:
      exit(1);
   }

   fh = fopen(fnm_out.c_str(), mode);
   if (!fh) return false;

   if (elevation) {
      s = sin(rad(elev_angle));
      c = cos(rad(elev_angle));
   }

   /* Get drawing corners */
   min_x = min_y = min_z = HUGE_VAL;
   max_x = max_y = max_z = -HUGE_VAL;
   for (int band = 0; band < mainfrm->GetNumDepthBands(); ++band) {
	list<PointInfo*>::const_iterator pos = mainfrm->GetPoints(band);
	list<PointInfo*>::const_iterator end = mainfrm->GetPointsEnd(band);
	for ( ; pos != end; ++pos) {
	    p.x = (*pos)->GetX();
	    p.y = (*pos)->GetY();
	    p.z = (*pos)->GetZ();

	    if (elevation) {
		double xnew = p.x * c - p.y * s;
		double znew = - p.x * s - p.y * c;
		p.y = p.z;
		p.z = znew;
		p.x = xnew;
	    }

	    if (p.x < min_x) min_x = p.x;
	    if (p.x > max_x) max_x = p.x;
	    if (p.y < min_y) min_y = p.y;
	    if (p.y > max_y) max_y = p.y;
	    if (p.z < min_z) min_z = p.z;
	    if (p.z > max_z) max_z = p.z;
	}
   }
   {
	list<LabelInfo*>::const_iterator pos = mainfrm->GetLabels();
	list<LabelInfo*>::const_iterator end = mainfrm->GetLabelsEnd();
	for ( ; pos != end; ++pos) {
	    p.x = (*pos)->GetX();
	    p.y = (*pos)->GetY();
	    p.z = (*pos)->GetZ();

	    if (elevation) {
		double xnew = p.x * c - p.y * s;
		double znew = - p.x * s - p.y * c;
		p.y = p.z;
		p.z = znew;
		p.x = xnew;
	    }

	    if (p.x < min_x) min_x = p.x;
	    if (p.x > max_x) max_x = p.x;
	    if (p.y < min_y) min_y = p.y;
	    if (p.y > max_y) max_y = p.y;
	    if (p.z < min_z) min_z = p.z;
	    if (p.z > max_z) max_z = p.z;
	}
   }

   if (grid > 0) {
      min_x -= grid / 2;
      max_x += grid / 2;
      min_y -= grid / 2;
      max_y += grid / 2;
   }

   /* handle empty file gracefully */
   if (min_x > max_x) {
      min_x = min_y = min_z = 0;
      max_x = max_y = max_z = 0;
   }

   /* Header */
   header();

   p1.x = p1.y = p1.z = 0; /* avoid compiler warning */

   while (*pass) {
      bool legs_this_pass = ((*pass & LEGS) && legs);
      bool crosses_this_pass = ((*pass & STNS) && crosses);
      bool labels_this_pass = ((*pass & LABELS) && labels);
      if (legs_this_pass || crosses_this_pass || labels_this_pass) {
	  start_pass(*pass);
      }
      if (legs_this_pass) {
	 for (int band = 0; band < mainfrm->GetNumDepthBands(); ++band) {
	     list<PointInfo*>::const_iterator pos = mainfrm->GetPoints(band);
	     list<PointInfo*>::const_iterator end = mainfrm->GetPointsEnd(band);
	     for ( ; pos != end; ++pos) {
		 p.x = (*pos)->GetX();
		 p.y = (*pos)->GetY();
		 p.z = (*pos)->GetZ();

		 if (format == FMT_SKETCH) {
		     p.x -= min_x;
		     p.y -= min_y;
		     p.z -= min_z;
		 }

		 if (elevation) {
		     double xnew = p.x * c - p.y * s;
		     double znew = - p.x * s - p.y * c;
		     p.y = p.z;
		     p.z = znew;
		     p.x = xnew;
		 }

		 if ((*pos)->IsLine()) {
#ifdef DEBUG_CAD3D
		     printf("line to %9.2f %9.2f %9.2f\n", p.x, p.y, p.z);
#endif
		     assert(fSeenMove);
		     line(&p1, &p, (*pos)->IsSurface());
		 } else {
#ifdef DEBUG_CAD3D
		     printf("move to %9.2f %9.2f %9.2f\n",x,y,z);
#endif
		     move(&p);
		     fSeenMove = 1;
		 }
		 p1 = p;
	     }
	 }
      }
      if (crosses_this_pass || labels_this_pass) {
	list<LabelInfo*>::const_iterator pos = mainfrm->GetLabels();
	list<LabelInfo*>::const_iterator end = mainfrm->GetLabelsEnd();
	for ( ; pos != end; ++pos) {
	    p.x = (*pos)->GetX();
	    p.y = (*pos)->GetY();
	    p.z = (*pos)->GetZ();

	    if (format == FMT_SKETCH) {
	       p.x -= min_x;
	       p.y -= min_y;
	       p.z -= min_z;
	    }

	    if (elevation) {
		double xnew = p.x * c - p.y * s;
		double znew = - p.x * s - p.y * c;
		p.y = p.z;
		p.z = znew;
		p.x = xnew;
	    }
#ifdef DEBUG_CAD3D
	    printf("label `%s' at %9.2f %9.2f %9.2f\n",(*pos)->GetText(),x,y,z);
#endif
	    /* Use !UNDERGROUND as the criterion - we want stations where
	     * a surface and underground survey meet to be in the
	     * underground layer */
	    if (labels_this_pass)
		label(&p, (*pos)->GetText(), !(*pos)->IsUnderground());
	    if (crosses_this_pass)
		cross(&p, !(*pos)->IsUnderground());
	}
      }
      pass++;
   }
   footer();
   safe_fclose(fh);
   osfree(htab);
   htab = NULL;
   return true;
}
