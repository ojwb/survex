/* network.c
 * Survex network reduction - find patterns and apply network reductions
 * Copyright (C) 1991-2002,2005,2024 Olly Betts
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
#define DEBUG_INVALID 1
#define VALIDATE 1
#define DUMP_NETWORK 1
#endif

#include <config.h>

#include "validate.h"
#include "debug.h"
#include "cavern.h"
#include "message.h"
#include "netbits.h"
#include "network.h"
#include "out.h"

typedef struct reduction {
   struct reduction *next;
   enum {
       TYPE_PARALLEL,
       TYPE_LOLLIPOP,
       TYPE_DELTASTAR,
   } type;
   linkfor* join[];
} reduction;

#define allocate_reduction(N) osmalloc(sizeof(reduction) + (N) * sizeof(linkfor*))

// Head of linked list of reductions.
static reduction *reduction_stack;

/* can be altered by -z<letters> on command line */
unsigned long optimize = BITA('l') | BITA('p') | BITA('d');
/* Lollipops, Parallel legs, Iterate mx, Delta* */

extern void
remove_subnets(void)
{
   node *stn, *stn2, *stn3, *stn4;
   int dirn, dirn2, dirn3, dirn4;
   reduction *trav;
   linkfor *newleg, *newleg2;
   bool fMore = true;

   reduction_stack = NULL;

   out_current_action(msg(/*Simplifying network*/129));

   while (fMore) {
      fMore = false;
      if (optimize & BITA('l')) {
#if PRINT_NETBITS
	 printf("replacing lollipops\n");
#endif
	 /* NB can have non-fixed 0 nodes */
	 FOR_EACH_STN(stn, stnlist) {
	    if (three_node(stn)) {
	       dirn = -1;
	       if (stn->leg[1]->l.to == stn) dirn++;
	       if (stn->leg[0]->l.to == stn) dirn += 2;
	       if (dirn < 0) continue;

	       stn2 = stn->leg[dirn]->l.to;
	       if (fixed(stn2)) {
		   /*    _
		    *   ( )
		    *    * stn
		    *    |
		    *    * stn2 (fixed)
		    *    : (may have other connections)
		    *
		    * The leg forming the "stick" of the lollipop is
		    * articulating so we can just fix stn with coordinates
		    * calculated by adding or subtracting the leg's vector.
		    */
		   linkfor *leg = stn->leg[dirn];
		   linkfor *rev_leg = reverse_leg(leg);
		   leg->l.reverse |= FLAG_ARTICULATION;
		   rev_leg->l.reverse |= FLAG_ARTICULATION;
		   if (data_here(leg)) {
		       subdd(&POSD(stn), &POSD(stn2), &leg->d);
		   } else {
		       adddd(&POSD(stn), &POSD(stn2), &rev_leg->d);
		   }
		   remove_stn_from_list(&stnlist, stn);
		   add_stn_to_list(&fixedlist, stn);
		   continue;
	       }

	       SVX_ASSERT(three_node(stn2));

	       /*        _
		*       ( )
		*        * stn
		*        |
		*        * stn2
		*       / \
		* stn4 *   * stn3  -->  stn4 *---* stn3
		*      :   :                 :   :
		*/
	       dirn2 = reverse_leg_dirn(stn->leg[dirn]);
	       dirn2 = (dirn2 + 1) % 3;
	       stn3 = stn2->leg[dirn2]->l.to;
	       if (stn2 == stn3) continue; /* dumb-bell - leave alone */

	       dirn3 = reverse_leg_dirn(stn2->leg[dirn2]);

	       trav = allocate_reduction(2);
	       trav->type = TYPE_LOLLIPOP;

	       newleg2 = (linkfor*)osnew(linkcommon);

	       newleg = copy_link(stn3->leg[dirn3]);

	       dirn2 = (dirn2 + 1) % 3;
	       stn4 = stn2->leg[dirn2]->l.to;
	       dirn4 = reverse_leg_dirn(stn2->leg[dirn2]);
#if 0
	       printf("Lollipop found with stn...stn4 = \n");
	       print_prefix(stn->name); putnl();
	       print_prefix(stn2->name); putnl();
	       print_prefix(stn3->name); putnl();
	       print_prefix(stn4->name); putnl();
#endif

	       addto_link(newleg, stn2->leg[dirn2]);

	       /* remove stn and stn2 */
	       remove_stn_from_list(&stnlist, stn);
	       remove_stn_from_list(&stnlist, stn2);

	       /* stack lollipop and replace with a leg between stn3 and stn4 */
	       trav->join[0] = stn3->leg[dirn3];
	       newleg->l.to = stn4;
	       newleg->l.reverse = dirn4 | FLAG_DATAHERE | FLAG_REPLACEMENTLEG;

	       trav->join[1] = stn4->leg[dirn4];
	       newleg2->l.to = stn3;
	       newleg2->l.reverse = dirn3 | FLAG_REPLACEMENTLEG;

	       stn3->leg[dirn3] = newleg;
	       stn4->leg[dirn4] = newleg2;

	       trav->next = reduction_stack;
#if PRINT_NETBITS
	       printf("remove lollipop\n");
#endif
	       reduction_stack = trav;
	       fMore = true;
	    }
	 }
      }

      if (optimize & BITA('p')) {
#if PRINT_NETBITS
	 printf("replacing parallel legs\n");
#endif
	 FOR_EACH_STN(stn, stnlist) {
	    /*
	     *  :            :
	     *  * stn3       * stn3
	     *  |            |
	     *  * stn        |
	     * ( )      -->  |
	     *  * stn2       |
	     *  |            |
	     *  * stn4       * stn4
	     *  :            :
	     */
	    if (three_node(stn)) {
	       stn2 = stn->leg[0]->l.to;
	       if (stn2 == stn->leg[1]->l.to) {
		  dirn = 2;
	       } else if (stn2 == stn->leg[2]->l.to) {
		  dirn = 1;
	       } else {
		  if (stn->leg[1]->l.to != stn->leg[2]->l.to) continue;
		  stn2 = stn->leg[1]->l.to;
		  dirn = 0;
	       }

	       /* stn == stn2 => lollipop */
	       if (stn == stn2 || fixed(stn2)) continue;

	       SVX_ASSERT(three_node(stn2));

	       stn3 = stn->leg[dirn]->l.to;
	       /* 3 parallel legs (=> nothing else) so leave */
	       if (stn3 == stn2) continue;

	       dirn3 = reverse_leg_dirn(stn->leg[dirn]);
	       dirn2 = (0 + 1 + 2 - reverse_leg_dirn(stn->leg[(dirn + 1) % 3])
			- reverse_leg_dirn(stn->leg[(dirn + 2) % 3]));

	       stn4 = stn2->leg[dirn2]->l.to;
	       dirn4 = reverse_leg_dirn(stn2->leg[dirn2]);

	       trav = allocate_reduction(2);
	       trav->type = TYPE_PARALLEL;

	       newleg = copy_link(stn->leg[(dirn + 1) % 3]);
	       /* use newleg2 for scratch */
	       newleg2 = copy_link(stn->leg[(dirn + 2) % 3]);
		 {
#ifdef NO_COVARIANCES
		    vars sum;
		    var prod;
		    delta temp, temp2;
		    addss(&sum, &newleg->v, &newleg2->v);
		    SVX_ASSERT2(!fZeros(&sum), "loop of zero variance found");
		    mulss(&prod, &newleg->v, &newleg2->v);
		    mulsd(&temp, &newleg2->v, &newleg->d);
		    mulsd(&temp2, &newleg->v, &newleg2->d);
		    adddd(&temp, &temp, &temp2);
		    divds(&newleg->d, &temp, &sum);
		    sdivvs(&newleg->v, &prod, &sum);
#else
		    svar inv1, inv2, sum;
		    delta temp, temp2;
		    /* if leg one is an equate, we can just ignore leg two
		     * whatever it is */
		    if (invert_svar(&inv1, &newleg->v)) {
		       if (invert_svar(&inv2, &newleg2->v)) {
			  addss(&sum, &inv1, &inv2);
			  if (!invert_svar(&newleg->v, &sum)) {
			     BUG("matrix singular in parallel legs replacement");
			  }

			  mulsd(&temp, &inv1, &newleg->d);
			  mulsd(&temp2, &inv2, &newleg2->d);
			  adddd(&temp, &temp, &temp2);
			  mulsd(&newleg->d, &newleg->v, &temp);
		       } else {
			  /* leg two is an equate, so just ignore leg 1 */
			  linkfor *tmpleg;
			  tmpleg = newleg;
			  newleg = newleg2;
			  newleg2 = tmpleg;
		       }
		    }
#endif
		 }
	       osfree(newleg2);
	       newleg2 = (linkfor*)osnew(linkcommon);

	       addto_link(newleg, stn2->leg[dirn2]);
	       addto_link(newleg, stn3->leg[dirn3]);

#if 0
	       printf("Parallel found with stn...stn4 = \n");
	       (dump_node)(stn); (dump_node)(stn2); (dump_node)(stn3); (dump_node)(stn4);
	       printf("dirns = %d %d %d %d\n", dirn, dirn2, dirn3, dirn4);
#endif
	       SVX_ASSERT2(stn3->leg[dirn3]->l.to == stn, "stn3 end of || doesn't recip");
	       SVX_ASSERT2(stn4->leg[dirn4]->l.to == stn2, "stn4 end of || doesn't recip");
	       SVX_ASSERT2(stn->leg[(dirn+1)%3]->l.to == stn2 && stn->leg[(dirn + 2) % 3]->l.to == stn2, "|| legs aren't");

	       /* remove stn and stn2 (already discarded triple parallel) */
	       /* so stn!=stn4 <=> stn2!=stn3 */
	       remove_stn_from_list(&stnlist, stn);
	       remove_stn_from_list(&stnlist, stn2);

	       /* stack parallel and replace with a leg between stn3 and stn4 */
	       trav->join[0] = stn3->leg[dirn3];
	       newleg->l.to = stn4;
	       newleg->l.reverse = dirn4 | FLAG_DATAHERE | FLAG_REPLACEMENTLEG;

	       trav->join[1] = stn4->leg[dirn4];
	       newleg2->l.to = stn3;
	       newleg2->l.reverse = dirn3 | FLAG_REPLACEMENTLEG;

	       stn3->leg[dirn3] = newleg;
	       stn4->leg[dirn4] = newleg2;

	       trav->next = reduction_stack;
#if PRINT_NETBITS
	       printf("remove parallel\n");
#endif
	       reduction_stack = trav;
	       fMore = true;
	    }
	 }
      }

      if (optimize & BITA('d')) {
	 node *stn5, *stn6;
	 int dirn5, dirn6;
	 linkfor *legAB, *legBC, *legCA;
#if PRINT_NETBITS
	 printf("replacing deltas with stars\n");
#endif
	 FOR_EACH_STN(stn, stnlist) {
	    /*    printf("*");*/
	    /*
	     *          :                     :
	     *          * stn5                * stn5
	     *          |                     |
	     *          * stn2                |
	     *         / \        -->         O stnZ
	     *        |   |                  / \
	     *    stn *---* stn3            /   \
	     *       /     \               /     \
	     * stn4 *       * stn6   stn4 *       * stn6
	     *      :       :             :       :
	     */
	    if (three_node(stn)) {
	       for (int dirn12 = 0; dirn12 <= 2; dirn12++) {
		  stn2 = stn->leg[dirn12]->l.to;
		  if (stn2 == stn || fixed(stn2)) continue;
		  SVX_ASSERT(three_node(stn2));
		  int dirn13 = (dirn12 + 1) % 3;
		  stn3 = stn->leg[dirn13]->l.to;
		  if (stn3 == stn || stn3 == stn2 || fixed(stn3)) continue;
		  SVX_ASSERT(three_node(stn3));
		  int dirn23 = reverse_leg_dirn(stn->leg[dirn12]);
		  dirn23 = (dirn23 + 1) % 3;
		  if (stn2->leg[dirn23]->l.to != stn3) {
		      dirn23 = (dirn23 + 1) % 3;
		      if (stn2->leg[dirn23]->l.to != stn3) {
			  continue;
		      }
		  }
		  legAB = copy_link(stn->leg[dirn12]);
		  legBC = copy_link(stn2->leg[dirn23]);
		  legCA = copy_link(stn3->leg[reverse_leg_dirn(stn->leg[dirn13])]);
		  dirn = (0 + 1 + 2) - dirn12 - dirn13;
		  dirn2 = (0 + 1 + 2) - dirn23 - reverse_leg_dirn(stn->leg[dirn12]);
		  dirn3 = (0 + 1 + 2) - reverse_leg_dirn(stn->leg[dirn13]) - reverse_leg_dirn(stn2->leg[dirn23]);
		  stn4 = stn->leg[dirn]->l.to;
		  stn5 = stn2->leg[dirn2]->l.to;
		  stn6 = stn3->leg[dirn3]->l.to;
		  if (stn4 == stn2 || stn4 == stn3 || stn5 == stn3) continue;
		  dirn4 = reverse_leg_dirn(stn->leg[dirn]);
		  dirn5 = reverse_leg_dirn(stn2->leg[dirn2]);
		  dirn6 = reverse_leg_dirn(stn3->leg[dirn3]);
#if 0
		  printf("delta-star, stn ... stn6 are:\n");
		  (dump_node)(stn);
		  (dump_node)(stn2);
		  (dump_node)(stn3);
		  (dump_node)(stn4);
		  (dump_node)(stn5);
		  (dump_node)(stn6);
#endif
		  SVX_ASSERT(stn4->leg[dirn4]->l.to == stn);
		  SVX_ASSERT(stn5->leg[dirn5]->l.to == stn2);
		  SVX_ASSERT(stn6->leg[dirn6]->l.to == stn3);

		  trav = allocate_reduction(3);
		  trav->type = TYPE_DELTASTAR;
		  {
		    linkfor *legAZ, *legBZ, *legCZ;
		    node *stnZ;
		    prefix *nameZ;
		    svar invAB, invBC, invCA, tmp, sum, inv;
		    var vtmp;
		    svar sumAZBZ, sumBZCZ, sumCZAZ;
		    delta temp, temp2;

		    /* FIXME: ought to handle cases when some legs are
		     * equates, but handle as a special case maybe? */
		    if (!invert_svar(&invAB, &legAB->v)) break;
		    if (!invert_svar(&invBC, &legBC->v)) break;
		    if (!invert_svar(&invCA, &legCA->v)) break;

		    addss(&sum, &legBC->v, &legCA->v);
		    addss(&tmp, &sum, &legAB->v);
		    if (!invert_svar(&inv, &tmp)) {
		       /* impossible - loop of zero variance */
		       BUG("loop of zero variance found");
		    }

		    legAZ = osnew(linkfor);
		    legBZ = osnew(linkfor);
		    legCZ = osnew(linkfor);

		    /* AZBZ */
		    /* done above: addvv(&sum, &legBC->v, &legCA->v); */
		    mulss(&vtmp, &sum, &inv);
		    smulvs(&sumAZBZ, &vtmp, &legAB->v);

		    adddd(&temp, &legBC->d, &legCA->d);
		    divds(&temp2, &temp, &sum);
		    mulsd(&temp, &invAB, &legAB->d);
		    subdd(&temp, &temp2, &temp);
		    mulsd(&legBZ->d, &sumAZBZ, &temp);

		    /* leg vectors after transform are determined up to
		     * a constant addition, so arbitrarily fix AZ = 0 */
		    legAZ->d[2] = legAZ->d[1] = legAZ->d[0] = 0;

		    /* BZCZ */
		    addss(&sum, &legCA->v, &legAB->v);
		    mulss(&vtmp, &sum, &inv);
		    smulvs(&sumBZCZ, &vtmp, &legBC->v);

		    /* CZAZ */
		    addss(&sum, &legAB->v, &legBC->v);
		    mulss(&vtmp, &sum, &inv);
		    smulvs(&sumCZAZ, &vtmp, &legCA->v);

		    adddd(&temp, &legAB->d, &legBC->d);
		    divds(&temp2, &temp, &sum);
		    mulsd(&temp, &invCA, &legCA->d);
		    /* NB: swapped arguments to negate answer for legCZ->d */
		    subdd(&temp, &temp, &temp2);
		    mulsd(&legCZ->d, &sumCZAZ, &temp);

		    osfree(legAB);
		    osfree(legBC);
		    osfree(legCA);

		    /* Now add two, subtract third, and scale by 0.5 */
		    addss(&sum, &sumAZBZ, &sumCZAZ);
		    subss(&sum, &sum, &sumBZCZ);
		    mulsc(&legAZ->v, &sum, 0.5);

		    addss(&sum, &sumBZCZ, &sumAZBZ);
		    subss(&sum, &sum, &sumCZAZ);
		    mulsc(&legBZ->v, &sum, 0.5);

		    addss(&sum, &sumCZAZ, &sumBZCZ);
		    subss(&sum, &sum, &sumAZBZ);
		    mulsc(&legCZ->v, &sum, 0.5);

		    nameZ = osnew(prefix);
		    nameZ->pos = osnew(pos);
		    nameZ->ident.p = NULL;
		    nameZ->shape = 3;
		    stnZ = osnew(node);
		    stnZ->name = nameZ;
		    nameZ->stn = stnZ;
		    nameZ->up = NULL;
		    nameZ->min_export = nameZ->max_export = 0;
		    unfix(stnZ);
		    add_stn_to_list(&stnlist, stnZ);
		    legAZ->l.to = stnZ;
		    legAZ->l.reverse = 0 | FLAG_DATAHERE | FLAG_REPLACEMENTLEG;
		    legBZ->l.to = stnZ;
		    legBZ->l.reverse = 1 | FLAG_DATAHERE | FLAG_REPLACEMENTLEG;
		    legCZ->l.to = stnZ;
		    legCZ->l.reverse = 2 | FLAG_DATAHERE | FLAG_REPLACEMENTLEG;
		    stnZ->leg[0] = (linkfor*)osnew(linkcommon);
		    stnZ->leg[1] = (linkfor*)osnew(linkcommon);
		    stnZ->leg[2] = (linkfor*)osnew(linkcommon);
		    stnZ->leg[0]->l.to = stn4;
		    stnZ->leg[0]->l.reverse = dirn4;
		    stnZ->leg[1]->l.to = stn5;
		    stnZ->leg[1]->l.reverse = dirn5;
		    stnZ->leg[2]->l.to = stn6;
		    stnZ->leg[2]->l.reverse = dirn6;
		    addto_link(legAZ, stn4->leg[dirn4]);
		    addto_link(legBZ, stn5->leg[dirn5]);
		    addto_link(legCZ, stn6->leg[dirn6]);
		    /* stack stuff */
		    trav->join[0] = stn4->leg[dirn4];
		    trav->join[1] = stn5->leg[dirn5];
		    trav->join[2] = stn6->leg[dirn6];
		    trav->next = reduction_stack;
#if PRINT_NETBITS
		    printf("remove delta*\n");
#endif
		    reduction_stack = trav;
		    fMore = true;

		    remove_stn_from_list(&stnlist, stn);
		    remove_stn_from_list(&stnlist, stn2);
		    remove_stn_from_list(&stnlist, stn3);
		    stn4->leg[dirn4] = legAZ;
		    stn5->leg[dirn5] = legBZ;
		    stn6->leg[dirn6] = legCZ;
		  }
		  break;
	       }
	    }
	 }
      }

   }
}

extern void
replace_subnets(void)
{
   node *stn2, *stn3, *stn4;
   int dirn2, dirn3, dirn4;

   /* help to catch bad accesses */
   stn2 = stn3 = stn4 = NULL;
   dirn2 = dirn3 = dirn4 = 0;

   out_current_action(msg(/*Calculating network*/130));

   while (reduction_stack != NULL) {
      /*  printf("replace_subnets() type %d\n", reduction_stack->type);*/

#if PRINT_NETBITS
      printf("replace_subnets\n");
      if (reduction_stack->type == TYPE_LOLLIPOP) printf("islollipop\n");
      if (reduction_stack->type == TYPE_PARALLEL) printf("isparallel\n");
      if (reduction_stack->type == TYPE_DELTASTAR) printf("isdelta*\n");
#endif

      if (reduction_stack->type != TYPE_DELTASTAR) {
	 linkfor *leg;
	 leg = reduction_stack->join[0]; leg = reverse_leg(leg);
	 stn3 = leg->l.to; dirn3 = reverse_leg_dirn(leg);
	 leg = reduction_stack->join[1]; leg = reverse_leg(leg);
	 stn4 = leg->l.to; dirn4 = reverse_leg_dirn(leg);

	 if (!fixed(stn3) || !fixed(stn4)) {
	     SVX_ASSERT(!fixed(stn3) && !fixed(stn4));
	     goto skip;
	 }
	 SVX_ASSERT(data_here(stn3->leg[dirn3]));
      }

      if (reduction_stack->type == TYPE_LOLLIPOP) {
	 node *stn;
	 delta e;
	 linkfor *leg;
	 int zero;

	 leg = stn3->leg[dirn3];
	 stn2 = reduction_stack->join[0]->l.to;
	 dirn2 = reverse_leg_dirn(reduction_stack->join[0]);

	 zero = fZeros(&leg->v);
	 if (!zero) {
	    delta tmp;
	    subdd(&e, &POSD(stn4), &POSD(stn3));
	    subdd(&tmp, &e, &leg->d);
	    divds(&e, &tmp, &leg->v);
	 }
	 if (data_here(reduction_stack->join[0])) {
	    adddd(&POSD(stn2), &POSD(stn3), &reduction_stack->join[0]->d);
	    if (!zero) {
	       delta tmp;
	       mulsd(&tmp, &reduction_stack->join[0]->v, &e);
	       adddd(&POSD(stn2), &POSD(stn2), &tmp);
	    }
	 } else {
	    subdd(&POSD(stn2), &POSD(stn3), &stn2->leg[dirn2]->d);
	    if (!zero) {
	       delta tmp;
	       mulsd(&tmp, &stn2->leg[dirn2]->v, &e);
	       adddd(&POSD(stn2), &POSD(stn2), &tmp);
	    }
	 }
	 dirn2 = (dirn2 + 2) % 3; /* point back at stn again */
	 stn = stn2->leg[dirn2]->l.to;
#if 0
	 printf("Replacing lollipop with stn...stn4 = \n");
	 print_prefix(stn->name); putnl();
	 print_prefix(stn2->name); putnl();
	 print_prefix(stn3->name); putnl();
	 print_prefix(stn4->name); putnl();
#endif
	 if (data_here(stn2->leg[dirn2]))
	    adddd(&POSD(stn), &POSD(stn2), &stn2->leg[dirn2]->d);
	 else
	    subdd(&POSD(stn), &POSD(stn2), &reverse_leg(stn2->leg[dirn2])->d);

	 /* The "stick" of the lollipop is a new articulation. */
	 stn2->leg[dirn2]->l.reverse |= FLAG_ARTICULATION;
	 reverse_leg(stn2->leg[dirn2])->l.reverse |= FLAG_ARTICULATION;

	 add_stn_to_list(&fixedlist, stn);
	 add_stn_to_list(&fixedlist, stn2);

	 osfree(stn3->leg[dirn3]);
	 stn3->leg[dirn3] = reduction_stack->join[0];
	 osfree(stn4->leg[dirn4]);
	 stn4->leg[dirn4] = reduction_stack->join[1];
      } else if (reduction_stack->type == TYPE_PARALLEL) {
	 /* parallel legs */
	 node *stn;
	 delta e, e2;
	 linkfor *leg;
	 int dirn;

	 stn = reduction_stack->join[0]->l.to;
	 stn2 = reduction_stack->join[1]->l.to;

	 dirn = reverse_leg_dirn(reduction_stack->join[0]);
	 dirn2 = reverse_leg_dirn(reduction_stack->join[1]);

	 leg = stn3->leg[dirn3];

	 if (leg->l.reverse & FLAG_ARTICULATION) {
	    reduction_stack->join[0]->l.reverse |= FLAG_ARTICULATION;
	    stn->leg[dirn]->l.reverse |= FLAG_ARTICULATION;
	    reduction_stack->join[1]->l.reverse |= FLAG_ARTICULATION;
	    stn2->leg[dirn2]->l.reverse |= FLAG_ARTICULATION;
	 }

	 if (fZeros(&leg->v))
	    e[0] = e[1] = e[2] = 0.0;
	 else {
	    delta tmp;
	    subdd(&e, &POSD(stn4), &POSD(stn3));
	    subdd(&tmp, &e, &leg->d);
	    divds(&e, &tmp, &leg->v);
	 }

	 if (data_here(reduction_stack->join[0])) {
	    leg = reduction_stack->join[0];
	    adddd(&POSD(stn), &POSD(stn3), &leg->d);
	 } else {
	    leg = stn->leg[dirn];
	    subdd(&POSD(stn), &POSD(stn3), &leg->d);
	 }
	 mulsd(&e2, &leg->v, &e);
	 adddd(&POSD(stn), &POSD(stn), &e2);

	 if (data_here(reduction_stack->join[1])) {
	    leg = reduction_stack->join[1];
	    adddd(&POSD(stn2), &POSD(stn4), &leg->d);
	 } else {
	    leg = stn2->leg[dirn2];
	    subdd(&POSD(stn2), &POSD(stn4), &leg->d);
	 }
	 mulsd(&e2, &leg->v, &e);
	 subdd(&POSD(stn2), &POSD(stn2), &e2);
#if 0
	 printf("Replacing parallel with stn...stn4 = \n");
	 print_prefix(stn->name); putnl();
	 print_prefix(stn2->name); putnl();
	 print_prefix(stn3->name); putnl();
	 print_prefix(stn4->name); putnl();
#endif

	 add_stn_to_list(&fixedlist, stn);
	 add_stn_to_list(&fixedlist, stn2);

	 osfree(stn3->leg[dirn3]);
	 stn3->leg[dirn3] = reduction_stack->join[0];
	 osfree(stn4->leg[dirn4]);
	 stn4->leg[dirn4] = reduction_stack->join[1];
      } else if (reduction_stack->type == TYPE_DELTASTAR) {
	 node *stnZ;
	 node *stn[3];
	 int dirn[3];
	 int i;
	 linkfor *leg;

	 /* work out ends as we don't bother stacking them */
	 leg = reverse_leg(reduction_stack->join[0]);
	 stn[0] = leg->l.to;
	 dirn[0] = reverse_leg_dirn(leg);
	 stnZ = stn[0]->leg[dirn[0]]->l.to;
	 SVX_ASSERT(fixed(stnZ));
	 if (!fixed(stnZ)) {
	    SVX_ASSERT(!fixed(stn[0]));
	    goto skip;
	 }
	 stn[1] = stnZ->leg[1]->l.to;
	 dirn[1] = reverse_leg_dirn(stnZ->leg[1]);
	 stn[2] = stnZ->leg[2]->l.to;
	 dirn[2] = reverse_leg_dirn(stnZ->leg[2]);
	 /*print_prefix(stnZ->name);printf(" %p\n",(void*)stnZ);*/

	 for (i = 0; i < 3; i++) {
	    SVX_ASSERT2(fixed(stn[i]), "stn not fixed for D*");

	    leg = stn[i]->leg[dirn[i]];

	    SVX_ASSERT2(data_here(leg), "data not on leg for D*");
	    SVX_ASSERT2(leg->l.to == stnZ, "bad sub-network for D*");

	    stn2 = reduction_stack->join[i]->l.to;

	    if (data_here(reduction_stack->join[i])) {
	       adddd(&POSD(stn2), &POSD(stn[i]), &reduction_stack->join[i]->d);
	    } else {
	       subdd(&POSD(stn2), &POSD(stn[i]), &reverse_leg(reduction_stack->join[i])->d);
	    }

	    if (!fZeros(&leg->v)) {
	       delta e, tmp;
	       subdd(&e, &POSD(stnZ), &POSD(stn[i]));
	       subdd(&e, &e, &leg->d);
	       divds(&tmp, &e, &leg->v);
	       if (data_here(reduction_stack->join[i])) {
		  mulsd(&e, &reduction_stack->join[i]->v, &tmp);
	       } else {
		  mulsd(&e, &reverse_leg(reduction_stack->join[i])->v, &tmp);
	       }
	       adddd(&POSD(stn2), &POSD(stn2), &e);
	    }
	    add_stn_to_list(&fixedlist, stn2);
	    osfree(leg);
	    stn[i]->leg[dirn[i]] = reduction_stack->join[i];
	    /* transfer the articulation status of the radial legs */
	    if (stnZ->leg[i]->l.reverse & FLAG_ARTICULATION) {
	       reduction_stack->join[i]->l.reverse |= FLAG_ARTICULATION;
	       reverse_leg(reduction_stack->join[i])->l.reverse |= FLAG_ARTICULATION;
	    }
	    osfree(stnZ->leg[i]);
	    stnZ->leg[i] = NULL;
	 }
/*printf("---%f %f %f\n",POS(stnZ, 0), POS(stnZ, 1), POS(stnZ, 2));*/
	 remove_stn_from_list(&fixedlist, stnZ);
	 osfree(stnZ->name);
	 osfree(stnZ);
      } else {
	 BUG("reduction_stack has unknown type");
      }

skip:;
      reduction *ptrOld = reduction_stack;
      reduction_stack = reduction_stack->next;
      osfree(ptrOld);
   }
}
