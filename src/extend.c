/* extend.c
 * Produce an extended elevation
 * Copyright (C) 1995-2001 Olly Betts
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

#include "cmdline.h"
#include "debug.h"
#include "filelist.h"
#include "filename.h"
#include "hash.h"
#include "img.h"
#include "message.h"
#include "useful.h"

/* To save memory we should probably use the prefix hash for the prefix on
 * point labels (FIXME) */
typedef struct POINT {
   img_point p;
   const char *label;
   unsigned int order;
   int flags;
   struct POINT *next;
} point;

typedef struct LEG {
   point *fr, *to;
   const char *prefix;
   int fDone;
   int flags;
   struct LEG *next;
} leg;

static point headpoint = {{0, 0, 0}, NULL, 0, 0, NULL};

static leg headleg = {NULL, NULL, NULL, 1, 0, NULL};

static img *pimg;

static void do_stn(point *, double);

typedef struct pfx {
   const char *label;
   struct pfx *next;
} pfx;

static pfx **htab;

static const char *
find_prefix(const char *prefix)
{
   pfx *p;
   int hash;

   ASSERT(prefix);

   hash = hash_string(prefix) & 0x1fff;
   for (p = htab[hash]; p; p = p->next) {
      if (strcmp(prefix, p->label) == 0) return p->label;
   }

   p = osnew(pfx);
   p->label = osstrdup(prefix);
   p->next = htab[hash];
   htab[hash] = p;

   return p->label;
}

static point *
find_point(const img_point *pt)
{
   point *p;
   for (p = headpoint.next; p != NULL; p = p->next) {
      if (pt->x == p->p.x && pt->y == p->p.y && pt->z == p->p.z) {
	 return p;
      }
   }

   p = osmalloc(ossizeof(point));
   p->p = *pt;
   p->label = "<none>";
   p->order = 0;
   p->next = headpoint.next;
   p->flags = 0;
   headpoint.next = p;
   return p;
}

static void
add_leg(point *fr, point *to, const char *prefix, int flags)
{
   leg *l;
   fr->order++;
   to->order++;
   l = osmalloc(ossizeof(leg));
   l->fr = fr;
   l->to = to;
   if (prefix)
      l->prefix = find_prefix(prefix);
   else
      l->prefix = NULL;
   l->next = headleg.next;
   l->fDone = 0;
   l->flags = flags;
   headleg.next = l;
}

static void
add_label(point *p, const char *label, int flags)
{
   p->label = osstrdup(label);
   p->flags = flags;
}

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
   {HLP_ENCODELONG(0),          "only load the sub-survey with this prefix"},
   {0, 0}
};

int
main(int argc, char **argv)
{
   const char *fnm_in, *fnm_out;
   char *desc;
   img_point pt;
   int result;
   point *fr = NULL, *to;
   double zMax = -DBL_MAX;
   point *start = NULL;
   point *p;
   const char *survey = NULL;

   msg_init(argv);

   cmdline_set_syntax_message("INPUT_3D_FILE [OUTPUT_3D_FILE]", NULL);
   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 1, 2);
   while (1) {
      int opt = cmdline_getopt();
      if (opt == EOF) break;
      if (opt == 's') survey = optarg;
   }
   fnm_in = argv[optind++];
   if (argv[optind]) {
      fnm_out = argv[optind];
   } else {
      fnm_out = add_ext("extend", EXT_SVX_3D);
   }

   /* try to open image file, and check it has correct header */
   pimg = img_open_survey(fnm_in, survey);
   if (pimg == NULL) fatalerror(img_error(), fnm_in);

   putnl();
   puts(msg(/*Reading in data - please wait...*/105));

   htab = osmalloc(ossizeof(pfx*) * 0x2000);

   do {
      result = img_read_item(pimg, &pt);
      switch (result) {
      case img_MOVE:
	 fr = find_point(&pt);
	 break;
      case img_LINE:
	 if (!fr) {
	    result = img_BAD;
	    break;
	 }
	 to = find_point(&pt);
	 if (!(pimg->flags & (img_FLAG_SURFACE|img_FLAG_SPLAY)))
	    add_leg(fr, to, pimg->label, pimg->flags);
	 fr = to;
	 break;
      case img_LABEL:
	 fr = find_point(&pt);
	 add_label(fr, pimg->label, pimg->flags);
	 break;
      case img_BAD:
	 (void)img_close(pimg);
	 fatalerror(img_error(), fnm_in);
      }
   } while (result != img_STOP);

   desc = osmalloc(strlen(pimg->title) + 11 + 1);
   strcpy(desc, pimg->title);
   strcat(desc, " (extended)");

   (void)img_close(pimg);

   /* start at the highest entrance with some legs attached */
   for (p = headpoint.next; p != NULL; p = p->next) {
      if (p->order > 0 && (p->flags & img_SFLAG_ENTRANCE) && p->p.z > zMax) {
	 start = p;
	 zMax = p->p.z;
      }
   }
   if (start == NULL) {
      /* if no entrances with legs, start at the highest 1-node */
      for (p = headpoint.next; p != NULL; p = p->next) {
	 if (p->order == 1 && p->p.z > zMax) {
	    start = p;
	    zMax = p->p.z;
	 }
      }
      /* of course we may have no 1-nodes... */
      if (start == NULL) {
	 for (p = headpoint.next; p != NULL; p = p->next) {
	    if (p->order != 0 && p->p.z > zMax) {
	       start = p;
	       zMax = p->p.z;
	    }
	 }
	 if (start == NULL) {
	    /* There are no legs - just pick the highest station... */
	    for (p = headpoint.next; p != NULL; p = p->next) {
	       if (p->p.z > zMax) {
		  start = p;
		  zMax = p->p.z;
	       }
	    }
	    if (!start) fatalerror(/*No survey data*/43);
	 }
      }
   }
   pimg = img_open_write(fnm_out, desc, fTrue);

   do_stn(start, 0.0); /* only does highest connected component currently */
   if (!img_close(pimg)) {
      (void)remove(fnm_out);
      fatalerror(img_error(), fnm_out);
   }

   return EXIT_SUCCESS;
}

static void
do_stn(point *p, double X)
{
   leg *l, *lp;
   double dX;
   img_write_item(pimg, img_LABEL, p->flags, p->label, X, 0, p->p.z);
   lp = &headleg;
   for (l = lp->next; l; lp = l, l = lp->next) {
      if (l->fDone) {
	 /* this case happens iff a recursive call causes the next leg to be
	  * removed, leaving our next pointing to a leg which has been dealt
	  * with... */
      } else {
	 if (l->to == p) {
	    lp->next = l->next;
	    dX = radius(l->fr->p.x - l->to->p.x, l->fr->p.y - l->to->p.y);
	    img_write_item(pimg, img_MOVE, 0, NULL, X + dX, 0, l->fr->p.z);
	    img_write_item(pimg, img_LINE, l->flags, l->prefix,
			   X, 0, l->to->p.z);
	    l->fDone = 1;
	    do_stn(l->fr, X + dX);
	    /* osfree(l); */
	    l = lp;
	 } else if (l->fr == p) {
	    lp->next = l->next;
	    dX = radius(l->fr->p.x - l->to->p.x, l->fr->p.y - l->to->p.y);
	    img_write_item(pimg, img_MOVE, 0, NULL, X, 0, l->fr->p.z);
	    img_write_item(pimg, img_LINE, l->flags, l->prefix,
			   X + dX, 0, l->to->p.z);
	    l->fDone = 1;
	    do_stn(l->to, X + dX);
	    /* osfree(l); */
	    l = lp;
	 }
      }
   }
}
