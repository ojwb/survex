/* > diffpos.c */
/* (Originally quick and dirty) program to compare two SURVEX .pos files */
/* Copyright (C) 1994,1996,1998,1999,2001 Olly Betts
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* size of line buffer */
#define BUFLEN 256

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "cavern.h" /* just for REAL_EPSILON */
#include "cmdline.h"

#define sqrd(X) ((X)*(X))

/* macro to just convert argument to a string */
#define STRING(X) _STRING(X)
#define _STRING(X) #X

/* very small value for comparing floating point numbers with */
#define EPSILON (REAL_EPSILON * 1000)

/* default threshold is 1cm */
#define DFLT_MAX_THRESHOLD 0.01

static int diff_pos(FILE *fh1, FILE *fh2, double threshold);
static int read_line(FILE *fh, double *px, double *py, double *pz, char *id);

static const struct option long_opts[] = {
   /* const char *name; int has_arg (0 no_argument, 1 required_*, 2 optional_*); int *flag; int val; */
   {"help", no_argument, 0, HLP_HELP},
   {"version", no_argument, 0, HLP_VERSION},
   {0, 0, 0, 0}
};

#define short_opts ""

static struct help_msg help[] = {
/*				<-- */
   {0, 0}
};

int
main(int argc, char **argv)
{
   double threshold = DFLT_MAX_THRESHOLD;
   char *fnm1, *fnm2;
   FILE *fh1, *fh2;

   msg_init(argv[0]);

   cmdline_set_syntax_message("POS_FILE POS_FILE [THRESHOLD]",
			      "THRESHOLD is the max. ignorable change along "
			      "any axis in metres (default "
			      STRING(DFLT_MAX_THRESHOLD)")");
   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 2, 3);
   while (cmdline_getopt() != EOF) {
      /* do nothing */
   }
   fnm1 = argv[optind++];
   fnm2 = argv[optind++];
   if (argv[optind]) {
      optarg = argv[optind];
      threshold = cmdline_double_arg();
   }
   fh1 = fopen(fnm1, "rb");
   if (!fh1) {
      printf("Can't open file `%s'\n", fnm1);
      exit(1);
   }
   fh2 = fopen(fnm2, "rb");
   if (!fh2) {
      printf("Can't open file `%s'\n", fnm2);
      exit(1);
   }
   return diff_pos(fh1, fh2, threshold);
}

typedef enum { Eof, Haveline, Needline } state;

int
diff_pos(FILE *fh1, FILE *fh2, double threshold)
{
   state pos1 = Needline, pos2 = Needline;
   int result = 0;

   while (1) {
      double x1, y1, z1, x2, y2, z2;
      char id1[BUFLEN], id2[BUFLEN];

      if (pos1 == Needline) {
	 pos1 = Haveline;
         if (!read_line(fh1, &x1, &y1, &z1, id1)) pos1 = Eof;
      }

      if (pos2 == Needline) {
	 pos2 = Haveline;
         if (!read_line(fh2, &x2, &y2, &z2, id2)) pos2 = Eof;
      }

      if (pos1 == Eof) {
	 if (pos2 == Eof) break;
	 result = 1;
	 printf("Added: %s (at end of file)\n", id2);
	 pos2 = Needline;
      } else if (pos2 == Eof) {
	 result = 1;
	 printf("Deleted: %s (at end of file)\n", id1);
	 pos1 = Needline;
      } else {
	 int cmp = strcmp(id1, id2);
	 if (cmp == 0) {
	    if (fabs(x1 - x2) - threshold > EPSILON ||
		fabs(y1 - y2) - threshold > EPSILON ||
		fabs(z1 - z2) - threshold > EPSILON) {
	       result = 1;
	       printf("Moved by (%3.2f,%3.2f,%3.2f): %s\n",
		      x1 - x2, y1 - y2, z1 - z2, id1);
	    }
	    pos1 = pos2 = Needline;
	 } else {
	    result = 1;
	    if (cmp < 0) {
	       printf("Deleted: %s\n", id1);
	       pos1 = Needline;
	    } else {
	       printf("Added: %s\n", id2);
	       pos2 = Needline;
	    }
	 }
      }
   }
   return result;
}

static int
read_line(FILE *fh, double *px, double *py, double *pz, char *id)
{
   char buf[BUFLEN];
   while (1) {
      if (!fgets(buf, BUFLEN, fh)) return 0;
      if (sscanf(buf, "(%lf,%lf,%lf )%s", px, py, pz, id) == 4) break;
      printf("Ignoring line: %s\n", buf);
   }
   return 1;
}
