/* 3dtopos.c */
/* Produce a .pos file from a .3d file */
/* Copyright (C) 2001,2002,2011,2013,2014 Olly Betts
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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
#include "filelist.h"
#include "filename.h"
#include "img_hosted.h"
#include "message.h"
#include "namecmp.h"
#include "osalloc.h"

static const struct option long_opts[] = {
   /* const char *name; int has_arg (0 no_argument, 1 required_*, 2 optional_*); int *flag; int val; */
   {"survey", required_argument, 0, 's'},
   {"help", no_argument, 0, HLP_HELP},
   {"version", no_argument, 0, HLP_VERSION},
   {0, 0, 0, 0}
};

#define short_opts "s:"

static struct help_msg help[] = {
   /*				<-- */
   {HLP_ENCODELONG(0),        /*only load the sub-survey with this prefix*/199, 0},
   {0, 0, 0}
};

typedef struct {
   const char *name;
   img_point pt;
} station;

static int separator;

static int
cmp_station(const void *a, const void *b)
{
   return name_cmp(((const station *)a)->name, ((const station *)b)->name,
		   separator);
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
   const char *survey = NULL;

   msg_init(argv);

   /* TRANSLATORS: Part of 3dtopos --help */
   cmdline_set_syntax_message(/*3D_FILE [POS_FILE]*/217, 0, NULL);
   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 1, 2);
   while (1) {
      int opt = cmdline_getopt();
      if (opt == EOF) break;
      if (opt == 's') survey = optarg;
   }

   fnm = argv[optind++];
   if (argv[optind]) {
      fnm_out = argv[optind];
   } else {
      char *base = baseleaf_from_fnm(fnm);
      fnm_out = add_ext(base, EXT_SVX_POS);
      osfree(base);
   }

   pimg = img_open_survey(fnm, survey);
   if (!pimg) fatalerror(img_error2msg(img_error()), fnm);
   separator = pimg->separator;

   fh_out = safe_fopen(fnm_out, "w");

   /* Output headings line */
   /* TRANSLATORS: Heading line for .pos file.  Please try to ensure the “,”s
    * (or at least the columns) are in the same place */
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
	 fatalerror(img_error2msg(img_error()), fnm);
      }
   } while (result != img_STOP);

   /* + 1 to allow for reading coordinates of legs after the last label */
   stns = osmalloc(ossizeof(station) * (c_labels + 1));
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
	    if (p + len <= p_end) {
	       memcpy(p, pimg->label, len);
	       stns[c_stns++].name = p;
	       p += len;
	       break;
	    }
	 }
	 /* FALLTHRU */
       case img_BAD:
	 img_close(pimg);
	 fatalerror(/*Bad 3d image file “%s”*/106, fnm);
      }
   } while (result != img_STOP);

   if (c_stns != c_labels || p != p_end) {
      img_close(pimg);
      fatalerror(/*Bad 3d image file “%s”*/106, fnm);
   }

   img_close(pimg);

   qsort(stns, c_stns, sizeof(station), cmp_station);

   {
      OSSIZE_T i;
      for (i = 0; i < c_stns; i++) {
	 fprintf(fh_out, "(%8.2f, %8.2f, %8.2f ) %s\n",
		 stns[i].pt.x, stns[i].pt.y, stns[i].pt.z, stns[i].name);
      }
   }

   safe_fclose(fh_out);

   return EXIT_SUCCESS;
}
