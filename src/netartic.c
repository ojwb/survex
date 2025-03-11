/* netartic.c
 * Split up network at articulation points
 * Copyright (C) 1993-2003,2005,2012,2014,2015,2024 Olly Betts
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

#if 0
# define DEBUG_INVALID 1
# define DEBUG_ARTIC
#endif

#include <config.h>

#include "debug.h"
#include "cavern.h"
#include "filename.h"
#include "listpos.h"
#include "message.h"
#include "netartic.h"
#include "netbits.h"
#include "netskel.h"
#include "matrix.h"
#include "out.h"

static void
colour_fixed_point_cluster(node *stn,
			   node **p_fixedlist,
			   node **p_component_fixedlist)
{
    // Fixed points are positive until we colour from them or visit them.
    stn->colour = -stn->colour;
    remove_stn_from_list(p_fixedlist, stn);
    add_stn_to_list(p_component_fixedlist, stn);

    for (int d = 0; d < 3; d++) {
	linkfor *leg = stn->leg[d];
	if (!leg) break;
	node *to = leg->l.to;
	if (to->colour > 0 && stn->name->pos == to->name->pos) {
	    colour_fixed_point_cluster(to, p_fixedlist, p_component_fixedlist);
	}
    }
}

/* We want to split the reduced network into a list of "articulations" which
 * can be solved in turn.  This means we solve several smaller matrices whose
 * sizes sum to the size of the full matrix we would otherwise solve, which is
 * more efficient because the matrix solving is O(n³).
 */

typedef struct articulation {
   struct articulation *next;
   node *stnlist;
} articulation;

extern void
articulate(void)
{
    // Find components and articulation points in the reduced network.
    articulation *articulation_list = NULL;
    node *new_fixedlist = NULL;

    long colour = 0;

    // The number of stations in stnlist is a ceiling on how
    // large our stacks need to be (since we only visit
    // unfixed stations).
    unsigned long stack_size = 0;
    for (node *stn = stnlist; stn; stn = stn->next) {
	stn->colour = 0;
	stack_size++;
    }

    for (node *stn = fixedlist; stn; stn = stn->next) {
	// Unvisited fixed points have positive values which are then
	// negated when we visit them.
	colour++;
	stn->colour = colour;
#ifdef DEBUG_ARTIC
	printf("Fixed stn ");
	print_prefix(stn->name);
	printf(" assigned colour %ld\n", colour);
#endif
    }
    colour = -colour;

    unsigned char *dirn_stack = osmalloc(stack_size);
    long *oldest_stack = osmalloc(stack_size * sizeof(long));

    /* fixedlist can be NULL here if we've had a *solve followed by survey
     * which is all hanging. */
    long cFixed = colour;
    (void)cFixed;

    while (fixedlist) {
	/* We've moved on to a new component (a chunk of survey data not
	 * connected to anything we've processed in this function yet).
	 */
	cComponents++;
	node *fixedlist_head = fixedlist;
#ifdef DEBUG_ARTIC
	printf("new component #%ld with root [%p] ", cComponents, fixedlist_head);
	print_prefix(fixedlist_head->name);
	printf(" which has colour = %d/%d\n", fixedlist_head->colour, cFixed);
#endif

	node *component_fixedlist = NULL;
	colour_fixed_point_cluster(fixedlist_head,
				   &fixedlist, &component_fixedlist);
	while (component_fixedlist) {
	    node *stn_start = component_fixedlist;
	    component_fixedlist = component_fixedlist->next;
	    if (component_fixedlist)
		component_fixedlist->prev = NULL;
	    stn_start->next = new_fixedlist;
	    if (new_fixedlist) new_fixedlist->prev = stn_start;
	    new_fixedlist = stn_start;

	    // Unfixed points in the current articulation.  Fixed points aren't
	    // included as they don't get their own row in the matrix.
	    node *artlist = NULL;

	    for (int i = 0; i <= 2 && stn_start->leg[i]; i++) {
		node *stn = stn_start->leg[i]->l.to;
		if (stn->colour < 0) {
		    // Already visited stn.
		    continue;
		}

		if (stn->colour > 0) {
		    // Unvisited fixed point.
		    colour_fixed_point_cluster(stn, &fixedlist, &component_fixedlist);
		    continue;
		}

		// Walk the network visiting unvisited stations and colouring
		// them with monotonically decreasing negative integers.
		//
		// We use negative values here so that solve_matrix() can rely
		// on the stations it is solving for having negative values to
		// start with, which avoids it having to reset them all before
		// it goes through and assigns matrix row numbers.
		int back = reverse_leg_dirn(stn_start->leg[i]);

		unsigned long tos = 0;
    iter:
		stn->colour = --colour;
		long oldest_reached = colour;
#ifdef DEBUG_ARTIC
		printf("visit: stn [%p], back=%d,", stn, back);
		print_prefix(stn->name);
		printf(" set to colour %ld -> oldest_reached\n", colour);
#endif
		for (int j = 0; j <= 2 && stn->leg[j]; j++) {
		    if (j == back) {
			// Ignore the reverse of the leg we just took to get
			// here.
			continue;
		    }

		    node *to = stn->leg[j]->l.to;
		    long col = to->colour;
		    if (col == 0) {
			SVX_ASSERT(tos < stack_size);
			dirn_stack[tos] = back;
			oldest_stack[tos] = oldest_reached;
			tos++;
			back = reverse_leg_dirn(stn->leg[j]);
			stn = to;
			goto iter;
			/* The algorithm to find articulation points is perhaps
			 * most naturally expressed using recursion, but the
			 * recursion depth is only bounded by the number of
			 * unfixed stations in the reduced network.  When the
			 * network is large that can result in excessive stack
			 * use and we can even run out of stack space.
			 *
			 * To avoid this we have converted the algorithm into
			 * an iterative one with an explicit stack which needs
			 * only 5 bytes per recursion level of the recursive
			 * version.
			 *
			 * This is the point where the recursive call would be.
			 */
    uniter:
			SVX_ASSERT(tos > 0);
			--tos;
			j = reverse_leg_dirn(stn->leg[back]);
			to = stn;
			stn = to->leg[back]->l.to;
			back = dirn_stack[tos];
			if (oldest_stack[tos] > oldest_reached)
			    oldest_reached = oldest_stack[tos];

#ifdef DEBUG_ARTIC
			printf("unwind: stn [%p] ", stn);
			print_prefix(stn->name);
			printf(" colour %d, oldest_reached %d, "
			       "station after %d\n",
			       stn->colour, oldest_reached, to->colour);
			printf("Putting stn ");
			print_prefix(to->name);
			printf(" on artlist\n");
#endif
			if (fixed(to)) {
			    printf("[%p] ", to); print_prefix(to->name);
			    printf(" is fixed\n");
			    remove_stn_from_list(&fixedlist, to);
			} else {
			    remove_stn_from_list(&stnlist, to);
			}
			add_stn_to_list(&artlist, to);

			if (to->colour >= oldest_reached) {
			    oldest_reached = to->colour;

			    /*                          ..
			     *                 /\      /
			     * [fixed point(s)]  *----*
			     *                 \/      \..
			     *                  stn   to
			     *
			     * artlist contains all the stations from `to` and
			     * rightwards in the diagram above (excluding any
			     * beyond further articulations), so we add artlist
			     * to the articulation_list linked list and start a
			     * new artlist.
			     *
			     * FIXME: We could do a replacement to eliminate
			     * `station` so we have one fewer station to solve
			     * in that matrix, i.e.:
			     *
			     *                 /\
			     * [fixed point(s)]  )
			     *                 \/
			     *
			     * Then after solving, we would calculate the
			     * position of `stn` (akin to resistors in series),
			     * then the position of `to` simply by adding on
			     * the articulating leg (saving a station in the
			     * next matrix too).
			     */

			    // Mark leg as an articulation, which is used while
			    // building the matrix and when reporting
			    // misclosures.
			    to->leg[back]->l.reverse |= FLAG_ARTICULATION;
			    stn->leg[j]->l.reverse |= FLAG_ARTICULATION;

			    /* start new articulation */
			    articulation *art = osnew(articulation);
			    art->stnlist = artlist;
			    art->next = articulation_list;
			    articulation_list = art;
			    artlist = NULL;

#ifdef DEBUG_ARTIC
			    printf("Articulate *-");
			    print_prefix(stn->name);
			    printf("-");
			    print_prefix(to->name);
			    printf("-...\n");
#endif
			}
		    } else {
			/* back edge case */
			if (col > 0) {
			    /* we've found a fixed point */
			    col = -col;
			    colour_fixed_point_cluster(to, &fixedlist, &component_fixedlist);
			}

			if (col > oldest_reached) oldest_reached = col;
		    }
		}

		SVX_ASSERT(!stn->leg[0] || stn->leg[0]->l.to->colour < 0);
		SVX_ASSERT(!stn->leg[1] || stn->leg[1]->l.to->colour < 0);
		SVX_ASSERT(!stn->leg[2] || stn->leg[2]->l.to->colour < 0);

		if (tos > 0) goto uniter;

#ifdef DEBUG_ARTIC
		printf("Putting stn ");
		print_prefix(stn->name);
		printf(" on artlist\n");
#endif
		remove_stn_from_list(&stnlist, stn);
		add_stn_to_list(&artlist, stn);

#ifdef DEBUG_ARTIC
		print_prefix(stn_start->name);
		printf(" -> ");
		print_prefix(stn->name);
		printf(" col %d cFixed %d\n", col, cFixed);
#endif
		if (artlist) {
		    // We can process the part of the network we just visited by
		    // itself.
		    articulation *art = osnew(articulation);
		    art->stnlist = artlist;
		    art->next = articulation_list;
		    articulation_list = art;
		    artlist = NULL;
		}
	    }
	}
    }

#ifdef DEBUG_ARTIC
    printf("%ld components\n",cComponents);
#endif

    if (stnlist) {
	/* Any stations still in stnlist are unreachable from fixed points
	 * which means we have one or more hanging surveys.
	 *
	 * The cause of the problem is pretty likely to be a typo, so run the
	 * checks which report errors and warnings about issues which such a
	 * typo is likely to result in.
	 */
	check_node_stats();

	bool fNotAttached = false;
	/* TRANSLATORS: At the end of processing (or if a *SOLVE command is used)
	 * cavern will issue this warning if there are any sections of the survey
	 * network which are hanging. */
	warning(/*Survey not all connected to fixed stations*/45);
	for (node *stn = stnlist; stn; stn = stn->next) {
	    prefix *name = find_non_anon_stn(stn)->name;
	    if (TSTBIT(name->sflags, SFLAGS_HANGING)) {
		/* Already reported this name as hanging. */
		continue;
	    }
	    name->sflags |= BIT(SFLAGS_HANGING);
	    if (prefix_ident(name)) {
		if (!fNotAttached) {
		    fNotAttached = true;
		    /* TRANSLATORS: Here "station" is a survey station, not a
		     * train station. */
		    puts(msg(/*The following survey stations are not attached to a fixed point:*/71));
		}
		printf("%s:%d: %s: ",
		       name->filename, name->line, msg(/*info*/485));
		print_prefix(name);
		putnl();
	    }
	}

	// We need to include hanging surveys in the count of connected
	// components so that the loop count is correct.
	//
	// To do this we walk the hanging survey network from the first entry
	// in stnlist, visiting unvisited stations and removing them from
	// stnlist.  Each time we need to start a new walk is a new component.
	while (stnlist) {
	    ++cComponents;

	    node *stn = stnlist;
	    int back = -1; // Dummy value that won't match a real direction.
	    unsigned long tos = 0;

    iter2:
	    // Colour each station we visit so we know which we've already
	    // visited.
	    stn->colour = 1;
	    remove_stn_from_list(&stnlist, stn);
	    for (int j = 0; j <= 2 && stn->leg[j]; j++) {
		if (j == back) {
		    // Ignore the reverse of the leg we just took to get
		    // here.
		    continue;
		}

		node *to = stn->leg[j]->l.to;
		if (to->colour) {
		    // Ignore station we already visited.
		    continue;
		}

		SVX_ASSERT(tos < stack_size);
		dirn_stack[tos] = back;
		tos++;
		back = reverse_leg_dirn(stn->leg[j]);
		stn = to;
		goto iter2;
    uniter2:
		--tos;
		j = reverse_leg_dirn(stn->leg[back]);
		to = stn;
		stn = to->leg[back]->l.to;
		back = dirn_stack[tos];
	    }

	    if (tos > 0) goto uniter2;
	}
    }

    free(dirn_stack);
    dirn_stack = NULL;
    free(oldest_stack);
    oldest_stack = NULL;

    SVX_ASSERT(!fixedlist);
    fixedlist = new_fixedlist;

    articulation *art = articulation_list;
    while (art) {
	SVX_ASSERT(art->stnlist);
#ifdef DEBUG_ARTIC
	printf("  Articulation (%p):\n", art->stnlist);
	putnl();
	for (node *stn = art->stnlist; stn; stn = stn->next) {
	    printf("MX: %c %p (", fixed(stn)?'*':' ', stn);
	    print_prefix(stn->name);
	    printf(")\n");
	}
#endif
	solve_matrix(art->stnlist);

	articulation *old_art = art;
	art = art->next;
	free(old_art);
    }

#ifdef DEBUG_ARTIC
    printf("done articulating\n");
#endif

#ifdef DEBUG_ARTIC
    /* test solved network */
    for (node *stn = fixedlist; stn; stn = stn->next) {
	if (prefix_ident(stn->name) && TSTBIT(stn->name->sflags, SFLAGS_FIXED)) {
	    stn->colour = 1;
	} else {
	    stn->colour = 0;
	}
	int f = 0;
	for (int d = 0; d < 3; d++) {
	    if (stn->leg[d]) {
		if (f) {
		    printf("awooga - gap in legs\n");
		}
		if (stn->leg[d]->l.reverse & FLAG_ARTICULATION) {
		    if (!(reverse_leg(stn->leg[d])->l.reverse & FLAG_ARTICULATION)) {
			printf("awooga - bad articulation (one way art)\n");
		    }
		} else {
		    if (reverse_leg(stn->leg[d])->l.reverse & FLAG_ARTICULATION) {
			printf("awooga - bad articulation (one way art)\n");
		    }
		}
	    } else {
		f = 1;
	    }
	}
    }

    colour = 2;

    while (1) {
	int c;
	do {
	    c = 0;
	    for (node *stn = fixedlist; stn; stn = stn->next) {
		int f = 0;
		for (int d = 0; d < 3; d++) {
		    if (stn->leg[d]) {
			node *stn2 = stn->leg[d]->l.to;
			if (f) {
			    printf("awooga - gap in legs\n");
			}
			if (stn2->colour) {
			    if (!(stn->leg[d]->l.reverse & FLAG_ARTICULATION)) {
				if (stn->colour == 0) {
				    stn->colour = stn2->colour;
				    c++;
				}
			    }
			}
		    } else {
			f = 1;
		    }
		}
	    }
	} while (c);

	/* colour bits */
	node *stn;
	for (stn = fixedlist; stn; stn = stn->next) {
	    if (stn->colour == 0) break;
	}
	if (!stn) break; /* all coloured */

	stn->colour = colour++;
    }

    for (node *stn = fixedlist; stn; stn = stn->next) {
	int f = 0;
	for (int d = 0; d < 3; d++) {
	    if (stn->leg[d]) {
		if (f) {
		    printf("awooga - gap in legs\n");
		}
#ifdef DEBUG_ARTIC
		if (stn->leg[d]->l.reverse & FLAG_ARTICULATION) {
		    node *stn2 = stn->leg[d]->l.to;
		    printf("art: %ld %ld [%p] ", stn->colour, stn2->colour, stn);
		    print_prefix(stn->name);
		    printf(" - [%p] ", stn2);
		    print_prefix(stn2->name);
		    printf("\n");
		}
#endif
	    } else {
		f = 1;
	    }
	}
    }
#endif

    for (node *stn = fixedlist; stn; stn = stn->next) {
	SVX_ASSERT(fixed(stn));
    }
}
