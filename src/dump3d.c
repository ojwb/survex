/* dump3d.c */
/* Show raw contents of .3d file in text form */
/* Copyright (C) 2001,2002,2006,2011,2012 Olly Betts
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
#include "debug.h"
#include "filelist.h"
#include "img.h"

static const struct option long_opts[] = {
   /* const char *name; int has_arg (0 no_argument, 1 required_*, 2 optional_*); int *flag; int val; */
   {"survey", required_argument, 0, 's'},
   {"rewind", no_argument, 0, 'r'},
   {"help", no_argument, 0, HLP_HELP},
   {"version", no_argument, 0, HLP_VERSION},
   {0, 0, 0, 0}
};

#define short_opts "rs:"

static struct help_msg help[] = {
/*				<-- */
   {HLP_ENCODELONG(0),	      /*only load the sub-survey with this prefix*/199, 0},
   {HLP_ENCODELONG(1),	      /*rewind file and read it a second time*/204, 0},
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

   msg_init(argv);

   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 1, 1);
   while (1) {
      int opt = cmdline_getopt();
      if (opt == EOF) break;
      if (opt == 's') survey = optarg;
      if (opt == 'r') fRewind = fTrue;
   }
   fnm = argv[optind];

#if 0
   pimg = img_open_write("dump3d.3d", "dump3d", fTrue);
   if (!pimg) fatalerror(img_error(), "dump3d.3d");
   img_write_item(pimg, img_MOVE, 0, NULL, 0, 0, 0);
   img_write_item(pimg, img_LINE, 0, "161.lostworld.upstream",
		  36920.120000, 82672.840000, 1528.820000);
   img_write_item(pimg, img_LINE, 0, "161.keinzimmer",
		  36918.480000, 82663.360000, 1527.670000);
   img_write_item(pimg, img_LINE, 0, "161.lostworld.upstream",
		  36959.760000, 82679.880000, 1537.900000);
   img_close(pimg);
#endif

   pimg = img_open_survey(fnm, survey);
   if (!pimg) fatalerror(img_error(), fnm);

   printf("TITLE \"%s\"\n", pimg->title);
   printf("DATE \"%s\"\n", pimg->datestamp);
   printf("VERSION %d\n", pimg->version);
   printf("--\n");

   code = img_BAD;
   do {
      if (code == img_STOP) {
	 printf("<<< REWIND <<<\n");
	 fRewind = fFalse;
	 if (!img_rewind(pimg)) fatalerror(img_error(), fnm);
      }

      do {
	 code = img_read_item(pimg, &pt);
	 switch (code) {
	  case img_MOVE:
	    printf("MOVE %f %f %f\n", pt.x, pt.y, pt.z);
	    break;
	  case img_LINE:
	    printf("LINE %f %f %f [%s]", pt.x, pt.y, pt.z, pimg->label);
	    if (pimg->flags & img_FLAG_SURFACE) printf(" SURFACE");
	    if (pimg->flags & img_FLAG_DUPLICATE) printf(" DUPLICATE");
	    if (pimg->flags & img_FLAG_SPLAY) printf(" SPLAY");
	    printf("\n");
	    break;
	  case img_LABEL:
	    printf("NODE %f %f %f [%s]", pt.x, pt.y, pt.z, pimg->label);
	    if (pimg->flags & img_SFLAG_SURFACE) printf(" SURFACE");
	    if (pimg->flags & img_SFLAG_UNDERGROUND) printf(" UNDERGROUND");
	    if (pimg->flags & img_SFLAG_ENTRANCE) printf(" ENTRANCE");
	    if (pimg->flags & img_SFLAG_EXPORTED) printf(" EXPORTED");
	    if (pimg->flags & img_SFLAG_FIXED) printf(" FIXED");
	    printf("\n");
	    break;
	  case img_XSECT:
	    printf("XSECT %f %f %f %f [%s]\n",
		   pimg->l, pimg->r, pimg->u, pimg->d, pimg->label);
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
	    fatalerror(img_error(), fnm);
	  default:
	    printf("CODE_0x%02x\n", code);
	 }
      } while (code != img_STOP);
   } while (fRewind);

   img_close(pimg);

   return 0;
}
