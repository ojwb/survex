/* > 3dtodxf.c
 * Converts a .3d file to a DXF file, or other CAD-like formats
 * Also useful as an example of how to use the img code in your own programs
 */

/* Copyright (C) 1994-2001 Olly Betts
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

/* #define DEBUG_3DTODXF */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include "img.h"
#include "getopt.h"
#include "useful.h"
#include "cmdline.h"

#include "message.h"

/* default values - can be overridden with --htext and --msize respectively */
#define TEXT_HEIGHT	0.6
#define MARKER_SIZE	0.8

#define GRID_SPACING	100

static FILE *fh;

/* bounds */
static double min_x, min_y, min_z, max_x, max_y, max_z;

static double text_height; /* for station labels */
static double marker_size; /* for station markers */
static double grid; /* grid spacing (or 0 for no grid) */

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
#ifdef DEBUG_3DTODXF
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
dxf_move(img_point p)
{
   p = p;
}

static void
dxf_line(img_point p1, img_point p)
{
   fprintf(fh, "0\nLINE\n");
   fprintf(fh, "8\nCentreLine\n"); /* Layer */
   fprintf(fh, "10\n%6.2f\n", p1.x);
   fprintf(fh, "20\n%6.2f\n", p1.y);
   fprintf(fh, "30\n%6.2f\n", p1.z);
   fprintf(fh, "11\n%6.2f\n", p.x);
   fprintf(fh, "21\n%6.2f\n", p.y);
   fprintf(fh, "31\n%6.2f\n", p.z);
}

static void
dxf_label(img_point p, const char *s)
{
   /* write station label to dxf file */
   fprintf(fh, "0\nTEXT\n");
   fprintf(fh, "8\nLabels\n"); /* Layer */
   fprintf(fh, "10\n%6.2f\n", p.x);
   fprintf(fh, "20\n%6.2f\n", p.y);
   fprintf(fh, "30\n%6.2f\n", p.z);
   fprintf(fh, "40\n%6.2f\n", text_height);
   fprintf(fh, "1\n%s\n", s);
}

static void
dxf_cross(img_point p)
{
   /* write station marker to dxf file */
   fprintf(fh, "0\nPOINT\n");
   fprintf(fh, "8\nStations\n"); /* Layer */
   fprintf(fh, "10\n%6.2f\n", p.x);
   fprintf(fh, "20\n%6.2f\n", p.y);
   fprintf(fh, "30\n%6.2f\n", p.z);
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
   fprintf(fh, "layout((%.3f,%.3f),0)\n", max_x - min_x, max_y - min_y);
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
sketch_move(img_point p)
{
   fprintf(fh, "b()\n");
   fprintf(fh, "bs(%.3f,%.3f,%.3f)\n", p.x, p.y, 0.0);
}

static void
sketch_line(img_point p1, img_point p)
{
   p1 = p1;
   fprintf(fh, "bs(%.3f,%.3f,%.3f)\n", p.x, p.y, 0.0);
}

static void
sketch_label(img_point p, const char *s)
{
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
   fprintf(fh, "',(%.3f,%.3f))\n", p.x, p.y);
}

static void
sketch_cross(img_point p)
{
   fprintf(fh, "b()\n");
   fprintf(fh, "bs(%.3f,%.3f,%.3f)\n",
	   p.x - MARKER_SIZE, p.y - MARKER_SIZE, 0.0);
   fprintf(fh, "bs(%.3f,%.3f,%.3f)\n",
	   p.x + MARKER_SIZE, p.y + MARKER_SIZE, 0.0);
   fprintf(fh, "bn()\n");
   fprintf(fh, "bs(%.3f,%.3f,%.3f)\n",
	   p.x + MARKER_SIZE, p.y - MARKER_SIZE, 0.0);
   fprintf(fh, "bs(%.3f,%.3f,%.3f)\n",
	   p.x - MARKER_SIZE, p.y + MARKER_SIZE, 0.0);
}

static void
sketch_footer(void)
{
   fprintf(fh, "guidelayer('Guide Lines',1,0,0,1,(0,0,1))\n");
   fprintf(fh, "grid((0,0,20,20),0,(0,0,1),'Grid')\n");
}

#define LEGS 1
#define STNS 2
#define LABELS 4
static int dxf_passes[] = { LEGS|STNS|LABELS, 0 };
static int sketch_passes[] = { LEGS, STNS, LABELS, 0 };

int
main(int argc, char **argv)
{
   char *fnm_3d, *fnm_out;
   unsigned char labels, crosses, legs;
   img *pimg;
   int item;
   int fSeenMove = 0;
   img_point p, p1;
   int elevation = 0;
   double elev_angle = 0;
   double s = 0, c = 0;
   const char *ext;
   enum { FMT_DXF, FMT_SKETCH } format;
   int *pass;
   const char *survey = NULL;

   void (*header)(void);
   void (*start_pass)(int);
   void (*move)(img_point);
   void (*line)(img_point, img_point);
   void (*label)(img_point, const char *);
   void (*cross)(img_point);
   void (*footer)(void);

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
        {"dxf", no_argument, 0, 'D'},
        {"sketch", no_argument, 0, 'S'},
	{"help", no_argument, 0, HLP_HELP},
	{"version", no_argument, 0, HLP_VERSION},
	{0,0,0,0}
   };

#define short_opts "s:cnlg:t:m:eDSh"
	
   /* TRANSLATE */
   static struct help_msg help[] = {
	{HLP_ENCODELONG(0), "only load the sub-survey with this prefix"},
	{HLP_ENCODELONG(1), "do not generate station markers"},
	{HLP_ENCODELONG(2), "do not generate station labels"},
	{HLP_ENCODELONG(3), "do not generate the survey legs"},
	{HLP_ENCODELONG(4), "generate grid (default: "STRING(GRID_SPACING)"m)"},
	{HLP_ENCODELONG(5), "station labels text height (default: "STRING(TEXT_HEIGHT)")"},
	{HLP_ENCODELONG(6), "station marker size (default: "STRING(MARKER_SIZE)")"},
	{HLP_ENCODELONG(7), "produce an elevation view"},
	{HLP_ENCODELONG(8), "produce DXF output"},
	{HLP_ENCODELONG(9), "produce sketch output"},
	{0,0}
   };

   msg_init(argv[0]);

   /* Defaults */
   crosses = 1;
   labels = 1;
   legs = 1;
   grid = 0;
   text_height = TEXT_HEIGHT;
   marker_size = MARKER_SIZE;
   format = FMT_DXF;

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
	 if (optarg)
	    grid = cmdline_double_arg();
	 else
	    grid = GRID_SPACING;
	 break;
       case 't': /* Text height */
	 text_height = cmdline_double_arg();
#ifdef DEBUG_3DTODXF
	 printf("Text Height: `%s' input, converted to %6.2f\n", optarg, text_height);
#endif
	 break;
       case 'm': /* Marker size */
	 marker_size = cmdline_double_arg();
#ifdef DEBUG_3DTODXF
	 printf("Marker Size: `%s', converted to %6.2f\n", optarg, marker_size);
#endif
	 break;
       case 'D':
	 format = FMT_DXF;
	 break;
       case 'S':
	 format = FMT_SKETCH;
	 break;
       case 's':
	 survey = optarg;
	 break;
#ifdef DEBUG_3DTODXF
       default:
	 printf("Internal Error: 'getopt' returned '%c' %d\n", opt, opt);
#endif
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
      ext = "dxf";
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
      ext = "sk";
      pass = sketch_passes;
      break;
    default:
      exit(1);
   }

   fnm_3d = argv[optind++];
   if (argv[optind]) {
      fnm_out = argv[optind];
   } else {
      char *base = base_from_fnm(fnm_3d);
      fnm_out = add_ext(base, ext);
      osfree(base);
   }

   pimg = img_open_survey(fnm_3d, NULL, NULL, survey);
   if (!pimg) fatalerror(img_error(), fnm_3d);

   fh = safe_fopen(fnm_out, "w");

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
#ifdef DEBUG_3DTODXF
	       printf("line to %9.2f %9.2f %9.2f\n", p.x, p.y, p.z);
#endif
	       if (!fSeenMove) {
#ifdef DEBUG_3DTODXF
		  printf("Something is wrong -- img_LINE before any img_MOVE!\n");
#endif
		  img_close(pimg);
		  fatalerror(/*Bad 3d image file `%s'*/106, fnm_3d);
	       }
	       if ((*pass & LEGS) && legs) line(p1, p);
	       p1 = p;
	       break;
	     case img_MOVE:
#ifdef DEBUG_3DTODXF
	       printf("move to %9.2f %9.2f %9.2f\n",x,y,z);
#endif
	       if ((*pass & LEGS) && legs) move(p);
	       fSeenMove = 1;
	       p1 = p;
	       break;
	     case img_LABEL:
#ifdef DEBUG_3DTODXF
	       printf("label `%s' at %9.2f %9.2f %9.2f\n",pimg->label,x,y,z);
#endif
	       if ((*pass & LABELS) && labels) label(p, pimg->label);
	       if ((*pass & STNS) && crosses) cross(p);
	       break;
#ifdef DEBUG_3DTODXF
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
