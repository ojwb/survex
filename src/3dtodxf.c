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
#define STANDALONE

/* #define DEBUG_3DTODXF */

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include "img.h"

int main( int argc, char *argv[] ) {
   char szTitle[256], szDateStamp[256], szName[256];
   char *fnm3D, *fnmDXF, *fnmNew;
   img *pimg;
   FILE *fh;
   int item;
   int fSeenMove=0;
   float x1=FLT_MAX,y1=FLT_MAX,z1=FLT_MAX;
   float x,y,z;
   float min_x, min_y, min_z, max_x, max_y, max_z; // for HEADER section

   if (argc!=3) {
      printf("Syntax: %s <.3d file> <DXF file>\n",argv[0]);
      exit(1);
   }

   fnm3D=argv[1];
   fnmDXF=argv[2];

   pimg=img_open( fnm3D, szTitle, szDateStamp );
   if (!pimg) {
      static char *reasons[]={
         "No error",
         "File not found",
         "Out of memory",
         "Can't open for output",
         "File has incorrect format",
         "It's a directory"
      };
      if (img_error()==1) {
	 // Try again with .3D extension
	 fnmNew=(char *)malloc(strlen(fnm3D)+3);
	 if (!fnmNew) {
		printf("Not enough memory");
		exit(1);
	 }
	 strcpy(fnmNew, fnm3D);
	 strcat(fnmNew,".3D");
         pimg=img_open( fnmNew, szTitle, szDateStamp );
	 if (!pimg) {
            printf("Couldn't open input .3d file '%s'\n",fnm3D);
            printf("Reason was: %s\n",reasons[img_error()]);
	    free(fnmNew);
            exit(1);
 	 }
	 else {
	    fnm3D = fnmNew;
	 } 
      }
      else {
      	printf("Couldn't open input .3d file '%s'\n",fnm3D);
      	printf("Reason was: %s\n",reasons[img_error()]);
      	exit(1);
      }
   }

   fh=fopen(fnmDXF,"w");
   if (!fh) {
      printf("Couldn't open output file '%s'\n",fnmDXF);
      if (!fnmNew) free(fnmNew);
      exit(1);
   }

   printf("3d file title `%s'\n",szTitle);
   printf("Creation time `%s'\n",szDateStamp);

// begin New LAYER Stuff   
   // Get drawing corners
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
   // Header
   fprintf(fh,"0\nSECTION\n");  
   fprintf(fh,"2\nHEADER\n");
   fprintf(fh,"9\n$EXTMIN\n"); // lower left corner of drawing
   fprintf(fh,"10\n%#-.6f\n", min_x); // x
   fprintf(fh,"20\n%#-.6f\n", min_y); // y
   fprintf(fh,"30\n%#-.6f\n", min_z); // min z
   fprintf(fh,"9\n$EXTMAX\n"); // upper right corner of drawing
   fprintf(fh,"10\n%#-.6f\n", max_x); // x
   fprintf(fh,"20\n%#-.6f\n", max_y); // y
   fprintf(fh,"30\n%#-.6f\n", max_z); // max z
   fprintf(fh,"0\nENDSEC\n");  

   fprintf(fh,"0\nSECTION\n");
   fprintf(fh,"2\nTABLES\n");
   fprintf(fh,"0\nTABLE\n");
   fprintf(fh,"2\nLAYER\n");
   fprintf(fh,"70\n10\n"); // max # off layers in this DXF file : 10 
   // First Layer: CentreLine
   fprintf(fh,"0\nLAYER\n2\nCentreLine\n");
   fprintf(fh,"70\n64\n"); // shows the layer is referenced by entities
   fprintf(fh,"62\n5\n"); // color: kept the same used by SpeleoGen
   fprintf(fh,"6\nCONTINUOUS\n"); // linetype
   // Next Layer: Stations
   fprintf(fh,"0\nLAYER\n2\nStations\n");
   fprintf(fh,"70\n64\n"); // shows the layer is referenced by entities
   fprintf(fh,"62\n7\n"); // color: kept the same used by SpeleoGen
   fprintf(fh,"6\nCONTINUOUS\n"); // linetype
   // Next Layer: Labels
   fprintf(fh,"0\nLAYER\n2\nLabels\n");
   fprintf(fh,"70\n64\n"); // shows the layer is referenced by entities
   fprintf(fh,"62\n7\n"); // color: kept the same used by SpeleoGen
   fprintf(fh,"6\nCONTINUOUS\n"); // linetype
   fprintf(fh,"0\nENDTAB\n");
   fprintf(fh,"0\nENDSEC\n");
// end New LAYER Stuff

   fprintf(fh,"0\nSECTION\n");
   fprintf(fh,"002\nENTITIES\n");
   do {
      item=img_read_datum( pimg, szName, &x, &y, &z );
      switch (item) {
       case img_BAD:
         printf("Bad .3d image file\n");
         img_close(pimg);
         exit(1);
       case img_LINE:
#ifdef DEBUG_3DTODXF
         printf("line to %9.2f %9.2f %9.2f\n",x,y,z);
#endif
         if (!fSeenMove) {
            printf("Something is wrong -- img_LINE before any img_MOVE!\n");
            img_close(pimg);
	    if (!fnmNew) free(fnmNew);
            exit(1);
         }
         fprintf(fh,"000\nLINE\n");
// begin New Layer Stuff
         fprintf(fh,"8\nCentreLine\n");
// end New Layer Stuff
         fprintf(fh,"010\n%6.2f\n",x1);
         fprintf(fh,"020\n%6.2f\n",y1);
         fprintf(fh,"030\n%6.2f\n",z1);
         fprintf(fh,"011\n%6.2f\n",x);
         fprintf(fh,"021\n%6.2f\n",y);
         fprintf(fh,"031\n%6.2f\n",z);
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
         printf("other info tag (code %d) ignored\n",item);
      }
   } while (item!=img_STOP);
   img_close(pimg);
   fprintf(fh,"000\nENDSEC\n");
   fprintf(fh,"000\nEOF\n");
   fclose(fh);
   if (!fnmNew) free(fnmNew);
   return 0;
}
