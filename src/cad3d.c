/* cad3d.c
 * Converts a .3d file to CAD-like formats (DXF, Sketch) and also Compass PLT.
 * Also useful as an example of how to use the img code in your own programs
 */

/* Copyright (C) 1994-2004 Olly Betts
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

static img *pimg;
static const char *survey = NULL;

static void
dxf_header(void)
{
   fprintf(fh, "0\nSECTION\n");
   fprintf(fh, "2\nHEADER\n");
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

   fprintf(fh, "0\nSECTION\n");
   fprintf(fh, "2\nTABLES\n");
   fprintf(fh, "0\nTABLE\n");
   fprintf(fh, "2\nLAYER\n");
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
   fprintf(fh, "0\nENDTAB\n");
   fprintf(fh, "0\nENDSEC\n");

   fprintf(fh, "0\nSECTION\n");
   fprintf(fh, "2\nENTITIES\n");

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
plt_header(void)
{
   size_t i;
   htab = osmalloc(HTAB_SIZE * ossizeof(point *));
   for (i = 0; i < HTAB_SIZE; ++i) htab[i] = NULL;
   /* Survex is E, N, Alt - PLT file is N, E, Alt */
   fprintf(fh, "Z %.3f %.3f %.3f %.3f %.3f %.3f\r\n",
           min_y / METRES_PER_FOOT, max_y / METRES_PER_FOOT,
           min_x / METRES_PER_FOOT, max_x / METRES_PER_FOOT,
           min_z / METRES_PER_FOOT, max_z / METRES_PER_FOOT);
   fprintf(fh, "N%s D 1 1 1 C%s\r\n", survey ? survey : "X", pimg->title);
}

static void
plt_start_pass(int layer)
{
   layer = layer;
}

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

#define LEGS 1
#define STNS 2
#define LABELS 4
static int dxf_passes[] = { LEGS|STNS|LABELS, 0 };
static int sketch_passes[] = { LEGS, STNS, LABELS, 0 };
static int plt_passes[] = { LABELS, LEGS, 0 };

int
main(int argc, char **argv)
{
   char *fnm_3d, *fnm_out;
   unsigned char labels, crosses, legs;
   int item;
   int fSeenMove = 0;
   img_point p, p1;
   int elevation = 0;
   double elev_angle = 0;
   double s = 0, c = 0;
   enum { FMT_DXF = 0, FMT_SKETCH, FMT_PLT, FMT_AUTO } format;
   static const char *extensions[] = { "dxf", "sk", "plt" };
   int *pass;

   void (*header)(void);
   void (*start_pass)(int);
   void (*move)(const img_point *);
   void (*line)(const img_point *, const img_point *, bool);
   void (*label)(const img_point *, const char *, bool);
   void (*cross)(const img_point *, bool);
   void (*footer)(void);
   const char *mode = "w"; /* default to text output */

   /* TRANSLATE */
   static const struct option long_opts[] = {
	/* const char *name; int has_arg (0 no_argument, 1 required, 2 options_*); int *flag; int val */
	{"survey", required_argument, 0, 's'},
	{"no-crosses", no_argument, 0, 'c'},
	{"no-station-names", no_argument, 0, 'n'},
	{"no-legs", no_argument, 0, 'l'},
	{"grid", optional_argument, 0, 'g'},
	{"text-height", required_argument, 0, 't'},
	{"marker-size", required_argument, 0, 'm'},
	{"elevation", required_argument, 0, 'e'},
	{"reduction", required_argument, 0, 'r'},
	{"dxf", no_argument, 0, 'D'},
	{"sketch", no_argument, 0, 'S'},
	{"plt", no_argument, 0, 'P'},
	{"help", no_argument, 0, HLP_HELP},
	{"version", no_argument, 0, HLP_VERSION},
	{0,0,0,0}
   };

#define short_opts "s:cnlg::t:m:e:r:DSP"

   /* TRANSLATE */
   static struct help_msg help[] = {
	{HLP_ENCODELONG(0), "only load the sub-survey with this prefix"},
	{HLP_ENCODELONG(1), "do not generate station markers"},
	{HLP_ENCODELONG(2), "do not generate station labels"},
	{HLP_ENCODELONG(3), "do not generate the survey legs"},
	{HLP_ENCODELONG(4), "generate grid (default "STRING(GRID_SPACING)"m)"},
	{HLP_ENCODELONG(5), "station labels text height (default "STRING(TEXT_HEIGHT)")"},
	{HLP_ENCODELONG(6), "station marker size (default "STRING(MARKER_SIZE)")"},
	{HLP_ENCODELONG(7), "produce an elevation view"},
	{HLP_ENCODELONG(8), "factor to scale down by (default 500)"},
	{HLP_ENCODELONG(9), "produce DXF output"},
	{HLP_ENCODELONG(10), "produce Sketch output"},
	{HLP_ENCODELONG(11), "produce Compass PLT output for Carto"},
	{0,0}
   };

   msg_init(argv);

   /* Defaults */
   crosses = 1;
   labels = 1;
   legs = 1;
   grid = 0;
   text_height = TEXT_HEIGHT;
   marker_size = MARKER_SIZE;
   format = FMT_AUTO;

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
       case 's':
	 survey = optarg;
	 break;
#ifdef DEBUG_CAD3D
       default:
	 printf("Internal Error: 'getopt' returned '%c' %d\n", opt, opt);
#endif
      }
   }

   fnm_3d = argv[optind++];
   if (argv[optind]) {
      fnm_out = argv[optind];
      if (format == FMT_AUTO) {
	 size_t i;
	 size_t len = strlen(fnm_out);
	 format = FMT_DEFAULT;
	 for (i = 0; i < FMT_AUTO; ++i) {
	    size_t l = strlen(extensions[i]);
	    if (len > l + 1 && fnm_out[len - l - 1] == FNM_SEP_EXT &&
		strcasecmp(fnm_out + len - l, extensions[i]) == 0) {
	       format = i;
	       break;
	    }
	 }
      }
   } else {
      char *baseleaf = baseleaf_from_fnm(fnm_3d);
      if (format == FMT_AUTO) format = FMT_DEFAULT;
      /* note : memory allocated by fnm_out gets leaked in this case... */
      fnm_out = add_ext(baseleaf, extensions[format]);
      osfree(baseleaf);
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
    default:
      exit(1);
   }

   pimg = img_open_survey(fnm_3d, survey);
   if (!pimg) fatalerror(img_error(), fnm_3d);

   fh = safe_fopen(fnm_out, mode);

   if (elevation) {
      s = sin(rad(elev_angle));
      c = cos(rad(elev_angle));
   }

   /* Get drawing corners */
   min_x = min_y = min_z = HUGE_VAL;
   max_x = max_y = max_z = -HUGE_VAL;
   do {
      item = img_read_item(pimg, &p);

      if (elevation) {
	 double xnew = p.x * c - p.y * s;
	 double znew = - p.x * s - p.y * c;
	 p.y = p.z;
	 p.z = znew;
	 p.x = xnew;
      }

      switch (item) {
       case img_MOVE: case img_LINE: case img_LABEL:
	 if (p.x < min_x) min_x = p.x;
	 if (p.x > max_x) max_x = p.x;
	 if (p.y < min_y) min_y = p.y;
	 if (p.y > max_y) max_y = p.y;
	 if (p.z < min_z) min_z = p.z;
	 if (p.z > max_z) max_z = p.z;
	 break;
      }
   } while (item != img_STOP);

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
      if (((*pass & LEGS) && legs) ||
	  ((*pass & STNS) && crosses) ||
	  ((*pass & LABELS) && labels)) {
	 img_rewind(pimg);
	 start_pass(*pass);
	 do {
	    item = img_read_item(pimg, &p);

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

	    switch (item) {
	     case img_BAD:
	       img_close(pimg);
	       fatalerror(/*Bad 3d image file `%s'*/106, fnm_3d);
	       break;
	     case img_LINE:
#ifdef DEBUG_CAD3D
	       printf("line to %9.2f %9.2f %9.2f\n", p.x, p.y, p.z);
#endif
	       if (!fSeenMove) {
#ifdef DEBUG_CAD3D
		  printf("Something is wrong -- img_LINE before any img_MOVE!\n");
#endif
		  img_close(pimg);
		  fatalerror(/*Bad 3d image file `%s'*/106, fnm_3d);
	       }
	       if ((*pass & LEGS) && legs)
		  line(&p1, &p, pimg->flags & img_FLAG_SURFACE);
	       p1 = p;
	       break;
	     case img_MOVE:
#ifdef DEBUG_CAD3D
	       printf("move to %9.2f %9.2f %9.2f\n",x,y,z);
#endif
	       if ((*pass & LEGS) && legs) move(&p);
	       fSeenMove = 1;
	       p1 = p;
	       break;
	     case img_LABEL:
#ifdef DEBUG_CAD3D
	       printf("label `%s' at %9.2f %9.2f %9.2f\n",pimg->label,x,y,z);
#endif
	       /* Use !UNDERGROUND as the criterion - we want stations where
		* a surface and underground survey meet to be in the
		* underground layer */
	       if ((*pass & LABELS) && labels)
		  label(&p, pimg->label,
			!(pimg->flags & img_SFLAG_UNDERGROUND));
	       if ((*pass & STNS) && crosses)
		  cross(&p, !(pimg->flags & img_SFLAG_UNDERGROUND));
	       break;
#ifdef DEBUG_CAD3D
	     case img_STOP:
	       printf("stop\n");
	       break;
	     default:
	       printf("other info tag (code %d) ignored\n",item);
#endif
	    }
	 } while (item != img_STOP);
      }
      pass++;
   }
   img_close(pimg);
   footer();
   safe_fclose(fh);
   return 0;
}
