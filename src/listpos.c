/* listpos.c
 * SURVEX Cave surveying software: stuff to do with stn position output
 * Copyright (C) 1991-2002,2011,2012,2013,2014 Olly Betts
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

#define PRINT_STN_POS_LIST 1
#define NODESTAT 1

#include <limits.h>

#include "cavern.h"
#include "datain.h"
#include "debug.h"
#include "filename.h"
#include "message.h"
#include "filelist.h"
#include "netbits.h"
#include "listpos.h"
#include "out.h"

/* Traverse prefix tree depth first starting at from, and
 * calling function fn at each node */
static void
traverse_prefix_tree(prefix *from, void (*fn)(prefix *))
{
   prefix *p;

   fn(from);

   p = from->down;
   if (!p) return;

   while (1) {
      fn(p);
      if (p->down) {
	 p = p->down;
      } else {
	 if (!p->right) {
	    do {
	       p = p->up;
	       if (p == from) return; /* got back to start */
	    } while (!p->right);
	 }
	 p = p->right;
      }
   }
}

static void
check_node(prefix *p)
{
   if (!p->pos) {
      if (!TSTBIT(p->sflags, SFLAGS_SURVEY)) {
	 /* Could do away with the SFLAGS_SURVEY check and check
	  * p->min_export instead of p->max_export I think ... */
	 if (TSTBIT(p->sflags, SFLAGS_ENTRANCE) || p->max_export) {
	     /* p is a station which was referred to in "*entrance" and/or
	      * "*export" but not elsewhere (otherwise it'd have a position).
	      * p could also be a survey (SFLAGS_SURVEY) or be mentioned as
	      * a station, but only in a line of data which was rejected
	      * because of an error.
	      */
	     warning_in_file(p->filename, p->line,
		     /*Station “%s” referred to by *entrance or *export but never used*/190,
		     sprint_prefix(p));
	 }
      }
   } else {
      /* Do we need to worry about export violations in hanging surveys? */
      if (fExportUsed) {
#if 0
	 printf("L min %d max %d pfx %s\n",
		p->min_export, p->max_export, sprint_prefix(p));
#endif
	 if ((p->min_export > 1 && p->min_export != USHRT_MAX) ||
	     (p->min_export == 0 && p->max_export)) {
	    char *s;
	    prefix *where = p->up;
	    int msgno;
	    SVX_ASSERT(where);
	    s = osstrdup(sprint_prefix(where));
	    /* Report better when station called 2.1 for example */
	    while (!where->filename && where->up) where = where->up;

	    if (TSTBIT(where->sflags, SFLAGS_PREFIX_ENTERED)) {
	       msgno = /*Station “%s” not exported from survey “%s”*/26;
	    } else {
		/* TRANSLATORS: This error occurs if there's an attempt to
		 * export a station from a survey which doesn't actually exist.
		 *
		 * Here "survey" is a "cave map" rather than list of questions - it should be
		 * translated to the terminology that cavers using the language would use.
		 */
	       msgno = /*Reference to station “%s” from non-existent survey “%s”*/286;
	    }
	    compile_diagnostic_pfx(DIAG_ERR, where, msgno, sprint_prefix(p), s);
	    osfree(s);
	 }
      }

      if (TSTBIT(p->sflags, SFLAGS_SUSPECTTYPO)) {
	 /* TRANSLATORS: Here "station" is a survey station, not a train station. */
	 warning_in_file(p->filename, p->line,
		 /*Station “%s” referred to just once, with an explicit survey name - typo?*/70,
		 sprint_prefix(p));
      }
   }
}

#if NODESTAT
static int *cOrder;
static int icOrderMac;

static void
node_stat(prefix *p)
{
   if (p->pos) {
      int order;
      SVX_ASSERT(pfx_fixed(p));

      order = p->shape;

      if (order >= icOrderMac) {
	 int c = order * 2;
	 cOrder = osrealloc(cOrder, c * ossizeof(int));
	 while (icOrderMac < c) cOrder[icOrderMac++] = 0;
      }
      cOrder[order]++;
   }

   check_node(p);
}

static void
node_stats(prefix *from)
{
   prefix * p;
   icOrderMac = 8; /* Start value */
   cOrder = osmalloc(icOrderMac * ossizeof(int));
   memset(cOrder, 0, icOrderMac * ossizeof(int));
   traverse_prefix_tree(from, node_stat);
   for (p = anon_list; p; p = p->right) {
      node_stat(p);
   }
}
#endif

extern void
print_node_stats(void)
{
#if NODESTAT
   int c;
   int max_c = 9999; /* Always allow space for 4 digits. */
   int width;
   node_stats(root);
   for (c = 0; c < icOrderMac; c++) {
      if (cOrder[c] > max_c) {
	 max_c = cOrder[c];
      }
   }
   width = 1 + (int)log10((double)max_c);
   for (c = 0; c < icOrderMac; c++) {
      if (cOrder[c] > 0) {
	 printf("%*d %d-%s.\n", width, cOrder[c], c,
		msg(cOrder[c] == 1 ?
		    /* TRANSLATORS: node/nodes as in:  "Survey has 1 2-node and 2 3-nodes." */
		    /*node*/176 :
		    /* TRANSLATORS: node/nodes as in:  "Survey has 1 2-node and 2 3-nodes." */
		    /*nodes*/177));
      }
   }
#endif
}

extern void
check_node_stats(void)
{
   traverse_prefix_tree(root, check_node);
}
