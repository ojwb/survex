/* > netartic.c
 * Split up network at articulation points
 * Copyright (C) 1993-2000 Olly Betts
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

#if 0
# define DEBUG_INVALID 1
# define DEBUG_ARTIC
#endif

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "debug.h"
#include "cavern.h"
#include "filename.h"
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

static node *artlist;

static node *fixedlist;

/* list item visit */
/* FIXME: we shouldn't malloc each and every LIV separately! */
typedef struct LIV {
   struct LIV *next;
   uchar dirn;
} liv;

/* The goto iter/uniter avoids using recursion which could lead to stack
 * overflow.  Instead we use a linked list which will probably use
 * much less memory on most compilers.
 */

static long colour;

static ulong
visit(node *stn, int back)
{
   long min;
   int i;
   node *stn2;
   liv *livTos = NULL, *livTmp;
   int artic_flag = 0;
#ifdef DEBUG_ARTIC
   printf("visit(%p, %d) called\n", stn, back);
#endif

iter:
   min = ++colour;
#ifdef DEBUG_ARTIC
   printf("visit: stn [%p] ", stn);
   print_prefix(stn->name);
   printf(" set to colour %d -> min ", colour);
#endif
   for (i = 0; i <= 2 && stn->leg[i]; i++) {
      if (i != back) {
	 node *to = stn->leg[i]->l.to;
	 long c = to->colour;

	 if (c < 0) {
	    /* we've found a fixed point */
	    c = -c;
	    to->colour = c;
#ifdef DEBUG_ARTIC
	    printf("Putting FOUND FIXED stn ");
	    print_prefix(to->name);
	    printf(" on artlist\n");
#endif
	    remove_stn_from_list(&fixedlist, to);
	    add_stn_to_list(&artlist, to);
	 }

	 if (c && c < min) min = c;
      }
   }
   stn->colour = min;
#ifdef DEBUG_ARTIC
   printf("%d\n", min);
#endif
   for (i = 0; i <= 2 && stn->leg[i]; i++) {
      if (stn->leg[i]->l.to->colour == 0) {
	 livTmp = osnew(liv);
	 livTmp->next = livTos;
	 livTos = livTmp;
	 livTos->dirn = back = reverse_leg_dirn(stn->leg[i]);
	 stn = stn->leg[i]->l.to;
	 goto iter;
uniter:
	 i = reverse_leg_dirn(stn->leg[livTos->dirn]);
	 stn2 = stn->leg[livTos->dirn]->l.to;

#ifdef DEBUG_ARTIC
	 printf("unwind: stn [%p] ", stn2);
	 print_prefix(stn2->name);
	 printf(" colour %d, min %d, station after %d\n", stn2->colour,
		min, stn->colour);
#endif

	 /* FIXME: hmm, this code looks like it may get called more than once,
	  * or never - actually it seems to always get called exactly once! */
#ifdef DEBUG_ARTIC
	 printf("Putting stn ");
	 print_prefix(stn->name);
	 printf(" on artlist\n");
#endif
	 remove_stn_from_list(&stnlist, stn);
	 add_stn_to_list(&artlist, stn);

#ifdef DEBUG_ARTIC
	 printf(">> %p\n", stn);
#endif

	 if (artic_flag) {
	    articulation *art;

	    artic_flag = 0;
	    /* FIXME: note down leg (<-), remove and replace:
	     *                 /\   /        /\
             * [fixed point(s)]  *-*  -> [..]  )
	     *                 \/   \        \/
	     *                stn2 stn
	     */
#if 0
	    /* flag leg as an articulation for loop error reporting */
	    stn->leg[livTos->dirn]->l.reverse |= FLAG_ARTICULATION;
	    stn2->leg[i]->l.reverse |= FLAG_ARTICULATION;
#endif
	     
	    /* start new articulation */
	    component_list->artic->stnlist = artlist;
	    artlist = NULL;

	    art = osnew(articulation);
	    art->next = component_list->artic;
	    component_list->artic = art;
#if 1 /*def DEBUG_ARTIC*/
	    printf("Articulate *-");
	    print_prefix(stn2->name);
	    printf("-");
	    print_prefix(stn->name);
	    printf("-...\n");
#endif
	 }

	 if (stn2->colour == min) {
	    artic_flag = 1;
	 } else if (stn2->colour < min) {
	    min = stn2->colour;
	 }

	 stn = stn2;
	 livTmp = livTos;
	 livTos = livTos->next;
	 osfree(livTmp);
      }
   }
   if (livTos) goto uniter;
#ifdef DEBUG_ARTIC
   printf("Putting stn ");
   print_prefix(stn->name);
   printf(" on artlist\n");
#endif
   remove_stn_from_list(&stnlist, stn);
   add_stn_to_list(&artlist, stn);
   return min;
}

extern void
articulate(void)
{
   node *stn, *stnStart;
   int i;
#ifdef DEBUG_ARTIC
   ulong cFixed;
#endif

   component_list = NULL;
   artlist = NULL;
   fixedlist = NULL;

   /* find articulation points and components */
   colour = 0;
   stnStart = NULL;
   FOR_EACH_STN(stn, stnlist) {
      if (fixed(stn)) {
	 colour++;
	 stn->colour = -colour;
#ifdef DEBUG_ARTIC
	 printf("Putting stn ");
	 print_prefix(stn->name);
	 printf(" on fixedlist\n");
#endif
	 remove_stn_from_list(&stnlist, stn);
	 add_stn_to_list(&fixedlist, stn);
      } else {
	 stn->colour = 0;
      }
   }
   stnStart = fixedlist;
   ASSERT2(stnStart,"no fixed points!");
#ifdef DEBUG_ARTIC
   cFixed = colour;
#endif
   while (1) {
      int c;
      stn = stnStart;

      /* see if this is a fresh component - it may not be, we may be
       * processing the other way from a fixed point cut-line */
      if (stn->colour < 0) {
	 component *comp;
	 articulation *art;

#ifdef DEBUG_ARTIC
	 printf("new component\n");
#endif

	 stn->colour = -stn->colour; /* fixed points are negative until we colour from them */
	 cComponents++;

	 /* FIXME: logic to count components isn't the same as the logic
	  * to start a new one - we should start a new one for a fixed point
	  * cut-line I think */
	 if (component_list) component_list->artic->stnlist = artlist;

	 art = osnew(articulation);
	 art->next = NULL;
	 artlist = NULL;

	 comp = osnew(component);
	 comp->next = component_list;
	 comp->artic = art;
	 component_list = comp;
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
	     */
	    if (c) {
	       /* FIXME: stn2 is an articulation point! */
#if 0
	       /* flag leg as an articulation for loop error reporting */
	       stn->leg[i]->l.reverse |= FLAG_ARTICULATION;
	       reverse_leg(stn->leg[i])->l.reverse |= FLAG_ARTICULATION;
#endif
#if 1 /*def DEBUG_ARTIC*/
	       print_prefix(stn2->name);
	       printf(" is a special case start articulation point\n");
#endif
	    }

	    c++;
	    visit(stn2, reverse_leg_dirn(stn->leg[i]));
	 }
      }

#ifdef DEBUG_ARTIC
      printf("Putting FIXED stn ");
      print_prefix(stn->name);
      printf(" on artlist\n");
#endif
      remove_stn_from_list(&fixedlist, stn);
      add_stn_to_list(&artlist, stn);

#if 0 /* def DEBUG_ARTIC */
      printf("station colours:\n");
      FOR_EACH_STN(stn, stnlist) {
	 printf("%ld - ", stn->colour);
	 print_prefix(stn->name);
	 putnl();
      }
#endif
      if (stnStart->colour == 1) {
#ifdef DEBUG_ARTIC
	 printf("%ld components\n",cComponents);
#endif
	 break;
      }
      stnStart = fixedlist;
      if (!stnStart) break;
   }
   if (component_list) component_list->artic->stnlist = artlist;
   /* if (artlist) component_list->artic->stnlist = artlist; */

     {
	component *comp;
	articulation *art;
	node *stn;

	/* reverse component list */
	component *rev = NULL;
	comp = component_list;
	while (comp) {
	   component *tmp = comp->next;
	   comp->next = rev;
	   rev = comp;
	   comp = tmp;
	}
	component_list = rev;

#ifdef DEBUG_ARTIC
	printf("\nDump of %d components:\n", cComponents);
#endif
	for (comp = component_list; comp; comp = comp->next) {
	   node *list = NULL, *listend = NULL;
#ifdef DEBUG_ARTIC
	   printf("Component:\n");
#endif
	   ASSERT(comp->artic);
	   for (art = comp->artic; art; art = art->next) {
#ifdef DEBUG_ARTIC
	      printf("  Articulation (%p):\n", art->stnlist);
#endif
	      if (listend) {
		 listend->next = art->stnlist;
		 if (art->stnlist) art->stnlist->prev = listend;
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
	}
#ifdef DEBUG_ARTIC
	printf("done articulating\n");
#endif
     }

#if 0 /*def DEBUG_ARTIC*/
   /* highlight unfixed bits */
   FOR_EACH_STN(stn, stnlist) {
      if (!fixed(stn)) {
	 print_prefix(stn->name);
	 printf(" [%p] UNFIXED\n", stn);
      }
   }
#endif
}
