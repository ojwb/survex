/* > network.c
 * SURVEX Network reduction routines
 * Copyright (C) 1991-1995,1997 Olly Betts
 */

/*#define BLUNDER_DETECTION 1*/

/* This source file is in pretty bad need of tidying up, and probably needs
 * splitting up - it's the largest by quite a margin.  Or maybe the problem
 * is too much code reuse by cut&paste - there's a lot of code which looks
 * similar in here (at first glance anyway)
 */

#if 0
#define DEBUG_INVALID 1
#define VALIDATE 1
# define DUMP_NETWORK 1
#endif

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stddef.h>
#include "validate.h"
#include "debug.h"
#include "cavern.h"
#include "filename.h"
#include "message.h"
#include "filelist.h"
#include "netbits.h"
#include "matrix.h"
#include "network.h"
#include "readval.h"
#include "out.h"

#if PRINT_NETBITS
static void
print_var(const var v)
{
#ifdef NO_COVARIANCES
   printf("(%f,%f,%f)", v[0], v[1], v[2]);
#else
   int i;
   for (i = 0; i < 3; i++) printf("(%.10f,%.10f,%.10f)\n", v[i][0], v[i][1], v[i][2]);
#endif
}
#endif

#define sqrdd(X) (sqrd((X)[0]) + sqrd((X)[1]) + sqrd((X)[2]))

typedef struct Stack {
   struct Link *join1, *join2;
   struct Stack *next;
} stack;

/* type field isn't vital - join3 is unused except for deltastar, so
 * we can set its value to indicate which type this is:
 * join3 == NULL for noose, join3 == join1 for ||, otherwise D* */
#ifdef EXPLICIT_STACKRED_TYPE
#define IS_NOOSE(SR) ((SR)->type == 1)
#define IS_PARALLEL(SR) ((SR)->type == 0)
#define IS_DELTASTAR(SR) ((SR)->type == 2)
#else
#define IS_NOOSE(SR) ((SR)->join3 == NULL)
#define IS_PARALLEL(SR) ((SR)->join3 == (SR)->join1)
#define IS_DELTASTAR(SR) (!IS_NOOSE(SR) && (SR)->join3 != (SR)->join1)
#endif

typedef struct StackRed {
   struct Link *join1, *join2, *join3;
#ifdef EXPLICIT_STACKRED_TYPE
   int type; /* 1 => noose, 0 => parallel legs, 2 => delta-star */
#endif
   struct StackRed *next;
} stackRed;

typedef struct StackTr {
   struct Link *join1;
   struct StackTr *next;
} stackTrail;

/* string used between things in traverse printouts eg \1 - \2 - \3 -...*/
static const char *szLink = " - ";
static const char *szLinkEq = " = "; /* use this one for equates */

/* can be altered by -o<letters> on command line */
unsigned long optimize = BITA('l') | BITA('p') | BITA('d') | BITA('s');
/* Lollipops, Parallel legs, Iterate mx, Delta*, Split stnlist */

#if 0
#define fprint_prefix(FH, NAME) BLK((fprint_prefix)((FH), (NAME));\
                                    fprintf((FH), " [%p]", (void*)(NAME)); )
#endif

static stackRed *ptrRed; /* Ptr to TRaverse linked list for C*-*< , -*=*- */
static stack *ptr; /* Ptr to TRaverse linked list for in-between travs */
static stackTrail *ptrTrail; /* Ptr to TRaverse linked list for trail travs*/

static void remove_trailing_travs(void);
static void remove_travs(void);
static void replace_travs(void);
static void replace_trailing_travs(void);
static void remove_subnets(void);
static void replace_subnets(void);

static void concatenate_trav(node *stn, int i);

static void err_stat(int cLegsTrav, double lenTrav,
		     double eTot, double eTotTheo,
		     double hTot, double hTotTheo,
		     double vTot, double vTotTheo);

extern void
solve_network(void /*node *stnlist*/)
{
   static int first_solve = 1;
   node *stn;

   if (stnlist == NULL) fatalerror(/*No survey data*/43);
   ptr = NULL;
   ptrTrail = NULL;
   ptrRed = NULL;
   dump_network();

   if (first_solve) {
      /* only do this stuff on a first solve (if *solve is used we are
       * called several times).  Otherwise unattached data after a solve
       * will cause a fixed point to be invented which is bogus.
       */
      first_solve = 0;

      /* If there are no fixed points, invent one.  Do this first to
       * avoid problems with sub-nodes of the invented fix which have
       * been removed.  It also means we can fix the "first" station,
       * which makes more sense to the user. */
      FOR_EACH_STN(stn, stnlist)
	 if (fixed(stn)) break;

      if (!stn) {
	 node *stnFirst = NULL;
	 /* new stations are pushed onto the head of the list, so the
	  * first station added is the last in the list */
	 FOR_EACH_STN(stn, stnlist) stnFirst = stn;
	 
	 ASSERT2(stnFirst, "no stations left in net!");
	 stn = stnFirst;
	 out_printf((msg(/*Survey has no fixed points. Therefore I've fixed %s at (0,0,0)*/72), sprint_prefix(stn->name)));
	 POS(stn,0) = (real)0.0;
	 POS(stn,1) = (real)0.0;
	 POS(stn,2) = (real)0.0;
	 fix(stn);
      }
   }

   remove_trailing_travs();
   validate(); dump_network();
   remove_travs();
   validate(); dump_network();
   remove_subnets();
   validate(); dump_network();
   solve_matrix();
   validate(); dump_network();
   replace_subnets();
   validate(); dump_network();
   replace_travs();
   validate(); dump_network();
   replace_trailing_travs();
   validate(); dump_network();
}

static void
remove_trailing_travs(void)
{
   node *stn;
   out_current_action(msg(/*Removing trailing traverses*/125));
   FOR_EACH_STN(stn, stnlist) {
      if (!fixed(stn) && one_node(stn)) {
	 int i = 0;
	 int j;
	 node *stn2 = stn;
	 stackTrail *trav;

#if PRINT_NETBITS
	 printf("Removed trailing trav ");
#endif
	 do {
	    struct Link *leg;
#if PRINT_NETBITS
	    print_prefix(stn2->name); printf("<%p>",stn2); printf(szLink);
#endif
	    remove_stn_from_list(&stnlist, stn2);
	    leg = stn2->leg[i];
	    j = reverse_leg_dirn(leg);
	    stn2 = leg->l.to;
	    i = j ^ 1; /* flip direction for other leg of 2 node */
	    /* stop if fixed or 3 or 1 node */
	 } while (two_node(stn2) && !fixed(stn2));
	    
	 /* put traverse on stack */
	 trav = osnew(stackTrail);
	 trav->join1 = stn2->leg[j];
	 trav->next = ptrTrail;
	 ptrTrail = trav;

	 /* We want to keep all 2-nodes using legs 0 and 1 and all one nodes
	  * using leg 0 so we may need to swap leg j with leg 2 (for a 3 node)
	  * or leg 1 (for a fixed 2 node) */
         if ((j == 0 && !one_node(stn2)) || (j == 1 && three_node(stn2))) {
	    /* i is the direction to swap with */
	    int i = (three_node(stn2)) ? 2 : 1;
	    /* change the other direction of leg i to use leg j */
	    reverse_leg(stn2->leg[i])->l.reverse += j - i;
	    stn2->leg[j] = stn2->leg[i];
	    j = i;
	 }
	 stn2->leg[j] = NULL;

#if PRINT_NETBITS
	 print_prefix(stn2->name); printf("<%p>",stn2); putnl();
#endif
      }
   }
}

static void
remove_travs(void)
{
   node *stn;
   out_current_action(msg(/*Concatenating traverses between nodes*/126));
   FOR_EACH_STN(stn, stnlist) {
      if (fixed(stn) || !two_node(stn)) {
	 if (stn->leg[0]) concatenate_trav(stn, 0);
	 if (stn->leg[1]) concatenate_trav(stn, 1);
	 if (stn->leg[2]) concatenate_trav(stn, 2);
      }
   }
}

static void
concatenate_trav(node *stn, int i)
{
   int j;
   stack *trav;
   node *stn2;
   linkfor *newleg, *newleg2;

   stn2 = stn->leg[i]->l.to;
   /* Reject single legs as they may be already concatenated traverses */
   if (fixed(stn2) || !two_node(stn2)) return;

   trav = osnew(stack);
   newleg2 = (linkfor*)osnew(linkrev);

#if PRINT_NETBITS
   printf("Concatenating trav "); print_prefix(stn->name); printf("<%p>",stn);
#endif

   newleg2->l.to = stn;
   newleg2->l.reverse = i | FLAG_REPLACEMENTLEG;
   trav->join1 = stn->leg[i];

   j = reverse_leg_dirn(stn->leg[i]);

   newleg = copy_link(stn->leg[i]);

   stn->leg[i] = newleg;

   while (1) {      
      stn = stn2;

#if PRINT_NETBITS
      printf(szLink); print_prefix(stn->name); printf("<%p>",stn);
#endif
      
      /* stop if fixed or 3 or 1 node */
      if (fixed(stn) || !two_node(stn)) break;
      
      remove_stn_from_list(&stnlist, stn);

      i = j ^ 1; /* flip direction for other leg of 2 node */

      stn2 = stn->leg[i]->l.to;
      j = reverse_leg_dirn(stn->leg[i]);

      addto_link(newleg, stn->leg[i]);
   }
   
   trav->join2 = stn->leg[j];
   trav->next = ptr;
   ptr = trav;

   newleg->l.to = stn;
   newleg->l.reverse= j | FLAG_DATAHERE | FLAG_REPLACEMENTLEG;

   stn->leg[j] = newleg2;

#if PRINT_NETBITS
   putchar(' ');
   print_var(newleg->v);
   printf("\nStacked ");
   print_prefix(newleg2->l.to->name);
   printf(",%d-", reverse_leg_dirn(newleg2));
   print_prefix(stn->name);
   printf(",%d\n", j);
#endif
}

static void
remove_subnets(void)
{
   node *stn, *stn2, *stn3, *stn4;
   int dirn, dirn2, dirn3, dirn4;
   stackRed *trav;
   linkfor *newleg, *newleg2;
   bool fMore = fTrue;

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
#ifdef EXPLICIT_STACKRED_TYPE
	       trav->type = 1; /* NOOSE */
#else
	       trav->join3 = NULL;
#endif
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
		    var sum, prod;
		    d temp, temp2;
#if 0 /*ndef NO_COVARIANCES*/
		    print_var(newleg->v);
		    printf("plus\n");
		    print_var(newleg2->v);
		    printf("equals\n");
#endif
		    addvv(&sum, &newleg->v, &newleg2->v);
		    ASSERT2(!fZero(&sum), "loop of zero variance found");
#if 0 /*ndef NO_COVARIANCES*/
		    print_var(sum);
#endif
		    mulvv(&prod, &newleg->v, &newleg2->v);
		    mulvd(&temp, &newleg2->v, &newleg->d);
		    mulvd(&temp2, &newleg->v, &newleg2->d);
		    adddd(&temp, &temp, &temp2);
/*printf("divdv(&newleg->d, &temp, &sum)\n");*/
		    divdv(&newleg->d, &temp, &sum);
/*printf("divvv(&newleg->v, &prod, &sum)\n");*/
		    divvv(&newleg->v, &prod, &sum);
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
#ifdef EXPLICIT_STACKRED_TYPE
	       trav->type = 0; /* PARALLEL */
#else
	       trav->join3 = trav->join1;
#endif
#if PRINT_NETBITS
	       printf("remove parallel\n");
#endif
	       ptrRed = trav;
	       fMore = fTrue;
	    }
	 }
/*printf("done replacing parallel legs\n");*/
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
	    if (fixed(stn) && three_node(stn)) {
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
		    var sum;
		    d temp;
		    node *stnZ;
		    prefix *nameZ;
		    addvv(&sum, &legAB->v, &legBC->v);
		    addvv(&sum, &sum, &legCA->v);
		    ASSERT2(!fZero(&sum), "loop of zero variance found");

		    mulvv(&legAZ->v, &legCA->v, &legAB->v);
		    divvv(&legAZ->v, &legAZ->v, &sum);
		    mulvv(&legBZ->v, &legAB->v, &legBC->v);
		    divvv(&legBZ->v, &legBZ->v, &sum);
		    mulvv(&legCZ->v, &legBC->v, &legCA->v);
		    divvv(&legCZ->v, &legCZ->v, &sum);

		    mulvd(&legAZ->d, &legCA->v, &legAB->d);
		    mulvd(&temp,     &legAB->v, &legCA->d);
		    subdd(&legAZ->d, &legAZ->d, &temp);
		    divdv(&legAZ->d, &legAZ->d, &sum);
		    mulvd(&legBZ->d, &legAB->v, &legBC->d);
		    mulvd(&temp,     &legBC->v, &legAB->d);
		    subdd(&legBZ->d, &legBZ->d, &temp);
		    divdv(&legBZ->d, &legBZ->d, &sum);
		    mulvd(&legCZ->d, &legBC->v, &legCA->d);
		    mulvd(&temp,     &legCA->v, &legBC->d);
		    subdd(&legCZ->d, &legCZ->d, &temp);
		    divdv(&legCZ->d, &legCZ->d, &sum);

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
		    addto_link(legAZ , stn4->leg[dirn4]);
		    addto_link(legBZ , stn5->leg[dirn5]);
		    addto_link(legCZ , stn6->leg[dirn6]);
		    /* stack stuff */
		    trav->join1 = stn4->leg[dirn4];
		    trav->join2 = stn5->leg[dirn5];
		    trav->join3 = stn6->leg[dirn6];
		    trav->next = ptrRed;
#ifdef EXPLICIT_STACKRED_TYPE
		    trav->type = 2; /* DELTASTAR */
#endif
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
		    /*print_prefix(stnZ->name);printf(" %p\n", (void*)stnZ);*/
		 }
	       
	    }
	    nodeltastar:;
	 }
	 /*printf("done replacing delta with star\n");*/
      }

   }
   /* printf("\ndone\n");*/
}

static void
replace_subnets(void)
{
   stackRed *ptrOld;
   node *stn, *stn2, *stn3, *stn4;
   int dirn, dirn2, dirn3, dirn4;
   linkfor *leg;
   unsigned long cBogus = 0; /* how many invented stations? */

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
            leg = stn3->leg[dirn3];
            stn2 = ptrRed->join1->l.to;
            dirn2 = reverse_leg_dirn(ptrRed->join1);

            if (fZero(&leg->v))
               e[0] = e[1] = e[2] = 0.0;
            else {
               subdd(&e, &POSD(stn4), &POSD(stn3) );
               subdd(&e, &e, &leg->d );
               divdv(&e, &e, &leg->v );
            }
            if (data_here(ptrRed->join1)) {
               mulvd(&e, &ptrRed->join1->v, &e);
               adddd(&POSD(stn2), &POSD(stn3), &ptrRed->join1->d);
               adddd(&POSD(stn2), &POSD(stn2), &e);
            } else {
               mulvd(&e, &stn2->leg[dirn2]->v, &e);
               subdd(&POSD(stn2), &POSD(stn3), &stn2->leg[dirn2]->d);
               adddd(&POSD(stn2), &POSD(stn2), &e);
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
               subdd(&e, &POSD(stn4), &POSD(stn3));
               subdd(&e, &e, &leg->d);
               divdv(&e, &e, &leg->v);
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
         d e;
         linkfor *leg1, *leg2;
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
               leg2 = stn[i]->leg[dirn[i]];
               leg1 = copy_link(leg[i]);
               stn2 = leg[i]->l.to;
               if (fZero(&leg2->v))
                  e[0] = e[1] = e[2] = 0.0;
               else {
                  subdd(&e, &POSD(stnZ), &POSD(stn[i]));
                  subdd(&e, &e, &leg2->d);
                  divdv(&e, &e, &leg2->v);
                  mulvd(&e, &leg1->v, &e);
               }
               adddd(&POSD(stn2), &POSD(stn[i]), &leg1->d);
               adddd(&POSD(stn2), &POSD(stn2), &e);
               fix(stn2);
               osfree(leg1);
               osfree(leg2);
               stn[i]->leg[dirn[i]] = leg[i];
               osfree(stnZ->leg[i]);
               stnZ->leg[i] = NULL;
            }
/*printf("---%d %f %f %f\n",cBogus,POS(stnZ, 0), POS(stnZ, 1), POS(stnZ, 2));*/
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
               stnZ->leg[i] = NULL;
            }
/*     printf("---%d not fixed\n", cBogus);*/
	 }

	 cBogus++;
      } else {
         BUG("ptrRed has unknown type");
      }

      ptrOld = ptrRed;
      ptrRed = ptrRed->next;
      osfree(ptrOld);
   }

   if (cBogus) {
      int cBogus2 = 0;
      /* this loop could stop when it has found cBogus, but continue for now as
       * it's a useful sanity check */
      FOR_EACH_STN(stn, stnlist) {
	 if (stn->name->up == NULL && stn->name->ident[0] == '\0') {
	    /* printf(":::%d %f %f %f\n", cBogus2, POS(stn, 0), POS(stn, 1), POS(stn, 2));*/
	    remove_stn_from_list(&stnlist, stn);
	    osfree(stn->name);
	    osfree(stn);
	    cBogus2++;
	    /* cBogus--;*/
	 }
      }
      ASSERT2(cBogus == cBogus2, "bogus station count is wrong");
   }
}

#ifdef BLUNDER_DETECTION
/* expected_error is actually squared... */
static void
do_gross(d e, d v, prefix *name1, prefix *name2, double expected_error)
{   
   double hsqrd, rsqrd, s, cx, cy, cz;
   double tot;
   int i;
   int output = 0;

#if 0
printf( "e = ( %.2f, %.2f, %.2f )", e[0], e[1], e[2] );
printf( " v = ( %.2f, %.2f, %.2f )\n", v[0], v[1], v[2] );
#endif
   hsqrd = sqrd(v[0]) + sqrd(v[1]);
   rsqrd = hsqrd + sqrd(v[2]);
   if (rsqrd == 0.0) return;

   cx = v[0] + e[0];
   cy = v[1] + e[1];
   cz = v[2] + e[2];

   s = (e[0] * v[0] + e[1] * v[1] + e[2] * v[2]) / rsqrd;
   tot = 0;
   for (i = 2; i >= 0; i--) tot += sqrd(e[i] - v[i] * s);

   if (tot <= expected_error) {
      if (!output) {
	 fprint_prefix(fhErrStat, name1);
	 fputs("->", fhErrStat);
	 fprint_prefix(fhErrStat, name2);
      }
      fprintf(fhErrStat, " L: %.2f", tot);
      output = 1;
   }

   s = sqrd(cx) + sqrd(cy);
   if (s > 0.0) {
      s = hsqrd / s;
      s = s < 0.0 ? 0.0 : sqrt(s);
      s = 1 - s;
      tot = sqrd(cx * s) + sqrd(cy * s) + sqrd(e[2]);
      if (tot <= expected_error) {
	 if (!output) {
	    fprint_prefix(fhErrStat, name1);
	    fputs("->", fhErrStat);
	    fprint_prefix(fhErrStat, name2);
	 }
	 fprintf(fhErrStat, " B: %.2f", tot);
	 output = 1;
      }
   }

   if (hsqrd > 0.0) {
      double nx, ny;
      s = (e[0] * v[1] - e[1] * v[0]) / hsqrd;
      nx = cx - s * v[1];
      ny = cy + s * v[0];
      s = sqrd(nx) + sqrd(ny) + sqrd(cz);
      if (s > 0.0) {
         s = rsqrd / s;
         s = s < 0.0 ? 0.0 : sqrt(s);
         tot = sqrd(cx - s * nx) + sqrd(cy - s * ny) + sqrd(cz - s * cz);
	 if (tot <= expected_error) {
	    if (!output) {
	       fprint_prefix(fhErrStat, name1);
	       fputs("->", fhErrStat);
	       fprint_prefix(fhErrStat, name2);
	    }
	    fprintf(fhErrStat, " G: %.2f", tot);
	    output = 1;
	 }
      }
   }
   if (output) fputnl(fhErrStat);
}
#endif

static void
replace_travs(void)
{
   stack *ptrOld;
   node *stn1, *stn2, *stn3;
   int i, j, k;
   double eTot, lenTrav, lenTot;
   double eTotTheo;
   double vTot, vTotTheo, hTot, hTotTheo;
   d sc, e;
   bool fEquate; /* used to indicate equates in output */
   int cLegsTrav;
   prefix *nmPrev;
   linkfor *leg;

   out_current_action(msg(/*Calculating traverses between nodes*/127));

   if (!fhErrStat)
      fhErrStat = safe_fopen_with_ext(fnm_output_base, EXT_SVX_ERRS, "w");

   if (!pimgOut) {
      char *fnmImg3D;
      char buf[256];

      fnmImg3D = add_ext(fnm_output_base, EXT_SVX_3D);
      sprintf(buf, msg(/*Writing out 3d image file '%s'*/121), fnmImg3D);
      out_current_action(buf); /* writing .3d file */
#ifdef NEW3DFORMAT
      pimgOut = cave_open_write(fnmImg3D, survey_title);
      if (!pimgOut) {
	 fputsnl(fnmImg3D, STDERR);
	 fatalerror(NULL, 0, cave_error(), fnmImg3D);
      }
#else
      pimgOut = img_open_write(fnmImg3D, survey_title, !fAscii);
      if (!pimgOut) {
	 fputsnl(fnmImg3D, STDERR);
	 fatalerror(img_error(), fnmImg3D);
      }
#endif
      osfree(fnmImg3D);
   }

   /* First do all the one leg traverses */
   FOR_EACH_STN(stn1, stnlist) {
      int i;
      for (i = 0; i <= 2; i++) {
	 linkfor *leg = stn1->leg[i];
	 if (leg && data_here(leg) &&
	     !(leg->l.reverse & FLAG_REPLACEMENTLEG) && !fZero(&leg->v)) {
	    if (fixed(stn1)) {
	       stn2 = leg->l.to;
	       fprint_prefix(fhErrStat, stn1->name);
	       fputs(szLink, fhErrStat);
	       fprint_prefix(fhErrStat, stn2->name);
#ifdef NEW3DFORMAT
	       if (stn1->name->pos->id == 0) cave_write_stn(stn1);
	       if (stn2->name->pos->id == 0) cave_write_stn(stn2);
	       cave_write_leg(leg);
#else
	       img_write_datum(pimgOut, img_MOVE, NULL,
			       POS(stn1, 0), POS(stn1, 1), POS(stn1, 2));
	       img_write_datum(pimgOut, img_LINE, NULL,
			       POS(stn2, 0), POS(stn2, 1), POS(stn2, 2));
#endif
	       subdd(&e, &POSD(stn2), &POSD(stn1));
	       subdd(&e, &e, &leg->d);
	       eTot = sqrdd(e);
	       hTot = sqrd(e[0]) + sqrd(e[1]);
	       vTot = sqrd(e[2]);
#ifndef NO_COVARIANCES
	       /* FIXME what about covariances? */
	       eTotTheo = leg->v[0][0] + leg->v[1][1] + leg->v[2][2];
	       hTotTheo = leg->v[0][0] + leg->v[1][1];
	       vTotTheo = leg->v[2][2];
#else
	       eTotTheo = leg->v[0] + leg->v[1] + leg->v[2];
	       hTotTheo = leg->v[0] + leg->v[1];
	       vTotTheo = leg->v[2];
#endif
	       err_stat(1, sqrt(sqrdd(leg->d)), eTot, eTotTheo,
			hTot, hTotTheo, vTot, vTotTheo);
	    }
	 }
      }
   }

   while (ptr != NULL) {
      bool fFixed;
      /* work out where traverse should be reconnected */
      leg = ptr->join1;
      leg = reverse_leg(leg);
      stn1 = leg->l.to;
      i = reverse_leg_dirn(leg);

      leg = ptr->join2;
      leg = reverse_leg(leg);
      stn2 = leg->l.to;
      j = reverse_leg_dirn(leg);

#if PRINT_NETBITS
      printf(" Trav ");
      print_prefix(stn1->name);
      printf("<%p>%s...%s", stn1, szLink, szLink);
      print_prefix(stn2->name);
      printf("<%p>\n", stn2);
#endif
      /*  ASSERT( fixed(stn1) );
	  ASSERT2( fixed(stn2), "stn2 not fixed in replace_travs"); */
      /* calculate scaling factors for error distribution */
      fFixed = fixed(stn1);
      if (fFixed) {
	 eTot = 0.0;
	 hTot = vTot = 0.0;
	 ASSERT(data_here(stn1->leg[i]));
	 if (fZero(&stn1->leg[i]->v)) {
	    sc[0] = sc[1] = sc[2] = 0.0;
	 } else {
	    subdd(&e, &POSD(stn2), &POSD(stn1));
	    subdd(&e, &e, &stn1->leg[i]->d);
	    eTot = sqrdd(e);
	    hTot = sqrd(e[0]) + sqrd(e[1]);
	    vTot = sqrd(e[2]);
	    divdv(&sc, &e, &stn1->leg[i]->v);
	 }
#ifndef NO_COVARIANCES
	 /* FIXME what about covariances? */
	 hTotTheo = stn1->leg[i]->v[0][0] + stn1->leg[i]->v[1][1];
	 vTotTheo = stn1->leg[i]->v[2][2];
	 eTotTheo = hTotTheo + vTotTheo;
#else
	 hTotTheo = stn1->leg[i]->v[0] + stn1->leg[i]->v[1];
	 vTotTheo = stn1->leg[i]->v[2];
	 eTotTheo = hTotTheo + vTotTheo;
#endif
	 cLegsTrav = 0;
	 lenTrav = 0.0;
	 nmPrev = stn1->name;
#ifdef NEW3DFORMAT
	 if (stn1->name->pos->id == 0) cave_write_stn(stn1);
#else
	 img_write_datum(pimgOut, img_MOVE, NULL,
			 POS(stn1, 0), POS(stn1, 1), POS(stn1, 2));
#endif
      }
      osfree(stn1->leg[i]);
      stn1->leg[i] = ptr->join1; /* put old link back in */

      osfree(stn2->leg[j]);
      stn2->leg[j] = ptr->join2; /* and the other end */

      if (fFixed) {
#ifdef BLUNDER_DETECTION
	 d err;
	 int do_blunder;
	 memcpy(&err, &e, sizeof(d));
	 do_blunder = (eTot > eTotTheo);
#if 0
	 fputs("\ntraverse ", fhErrStat);
	 fprint_prefix(fhErrStat, stn1->name);
	 fputs("->", fhErrStat);
	 fprint_prefix(fhErrStat, stn2->name);
	 fprintf(fhErrStat, " e=(%.2f, %.2f, %.2f) mag^2=%.2f %s\n",
		 e[0], e[1], e[2], eTot, (do_blunder ? "suspect:" : "OK"));
#else
	 fprintf(fhErrStat, "For next traverse e=(%.2f, %.2f, %.2f) "
		 "mag^2=%.2f %s\n", e[0], e[1], e[2], eTot,
		 (do_blunder ? "suspect:" : "OK"));
#endif
#endif
	 while (fTrue) {
	    int reached_end;
	    
  	    fEquate = fTrue;
	    lenTot = 0.0;
	    /* get next node in traverse
	     * should have stn3->leg[k]->l.to == stn1 */
	    stn3 = stn1->leg[i]->l.to;
	    k = reverse_leg_dirn(stn1->leg[i]);
	    ASSERT2(stn3->leg[k]->l.to == stn1,
		    "reverse leg doesn't reciprocate");

	    reached_end = (stn3 == stn2 && k == j);

	    if (data_here(stn1->leg[i])) {
 	       leg = stn1->leg[i];
#ifdef BLUNDER_DETECTION
	       if (do_blunder) do_gross(err, leg->d, stn1, stn3, eTotTheo);
#endif
	       if (reached_end) break;
	       adddd(&POSD(stn3), &POSD(stn1), &leg->d);	       
	    } else {
 	       leg = stn3->leg[k];
#ifdef BLUNDER_DETECTION
	       if (do_blunder) do_gross(err, leg->d, stn3, stn1, eTotTheo);
#endif
	       if (reached_end) break;
	       subdd(&POSD(stn3), &POSD(stn1), &leg->d);
	    }

	    if (!reached_end) add_stn_to_list(&stnlist, stn3);

	    mulvd(&e, &leg->v, &sc);
	    adddd(&POSD(stn3), &POSD(stn3), &e);
	    if (!fZero(&leg->v)) fEquate = fFalse;
	    lenTot += sqrdd(leg->d);
	    fix(stn3);
#ifdef NEW3DFORMAT
	    if (stn3->name->pos->id == 0) cave_write_stn(stn3);
	    cave_write_leg(leg);
#else
	    img_write_datum(pimgOut, img_LINE, NULL,
			    POS(stn3, 0), POS(stn3, 1), POS(stn3, 2));
#endif

	    if (nmPrev != stn3->name && !(fEquate && cLegsTrav == 0)) {
	       /* (node not part of same stn) &&
		* (not equate at start of traverse) */
	       fprint_prefix(fhErrStat, nmPrev);
#if PRINT_NAME_PTRS
	       fprintf(fhErrStat, "[%p|%p]", nmPrev, stn3->name);
#endif
	       fputs(fEquate ? szLinkEq : szLink, fhErrStat);
	       nmPrev = stn3->name;
#if PRINT_NAME_PTRS
	       fprintf(fhErrStat, "[%p]", nmPrev);
#endif
	       if (!fEquate) {
		  cLegsTrav++;
		  lenTrav += sqrt(lenTot);
	       } else if (lenTot > 0.0) {
#if DEBUG_INVALID
		  fprintf(stderr, "lenTot = %8.4f ", lenTot);
		  fprint_prefix(stderr, nmPrev);
		  fprintf(stderr, " -> ");
		  fprint_prefix(stderr, stn3->name);
#endif
		  BUG("during calculation of closure errors");
	       }
	    } else {
#if SHOW_INTERNAL_LEGS
	       fprintf(fhErrStat, "+");
#endif
	       if (lenTot > 0.0) {
#if DEBUG_INVALID
		  fprintf(stderr, "lenTot = %8.4f ", lenTot);
		  fprint_prefix(stderr, nmPrev);
		  fprintf(stderr, " -> ");
		  fprint_prefix(stderr, stn3->name);
#endif
		  BUG("during calculation of closure errors");
	       }
	    }

	    i = k ^ 1; /* flip direction for other leg of 2 node */

	    stn1 = stn3;
	 } /* endwhile */

#ifdef NEW3DFORMAT
	 if (stn2->name->pos->id == 0) cave_write_stn(stn2);
	 cave_write_leg(leg);
#else
	 img_write_datum(pimgOut, img_LINE, NULL,
			 POS(stn2,0), POS(stn2, 1), POS(stn2, 2));
#endif

#if PRINT_NETBITS
	 printf("Reached fixed or non-2node station\n");
#endif
#if 0 /* no point testing - this is the exit condn now ... */
	 if (stn3 != stn2 || k != j) {
	    print_prefix(stn3->name);
	    printf(",%d NOT ", k);
	    print_prefix(stn2->name);
	    printf(",%d\n", j);
	 }
#endif
	 if (data_here(stn1->leg[i])) {
	    if (!fZero(&stn1->leg[i]->v)) fEquate = fFalse;
	    lenTot += sqrdd(stn1->leg[i]->d);
	 } else {
	    ASSERT2(data_here(stn2->leg[j]),
		    "data not on either direction of leg");
	    if (!fZero(&stn2->leg[j]->v)) fEquate = fFalse;
	    lenTot += sqrdd(stn2->leg[j]->d);
	 }
#if PRINT_NETBITS
	 printf("lenTot increased okay\n");
#endif
	 if (cLegsTrav) {
	    if (stn2->name != nmPrev) {
	       fprint_prefix(fhErrStat, nmPrev);
#if PRINT_NAME_PTRS
	       fprintf(fhErrStat, "[%p]", nmPrev);
#endif
	       fputs(fEquate ? szLinkEq : szLink, fhErrStat);
	       if (!fEquate) cLegsTrav++;
	    }
#if SHOW_INTERNAL_LEGS
	    else
	       fputc('+', fhErrStat);
#endif
	    fprint_prefix(fhErrStat, stn2->name);
#if PRINT_NAME_PTRS
	    fprintf(fhErrStat, "[%p]", stn2->name);
#endif
	    lenTrav += sqrt(lenTot);
	 }
	 if (cLegsTrav)
	    err_stat(cLegsTrav, lenTrav, eTot, eTotTheo, 
		     hTot, hTotTheo, vTot, vTotTheo);
      }

      ptrOld = ptr;
      ptr = ptr->next;
      osfree(ptrOld);
   }

   /* leave fhErrStat open in case we're asked to close loops again */
#if 0
   fclose(fhErrStat);
#endif
}

static void
err_stat(int cLegsTrav, double lenTrav,
	 double eTot, double eTotTheo,
	 double hTot, double hTotTheo,
	 double vTot, double vTotTheo)
{
   fputnl(fhErrStat);
   fprintf(fhErrStat, msg(/*Original length%7.2fm (%3d legs), moved%7.2fm (%5.2fm/leg). */145),
	   lenTrav, cLegsTrav, sqrt(eTot), sqrt(eTot) / cLegsTrav);
   if (lenTrav > 0.0)
      fprintf(fhErrStat, msg(/*Error%7.2f%%*/146), 100 * sqrt(eTot) / lenTrav);
   else
      fputs(msg(/*Error    N/A*/147), fhErrStat);
   fputnl(fhErrStat);
   fprintf(fhErrStat, "%f\n", sqrt(eTot / eTotTheo));
   fprintf(fhErrStat, "H: %f V: %f\n",
	   sqrt(hTot / hTotTheo), sqrt(vTot / vTotTheo));
   fputnl(fhErrStat);
}

#if 0
static void
deletenode(node *stn)
{
   /* release any legs attached - reverse legs will be released since any */
   /* attached nodes aren't attached to fixed points */
   int d;
   for (d = 0; d <= 2; d++) osfree(stn->leg[d]); /* ignored if NULL */
   /* could delete prefix now... but this (slight) fudge is easier... */
   stn->name->stn=NULL;
   /* and release node itself */
   osfree(stn);
}
#endif

static void
replace_trailing_travs(void)
{
   stackTrail *ptrOld;
   node *stn1, *stn2;
   linkfor *leg;
   int i;
   bool fNotAttached = fFalse;

   out_current_action(msg(/*Calculating trailing traverses*/128));

   while (ptrTrail != NULL) {
      leg = ptrTrail->join1;
      leg = reverse_leg(leg);
      stn1 = leg->l.to;
      i = reverse_leg_dirn(leg);
#if PRINT_NETBITS
      printf(" Trailing trav ");
      print_prefix(stn1->name);
      printf("<%p>", stn1);
      printf("%s...\n", szLink);
      printf("    attachment stn is at (%f, %f, %f)\n",
	     POS(stn1, 0), POS(stn1, 1), POS(stn1, 2));
#endif
      /* We may have swap the links round when we removed the leg.  If we did
       * then stn1->leg[i] will be in use.  The link we swapped with is the
       * first free leg */
      if (stn1->leg[i]) {
	 /* j is the direction to swap with */
	 int j = (stn1->leg[1]) ? 2 : 1;
	 /* change the other direction of leg i to use leg j */
	 reverse_leg(stn1->leg[i])->l.reverse += j - i;
	 stn1->leg[j] = stn1->leg[i];
      }
      stn1->leg[i] = ptrTrail->join1;
      /* ASSERT(fixed(stn1)); */
      if (fixed(stn1)) {
#ifdef NEW3DFORMAT
	 if (stn1->name->pos->id == 0) cave_write_stn(stn1);
#else
	 img_write_datum(pimgOut, img_MOVE, NULL,
			 POS(stn1, 0), POS(stn1, 1), POS(stn1, 2));
#endif

	 while (1) {
	    int j;
	    leg = stn1->leg[i];
	    stn2 = leg->l.to;
	    j = reverse_leg_dirn(leg);
	    if (data_here(leg)) {
	       adddd(&POSD(stn2), &POSD(stn1), &leg->d);
#if 0
	       printf("Adding leg (%f, %f, %f)\n", leg->d[0], leg->d[1], leg->d[2]);
#endif
	    } else {
	       leg = stn2->leg[j];
	       subdd(&POSD(stn2), &POSD(stn1), &leg->d);
#if 0
	       printf("Subtracting reverse leg (%f, %f, %f)\n", leg->d[0], leg->d[1], leg->d[2]);
#endif
	    }

	    fix(stn2);
	    add_stn_to_list(&stnlist, stn2);
#ifdef NEW3DFORMAT
	    if (stn2->name->pos->id == 0) cave_write_stn(stn2);
	    cave_write_leg(leg);
#else
	    img_write_datum(pimgOut, img_LINE, NULL,
			    POS(stn2, 0), POS(stn2, 1), POS(stn2, 2));
#endif

	    /* stop if not 2 node */
	    if (!two_node(stn2)) break;

	    stn1 = stn2;	    
	    i = j ^ 1; /* flip direction for other leg of 2 node */
	 }
      }

      ptrOld = ptrTrail;
      ptrTrail = ptrTrail->next;
      osfree(ptrOld);
   }

   /* write stations to .3d file and free legs and stations */
   FOR_EACH_STN(stn1, stnlist) {
      if (fixed(stn1)) {
	 int d;
	 /* take care of unused fixed points */
#ifdef NEW3DFORMAT
	 if (stn1->name->pos->id == 0) cave_write_stn(stn1);      
#else
	 if (stn1->name->stn == stn1)
	    img_write_datum(pimgOut, img_LABEL, sprint_prefix(stn1->name),
			    POS(stn1, 0), POS(stn1, 1), POS(stn1, 2));
#endif
	 /* update coords of bounding box */
	 for (d = 0; d < 3; d++) {
	    if (POS(stn1, d) < min[d]) {
	       min[d] = POS(stn1, d);
	       pfxLo[d] = stn1->name;
	    }
	    if (POS(stn1, d) > max[d]) {
	       max[d] = POS(stn1, d);
	       pfxHi[d] = stn1->name;
	    }
	 }
      } else {
	 if (!fNotAttached) {
	    fNotAttached = fTrue;
	    error(/*Survey not all connected to fixed stations*/45);
	    out_puts(msg(/*The following survey stations are not attached to a fixed point:*/71));
	 }
	 out_puts(sprint_prefix(stn1->name));
      }
      for (i = 0; i <= 2; i++) {
	 linkfor *leg, *legRev;
	 leg = stn1->leg[i];
	 /* only want to think about forwards legs */
	 if (leg && data_here(leg)) {
	    node *stnB;
	    int iB;
	    stnB = leg->l.to;
	    iB = reverse_leg_dirn(leg);
	    legRev = stnB->leg[iB];
	    ASSERT2(legRev->l.to == stn1, "leg doesn't reciprocate");
	    if (fixed(stn1)) {
	       /* check not an equating leg */
	       if (!fZero(&leg->v)) {
		  totadj += sqrt(sqrd(POS(stnB, 0) - POS(stn1, 0)) +
				 sqrd(POS(stnB, 1) - POS(stn1, 1)) +
				 sqrd(POS(stnB, 2) - POS(stn1, 2)));
		  total += sqrt(sqrdd(leg->d));
		  totplan += sqrt(sqrd(leg->d[0]) + sqrd(leg->d[1]));
		  totvert += fabs(leg->d[2]);
	       }
	    }
	    osfree(leg);
	    osfree(legRev);
	    stn1->leg[i] = stnB->leg[iB] = NULL;
	 }
      }
   }

   /* The station position is attached to the name, so we leave the names and
    * positions in place - they can then be picked up if we have a *solve
    * followed by more data */
   for (stn1 = stnlist; stn1; stn1 = stn2) {
      stn2 = stn1->next;
      stn1->name->stn = NULL;
      osfree(stn1);
   }
   stnlist = NULL;
}
