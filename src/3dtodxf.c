/* > 3dtodxf.c */

/* Converts a .3d file to a DXF file */
/* Also useful as an example of how to use the img code in your own programs */

/* Copyright (C) 1994-2000 Olly Betts
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

/* Tell img.h to work in standalone mode */
/* #define STANDALONE */

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

#ifndef STANDALONE
#include "message.h"
#endif

/* default values - can be overridden with --htext and --msize respectively */
#define TEXT_HEIGHT	0.6
#define MARKER_SIZE	0.8

#define GRID_SPACING	100

int
main(int argc, char **argv)
{
   char szTitle[256], szDateStamp[256], szName[256];
   char *fnm3D, *fnmDXF;
   unsigned char labels, crosses, legs;
   img *pimg;
   FILE *fh;
   int item;
   int fSeenMove = 0;
   float x, y, z;
   double x1, y1, z1;
   double min_x, min_y, min_z, max_x, max_y, max_z; /* for HEADER section */
   double text_height; /* for station labels */
   double marker_size; /* for station markers */
   double grid; /* grid spacing (or 0 for no grid) */
   int elevation = 0;
   double elev_angle = 0;
   double s = 0, c = 0;

   /* TRANSLATE */
   static const struct option long_opts[] = {
	/* const char *name; int has_arg (0 no_argument, 1 required, 2 options_*); int *flag; int val */
	{"no-crosses", no_argument, 0, 'c'},
	{"no-station-names", no_argument, 0, 'n'},
	{"no-legs", no_argument, 0, 'l'},
	{"grid", optional_argument, 0, 'g'},
	{"text-height", required_argument, 0, 't'},
	{"marker-size", required_argument, 0, 'm'},
	{"elevation", required_argument, 0, 'e'},
	{"htext", required_argument, 0, 't'},
	{"msize", required_argument, 0, 'm'},
	{"help", no_argument, 0, HLP_HELP},
	{"version", no_argument, 0, HLP_VERSION},
	{0,0,0,0}
   };

#define short_opts "cnlg:t:m:h"

   /* TRANSLATE */
   static struct help_msg help[] = {
	{HLP_ENCODELONG(0), "do not generate station markers"},
	{HLP_ENCODELONG(1), "do not generate station labels"},
	{HLP_ENCODELONG(2), "do not generate the survey legs"},
	{HLP_ENCODELONG(3), "generate grid (default: "STRING(GRID_SPACING)"m)"},
	{HLP_ENCODELONG(4), "station labels text height (default: "STRING(TEXT_HEIGHT)")"},
	{HLP_ENCODELONG(5), "station marker size (default: "STRING(MARKER_SIZE)")"},
	{HLP_ENCODELONG(6), "produce an elevation view"},
	{0,0}
   };

#ifndef STANDALONE
   msg_init(argv[0]);
#endif

   /* Defaults */
   crosses = 1;
   labels = 1;
   legs = 1;
#ifndef XXX
   grid = 0;
#endif /* not XXX */
   text_height = TEXT_HEIGHT;
   marker_size = MARKER_SIZE;

#ifndef XXX
   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 1, 2);
#else /* XXX */
   /* exactly two arguments must be given */
   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 2, 2);
#endif /* XXX */
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
	 printf("Text Height: '%s' input, converted to %6.2f\n", optarg, text_height);
#endif
	 break;
       case 'm': /* Marker size */
	 marker_size = cmdline_double_arg();
#ifdef DEBUG_3DTODXF
	 printf("Marker Size: '%s', converted to %6.2f\n", optarg, marker_size);
#endif
	 break;
#ifdef DEBUG_3DTODXF
       default:
	 printf("Internal Error: 'getopt' returned '%c' %d\n", opt, opt);
#endif
      }
   }

   fnm3D = argv[optind++];
   if (argv[optind]) {
      fnmDXF = argv[optind];
   } else {
      char *base = base_from_fnm(fnm3D);
      fnmDXF = add_ext(base, "dxf");
      osfree(base);
   }

   pimg = img_open(fnm3D, szTitle, szDateStamp);
   if (!pimg) {
#ifndef STANDALONE
      fatalerror(img_error(), fnm3D);
#else
      printf("Bad .3d file\n");
      exit(1);
#endif
   }

   fh = safe_fopen(fnmDXF, "w");

#ifdef DEBUG_3DTODXF
   printf("3d file title `%s'\n",szTitle);
   printf("Creation time `%s'\n",szDateStamp);
#endif

   if (elevation) {
       s = sin(rad(elev_angle));
       c = cos(rad(elev_angle));
   }

   /* Get drawing corners */
   min_x = min_y = min_z = FLT_MAX;
   max_x = max_y = max_z = -FLT_MAX;
   do {
      item = img_read_datum(pimg, szName, &x, &y, &z);

      if (elevation) {
	  double xnew = x * c - y * s;
	  double znew = - x * s - y * c;
	  y = z;
	  z = znew;
	  x = xnew;
      }

      switch (item) {
       case img_MOVE: case img_LINE: case img_LABEL:
         if (x < min_x) min_x = x;
         if (x > max_x) max_x = x;
         if (y < min_y) min_y = y;
         if (y > max_y) max_y = y;
         if (z < min_z) min_z = z;
         if (z > max_z) max_z = z;
         break;
      }
   } while (item != img_STOP);
   img_close(pimg);

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

   pimg = img_open(fnm3D, szTitle, szDateStamp);

   /* Header */
   fprintf(fh,"0\nSECTION\n");
   fprintf(fh,"2\nHEADER\n");
   fprintf(fh,"9\n$EXTMIN\n"); /* lower left corner of drawing */
   fprintf(fh,"10\n%#-.6f\n", min_x); /* x */
   fprintf(fh,"20\n%#-.6f\n", min_y); /* y */
   fprintf(fh,"30\n%#-.6f\n", min_z); /* min z */
   fprintf(fh,"9\n$EXTMAX\n"); /* upper right corner of drawing */
   fprintf(fh,"10\n%#-.6f\n", max_x); /* x */
   fprintf(fh,"20\n%#-.6f\n", max_y); /* y */
   fprintf(fh,"30\n%#-.6f\n", max_z); /* max z */
   fprintf(fh,"9\n$PDMODE\n70\n3\n"); /* marker style as CROSS */
   fprintf(fh,"9\n$PDSIZE\n40\n%6.2f\n", marker_size); /* marker size */
   fprintf(fh,"0\nENDSEC\n");

   fprintf(fh,"0\nSECTION\n");
   fprintf(fh,"2\nTABLES\n");
   fprintf(fh,"0\nTABLE\n");
   fprintf(fh,"2\nLAYER\n");
   fprintf(fh,"70\n10\n"); /* max # off layers in this DXF file : 10 */
   /* First Layer: CentreLine */
   fprintf(fh,"0\nLAYER\n2\nCentreLine\n");
   fprintf(fh,"70\n64\n"); /* shows layer is referenced by entities */
   fprintf(fh,"62\n5\n"); /* color: kept the same used by SpeleoGen */
   fprintf(fh,"6\nCONTINUOUS\n"); /* linetype */
   /* Next Layer: Stations */
   fprintf(fh,"0\nLAYER\n2\nStations\n");
   fprintf(fh,"70\n64\n"); /* shows layer is referenced by entities */
   fprintf(fh,"62\n7\n"); /* color: kept the same used by SpeleoGen */
   fprintf(fh,"6\nCONTINUOUS\n"); /* linetype */
   /* Next Layer: Labels */
   fprintf(fh,"0\nLAYER\n2\nLabels\n");
   fprintf(fh,"70\n64\n"); /* shows layer is referenced by entities */
   fprintf(fh,"62\n7\n"); /* color: kept the same used by SpeleoGen */
   fprintf(fh,"6\nCONTINUOUS\n"); /* linetype */
   if (grid > 0) {
      /* Next Layer: Grid */
      fprintf(fh,"0\nLAYER\n2\nGrid\n");
      fprintf(fh,"70\n64\n"); /* shows layer is referenced by entities */
      fprintf(fh,"62\n7\n"); /* color: kept the same used by SpeleoGen */
      fprintf(fh,"6\nCONTINUOUS\n"); /* linetype */
   }
   fprintf(fh,"0\nENDTAB\n");
   fprintf(fh,"0\nENDSEC\n");

   fprintf(fh,"0\nSECTION\n");
   fprintf(fh,"2\nENTITIES\n");

   if (grid > 0) {
      x1 = floor(min_x / grid) * grid + grid;
      y1 = floor(min_y / grid) * grid + grid;
#ifdef DEBUG_3DTODXF
      printf("x_min: %d  y_min: %d\n", (int)x1, (int)y1);
#endif
      while (x1 < max_x) {
	 /* horizontal line */
         fprintf(fh,"0\nLINE\n");
         fprintf(fh,"8\nGrid\n"); /* Layer */
         fprintf(fh,"10\n%6.2f\n",x1);
         fprintf(fh,"20\n%6.2f\n",min_y);
         fprintf(fh,"30\n0\n");
         fprintf(fh,"11\n%6.2f\n",x1);
         fprintf(fh,"21\n%6.2f\n",max_y);
         fprintf(fh,"31\n0\n");
	 x1 += grid;
      }
      while (y1 < max_y) {
         /* vertical line */
         fprintf(fh,"0\nLINE\n");
         fprintf(fh,"8\nGrid\n"); /* Layer */
         fprintf(fh,"10\n%6.2f\n",min_x);
         fprintf(fh,"20\n%6.2f\n",y1);
         fprintf(fh,"30\n0\n");
         fprintf(fh,"11\n%6.2f\n",max_x);
         fprintf(fh,"21\n%6.2f\n",y1);
         fprintf(fh,"31\n0\n");
	 y1 += grid;
      }
   }

   x1 = y1 = z1 = 0; /* avoid compiler warning */

   do {
      item = img_read_datum(pimg, szName, &x, &y, &z);

      if (elevation) {
	  double xnew = x * c - y * s;
	  double znew = - x * s - y * c;
	  y = z;
	  z = znew;
	  x = xnew;
      }

      switch (item) {
       case img_BAD:
#ifndef STANDALONE
         img_close(pimg);
         fatalerror(/*Bad 3d image file '%s'*/106, fnm3D);
#else
         printf("Bad .3d image file\n");
         exit(1);
#endif
	 break;
       case img_LINE:
#ifdef DEBUG_3DTODXF
         printf("line to %9.2f %9.2f %9.2f\n",x,y,z);
#endif
         if (!fSeenMove) {
#ifdef DEBUG_3DTODXF
            printf("Something is wrong -- img_LINE before any img_MOVE!\n"); /* <<<<<<< create message in messages.txt ? */
#endif
#ifndef STANDALONE
	    img_close(pimg);
	    fatalerror(/*Bad 3d image file '%s'*/106, fnm3D);
#else
	    printf("Bad .3d image file\n");
	    exit(1);
#endif
         }
	 if (legs) {
            fprintf(fh,"0\nLINE\n");
            fprintf(fh,"8\nCentreLine\n"); /* Layer */
            fprintf(fh,"10\n%6.2f\n",x1);
            fprintf(fh,"20\n%6.2f\n",y1);
            fprintf(fh,"30\n%6.2f\n",z1);
            fprintf(fh,"11\n%6.2f\n",x);
            fprintf(fh,"21\n%6.2f\n",y);
            fprintf(fh,"31\n%6.2f\n",z);
	 }
         x1=x; y1=y; z1=z;
         break;
       case img_MOVE:
#ifdef DEBUG_3DTODXF
         printf("move to %9.2f %9.2f %9.2f\n",x,y,z);
#endif
         fSeenMove = 1;
         x1=x; y1=y; z1=z;
         break;
#ifdef DEBUG_3DTODXF
       case img_CROSS:
         printf("cross at %9.2f %9.2f %9.2f\n",x,y,z);
         break;
#endif
       case img_LABEL:
#ifdef DEBUG_3DTODXF
	 printf("label `%s' at %9.2f %9.2f %9.2f\n",szName,x,y,z);
#endif
	 if (labels) {
	    /* write station labels to dxf file */
            fprintf(fh,"0\nTEXT\n");
            fprintf(fh,"8\nLabels\n"); /* Layer */
            fprintf(fh,"10\n%6.2f\n",x);
            fprintf(fh,"20\n%6.2f\n",y);
            fprintf(fh,"30\n%6.2f\n",z);
            fprintf(fh,"40\n%6.2f\n", text_height);
            fprintf(fh,"1\n%s\n",szName);
	 }
         if (crosses) {
	    /* write station markers to dxf file */
            fprintf(fh,"0\nPOINT\n");
            fprintf(fh,"8\nStations\n"); /* Layer */
            fprintf(fh,"10\n%6.2f\n",x);
            fprintf(fh,"20\n%6.2f\n",y);
            fprintf(fh,"30\n%6.2f\n",z);
         }
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
   img_close(pimg);
   fprintf(fh,"000\nENDSEC\n");
   fprintf(fh,"000\nEOF\n");
   fclose(fh);
   return 0;
}
