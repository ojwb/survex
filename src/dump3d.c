/* dump3d.c */
/* Show raw contents of .3d file in text form */
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
   {"help", no_argument, 0, HLP_HELP},
   {"version", no_argument, 0, HLP_VERSION},
   {0, 0, 0, 0}
};

#define short_opts "s:"

static struct help_msg help[] = {
/*				<-- */
   {HLP_ENCODELONG(0),          "Only load the sub-survey with this prefix"},
   {0, 0}
};

int
main(int argc, char **argv)
{
   char *fnm;
   img *pimg;
   img_point pt;
   int code;
   const char *survey = NULL;

   msg_init(argv[0]);

   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 1, 1);
   while (1) {
      int opt = cmdline_getopt();
      if (opt == EOF) break;
      if (opt == 's') survey = optarg;
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

   do {
      code = img_read_item(pimg, &pt);
      switch (code) {
       case img_MOVE:
	 printf("MOVE %f %f %f\n", pt.x, pt.y, pt.z);
	 break;
       case img_LINE:
	 printf("LINE %f %f %f [%s]\n", pt.x, pt.y, pt.z, pimg->label);
	 break;
       case img_LABEL:
	 printf("NODE %f %f %f [%s]\n", pt.x, pt.y, pt.z, pimg->label);
	 break;
       case img_BAD:
	 img_close(pimg);
	 fatalerror(img_error(), fnm);
      }
   } while (code != img_STOP);
      
   img_close(pimg);

   return 0;
}
