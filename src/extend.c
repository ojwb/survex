/* > extend.c
 * Produce an extended elevation
 * Copyright (C) 1995-2000 Olly Betts
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

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "cmdline.h"
#include "useful.h"
#include "filename.h"
#include "message.h"
#include "filelist.h"
#include "img.h"

typedef struct POINT {
   double x, y, z;
   char *label;
   unsigned int order;
   struct POINT *next;
} point;

typedef struct LEG {
   point *fr, *to;
   struct LEG *next;
   int fDone;
} leg;

static point headpoint = {0, 0, 0, NULL, 0, NULL};

static leg headleg = {NULL, NULL, NULL, 1};

static img *pimg;

static void do_stn(point *, double);

static point *
find_point(double x, double y, double z)
{
   point *p;
   for (p = headpoint.next; p != NULL; p = p->next) {
      if (x == p->x && y == p->y && z == p->z) {
	 p->order++;
	 return p;
      }
   }

   p = osmalloc(ossizeof(point));
   p->x = x;
   p->y = y;
   p->z = z;
   p->label = "<none>";
   p->order = 1;
   p->next = headpoint.next;
   headpoint.next = p;
   return p;
}

static void
add_leg(point *fr, point *to)
{
   leg *p;
   p = osmalloc(ossizeof(leg));
   p->fr = fr;
   p->to = to;
   p->next = headleg.next;
   p->fDone = 0;
   headleg.next = p;
}

static void
add_label(point *p, const char *label)
{
   p->label = osstrdup(label);
}

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
   const char *fnmData, *fnmOutput;
   char szDesc[256];
   float x, y, z;
   char sz[256];
   int result;
   point *fr = NULL, *to;
   double zMax = -DBL_MAX;
   point *start = NULL;
   point *p;

   msg_init(argv[0]);

   cmdline_set_syntax_message("INPUT_3D_FILE [OUTPUT_3D_FILE]", NULL);
   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 1, 2);
   while (cmdline_getopt() != EOF) {
      /* do nothing */
   }
   fnmData = argv[optind++];
   if (argv[optind]) {
      fnmOutput = argv[optind];
   } else {
      fnmOutput = add_ext("extend", EXT_SVX_3D);
   }

   /* try to open image file, and check it has correct header */
   pimg = img_open(fnmData, szDesc, NULL);
   if (pimg == NULL) fatalerror(img_error(), fnmData);

   putnl();
   puts(msg(/*Reading in data - please wait...*/105));

   do {
      result = img_read_datum(pimg, sz, &x, &y, &z);
      switch (result) {
      case img_BAD:
         puts(msg(/*Bad 3d image file '%s'*/106));
         break;
      case img_MOVE:
         fr = find_point(x, y, z);
         break;
      case img_LINE:
       	 if (!fr) {
       	    printf("img_LINE before any img_MOVE\n");
       	    exit(1);
       	 }
         to = find_point(x, y, z);
         add_leg(fr, to);
         fr = to;
         break;
      case img_CROSS:
         /* Use labels to position crosses too - newer format .3d files
          * don't have crosses in */
         break;
      case img_LABEL:
         fr = find_point(x, y, z);
         add_label(fr, sz);
         break;
      }
   } while (result != img_BAD && result != img_STOP);

   img_close(pimg);

   /* start at the highest 1-node */
   for (p = headpoint.next; p != NULL; p = p->next) {
      if (p->order == 1 && p->z > zMax) {
	 start = p;
	 zMax = p->z;
      }
   }
   /* of course we may have no 1-nodes... */
   if (start == NULL) {
      for (p = headpoint.next; p != NULL; p = p->next) {
	 if (p->z > zMax && p->order != 0) {
	    start = p;
	    zMax = p->z;
	 }
      }
      if (start == NULL) {
	 fprintf(stderr, "No legs in input .3d file\n"); /* TRANSLATE */
	 return EXIT_FAILURE;
      }
   }
   strcat(szDesc, " (extended)");
   pimg = img_open_write(fnmOutput, szDesc, fTrue);

   do_stn(start, 0.0f); /* only does highest connected component currently */
   img_close(pimg);

   return EXIT_SUCCESS;
}

static void
do_stn(point *p, double X)
{
   leg *l, *lp;
   double dX;
   img_write_datum(pimg, img_LABEL, p->label, X, 0, p->z);
   l = &headleg;
   while (lp = l, (l = l->next) != NULL) {
      if (!l->fDone) { /* otherwise we extend loops multiple times */
         if (l->to == p) {
            lp->next = l->next; /* unlink FIXME ought to free memory too */
            dX = radius(l->fr->x - l->to->x, l->fr->y - l->to->y);
            img_write_datum(pimg, img_MOVE, NULL, X + dX, 0, l->fr->z);
            img_write_datum(pimg, img_LINE, NULL, X, 0, l->to->z);
            l->fDone = 1;
            do_stn(l->fr, X + dX);
         } else if (l->fr == p) {
            lp->next = l->next; /* unlink FIXME ought to free memory too */
            dX = radius(l->fr->x - l->to->x, l->fr->y - l->to->y);
            img_write_datum(pimg, img_MOVE, NULL, X, 0, l->fr->z);
            img_write_datum(pimg, img_LINE, NULL, X + dX, 0, l->to->z);
            l->fDone = 1;
            do_stn(l->to, X + dX);
         }
      }
   }
}
