/* > network.c
 * Survex network reduction - find patterns and apply network reductions
 * Copyright (C) 1991-2000 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if 0
#define DEBUG_INVALID 1
#define VALIDATE 1
#define DUMP_NETWORK 1
#endif

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stddef.h>

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
   bool fMore = fTrue;

   ptrRed = NULL;
   
   out_current_action(msg(/*Simplifying network*/129));

   while (fMore) {
      fMore = fFalse;
      if (optimize & BITA('l')) {
         /*      _
	  *     ( )
	  *      * stn
	  *      |
	  *      * stn2
	  * stn /|
	  *  4 * * stn3  -->  stn4 *-* stn3
	  *    : :                 : :
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

	       ASSERT(three_node(stn2));

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
	       fMore = fTrue;
	    }
	 }
      }

      if (optimize & BITA('p')) {
#if PRINT_NETBITS
	 printf("replacing parallel legs\n");
#endif
	 FOR_EACH_STN(stn, stnlist) {
	    /*
	     *  :
	     *  * stn3
	     *  |            :
	     *  * stn        * stn3
	     * ( )      ->   |
	     *  * stn2       * stn4
	     *  |            :
	     *  * stn4
	     *  :
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

	       ASSERT(three_node(stn2));

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
		    var sum, prod;
		    d temp, temp2;
		    addvv(&sum, &newleg->v, &newleg2->v);
		    ASSERT2(!fZero(&sum), "loop of zero variance found");
		    mulvv(&prod, &newleg->v, &newleg2->v);
		    mulvd(&temp, &newleg2->v, &newleg->d);
		    mulvd(&temp2, &newleg->v, &newleg2->d);
		    adddd(&temp, &temp, &temp2);
		    divdv(&newleg->d, &temp, &sum);
		    divvv(&newleg->v, &prod, &sum);
#else
		    var inv1, inv2, sum;
		    d temp, temp2;
		    /* if leg one is an equate, we can just ignore leg two
		     * whatever it is */
		    if (invert_var(&inv1, &newleg->v)) {
		       if (invert_var(&inv2, &newleg2->v)) {
			  addvv(&sum, &inv1, &inv2);
			  if (!invert_var(&newleg->v, &sum)) {
			     ASSERT2(0, "matrix singular in parallel legs replacement");
			  }
			  
			  mulvd(&temp, &inv1, &newleg->d);
			  mulvd(&temp2, &inv2, &newleg2->d);
			  adddd(&temp, &temp, &temp2);
			  mulvd(&newleg->d, &newleg->v, &temp);
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
	       ASSERT2(stn3->leg[dirn3]->l.to == stn, "stn3 end of || doesn't recip");
	       ASSERT2(stn4->leg[dirn4]->l.to == stn2, "stn4 end of || doesn't recip");
	       ASSERT2(stn->leg[(dirn+1)%3]->l.to == stn2 && stn->leg[(dirn + 2) % 3]->l.to == stn2, "|| legs aren't");

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
	       fMore = fTrue;
	    }
	 }
      }

      if (optimize & BITA('d')) {
	 node *stn5, *stn6;
	 int dirn5, dirn6, dirn0;
	 linkfor *legAB, *legBC, *legCA;
	 linkfor *legAZ, *legBZ, *legCZ;
	 /*printf("replacing delta with star\n");*/
	 FOR_EACH_STN(stn, stnlist) {
	    /*    printf("*");*/
	    /*
	     *          :
	     *          * stn5            :
	     *          |                 * stn5
	     *          * stn2            |
	     *         / \        ->      O stnZ
	     *    stn *---* stn3         / \
             *       /     \       stn4 *   * stn6
	     * stn4 *       * stn6      :   :
	     *      :       :
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
		  
	       ASSERT(three_node(stn2));
	       ASSERT(three_node(stn3));

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
	       ASSERT(stn4->leg[dirn4]->l.to == stn);
	       ASSERT(stn5->leg[dirn5]->l.to == stn2);
	       ASSERT(stn6->leg[dirn6]->l.to == stn3);

	       trav = osnew(stackRed);
	       legAZ = osnew(linkfor);
	       legBZ = osnew(linkfor);
	       legCZ = osnew(linkfor);

		 {
		    node *stnZ;
		    prefix *nameZ;
		    var invAB, invBC, invCA, tmp, sum, inv;
		    var sumAZBZ, sumBZCZ, sumCZAZ;
		    d temp, temp2;
		    
		    /* FIXME: ought to handle cases when some legs are
		     * equates, but handle as a special case maybe? */
		    if (!invert_var(&invAB, &legAB->v)) break; /* FIXME */
		    if (!invert_var(&invBC, &legBC->v)) break; /* FIXME */
		    if (!invert_var(&invCA, &legCA->v)) break; /* FIXME */

		    addvv(&sum, &legBC->v, &legCA->v);
		    addvv(&tmp, &sum, &legAB->v);
		    if (!invert_var(&inv, &tmp)) break; /* FIXME: impossible - loop of zero variance */
		    
		    /* AZBZ */
		    /* done above: addvv(&sum, &legBC->v, &legCA->v); */
		    mulvv(&tmp, &sum, &inv);
		    mulvv(&sumAZBZ, &tmp, &legAB->v);

		    adddd(&temp, &legBC->d, &legCA->d);
		    divdv(&temp2, &temp, &sum);
		    mulvd(&temp, &invAB, &legAB->d);
		    subdd(&temp, &temp2, &temp);
		    mulvd(&legBZ->d, &sumAZBZ, &temp);

		    /* leg vectors after transform are determined up to
		     * a constant addition, so arbitrarily fix AZ = 0 */
		    legAZ->d[2] = legAZ->d[1] = legAZ->d[0] = 0;

		    /* BZCZ */
		    addvv(&sum, &legCA->v, &legAB->v);
		    mulvv(&tmp, &sum, &inv);
		    mulvv(&sumBZCZ, &tmp, &legBC->v);

		    /* CZAZ */
		    addvv(&sum, &legAB->v, &legBC->v);
		    mulvv(&tmp, &sum, &inv);
		    mulvv(&sumCZAZ, &tmp, &legCA->v);

		    adddd(&temp, &legAB->d, &legBC->d);
		    divdv(&temp2, &temp, &sum);
		    mulvd(&temp, &invCA, &legCA->d);
		    /* NB: swapped arguments to negate answer for legCZ->d */
		    subdd(&temp, &temp, &temp2);
		    mulvd(&legCZ->d, &sumCZAZ, &temp);

		    /* Now add two, subtract third, and scale by 0.5 */
		    addvv(&sum, &sumAZBZ, &sumCZAZ);
		    subvv(&sum, &sum, &sumBZCZ);
		    mulvc(&legAZ->v, &sum, 0.5);

		    addvv(&sum, &sumBZCZ, &sumAZBZ);
		    subvv(&sum, &sum, &sumCZAZ);
		    mulvc(&legBZ->v, &sum, 0.5);

		    addvv(&sum, &sumCZAZ, &sumBZCZ);
		    subvv(&sum, &sum, &sumAZBZ);
		    mulvc(&legCZ->v, &sum, 0.5);

		    nameZ = osnew(prefix);
		    nameZ->ident = ""; /* root has ident[0] == "\" */
		    stnZ = osnew(node);
		    stnZ->name = nameZ;
		    nameZ->pos = osnew(pos);
		    nameZ->pos->shape = 3;
		    nameZ->stn = stnZ;
		    nameZ->up = NULL;
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
		    fMore = fTrue;
		    
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
   node *stn, *stn2, *stn3, *stn4;
   int dirn, dirn2, dirn3, dirn4;
   linkfor *leg;

   /* help to catch bad accesses */
   stn = stn2 = stn3 = stn4 = NULL;
   dirn = dirn2 = dirn3 = dirn4 = 0;

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
	 
         leg = ptrRed->join1; leg = reverse_leg(leg);
         stn3 = leg->l.to; dirn3 = reverse_leg_dirn(leg);
         leg = ptrRed->join2; leg = reverse_leg(leg);
         stn4 = leg->l.to; dirn4 = reverse_leg_dirn(leg);
	 
         ASSERT(!(fixed(stn3) && !fixed(stn4)));
         ASSERT(!(!fixed(stn3) && fixed(stn4)));
         ASSERT(data_here(stn3->leg[dirn3]));
      }

      if (IS_NOOSE(ptrRed)) {
         /* noose (hanging-loop) */
         d e;
         linkfor *leg;
         if (fixed(stn3)) {
            /* NB either both or neither fixed */
	    int zero;

            leg = stn3->leg[dirn3];
            stn2 = ptrRed->join1->l.to;
            dirn2 = reverse_leg_dirn(ptrRed->join1);
	    
	    zero = fZero(&leg->v);
            if (!zero) {
	       d tmp;
               subdd(&e, &POSD(stn4), &POSD(stn3));
               subdd(&tmp, &e, &leg->d);
               divdv(&e, &tmp, &leg->v);
            }
            if (data_here(ptrRed->join1)) {
               adddd(&POSD(stn2), &POSD(stn3), &ptrRed->join1->d);
	       if (!zero) {
		  d tmp;
		  mulvd(&tmp, &ptrRed->join1->v, &e);
		  adddd(&POSD(stn2), &POSD(stn2), &tmp);
	       }
            } else {
               subdd(&POSD(stn2), &POSD(stn3), &stn2->leg[dirn2]->d);
               if (!zero) {
		  d tmp;
		  mulvd(&tmp, &stn2->leg[dirn2]->v, &e);
		  adddd(&POSD(stn2), &POSD(stn2), &tmp);
	       }
            }
            fix(stn2);
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
               subdd(&POSD(stn), &POSD(stn2),
		     &reverse_leg(stn2->leg[dirn2])->d);
            fix(stn);
         } else {
            stn2 = ptrRed->join1->l.to;
            dirn2 = reverse_leg_dirn(ptrRed->join1);
            add_stn_to_list(&stnlist, stn2);
            dirn2 = (dirn2 + 2) % 3; /* point back at stn again */
            stn = stn2->leg[dirn2]->l.to;
            add_stn_to_list(&stnlist, stn);
         }

         osfree(stn3->leg[dirn3]);
         stn3->leg[dirn3] = ptrRed->join1;
         osfree(stn4->leg[dirn4]);
         stn4->leg[dirn4] = ptrRed->join2;
      } else if (IS_PARALLEL(ptrRed)) {
         /* parallel legs */
         d e, e2;
         linkfor *leg;
         if (fixed(stn3)) {
            /* NB either both or neither fixed */
            stn = ptrRed->join1->l.to;
            dirn = reverse_leg_dirn(ptrRed->join1);

            stn2 = ptrRed->join2->l.to;
            dirn2 = reverse_leg_dirn(ptrRed->join2);

            leg = stn3->leg[dirn3];

            if (fZero(&leg->v))
               e[0] = e[1] = e[2] = 0.0;
            else {
	       d tmp;
               subdd(&e, &POSD(stn4), &POSD(stn3));
               subdd(&tmp, &e, &leg->d);
               divdv(&e, &tmp, &leg->v);
            }

            if (data_here(ptrRed->join1)) {
               leg = ptrRed->join1;
               adddd(&POSD(stn), &POSD(stn3), &leg->d);
            } else {
               leg = stn->leg[dirn];
               subdd(&POSD(stn), &POSD(stn3), &leg->d);
            }
            mulvd(&e2, &leg->v, &e);
            adddd(&POSD(stn), &POSD(stn), &e2);

            if (data_here(ptrRed->join2)) {
               leg = ptrRed->join2;
               adddd(&POSD(stn2), &POSD(stn4), &leg->d);
            } else {
               leg = stn2->leg[dirn2];
               subdd(&POSD(stn2), &POSD(stn4), &leg->d);
            }
            mulvd(&e2, &leg->v, &e);
            subdd(&POSD(stn2), &POSD(stn2), &e2);
            fix(stn);
            fix(stn2);
#if 0
            printf("Replacing parallel with stn...stn4 = \n");
            print_prefix(stn->name); putnl();
            print_prefix(stn2->name); putnl();
            print_prefix(stn3->name); putnl();
            print_prefix(stn4->name); putnl();
#endif
         } else {
            stn = ptrRed->join1->l.to;
            add_stn_to_list(&stnlist, stn);

            stn2 = ptrRed->join2->l.to;
            add_stn_to_list(&stnlist, stn2);
         }
         osfree(stn3->leg[dirn3]);
	 stn3->leg[dirn3] = ptrRed->join1;
         osfree(stn4->leg[dirn4]);
	 stn4->leg[dirn4] = ptrRed->join2;
      } else if (IS_DELTASTAR(ptrRed)) {
         linkfor *leg2;
         node *stnZ;
         node *stn[3];
         int dirn[3];
         linkfor *leg[3];
         int i;
         linkfor *legX;

         leg[0] = ptrRed->join1;
         leg[1] = ptrRed->join2;
         leg[2] = ptrRed->join3;

         /* work out ends as we don't bother stacking them */
         legX = reverse_leg(leg[0]);
         stn[0] = legX->l.to;
         dirn[0] = reverse_leg_dirn(legX);
         stnZ = stn[0]->leg[dirn[0]]->l.to;
         stn[1] = stnZ->leg[1]->l.to;
	 dirn[1] = reverse_leg_dirn(stnZ->leg[1]);
         stn[2] = stnZ->leg[2]->l.to;
	 dirn[2] = reverse_leg_dirn(stnZ->leg[2]);
	 /*print_prefix(stnZ->name);printf(" %p\n",(void*)stnZ);*/

         if (fixed(stnZ)) {
            for (i = 0; i < 3; i++) {
               ASSERT2(fixed(stn[i]), "stn not fixed for D*");
               ASSERT2(data_here(stn[i]->leg[dirn[i]]),
	               "data not on leg for D*");
               ASSERT2(stn[i]->leg[dirn[i]]->l.to == stnZ,
                       "bad sub-network for D*");
            }
            for (i = 0; i < 3; i++) {
               linkfor *leg1 = copy_link(leg[i]);
               leg2 = stn[i]->leg[dirn[i]];
               stn2 = leg[i]->l.to;
               adddd(&POSD(stn2), &POSD(stn[i]), &leg1->d);
               if (!fZero(&leg2->v)) {
		  d e, tmp;
                  subdd(&e, &POSD(stnZ), &POSD(stn[i]));
                  subdd(&e, &e, &leg2->d);
                  divdv(&tmp, &e, &leg2->v);
                  mulvd(&e, &leg1->v, &tmp);
		  adddd(&POSD(stn2), &POSD(stn2), &e);
               }
               fix(stn2);
	       add_stn_to_list(&stnlist, stn2);
               osfree(leg1);
               osfree(leg2);
               stn[i]->leg[dirn[i]] = leg[i];
               osfree(stnZ->leg[i]);
               stnZ->leg[i] = NULL;
            }
/*printf("---%f %f %f\n",POS(stnZ, 0), POS(stnZ, 1), POS(stnZ, 2));*/
         } else { /* not fixed case */
            for (i = 0; i < 3; i++) {
               ASSERT2(!fixed(stn[i]), "stn fixed for D*");
               ASSERT2(data_here(stn[i]->leg[dirn[i]]),
                       "data not on leg for D*");
               ASSERT2(stn[i]->leg[dirn[i]]->l.to == stnZ,
                       "bad sub-network for D*");
            }
            for (i = 0; i < 3; i++) {
/*print_prefix(stn[i]->name);printf("\n");*/
               leg2 = stn[i]->leg[dirn[i]];
               stn2 = leg[i]->l.to;
	       add_stn_to_list(&stnlist, stn2);
               osfree(leg2);
               stn[i]->leg[dirn[i]] = leg[i];
               osfree(stnZ->leg[i]);
               /* stnZ->leg[i] = NULL; */
            }
	 }
	 remove_stn_from_list(&stnlist, stnZ);
	 osfree(stnZ->name);
	 osfree(stnZ);
      } else {
         BUG("ptrRed has unknown type");
      }

      ptrOld = ptrRed;
      ptrRed = ptrRed->next;
      osfree(ptrOld);
   }
}
