/* > netartic.c
 * Split up network at articulation points
 * Copyright (C) 1993-2001 Olly Betts
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

#include "debug.h"
#include "cavern.h"
#include "filename.h"
#include "message.h"
#include "netartic.h"
#include "netbits.h"
#include "matrix.h"
#include "out.h"

static node *fixedlist;

static long colour;

/* The goto iter/uniter avoids using recursion which could lead to stack
 * overflow.  Instead we use one byte of memory per iteration in a malloc-ed
 * block which will probably use an awful lot less memory on most platforms.
 */

/* This is the number of station in stnlist which is a ceiling on how
 * deep visit will recurse */
static unsigned long cMaxVisits;

static unsigned char *dirn_stack = NULL;
static unsigned long *min_stack = NULL;

static unsigned long
visit(node *stn, int back)
{   
   long min_colour;
   int i;
   node *stn2;
   unsigned long tos = 0;
   ASSERT(dirn_stack && min_stack);

iter:
   min_colour = stn->colour = ++colour;
   for (i = 0; i <= 2 && stn->leg[i]; i++) {
      if (i != back) {	    
	 node *to = stn->leg[i]->l.to;
	 long col = to->colour;
	 if (col == 0) {
	    ASSERT(tos < cMaxVisits);
	    dirn_stack[tos] = back = reverse_leg_dirn(stn->leg[i]);
	    min_stack[tos] = min_colour;
	    tos++;
	    stn = to;
	    goto iter;
uniter:
	    ASSERT(tos > 0);
	    --tos;
	    i = reverse_leg_dirn(stn->leg[dirn_stack[tos]]);
	    stn2 = stn->leg[dirn_stack[tos]]->l.to;
	    if (min_stack[tos] < min_colour) min_colour = min_stack[tos];
	    
	    if (stn->colour <= min_colour) {
	       min_colour = stn->colour;
	       
	       /* FIXME: note down leg (<-), remove and replace:
		*                 /\   /        /\
		* [fixed point(s)]  *-*  -> [..]  )
		*                 \/   \        \/
		*                stn2 stn
		*/
	       /* flag leg as an articulation for loop error reporting */
	       stn->leg[dirn_stack[tos]]->l.reverse |= FLAG_ARTICULATION;
	       stn2->leg[i]->l.reverse |= FLAG_ARTICULATION;
	    }

	    stn = stn2;
	 } else {
	    /* back edge case */
	    if (col < 0) {
	       /* we've found a fixed point */
	       col = -col;
	       to->colour = col;
	       remove_stn_from_list(&fixedlist, to);
	       add_stn_to_list(&stnlist, to);
	    }
	    
	    if (col < min_colour) min_colour = col;
	 }	 
      }
   }
   
   if (tos > 0) goto uniter;

   return min_colour;
}

extern void
articulate(void)
{
   node *stn, *stnStart;
   int i;
   long cFixed;

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
      } else {
	 cMaxVisits++;
	 stn->colour = 0;
      }
   }
   dirn_stack = osmalloc(cMaxVisits);
   min_stack = osmalloc(cMaxVisits * sizeof(long));

   stnStart = fixedlist;
   ASSERT2(stnStart,"no fixed points!");
   cFixed = colour;
   while (1) {
      int c;
      stn = stnStart;

      /* see if this is a fresh component - it may not be, we may be
       * processing the other way from a fixed point cut-line */
      if (stn->colour < 0) {
	 stn->colour = -stn->colour; /* fixed points are negative until we colour from them */
	 cComponents++;
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
	    if (col > cFixed) {
		c |= 1 << i;
	    }
	 }
      }

      if (c) {
	 for (i = 0; i <= 2 && stn->leg[i]; i++) {
	    if (TSTBIT(c, i)) {
	       /* flag leg as an articulation for loop error reporting */
	       stn->leg[i]->l.reverse |= FLAG_ARTICULATION;
	       reverse_leg(stn->leg[i])->l.reverse |= FLAG_ARTICULATION;	       
	    }
	 }
      }

      remove_stn_from_list(&fixedlist, stn);
      add_stn_to_list(&stnlist, stn);

      if (stnStart->colour == 1) break;

      stnStart = fixedlist;
      if (!stnStart) break;
   }

#if 0
   /* test articulation */
   FOR_EACH_STN(stn, stnlist) {
      int d;
      int f;
      if (fixed(stn)) {
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
	    if (stn->leg[d]->l.reverse & FLAG_ARTICULATION) {
	       node *stn2 = stn->leg[d]->l.to;
	       printf("art: %d %d [%p] ", stn->colour, stn2->colour, stn);
	       print_prefix(stn->name);
	       printf(" - [%p] ", stn2);
	       print_prefix(stn2->name);
	       printf("\n");
	    }
	 } else {
	    f = 1;
	 }
      }
   }
#endif

   solve_matrix(stnlist);

   osfree(dirn_stack);
   osfree(min_stack);
   dirn_stack = NULL;
   min_stack = NULL;
}
