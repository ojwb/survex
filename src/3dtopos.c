/* 3dtopos.c */
/* Produce a .pos file from a .3d file */
/* Copyright (C) 2001 Olly Betts
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

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "message.h"
#include "osalloc.h"
#include "img.h"
#include "filelist.h"

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

typedef struct {
   const char *name;
   img_point pt;
} station;

//What about a better collating order for stations?  But rather than
//doing this in cavern, it's probably better to do it as a sort before
//presentation...  Here's a first stab at some code:
/* We ideally want a collating order where "2" sorts before "10"
 * (and "10a" sorts between "10" and "11").
 * So we want an compare cmp(A,B) s.t.
 * 1) cmp(A,A) = 0
 * 2) cmp(A,B) < 0 iff cmp(B,A) > 0
 * 3) cmp(A,B) > 0 and cmp(B,C) > 0 => cmp(A,C) > 0
 * 4) cmp(A,B) = 0 iff strcmp(A,B) = 0 (e.g. "001" and "1" are not equal)
 */
/* FIXME: is this ordering OK? */
/* Would also be nice if "xxx2" sorted before "xxx10"... */
static int
name_cmp(const char *a, const char *b)
{
   const char *dot_a = strchr(a, '.');
   const char *dot_b = strchr(b, '.');
   int res;

   if (dot_a) {
      if (dot_b) {
	 size_t len_a = dot_a - a;
	 size_t len_b = dot_b - b;
	 res = memcmp(a, b, min(len_a, len_b));
	 if (res == 0) res = len_a - len_b;
	 if (res == 0) res = name_cmp(dot_a + 1, dot_b + 1);
      } else {
	 return -1;
      }
   } else {
      if (dot_b) {
	 return 1;
      } else {
	 /* skip common prefix */
	 while (*a && !isdigit(*a) && *a == *b) {
	    a++;
	    b++;
	 }
	 /* sort numbers numerically and before non-numbers */
	 if (isdigit(a[0])) {
	    long n_a, n_b;
	    if (!isdigit(b[0])) return -1;
	    /* FIXME: check errno, etc in case out of range */
	    n_a = strtoul(a, NULL, 10);
	    n_b = strtoul(b, NULL, 10);
	    if (n_a != n_b) {
	       if (n_a > n_b)
		  return 1;
	       else
		  return -1;
	    }
	    /* drop through - the numbers match, but there may be a suffix
	     * and also we want to give an order to "01" vs "1"... */
	 } else if (isdigit(b[0])) {
	    return 1;
	 }
	 /* if numbers match, sort by suffix */
	 return strcmp(a,b);
      }
   }
   return res;
}

#if 0
foo()
{
}
#endif

static int
cmp_station(const void *a, const void *b)
{
   return name_cmp(((const station *)a)->name, ((const station *)b)->name);
}

/* FIXME: remove static limit here */
static station stns[100000];
static int c_stns = 0;

int
main(int argc, char **argv)
{   
   char *fnm, *fnm_out;
   FILE *fh_out;
   img *pimg;
   int result;

   msg_init(argv[0]);

   cmdline_set_syntax_message("3D_FILE [POS_FILE]", NULL); /* TRANSLATE */
   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 1, 2);
   while (1) {
      int opt = cmdline_getopt();
      if (opt == EOF) break;
   }

   fnm = argv[optind++];
   if (argv[optind]) {
      fnm_out = argv[optind];
   } else {
      char *base = base_from_fnm(fnm);
      fnm_out = add_ext(base, EXT_SVX_POS);
      osfree(base);
   }

   pimg = img_open(fnm, NULL, NULL);
   if (!pimg) fatalerror(img_error(), fnm);

   fh_out = safe_fopen(fnm_out, "w");

   /* Output headings line */
   fputsnl(msg(/*( Easting, Northing, Altitude )*/195), fh_out);

   do {
      result = img_read_item(pimg, &(stns[c_stns].pt));
      switch (result) {
       case img_MOVE:
       case img_LINE:
    	 break;
       case img_LABEL:
	 stns[c_stns++].name = osstrdup(pimg->label);
	 break;
       case img_BAD:
	 img_close(pimg);
	 exit(1);
      }
   } while (result != img_STOP);

   img_close(pimg);

   qsort(stns, c_stns, sizeof(station), cmp_station);

   {
      int i;
      for (i = 0; i < c_stns; i++) {
	 fprintf(fh_out, "(%8.2f, %8.2f, %8.2f ) %s\n",
		 stns[i].pt.x, stns[i].pt.y, stns[i].pt.z, stns[i].name);
      }
   }

   fclose(fh_out);

   return 0;
}
