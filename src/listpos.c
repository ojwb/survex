/* listpos.c
 * SURVEX Cave surveying software: stuff to do with stn position output
 * Copyright (C) 1991-2002,2011,2012,2013,2014,2024,2025 Olly Betts
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

#include <config.h>

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

/* Traverse prefix tree depth first starting at from, and calling function fn
 * at each prefix node in the tree for which:
 *   (prefix->sflags & mask) == need
 * (E.g. Pass 0 for mask and need to call the function on every node.)
 */
static void
traverse_prefix_tree(prefix *from, int mask, int need,
		     void (*fn)(prefix *))
{
    if ((from->sflags & mask) == need) fn(from);

    prefix *p = from->down;
    if (!p) return;

    while (1) {
	if ((p->sflags & mask) == need) fn(p);
	if (p->down) {
	    p = p->down;
	} else {
	    while (!p->right) {
		p = p->up;
		if (p == from) return; /* got back to start */
	    }
	    p = p->right;
	}
    }
}

static void
check_if_unused_fixed_point(prefix *name)
{
    // TRANSLATORS: fixed survey station that is not part of any survey
    warning_in_file(name->filename, name->line,
		    /*Unused fixed point “%s”*/73, sprint_prefix(name));
}

void
check_for_unused_fixed_points(void)
{
    traverse_prefix_tree(root,
			 BIT(SFLAGS_UNUSED_FIXED_POINT),
			 BIT(SFLAGS_UNUSED_FIXED_POINT),
			 check_if_unused_fixed_point);
}

static void
check_node(prefix *p)
{
    if (!p->pos) {
	/* We have no position for p.  Check if it was referred to in
	 * "*entrance" and/or "*export" and flag a warning if so.
	 * It could also be a survey (SFLAGS_SURVEY) or be mentioned as
	 * a station, but only in a line of data which was rejected
	 * because of an error.
	 */
	if (TSTBIT(p->sflags, SFLAGS_ENTRANCE)) {
	    /* TRANSLATORS: The first %s is replaced by a station name,
	     * the second %s by "entrance" or "export".
	     */
	    warning_in_file(p->filename, p->line,
			    /*Station “%s” referred to by *%s but never used*/190,
			    sprint_prefix(p), "entrance");
	}
	if (p->max_export) {
	    /* TRANSLATORS: The first %s is replaced by a station name,
	     * the second %s by "entrance" or "export".
	     */
	    warning_in_file(p->filename, p->line,
			    /*Station “%s” referred to by *%s but never used*/190,
			    sprint_prefix(p), "export");
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
	       prefix *where = p->up;
	       SVX_ASSERT(where);
	       char *s = osstrdup(sprint_prefix(where));
	       /* Report better when station called 2.1 for example */
	       while (!where->filename && where->up) where = where->up;

	       int msgno;
	       if (TSTBIT(where->sflags, SFLAGS_PREFIX_ENTERED)) {
		   msgno = /*Station “%s” not exported from survey “%s”*/26;
	       } else {
		   /* TRANSLATORS: This error occurs if there's an attempt to
		    * export a station from a survey which doesn't actually
		    * exist.
		    *
		    * Here "survey" is a "cave map" rather than list of
		    * questions - it should be translated to the terminology
		    * that cavers using the language would use.
		    */
		   msgno = /*Reference to station “%s” from non-existent survey “%s”*/286;
	       }
	       compile_diagnostic_pfx(DIAG_ERR, where, msgno,
				      sprint_prefix(p), s);
	       free(s);
	   }
       }

       if (TSTBIT(p->sflags, SFLAGS_SUSPECTTYPO)) {
	   /* TRANSLATORS: Here "station" is a survey station, not a train
	    * station. */
	   warning_in_file(p->filename, p->line,
			   /*Station “%s” referred to just once, with an explicit survey name - typo?*/70,
			   sprint_prefix(p));
       }
   }
}

extern void
check_node_stats(void)
{
    traverse_prefix_tree(root,
			 BIT(SFLAGS_SURVEY),
			 0,
			 check_node);
}
