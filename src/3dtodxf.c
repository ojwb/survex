/* > 3dtodxf.c */

/* Converts a .3d file to a DXF file */
/* Useful as an example of how to use the img code from your own programs */

/* Copyright (C) 1994,1995 Olly Betts */

/*
1994.09.28 written using imgtest.c as a base
1995.01.15 fettled
1995.01.26 check files open OK
1995.02.18 tweaked a bit
1995.03.21 minor tweaks
1995.03.25 more fettling; added img_error call
*/

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

#define TEXT_HEIGHT	0.6	
#define MARKER_SIZE	0.8

int main( int argc, char *argv[] ) {
   char szTitle[256], szDateStamp[256], szName[256];
   char *fnm3D, *fnmDXF;
   const char *pthMe;
   unsigned char labels, crosses, legs;
   img *pimg;
   FILE *fh;
   int item;
   int fSeenMove=0;
   float x1=FLT_MAX,y1=FLT_MAX,z1=FLT_MAX;
   float x,y,z;
   float min_x, min_y, min_z, max_x, max_y, max_z; /* for HEADER section */
   double text_height; /* for station labels */
   double  marker_size; /* for station markers */

   /* FIXME TRANSLATE */
   static const struct option long_opts[] = {
	/* const char *name; int has_arg (0 no_argument, 1 required, 2 options_*); int *flag; int val */
	{"no-crosses", no_argument, 0, 'c'},
	{"no-station-names", no_argument, 0, 'n'},
	{"no-legs", no_argument, 0, 'l'},
	{"htext", required_argument, 0, 't'},
	{"msize", required_argument, 0, 'm'},
	{"help", no_argument, 0, HLP_HELP},
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
   int min_args = 1;

#ifndef STANDALONE
   pthMe = ReadErrorFile(argv[0]);
#endif

   /* Defaults */
   crosses = 1;
   labels = 1;
   legs = 1;
   text_height = TEXT_HEIGHT;
   marker_size = MARKER_SIZE;

   while (1) {
      int opt = my_getopt_long(argc, argv, short_opts, long_opts, NULL, help, min_args);
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
	 text_height = strtod(optarg,NULL);  /* FIXME check for trailing garbage... */
#ifdef DEBUG
	printf("Text Height: '%s' input, converted to %6.2f\n", optarg, text_height);
#endif
	 break;
       case 'm': /* Marker size */
	 marker_size = strtod(optarg,NULL);
#ifdef DEBUG
	printf("Marker Size: '%s', converted to %6.2f\n", optarg, marker_size);
#endif
	 break;
       default:
	 printf("Internal Error: 'getopt' returned %c %d\n",opt, opt); /* <<<<< create message in messages.txt ? */ 
      }
   } 	 

   fnm3D=argv[optind++];
   fnmDXF=argv[optind++];

   pimg=img_open( fnm3D, szTitle, szDateStamp );
   if (!pimg) {
#ifndef STANDALONE
	fatalerror(img_error(), fnm3D); 
#else
	printf("Bad .3d file\n");
#endif
   }
   fh=fopen(fnmDXF,"w");
   if (!fh) {
#ifndef STANDALONE
      fatalerror(/*failed to open file*/47,fnmDXF);
#else
      printf("Couldn't open output file '%s'\n",fnmDXF);
#endif
      exit(1);
   }

#ifdef STANDALONE /* create these messages in msessages.txt ? */
   printf("3d file title `%s'\n",szTitle);
   printf("Creation time `%s'\n",szDateStamp);
#endif 

   /* Get drawing corners */
   min_x = min_y = min_z = max_x = max_y = max_z = 0;
   do {
      item=img_read_datum( pimg, szName, &x, &y, &z );
      switch (item) {
       case img_LINE:
         if (x < min_x) min_x = x1;
	 if (x > max_x) max_x = x1;
         if (y < min_y) min_y = y1;
	 if (y > max_y) max_y = y1;
	 if (z < min_z) min_z = z1;
	 if (z > max_z) max_z = z1;
         if (x < min_x) min_x = x;
         if (x > max_x) max_x = x;
         if (y < min_y) min_y = y;
         if (y > max_y) max_y = y;
         if (z < min_z) min_z = z;
         if (z > max_z) max_z = z;    
         x1=x; y1=y; z1=z;
         break;
       case img_MOVE:
         x1=x; y1=y; z1=z;
         break;                    
       default:
	 break;
      }
   } while (item!=img_STOP);    
   img_close(pimg);  
   pimg=img_open( fnm3D, szTitle, szDateStamp );

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
   do {
      item=img_read_datum( pimg, szName, &x, &y, &z );
      switch (item) {
       case img_BAD:
#ifndef STANDALONE
         fatalerror(/*bad 3d file*/106,fnm3D);
#else
         printf("Bad .3d image file\n");
#endif
         img_close(pimg);
         exit(1);
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
         fSeenMove=1;
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
       case img_STOP:
#ifdef DEBUG_3DTODXF
         printf("stop\n");
#endif
         break;
       default:
         printf("other info tag (code %d) ignored\n",item); /* <<<<< cerate message in messages.txt ? */
      }
   } while (item!=img_STOP);
   img_close(pimg);
   fprintf(fh,"000\nENDSEC\n");
   fprintf(fh,"000\nEOF\n");
   fclose(fh);
   return 0;
}
