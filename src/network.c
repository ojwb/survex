/* network.c
 * Survex network reduction - find patterns and apply network reductions
 * Copyright (C) 1991-2002,2005 Olly Betts
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

/* type field isn't vital - join3 is unused except for deltastar, so
 * we can set its value to indicate which type this is:
 * join3 == NULL for noose, join3 == join1 for ||, otherwise D* */
#ifdef EXPLICIT_STACKRED_TYPE
#define SET_NOOSE(SR) (SR)->type = 1
#define IS_NOOSE(SR) ((SR)->type == 1)
#define SET_PARALLEL(SR) (SR)->type = 0
#define IS_PARALLEL(SR) ((SR)->type == 0)
#define SET_DELTASTAR(SR) (SR)->type = 2
#define IS_DELTASTAR(SR) ((SR)->type == 2)
#else
#define IS_NOOSE(SR) ((SR)->join3 == NULL)
#define SET_NOOSE(SR) (SR)->join3 = NULL
#define IS_PARALLEL(SR) ((SR)->join3 == (SR)->join1)
#define SET_PARALLEL(SR) (SR)->join3 = (SR)->join1
#define IS_DELTASTAR(SR) (!IS_NOOSE(SR) && !IS_PARALLEL(SR))
#define SET_DELTASTAR(SR) NOP
#endif

typedef struct StackRed {
   struct Link *join1, *join2, *join3;
#ifdef EXPLICIT_STACKRED_TYPE
   int type; /* 1 => noose, 0 => parallel legs, 2 => delta-star */
#endif
   struct StackRed *next;
} stackRed;

static stackRed *ptrRed; /* Ptr to TRaverse linked list for C*-*< , -*=*- */

/* can be altered by -z<letters> on command line */
unsigned long optimize = BITA('l') | BITA('p') | BITA('d');
/* Lollipops, Parallel legs, Iterate mx, Delta* */

extern void
remove_subnets(void)
{
   node *stn, *stn2, *stn3, *stn4;
   int dirn, dirn2, dirn3, dirn4;
   stackRed *trav;
   linkfor *newleg, *newleg2;
   bool fMore = true;

   ptrRed = NULL;

   out_current_action(msg(/*Simplifying network*/129));

   while (fMore) {
      fMore = false;
      if (optimize & BITA('l')) {
#if PRINT_NETBITS
	 printf("replacing lollipops\n");
#endif
	 /*        _
	  *       ( )
	  *        * stn
	  *        |
	  *        * stn2
	  *       / \
	  * stn4 *   * stn3  -->  stn4 *---* stn3
	  *      :   :                 :   :
	  */
	 /* NB can have non-fixed 0 nodes */
	 FOR_EACH_STN(stn, stnlist) {
	    if (!fixed(stn) && three_node(stn)) {
	       dirn = -1;
	       if (stn->leg[1]->l.to == stn) dirn++;
	       if (stn->leg[0]->l.to == stn) dirn += 2;
	       if (dirn < 0) continue;

	       stn2 = stn->leg[dirn]->l.to;
	       if (fixed(stn2)) continue;

	       SVX_ASSERT(three_node(stn2));

	       dirn2 = reverse_leg_dirn(stn->leg[dirn]);
	       dirn2 = (dirn2 + 1) % 3;
	       stn3 = stn2->leg[dirn2]->l.to;
	       if (stn2 == stn3) continue; /* dumb-bell - leave alone */

	       dirn3 = reverse_leg_dirn(stn2->leg[dirn2]);

	       trav = osnew(stackRed);
	       newleg2 = (linkfor*)osnew(linkrev);

	       newleg = copy_link(stn3->leg[dirn3]);

	       dirn2 = (dirn2 + 1) % 3;
	       stn4 = stn2->leg[dirn2]->l.to;
	       dirn4 = reverse_leg_dirn(stn2->leg[dirn2]);
#if 0
	       printf("Noose found with stn...stn4 = \n");
	       print_prefix(stn->name); putnl();
	       print_prefix(stn2->name); putnl();
	       print_prefix(stn3->name); putnl();
	       print_prefix(stn4->name); putnl();
#endif

	       addto_link(newleg, stn2->leg[dirn2]);

	       /* remove stn and stn2 */
	       remove_stn_from_list(&stnlist, stn);
	       remove_stn_from_list(&stnlist, stn2);

	       /* stack noose and replace with a leg between stn3 and stn4 */
	       trav->join1 = stn3->leg[dirn3];
	       newleg->l.to = stn4;
	       newleg->l.reverse = dirn4 | FLAG_DATAHERE | FLAG_REPLACEMENTLEG;

	       trav->join2 = stn4->leg[dirn4];
	       newleg2->l.to = stn3;
	       newleg2->l.reverse = dirn3 | FLAG_REPLACEMENTLEG;

	       stn3->leg[dirn3] = newleg;
	       stn4->leg[dirn4] = newleg2;

	       trav->next = ptrRed;
	       SET_NOOSE(trav);
#if PRINT_NETBITS
	       printf("remove noose\n");
#endif
	       ptrRed = trav;
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
	    if (!fixed(stn) && three_node(stn)) {
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

	       /* stn == stn2 => noose */
	       if (fixed(stn2) || stn == stn2) continue;

	       SVX_ASSERT(three_node(stn2));

	       stn3 = stn->leg[dirn]->l.to;
	       /* 3 parallel legs (=> nothing else) so leave */
	       if (stn3 == stn2) continue;

	       dirn3 = reverse_leg_dirn(stn->leg[dirn]);
	       dirn2 = (0 + 1 + 2 - reverse_leg_dirn(stn->leg[(dirn + 1) % 3])
			- reverse_leg_dirn(stn->leg[(dirn + 2) % 3]));

	       stn4 = stn2->leg[dirn2]->l.to;
	       dirn4 = reverse_leg_dirn(stn2->leg[dirn2]);

	       trav = osnew(stackRed);

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
	       newleg2 = (linkfor*)osnew(linkrev);

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
	       trav->join1 = stn3->leg[dirn3];
	       newleg->l.to = stn4;
	       newleg->l.reverse = dirn4 | FLAG_DATAHERE | FLAG_REPLACEMENTLEG;

	       trav->join2 = stn4->leg[dirn4];
	       newleg2->l.to = stn3;
	       newleg2->l.reverse = dirn3 | FLAG_REPLACEMENTLEG;

	       stn3->leg[dirn3] = newleg;
	       stn4->leg[dirn4] = newleg2;

	       trav->next = ptrRed;
	       SET_PARALLEL(trav);
#if PRINT_NETBITS
	       printf("remove parallel\n");
#endif
	       ptrRed = trav;
	       fMore = true;
	    }
	 }
      }

      if (optimize & BITA('d')) {
	 node *stn5, *stn6;
	 int dirn5, dirn6, dirn0;
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
	    if (!fixed(stn) && three_node(stn)) {
	       for (dirn0 = 0; ; dirn0++) {
		  if (dirn0 >= 3) goto nodeltastar; /* continue outer loop */
		  dirn = dirn0;
		  stn2 = stn->leg[dirn]->l.to;
		  if (fixed(stn2) || stn2 == stn) continue;
		  dirn2 = reverse_leg_dirn(stn->leg[dirn]);
		  dirn2 = (dirn2 + 1) % 3;
		  stn3 = stn2->leg[dirn2]->l.to;
		  if (fixed(stn3) || stn3 == stn || stn3 == stn2)
		     goto nextdirn2;
		  dirn3 = reverse_leg_dirn(stn2->leg[dirn2]);
		  dirn3 = (dirn3 + 1) % 3;
		  if (stn3->leg[dirn3]->l.to == stn) {
		     legAB = copy_link(stn->leg[dirn]);
		     legBC = copy_link(stn2->leg[dirn2]);
		     legCA = copy_link(stn3->leg[dirn3]);
		     dirn = 0 + 1 + 2 - dirn - reverse_leg_dirn(stn3->leg[dirn3]);
		     dirn2 = (dirn2 + 1) % 3;
		     dirn3 = (dirn3 + 1) % 3;
		  } else if (stn3->leg[(dirn3 + 1) % 3]->l.to == stn) {
		     legAB = copy_link(stn->leg[dirn]);
		     legBC = copy_link(stn2->leg[dirn2]);
		     legCA = copy_link(stn3->leg[(dirn3 + 1) % 3]);
		     dirn = (0 + 1 + 2 - dirn
			     - reverse_leg_dirn(stn3->leg[(dirn3 + 1) % 3]));
		     dirn2 = (dirn2 + 1) % 3;
		     break;
		  } else {
		     nextdirn2:;
		     dirn2 = (dirn2 + 1) % 3;
		     stn3 = stn2->leg[dirn2]->l.to;
		     if (fixed(stn3) || stn3 == stn || stn3 == stn2) continue;
		     dirn3 = reverse_leg_dirn(stn2->leg[dirn2]);
		     dirn3 = (dirn3 + 1) % 3;
		     if (stn3->leg[dirn3]->l.to == stn) {
			legAB = copy_link(stn->leg[dirn]);
			legBC = copy_link(stn2->leg[dirn2]);
			legCA = copy_link(stn3->leg[dirn3]);
			dirn = (0 + 1 + 2 - dirn
				- reverse_leg_dirn(stn3->leg[dirn3]));
			dirn2 = (dirn2 + 2) % 3;
			dirn3 = (dirn3 + 1) % 3;
			break;
		     } else if (stn3->leg[(dirn3 + 1) % 3]->l.to == stn) {
			legAB = copy_link(stn->leg[dirn]);
			legBC = copy_link(stn2->leg[dirn2]);
			legCA = copy_link(stn3->leg[(dirn3 + 1) % 3]);
			dirn = (0 + 1 + 2 - dirn
				- reverse_leg_dirn(stn3->leg[(dirn3 + 1) % 3]));
			dirn2 = (dirn2 + 2) % 3;
			break;
		     }
		  }
	       }

	       SVX_ASSERT(three_node(stn2));
	       SVX_ASSERT(three_node(stn3));

	       stn4 = stn->leg[dirn]->l.to;
	       stn5 = stn2->leg[dirn2]->l.to;
	       stn6 = stn3->leg[dirn3]->l.to;

	       if (stn4 == stn2 || stn4 == stn3 || stn5 == stn3) break;

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

	       trav = osnew(stackRed);
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
		    stnZ->leg[0] = (linkfor*)osnew(linkrev);
		    stnZ->leg[1] = (linkfor*)osnew(linkrev);
		    stnZ->leg[2] = (linkfor*)osnew(linkrev);
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
		    trav->join1 = stn4->leg[dirn4];
		    trav->join2 = stn5->leg[dirn5];
		    trav->join3 = stn6->leg[dirn6];
		    trav->next = ptrRed;
		    SET_DELTASTAR(trav);
#if PRINT_NETBITS
		    printf("remove delta*\n");
#endif
		    ptrRed = trav;
		    fMore = true;

		    remove_stn_from_list(&stnlist, stn);
		    remove_stn_from_list(&stnlist, stn2);
		    remove_stn_from_list(&stnlist, stn3);
		    stn4->leg[dirn4] = legAZ;
		    stn5->leg[dirn5] = legBZ;
		    stn6->leg[dirn6] = legCZ;
		 }

	    }
	    nodeltastar:;
	 }
      }

   }
}

extern void
replace_subnets(void)
{
   stackRed *ptrOld;
   node *stn2, *stn3, *stn4;
   int dirn2, dirn3, dirn4;

   /* help to catch bad accesses */
   stn2 = stn3 = stn4 = NULL;
   dirn2 = dirn3 = dirn4 = 0;

   out_current_action(msg(/*Calculating network*/130));

   while (ptrRed != NULL) {
      /*  printf("replace_subnets() type %d\n", ptrRed->type);*/

#if PRINT_NETBITS
      printf("replace_subnets\n");
      if (IS_NOOSE(ptrRed)) printf("isnoose\n");
      if (IS_PARALLEL(ptrRed)) printf("isparallel\n");
      if (IS_DELTASTAR(ptrRed)) printf("isdelta*\n");
#endif

      if (!IS_DELTASTAR(ptrRed)) {
	 linkfor *leg;
	 leg = ptrRed->join1; leg = reverse_leg(leg);
	 stn3 = leg->l.to; dirn3 = reverse_leg_dirn(leg);
	 leg = ptrRed->join2; leg = reverse_leg(leg);
	 stn4 = leg->l.to; dirn4 = reverse_leg_dirn(leg);

	 if (!fixed(stn3) || !fixed(stn4)) {
	     SVX_ASSERT(!fixed(stn3) && !fixed(stn4));
	     goto skip;
	 }
	 SVX_ASSERT(data_here(stn3->leg[dirn3]));
      }

      if (IS_NOOSE(ptrRed)) {
	 /* noose (hanging-loop) */
	 node *stn;
	 delta e;
	 linkfor *leg;
	 int zero;

	 leg = stn3->leg[dirn3];
	 stn2 = ptrRed->join1->l.to;
	 dirn2 = reverse_leg_dirn(ptrRed->join1);

	 zero = fZeros(&leg->v);
	 if (!zero) {
	    delta tmp;
	    subdd(&e, &POSD(stn4), &POSD(stn3));
	    subdd(&tmp, &e, &leg->d);
	    divds(&e, &tmp, &leg->v);
	 }
	 if (data_here(ptrRed->join1)) {
	    adddd(&POSD(stn2), &POSD(stn3), &ptrRed->join1->d);
	    if (!zero) {
	       delta tmp;
	       mulsd(&tmp, &ptrRed->join1->v, &e);
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
	 printf("Replacing noose with stn...stn4 = \n");
	 print_prefix(stn->name); putnl();
	 print_prefix(stn2->name); putnl();
	 print_prefix(stn3->name); putnl();
	 print_prefix(stn4->name); putnl();
#endif
	 if (data_here(stn2->leg[dirn2]))
	    adddd(&POSD(stn), &POSD(stn2), &stn2->leg[dirn2]->d);
	 else
	    subdd(&POSD(stn), &POSD(stn2), &reverse_leg(stn2->leg[dirn2])->d);

	 /* the "rope" of the noose is a new articulation */
	 stn2->leg[dirn2]->l.reverse |= FLAG_ARTICULATION;
	 reverse_leg(stn2->leg[dirn2])->l.reverse |= FLAG_ARTICULATION;

	 add_stn_to_list(&stnlist, stn);
	 add_stn_to_list(&stnlist, stn2);

	 osfree(stn3->leg[dirn3]);
	 stn3->leg[dirn3] = ptrRed->join1;
	 osfree(stn4->leg[dirn4]);
	 stn4->leg[dirn4] = ptrRed->join2;
      } else if (IS_PARALLEL(ptrRed)) {
	 /* parallel legs */
	 node *stn;
	 delta e, e2;
	 linkfor *leg;
	 int dirn;

	 stn = ptrRed->join1->l.to;
	 stn2 = ptrRed->join2->l.to;

	 dirn = reverse_leg_dirn(ptrRed->join1);
	 dirn2 = reverse_leg_dirn(ptrRed->join2);

	 leg = stn3->leg[dirn3];

	 if (leg->l.reverse & FLAG_ARTICULATION) {
	    ptrRed->join1->l.reverse |= FLAG_ARTICULATION;
	    stn->leg[dirn]->l.reverse |= FLAG_ARTICULATION;
	    ptrRed->join2->l.reverse |= FLAG_ARTICULATION;
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

	 if (data_here(ptrRed->join1)) {
	    leg = ptrRed->join1;
	    adddd(&POSD(stn), &POSD(stn3), &leg->d);
	 } else {
	    leg = stn->leg[dirn];
	    subdd(&POSD(stn), &POSD(stn3), &leg->d);
	 }
	 mulsd(&e2, &leg->v, &e);
	 adddd(&POSD(stn), &POSD(stn), &e2);

	 if (data_here(ptrRed->join2)) {
	    leg = ptrRed->join2;
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

	 add_stn_to_list(&stnlist, stn);
	 add_stn_to_list(&stnlist, stn2);

	 osfree(stn3->leg[dirn3]);
	 stn3->leg[dirn3] = ptrRed->join1;
	 osfree(stn4->leg[dirn4]);
	 stn4->leg[dirn4] = ptrRed->join2;
      } else if (IS_DELTASTAR(ptrRed)) {
	 node *stnZ;
	 node *stn[3];
	 int dirn[3];
	 linkfor *legs[3];
	 int i;
	 linkfor *leg;

	 legs[0] = ptrRed->join1;
	 legs[1] = ptrRed->join2;
	 legs[2] = ptrRed->join3;

	 /* work out ends as we don't bother stacking them */
	 leg = reverse_leg(legs[0]);
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

	    stn2 = legs[i]->l.to;

	    if (data_here(legs[i])) {
	       adddd(&POSD(stn2), &POSD(stn[i]), &legs[i]->d);
	    } else {
	       subdd(&POSD(stn2), &POSD(stn[i]), &reverse_leg(legs[i])->d);
	    }

	    if (!fZeros(&leg->v)) {
	       delta e, tmp;
	       subdd(&e, &POSD(stnZ), &POSD(stn[i]));
	       subdd(&e, &e, &leg->d);
	       divds(&tmp, &e, &leg->v);
	       if (data_here(legs[i])) {
		  mulsd(&e, &legs[i]->v, &tmp);
	       } else {
		  mulsd(&e, &reverse_leg(legs[i])->v, &tmp);
	       }
	       adddd(&POSD(stn2), &POSD(stn2), &e);
	    }
	    add_stn_to_list(&stnlist, stn2);
	    osfree(leg);
	    stn[i]->leg[dirn[i]] = legs[i];
	    /* transfer the articulation status of the radial legs */
	    if (stnZ->leg[i]->l.reverse & FLAG_ARTICULATION) {
	       legs[i]->l.reverse |= FLAG_ARTICULATION;
	       reverse_leg(legs[i])->l.reverse |= FLAG_ARTICULATION;
	    }
	    osfree(stnZ->leg[i]);
	    stnZ->leg[i] = NULL;
	 }
/*printf("---%f %f %f\n",POS(stnZ, 0), POS(stnZ, 1), POS(stnZ, 2));*/
	 remove_stn_from_list(&stnlist, stnZ);
	 osfree(stnZ->name);
	 osfree(stnZ);
      } else {
	 BUG("ptrRed has unknown type");
      }

skip:
      ptrOld = ptrRed;
      ptrRed = ptrRed->next;
      osfree(ptrOld);
   }
}
