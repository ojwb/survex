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
#include "namecmp.h"

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

static int
cmp_station(const void *a, const void *b)
{
   return name_cmp(((const station *)a)->name, ((const station *)b)->name);
}

static station *stns;
static OSSIZE_T c_stns = 0;

int
main(int argc, char **argv)
{   
   char *fnm, *fnm_out;
   FILE *fh_out;
   img *pimg;
   int result;
   OSSIZE_T c_labels = 0;
   OSSIZE_T c_totlabel = 0;
   char *p, *p_end;

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
      img_point pt;
      result = img_read_item(pimg, &pt);
      switch (result) {
       case img_MOVE:
       case img_LINE:
    	 break;
       case img_LABEL:
	 c_labels++;
	 c_totlabel += strlen(pimg->label) + 1;
	 break;
       case img_BAD:
	 img_close(pimg);
	 exit(EXIT_FAILURE); /* FIXME report errors! */
      }
   } while (result != img_STOP);

   stns = osmalloc(ossizeof(station) * c_labels);
   p = osmalloc(c_totlabel);
   p_end = p + c_totlabel;
   
   img_rewind(pimg);
   
   do {
      result = img_read_item(pimg, &(stns[c_stns].pt));
      switch (result) {
       case img_MOVE:
       case img_LINE:
    	 break;
       case img_LABEL:
	 if (c_stns < c_labels) {
	    OSSIZE_T len = strlen(pimg->label) + 1;
	    if (p + len < p_end) {
	       memcpy(p, pimg->label, len);
	       p += len;
	       stns[c_stns++].name = p;
	       break;
	    }
	 }
	 /* FALLTHRU */
       case img_BAD:
	 img_close(pimg);
	 exit(EXIT_FAILURE); /* FIXME report errors! */
      }
   } while (result != img_STOP);

   img_close(pimg);

   if (c_stns != c_labels || p != p_end) {
      exit(EXIT_FAILURE); /* FIXME report errors! */
   }

   qsort(stns, c_stns, sizeof(station), cmp_station);

   {
      OSSIZE_T i;
      for (i = 0; i < c_stns; i++) {
	 fprintf(fh_out, "(%8.2f, %8.2f, %8.2f ) %s\n",
		 stns[i].pt.x, stns[i].pt.y, stns[i].pt.z, stns[i].name);
      }
   }

   fclose(fh_out);

   return EXIT_SUCCESS;
}
