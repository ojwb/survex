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
   char *fnm3D, *fnmDXF;
   img *pimg;
   FILE *fh;
   int item;
   int fSeenMove=0;
   float x1=FLT_MAX,y1=FLT_MAX,z1=FLT_MAX;
   float x,y,z;

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
      printf("Couldn't open input .3d file '%s'\n",fnm3D);
      printf("Reason was: %s\n",reasons[img_error()]);
      exit(1);
   }

   fh=fopen(fnmDXF,"w");
   if (!fh) {
      printf("Couldn't open output file '%s'\n",fnmDXF);
      exit(1);
   }

   printf("3d file title `%s'\n",szTitle);
   printf("Creation time `%s'\n",szDateStamp);

   fprintf(fh,"000\nSECTION\n");
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
            exit(1);
         }
         fprintf(fh,"000\nLINE\n");
/*
         fprintf(fh,"008\n0\n");
*/
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
   return 0;
}
