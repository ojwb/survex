/* dump3d.c */
/* Show raw contents of .3d file in text form */
/* Copyright (C) 2001,2002,2006,2011,2012,2013,2014,2015,2018 Olly Betts
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "cmdline.h"
#include "date.h"
#include "debug.h"
#include "filelist.h"
#include "img_hosted.h"

static const struct option long_opts[] = {
   /* const char *name; int has_arg (0 no_argument, 1 required_*, 2 optional_*); int *flag; int val; */
   {"survey", required_argument, 0, 's'},
   {"rewind", no_argument, 0, 'r'},
   {"show-dates", no_argument, 0, 'd'},
   {"help", no_argument, 0, HLP_HELP},
   {"version", no_argument, 0, HLP_VERSION},
   {0, 0, 0, 0}
};

#define short_opts "rds:"

static struct help_msg help[] = {
/*				<-- */
   {HLP_ENCODELONG(0),	      /*only load the sub-survey with this prefix*/199, 0},
   /* TRANSLATORS: --help output for dump3d --rewind option */
   {HLP_ENCODELONG(1),	      /*rewind file and read it a second time*/204, 0},
   {HLP_ENCODELONG(2),	      /*show survey date information (if present)*/396, 0},
   {0, 0, 0}
};

int
main(int argc, char **argv)
{
   char *fnm;
   img *pimg;
   img_point pt;
   int code;
   const char *survey = NULL;
   bool fRewind = fFalse;
   bool show_dates = fFalse;

   msg_init(argv);

   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 1, 1);
   while (1) {
      int opt = cmdline_getopt();
      if (opt == EOF) break;
      if (opt == 's') survey = optarg;
      if (opt == 'r') fRewind = fTrue;
      if (opt == 'd') show_dates = fTrue;
   }
   fnm = argv[optind];

   pimg = img_open_survey(fnm, survey);
   if (!pimg) fatalerror(img_error2msg(img_error()), fnm);

   printf("TITLE \"%s\"\n", pimg->title);
   printf("DATE \"%s\"\n", pimg->datestamp);
   printf("DATE_NUMERIC %ld\n", pimg->datestamp_numeric);
   if (pimg->cs)
      printf("CS %s\n", pimg->cs);
   printf("VERSION %d\n", pimg->version);
   if (pimg->is_extended_elevation)
      printf("EXTENDED ELEVATION\n");
   printf("SEPARATOR '%c'\n", pimg->separator);
   printf("--\n");

   code = img_BAD;
   do {
      if (code == img_STOP) {
	 printf("<<< REWIND <<<\n");
	 fRewind = fFalse;
	 if (!img_rewind(pimg)) fatalerror(img_error2msg(img_error()), fnm);
      }

      do {
	 code = img_read_item(pimg, &pt);
	 switch (code) {
	  case img_MOVE:
	    printf("MOVE %.2f %.2f %.2f\n", pt.x, pt.y, pt.z);
	    break;
	  case img_LINE:
	    printf("LINE %.2f %.2f %.2f [%s]", pt.x, pt.y, pt.z, pimg->label);
	    switch (pimg->style) {
		case img_STYLE_UNKNOWN:
		    break;
		case img_STYLE_NORMAL:
		    printf(" STYLE=NORMAL");
		    break;
		case img_STYLE_DIVING:
		    printf(" STYLE=DIVING");
		    break;
		case img_STYLE_CARTESIAN:
		    printf(" STYLE=CARTESIAN");
		    break;
		case img_STYLE_CYLPOLAR:
		    printf(" STYLE=CYLPOLAR");
		    break;
		case img_STYLE_NOSURVEY:
		    printf(" STYLE=NOSURVEY");
		    break;
	    }
	    if (pimg->flags & img_FLAG_SURFACE) printf(" SURFACE");
	    if (pimg->flags & img_FLAG_DUPLICATE) printf(" DUPLICATE");
	    if (pimg->flags & img_FLAG_SPLAY) printf(" SPLAY");
	    if (show_dates && pimg->days1 != -1) {
		int y, m, d;
		ymd_from_days_since_1900(pimg->days1, &y, &m, &d);
		printf(" %04d.%02d.%02d", y, m, d);
		if (pimg->days1 != pimg->days2) {
		    ymd_from_days_since_1900(pimg->days2, &y, &m, &d);
		    printf("-%04d.%02d.%02d", y, m, d);
		}
	    }
	    printf("\n");
	    break;
	  case img_LABEL:
	    printf("NODE %.2f %.2f %.2f [%s]", pt.x, pt.y, pt.z, pimg->label);
	    if (pimg->flags & img_SFLAG_SURFACE) printf(" SURFACE");
	    if (pimg->flags & img_SFLAG_UNDERGROUND) printf(" UNDERGROUND");
	    if (pimg->flags & img_SFLAG_ENTRANCE) printf(" ENTRANCE");
	    if (pimg->flags & img_SFLAG_EXPORTED) printf(" EXPORTED");
	    if (pimg->flags & img_SFLAG_FIXED) printf(" FIXED");
	    if (pimg->flags & img_SFLAG_ANON) printf(" ANON");
	    if (pimg->flags & img_SFLAG_WALL) printf(" WALL");
	    printf("\n");
	    break;
	  case img_XSECT:
	    printf("XSECT %.2f %.2f %.2f %.2f [%s]",
		   pimg->l, pimg->r, pimg->u, pimg->d, pimg->label);
	    if (show_dates && pimg->days1 != -1) {
		int y, m, d;
		ymd_from_days_since_1900(pimg->days1, &y, &m, &d);
		printf(" %04d.%02d.%02d", y, m, d);
		if (pimg->days1 != pimg->days2) {
		    ymd_from_days_since_1900(pimg->days2, &y, &m, &d);
		    printf("-%04d.%02d.%02d", y, m, d);
		}
	    }
	    printf("\n");
	    break;
	  case img_XSECT_END:
	    printf("XSECT_END\n");
	    break;
	  case img_ERROR_INFO:
	    printf("ERROR_INFO #legs %d, len %.2fm, E %.2f H %.2f V %.2f\n",
		   pimg->n_legs, pimg->length, pimg->E, pimg->H, pimg->V);
	    break;
	  case img_BAD:
	    img_close(pimg);
	    fatalerror(img_error2msg(img_error()), fnm);
	    /* fatalerror() won't return, but the compiler can't tell that and
	     * may warn about dropping through into the next case without a
	     * "break;" here.
	     */
	    break;
	  case img_STOP:
	    printf("STOP\n");
	    break;
	  default:
	    printf("CODE_0x%02x\n", code);
	 }
      } while (code != img_STOP);
   } while (fRewind);

   img_close(pimg);

   return 0;
}
