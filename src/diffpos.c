/* > diffpos.c */

/* (Originally quick and dirty) program to compare two SURVEX .pos files */
/* Copyright (C) 1994,1996,1998 Olly Betts */

/*
1994.04.16 Written
1994.11.22 Now displays names of stations added/deleted at end of .pos file
           Now displays and skips lines which it can't parse (eg headers)
           Negative thresholds allowed (to force a "full" compare)
           Syntax expanded
1996.04.04 fixed Borland C warning
1996.05.06 fixed STRING() macro to correct Syntax: message
1998.06.09 fettled layout
*/

/* size of line buffer */
#define BUFLEN 256

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define sqrd(X) ((X)*(X))

/* macro to just convert argument to a string */
#define STRING(X) _STRING(X)
#define _STRING(X) #X

/* very small value for comparing floating point numbers with */
/* (ought to use a real epsilon value) */
#define EPSILON 0.00001

/* default threshold is 1cm */
#define DFLT_MAX_THRESHOLD 0.01

static void diff_pos(FILE *fh1, FILE *fh2, double threshold);
static int read_line(FILE *fh, double *px, double *py, double *pz, char *id);

int main( int argc, char *argv[] ) {
   double threshold = DFLT_MAX_THRESHOLD;
   char *fnm1, *fnm2;
   FILE *fh1, *fh2;
   if (argc != 3) {
      char *p;
      if (argc == 4)
         threshold = strtod(argv[3], &p);
      if (argc != 4 || *p) {
         /* complain if not 4 args, or threshold didn't parse cleanly */
         printf("Syntax: %s <pos file> <pos file> [<threshold>]\n", argv[0]);
         printf(" where <threshold> is the max. permitted change along "
                "any axis in metres\n"
	        " (default <threshold> is "STRING(DFLT_MAX_THRESHOLD)"m)\n");
         exit(1);
      }
   }
   fnm1 = argv[1];
   fnm2 = argv[2];
   fh1 = fopen(fnm1, "rb");
   if (!fh1) {
      printf("Can't open file '%s'\n", fnm1);
      exit(1);
   }
   fh2 = fopen(fnm2, "rb");
   if (!fh2) {
      printf("Can't open file '%s'\n", fnm2);
      exit(1);
   }
   diff_pos(fh1, fh2, threshold);
   return 0;
}

static void diff_pos( FILE *fh1, FILE *fh2, double threshold ) {
   double x1, y1, z1, x2, y2, z2;
   int cmp;
   char id1[BUFLEN], id2[BUFLEN];
   int fRead1 = 1, fRead2 = 1;
   while (!feof(fh1) && !feof(fh2)) {
      if (fRead1) {
         if (!read_line(fh1, &x1, &y1, &z1, id1)) continue;
         fRead1 = 0;
      }
      if (fRead2) {
         if (!read_line(fh2, &x2, &y2, &z2, id2)) continue;
         fRead2 = 0;
      }
      cmp = strcmp(id1, id2);
      if (cmp == 0) {
         if (fabs(x1 - x2) - threshold > EPSILON
             || fabs(y1 - y2) - threshold > EPSILON
             || fabs(z1 - z2) - threshold > EPSILON) {
            printf("Moved by (%3.2f,%3.2f,%3.2f): %s\n", x1 - x2, y1 - y2,
                   z1 - z2, id1);
         }
         fRead1 = fRead2 = 1;
      } else {
         if (cmp < 0) {
            printf("Deleted: %s\n", id1);
            fRead1 = 1;
         } else {
            printf("Added: %s\n", id2);
            fRead2 = 1;
         }
      }
   }
   while (!feof(fh1) && read_line(fh1, &x1, &y1, &z1, id1))
      printf("Deleted: %s (at end of file)\n", id1);
   while (!feof(fh2) && read_line(fh2, &x2, &y2, &z2, id2))
      printf("Added: %s (at end of file)\n", id2);
}

static int read_line(FILE *fh, double *px, double *py, double *pz,
                     char *id) {
   char buf[BUFLEN];
   if (!fgets(buf, BUFLEN, fh)) return 0;
   if (sscanf(buf, "(%lf,%lf,%lf )%s", px, py, pz, id) < 4) {
      printf("Ignoring line: %s\n", buf);
      return 0;
   }
   return 1;
}
