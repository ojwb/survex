/* netartic.c
 * Split up network at articulation points
 * Copyright (C) 1993-2003,2005,2012,2014,2015 Olly Betts
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
#include "matrix.h"
#include "out.h"

/* We want to split station list into a list of components, each of which
 * consists of a list of "articulations" - the first has all the fixed points
 * in that component, and the others attach sequentially to produce the whole
 * component
 */

typedef struct articulation {
   struct articulation *next;
   node *stnlist;
} articulation;

typedef struct component {
   struct component *next;
   articulation *artic;
} component;

static component *component_list;

static articulation *articulation_list;

static node *artlist;

static node *fixedlist;

static long colour;

/* The goto iter/uniter avoids using recursion which could lead to stack
 * overflow.  Instead we use 5 bytes of memory per iteration in malloc-ed
 * blocks which will probably use an awful lot less memory on most platforms.
 */

/* This is the number of station in stnlist which is a ceiling on how
 * deep visit will recurse */
static unsigned long cMaxVisits;

static unsigned char *dirn_stack = NULL;
static long *min_stack = NULL;

static unsigned long
visit(node *stn, int back)
{
   long min_colour;
   int i;
   unsigned long tos = 0;
   SVX_ASSERT(dirn_stack && min_stack);
#ifdef DEBUG_ARTIC
   printf("visit(%p, %d) called\n", stn, back);
#endif

iter:
   min_colour = stn->colour = ++colour;
#ifdef DEBUG_ARTIC
   printf("visit: stn [%p] ", stn);
   print_prefix(stn->name);
   printf(" set to colour %ld -> min\n", colour);
#endif
   for (i = 0; i <= 2 && stn->leg[i]; i++) {
      if (i != back) {
	 node *to = stn->leg[i]->l.to;
	 long col = to->colour;
	 if (col == 0) {
	    SVX_ASSERT(tos < cMaxVisits);
	    dirn_stack[tos] = back;
	    min_stack[tos] = min_colour;
	    tos++;
	    back = reverse_leg_dirn(stn->leg[i]);
	    stn = to;
	    goto iter;
uniter:
	    SVX_ASSERT(tos > 0);
	    --tos;
	    i = reverse_leg_dirn(stn->leg[back]);
	    to = stn;
	    stn = to->leg[back]->l.to;
	    back = dirn_stack[tos];
	    if (min_stack[tos] < min_colour) min_colour = min_stack[tos];

#ifdef DEBUG_ARTIC
	    printf("unwind: stn [%p] ", stn);
	    print_prefix(stn->name);
	    printf(" colour %d, min %d, station after %d\n", stn->colour,
		   min_colour, to->colour);
	    printf("Putting stn ");
	    print_prefix(to->name);
	    printf(" on artlist\n");
#endif
	    remove_stn_from_list(&stnlist, to);
	    add_stn_to_list(&artlist, to);

	    if (to->colour <= min_colour) {
	       articulation *art;

	       min_colour = to->colour;

	       /* FIXME: note down leg (<-), remove and replace:
		*                 /\   /        /\
		* [fixed point(s)]  *-*  -> [..]  )
		*                 \/   \        \/
		*                 stn to
		*/
	       /* flag leg as an articulation for loop error reporting */
	       to->leg[dirn_stack[tos]]->l.reverse |= FLAG_ARTICULATION;
	       stn->leg[i]->l.reverse |= FLAG_ARTICULATION;

	       /* start new articulation */
	       art = osnew(articulation);
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
	    if (col < 0) {
	       /* we've found a fixed point */
	       col = -col;
	       to->colour = col;
#if 0
	       /* Removing this solves Graham Mullan's problem and makes more
		* sense since it means we'll recheck this point for further
		* legs. */
#ifdef DEBUG_ARTIC
	       printf("Putting FOUND FIXED stn ");
	       print_prefix(to->name);
	       printf(" on artlist\n");
#endif
	       remove_stn_from_list(&fixedlist, to);
	       add_stn_to_list(&artlist, to);
#endif
	    }

	    if (col < min_colour) min_colour = col;
	 }
      }
   }

   SVX_ASSERT(!stn->leg[0] || stn->leg[0]->l.to->colour > 0);
   SVX_ASSERT(!stn->leg[1] || stn->leg[1]->l.to->colour > 0);
   SVX_ASSERT(!stn->leg[2] || stn->leg[2]->l.to->colour > 0);

   if (tos > 0) goto uniter;

#ifdef DEBUG_ARTIC
   printf("Putting stn ");
   print_prefix(stn->name);
   printf(" on artlist\n");
#endif
   remove_stn_from_list(&stnlist, stn);
   add_stn_to_list(&artlist, stn);
   return min_colour;
}

extern void
articulate(void)
{
   node *stn, *stnStart;
   int i;
   long cFixed;

   component_list = NULL;
   articulation_list = NULL;
   artlist = NULL;
   fixedlist = NULL;

   /* find articulation points and components */
   colour = 0;
   stnStart = NULL;
   cMaxVisits = 0;
   FOR_EACH_STN(stn, stnlist) {
      if (fixed(stn)) {
	 remove_stn_from_list(&stnlist, stn);
	 add_stn_to_list(&fixedlist, stn);
	 colour++;
	 stn->colour = -colour;
#ifdef DEBUG_ARTIC
	 printf("Putting stn ");
	 print_prefix(stn->name);
	 printf(" on fixedlist\n");
#endif
      } else {
	 cMaxVisits++;
	 stn->colour = 0;
      }
   }
   dirn_stack = osmalloc(cMaxVisits);
   min_stack = osmalloc(cMaxVisits * sizeof(long));

   /* fixedlist can be NULL here if we've had a *solve followed by survey
    * which is all hanging. */
   cFixed = colour;
   while (fixedlist) {
      int c;
      stnStart = fixedlist;
      stn = stnStart;

      /* see if this is a fresh component - it may not be, we may be
       * processing the other way from a fixed point cut-line */
      if (stn->colour < 0) {
#ifdef DEBUG_ARTIC
	 printf("new component\n");
#endif
	 stn->colour = -stn->colour; /* fixed points are negative until we colour from them */
	 cComponents++;

	 /* FIXME: logic to count components isn't the same as the logic
	  * to start a new one - we should start a new one for a fixed point
	  * cut-line (see below) */
	 if (artlist) {
	     component *comp;
	     articulation *art;

	     art = osnew(articulation);
	     art->stnlist = artlist;
	     art->next = articulation_list;
	     articulation_list = art;
	     artlist = NULL;

	     comp = osnew(component);
	     comp->next = component_list;
	     comp->artic = articulation_list;
	     component_list = comp;
	     articulation_list = NULL;
	 }

#ifdef DEBUG_ARTIC
	 print_prefix(stn->name);
	 printf(" [%p] is root of component %ld\n", stn, cComponents);
	 printf(" and colour = %d/%d\n", stn->colour, cFixed);
#endif
      }

      c = 0;
      for (i = 0; i <= 2 && stn->leg[i]; i++) {
	 node *stn2 = stn->leg[i]->l.to;
	 if (stn2->colour < 0) {
	    stn2->colour = -stn2->colour;
	 } else if (stn2->colour == 0) {
	    /* Special case to check if start station is an articulation point
	     * which it is iff we have to colour from it in more than one dirn
	     *
	     * We're looking for articulation legs - these are those where
	     * colouring from here doesn't reach a fixed point (including
	     * stn - the fixed point we've started from)
	     *
	     * FIXME: this is a "fixed point cut-line" case where we could
	     * start a new component.
	     */
	    long col = visit(stn2, reverse_leg_dirn(stn->leg[i]));
#ifdef DEBUG_ARTIC
	    print_prefix(stn->name);
	    printf(" -> ");
	    print_prefix(stn2->name);
	    printf(" col %d cFixed %d\n", col, cFixed);
#endif
	    if (col > cFixed) {
		/* start new articulation - FIXME - overeager */
		articulation *art = osnew(articulation);
		art->stnlist = artlist;
		art->next = articulation_list;
		articulation_list = art;
		artlist = NULL;
		c |= 1 << i;
	    }
	 }
      }

      switch (c) {
       /* had to colour in 2 or 3 directions from start point */
       case 3: case 5: case 6: case 7:
#ifdef DEBUG_ARTIC
	 print_prefix(stn->name);
	 printf(" is a special case start articulation point [%d]\n", c);
#endif
	 for (i = 0; i <= 2 && stn->leg[i]; i++) {
	    if (TSTBIT(c, i)) {
	       /* flag leg as an articulation for loop error reporting */
	       stn->leg[i]->l.reverse |= FLAG_ARTICULATION;
#ifdef DEBUG_ARTIC
	       print_prefix(stn->leg[i]->l.to->name);
	       putnl();
#endif
	       reverse_leg(stn->leg[i])->l.reverse |= FLAG_ARTICULATION;
	    }
	 }
      }

#ifdef DEBUG_ARTIC
      printf("Putting FIXED stn ");
      print_prefix(stn->name);
      printf(" on artlist\n");
#endif
      remove_stn_from_list(&fixedlist, stn);
      add_stn_to_list(&artlist, stn);

      if (stnStart->colour == 1) {
#ifdef DEBUG_ARTIC
	 printf("%ld components\n",cComponents);
#endif
	 break;
      }
   }

   osfree(dirn_stack);
   dirn_stack = NULL;
   osfree(min_stack);
   min_stack = NULL;

   if (artlist) {
      articulation *art = osnew(articulation);
      art->stnlist = artlist;
      art->next = articulation_list;
      articulation_list = art;
      artlist = NULL;
   }
   if (articulation_list) {
      component *comp = osnew(component);
      comp->next = component_list;
      comp->artic = articulation_list;
      component_list = comp;
      articulation_list = NULL;
   }

   if (stnlist) {
      /* Any stations still in stnlist are unfixed, which is means we have
       * one or more hanging surveys.
       *
       * The cause of the problem is pretty likely to be a typo, so run the
       * checks which report errors and warnings about issues which such a
       * typo is likely to result in.
       */
      check_node_stats();

      /* Actually this error is fatal, but we want to list the survey
       * stations which aren't connected, so we report it as an error
       * and die after listing them...
       */
      bool fNotAttached = false;
      /* TRANSLATORS: At the end of processing (or if a *SOLVE command is used)
       * cavern will issue this error if there are any sections of the survey
       * network which are hanging. */
      warning(/*Survey not all connected to fixed stations*/45);
      FOR_EACH_STN(stn, stnlist) {
	 /* Anonymous stations must be at the end of a trailing traverse (since
	  * the same anonymous station can't be referred to more than once),
	  * and trailing traverses have been removed at this point.
	  *
	  * However, we may remove a trailing traverse back to an anonymous
	  * station.  FIXME: It's not helpful to fail to point to a station
	  * in such a case - it would be much nicer to look through the list
	  * of trailing traverses in such a case to find a relevant traverse
	  * and then report a station name from there.
	  */
	 /* SVX_ASSERT(!TSTBIT(stn->name->sflags, SFLAGS_ANON)); */
	 if (stn->name->ident) {
	    if (!fNotAttached) {
	       fNotAttached = true;
	       /* TRANSLATORS: Here "station" is a survey station, not a train
		* station. */
	       puts(msg(/*The following survey stations are not attached to a fixed point:*/71));
	    }
	    printf("%s:%d: %s: ", stn->name->filename, stn->name->line, msg(/*info*/485));
	    print_prefix(stn->name);
	    putnl();
	 }
      }
      //exit(EXIT_FAILURE);
      stnlist = NULL;
   }

   {
      component *comp = component_list;

#ifdef DEBUG_ARTIC
      printf("\nDump of %d components:\n", cComponents);
#endif
      while (comp) {
	 node *list = NULL, *listend = NULL;
	 articulation *art;
	 component * old_comp;
#ifdef DEBUG_ARTIC
	 printf("Component:\n");
#endif
	 SVX_ASSERT(comp->artic);
	 art = comp->artic;
	 while (art) {
	    articulation * old_art;
#ifdef DEBUG_ARTIC
	    printf("  Articulation (%p):\n", art->stnlist);
#endif
	    SVX_ASSERT(art->stnlist);
	    if (listend) {
	       listend->next = art->stnlist;
	       art->stnlist->prev = listend;
	    } else {
	       list = art->stnlist;
	    }

	    FOR_EACH_STN(stn, art->stnlist) {
#ifdef DEBUG_ARTIC
	       printf("    %d %p (", stn->colour, stn);
	       print_prefix(stn->name);
	       printf(")\n");
#endif
	       listend = stn;
	    }
	    old_art = art;
	    art = art->next;
	    osfree(old_art);
	 }
#ifdef DEBUG_ARTIC
	 putnl();
	 FOR_EACH_STN(stn, list) {
	    printf("MX: %c %p (", fixed(stn)?'*':' ', stn);
	    print_prefix(stn->name);
	    printf(")\n");
	 }
#endif
	 solve_matrix(list);
#ifdef DEBUG_ARTIC
	 putnl();
	 FOR_EACH_STN(stn, list) {
	    printf("%c %p (", fixed(stn)?'*':' ', stn);
	    print_prefix(stn->name);
	    printf(")\n");
	 }
#endif
	 listend->next = stnlist;
	 if (stnlist) stnlist->prev = listend;
	 stnlist = list;

	 old_comp = comp;
	 comp = comp->next;
	 osfree(old_comp);
      }
#ifdef DEBUG_ARTIC
      printf("done articulating\n");
#endif
   }

#ifdef DEBUG_ARTIC
   /* test articulation */
   FOR_EACH_STN(stn, stnlist) {
      int d;
      int f;
      if (stn->name->ident && TSTBIT(stn->name->sflags, SFLAGS_FIXED)) {
	 stn->colour = 1;
      } else {
	 stn->colour = 0;
      }
      f = 0;
      for (d = 0; d < 3; d++) {
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
      int f;
      do {
	 c = 0;
	 FOR_EACH_STN(stn, stnlist) {
	    int d;
	    f = 0;
	    for (d = 0; d < 3; d++) {
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
      FOR_EACH_STN(stn, stnlist) {
	 if (stn->colour == 0) break;
      }
      if (!stn) break; /* all coloured */

      stn->colour = colour++;
   }

   FOR_EACH_STN(stn, stnlist) {
      int d;
      int f;
      f = 0;
      for (d = 0; d < 3; d++) {
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
   FOR_EACH_STN(stn, stnlist) {
      SVX_ASSERT(fixed(stn));
   }
}
