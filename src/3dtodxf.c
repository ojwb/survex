/* > 3dtodxf.c */

/* Converts a .3d file to a DXF file */
/* Also useful as an example of how to use the img code in your own programs */

/* Copyright (C) 1994,1995 Olly Betts */

/* Tell img.h to work in standalone mode */
/* #define STANDALONE */

/* #define DEBUG_3DTODXF */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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

int main( int argc, char *argv[] ) {
   char szTitle[256], szDateStamp[256], szName[256];
   char *fnm3D, *fnmDXF;
   unsigned char labels, crosses, legs;
   img *pimg;
   FILE *fh;
   int item;
   int fSeenMove=0;
   float x, y, z;
   float x1, y1, z1;
   float min_x, min_y, min_z, max_x, max_y, max_z; /* for HEADER section */
   double text_height; /* for station labels */
   double marker_size; /* for station markers */

   /* FIXME TRANSLATE */
   static const struct option long_opts[] = {
	/* const char *name; int has_arg (0 no_argument, 1 required, 2 options_*); int *flag; int val */
	{"no-crosses", no_argument, 0, 'c'},
	{"no-station-names", no_argument, 0, 'n'},
	{"no-legs", no_argument, 0, 'l'},
	{"text-height", required_argument, 0, 't'},
	{"marker-size", required_argument, 0, 'm'},
	{"htext", required_argument, 0, 't'},
	{"msize", required_argument, 0, 'm'},
	{"help", no_argument, 0, HLP_HELP},
	{"version", no_argument, 0, HLP_VERSION},
	{0,0,0,0}
   };

#define short_opts "cnlt:m:h"

   /* FIXME TRANSLATE */
   static struct help_msg help[] = {
	{HLP_ENCODELONG(0), "do not generate station markers"},
	{HLP_ENCODELONG(1), "do not generate station labels"},
	{HLP_ENCODELONG(2), "do not generate the survey legs"},
	{HLP_ENCODELONG(3), "station labels text height (default: 0.6)"},
	{HLP_ENCODELONG(4), "station marker size (default: 0.8)"},
	{0,0} 
   };

#ifndef STANDALONE
   msg_init(argv[0]);
#endif

   /* Defaults */
   crosses = 1;
   labels = 1;
   legs = 1;
   text_height = TEXT_HEIGHT;
   marker_size = MARKER_SIZE;

   /* exactly two arguments must be given */
   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 2, 2);
   while (1) {      
      int opt = cmdline_getopt();
      if (opt == EOF) break;
      switch (opt) {
       case 'c': /* Crosses */
         crosses = 0;
         break;
       case 'n': /* Labels */
	 labels = 0;
	 break;
       case 'l': /* Legs */
	 legs = 0;
	 break;
       case 't': /* Text height */
	 text_height = cmdline_float_arg();
#ifdef DEBUG_3DTODXF
	 printf("Text Height: '%s' input, converted to %6.2f\n", optarg, text_height);
#endif
	 break;
       case 'm': /* Marker size */
	 marker_size = cmdline_float_arg();
#ifdef DEBUG_3DTODXF
	 printf("Marker Size: '%s', converted to %6.2f\n", optarg, marker_size);
#endif
	 break;
#ifdef DEBUG_3DTODXF
       default:
	 printf("Internal Error: 'getopt' returned %c %d\n",opt, opt);
#endif
      }
   } 	 

   fnm3D=argv[optind++];
   fnmDXF=argv[optind++];

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

   /* Get drawing corners */
   min_x = min_y = min_z = FLT_MAX;
   max_x = max_y = max_z = FLT_MIN;
   do {
      item = img_read_datum(pimg, szName, &x, &y, &z);
      switch (item) {
       case img_MOVE: case img_LINE: case img_CROSS:
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
   fprintf(fh,"0\nENDTAB\n");
   fprintf(fh,"0\nENDSEC\n");

   fprintf(fh,"0\nSECTION\n");
   fprintf(fh,"2\nENTITIES\n");

   x1 = y1 = z1 = 0; /* avoid compiler warning */

   do {
      item = img_read_datum(pimg, szName, &x, &y, &z);
      switch (item) {
       case img_BAD:
#ifndef STANDALONE
         img_close(pimg);
         fatalerror(/*bad 3d file*/106,fnm3D);
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
            printf("Something is wrong -- img_LINE before any img_MOVE!\n"); /* <<<<<<< create message in messages.txt ? */
            img_close(pimg);
            exit(1);
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
       case img_CROSS:
#ifdef DEBUG_3DTODXF
         printf("cross at %9.2f %9.2f %9.2f\n",x,y,z);
#endif
         break;
       case img_LABEL:
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
#ifdef DEBUG_3DTODXF
            printf("label `%s' at %9.2f %9.2f %9.2f\n",szName,x,y,z);
#endif
         break;
#ifdef DEBUG_3DTODXF
       case img_STOP:
         printf("stop\n");
         break;
       default:
         printf("other info tag (code %d) ignored\n",item);
#endif
      }
   } while (item!=img_STOP);
   img_close(pimg);
   fprintf(fh,"000\nENDSEC\n");
   fprintf(fh,"000\nEOF\n");
   fclose(fh);
   return 0;
}
