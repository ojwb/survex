/* > netartic.c
 * Split up network at articulation points
 * Copyright (C) 1993-1999 Olly Betts
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
#include "netbits.h"
#include "matrix.h"
#include "out.h"

static ulong colour;

/* list item visit */
typedef struct LIV { struct LIV *next; ulong min; uchar dirn; } liv;

/* The goto iter/uniter avoids using recursion which could lead to stack
 * overflow.  Instead we use a linked list or array which will probably use
 * much less memory on most compilers.  Particularly true in visit_count().
 */

static ulong
visit(node *stn)
{
   ulong min;
   int i;
   liv *livTos = NULL, *livTmp;
iter:
   stn->colour = ++colour;
#ifdef DEBUG_ARTIC
   printf("visit: stn [%p] ", stn);
   print_prefix(stn->name);
   printf(" set to colour %d\n", colour);
#endif
   min = colour;
   for (i = 0; i <= 2 && stn->leg[i]; i++) {
      if (stn->leg[i]->l.to->colour == 0) {
	 livTmp = osnew(liv);
	 livTmp->next = livTos;
	 livTos = livTmp;
	 livTos->min = min;
	 livTos->dirn = reverse_leg_dirn(stn->leg[i]);
	 stn = stn->leg[i]->l.to;
	 goto iter;
uniter:
	 i = reverse_leg_dirn(stn->leg[livTos->dirn]);
	 stn = stn->leg[livTos->dirn]->l.to;
	 if (!(min < livTos->min)) {
	    if (min >= stn->colour) stn->fArtic = fTrue;
	    min = livTos->min;
	 }
	 livTmp = livTos;
	 livTos = livTos->next;
	 osfree(livTmp);
      } else if (stn->leg[i]->l.to->colour < min) {
	 min = stn->leg[i]->l.to->colour;
      }
   }
   if (livTos) goto uniter;
   return min;
}

static void
visit_count(node *stn, ulong max)
{
   int i;
   int sp = 0;
   node *stn2;
   uchar *j_s;
   stn->status = statFixed;
   if (fixed(stn)) return;
   add_stn_to_tab(stn);
   if (stn->fArtic) return;
   j_s = (uchar*)osmalloc((OSSIZE_T)max);
iter:
#ifdef DEBUG_ARTIC
   printf("visit_count: stn [%p] ", stn);
   print_prefix(stn->name);
   printf("\n");
#endif
   for (i = 0; i <= 2; i++) {
      if (stn->leg[i]) {
	 stn2 = stn->leg[i]->l.to;
	 if (fixed(stn2)) {
	    stn2->status = statFixed;
	 } else if (stn2->status != statFixed) {
	    add_stn_to_tab(stn2);
	    stn2->status = statFixed;
	    if (!stn2->fArtic) {
	       ASSERT2(sp<max, "dirn stack too small in visit_count");
	       j_s[sp] = reverse_leg_dirn(stn->leg[i]);
	       sp++;
	       stn = stn2;
	       goto iter;
uniter:
	       sp--;
	       i = reverse_leg_dirn(stn->leg[j_s[sp]]);
	       stn = stn->leg[j_s[sp]]->l.to;
	    }
	 }
      }
   }
   if (sp > 0) goto uniter;
   osfree(j_s);
   return;
}

/* We want to split stations list into a list of components, each of which
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

component *component_list = NULL;

extern void
articulate(void)
{
   node *stn, *stnStart;
   node *matrixlist = NULL;
   node *fixedlist = NULL;
   int c, i;
#ifdef DEBUG_ARTIC
   ulong cFixed;
#endif
   stn_tab = NULL; /* so we can just osfree it */
   /* find articulation points and components */
   cComponents = 0;
   colour = 0;
   stnStart = NULL;
   FOR_EACH_STN(stn, stnlist) {
      stn->fArtic = fFalse;
      if (fixed(stn)) {
	 stn->colour = ++colour;
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
      int cUncolouredNeighbours = 0;
      stn = stnStart;
#if 0
      /* see if this is a fresh component */
      if (stn->status != statFixed) cComponents++;
#endif
      /* FIXME: do we always have a fresh component?  I think so, but the
       * comment above suggests otherwise */
      cComponents++;

      for (i = 0; i <= 2; i++) {
	 if (stn->leg[i]) {
	    if (stn->leg[i]->l.to->colour == 0) {
	       cUncolouredNeighbours++;
	    } else {
/*	       stn->leg[i]->l.to->status = statFixed;*/
	    }
	 }
      }
#ifdef DEBUG_ARTIC
      print_prefix(stn->name);
      printf(" [%p] is root of component %ld\n", stn, cComponents);
      dump_node(stn);
      printf(" and colour = %d/%d\n", stn->colour, cFixed);
/*      if (cNeighbours == 0) printf("0-node\n");*/
#endif
      if (cUncolouredNeighbours) {
	 c = 0;
	 for (i = 0; i <= 2; i++) {
	    if (stn->leg[i] && stn->leg[i]->l.to->colour == 0) {
	       ulong colBefore = colour;
	       node *stn2;

	       c++;
	       visit(stn->leg[i]->l.to);
	       n = colour - colBefore;
#ifdef DEBUG_ARTIC
	       printf("visited %lu nodes\n", n);
#endif
	       if (n == 0) continue;
	       osfree(stn_tab);

	       /* we just need n to be a reasonable estimate >= the number
		* of stations left after reduction. If memory is
		* plentiful, we can be crass.
		*/
	       stn_tab = osmalloc((OSSIZE_T)(n*ossizeof(prefix*)));
	       
	       /* Solve chunk of net from stn in dirn i up to stations
		* with fArtic set or fixed() true. Then solve stations
		* up to next set of fArtic points, and repeat until all
		* this bit done.
		*/
	       stn->status = statFixed;
	       stn2 = stn->leg[i]->l.to;
more:
	       n_stn_tab = 0;
	       visit_count(stn2, n);
#ifdef DEBUG_ARTIC
	       printf("visit_count returned okay\n");
#endif
	       build_matrix(n_stn_tab, stn_tab);
	       FOR_EACH_STN(stn2, stnlist) {
		  if (stn2->fArtic && fixed(stn2)) {
		     int d;
		     for (d = 0; d <= 2; d++) {
			if (USED(stn2, d)) {
			   node *stn3 = stn2->leg[d]->l.to;
			   if (!fixed(stn3)) {
			      stn2 = stn3;
			      goto more;
			   }
			}
		     }
		     stn2->fArtic = fFalse;
		  }
	       }
            }
	 }
	 /* Special case to check if start station is an articulation point
	  * which it is iff we have to colour from it in more than one dirn
	  */
	 if (c > 1) stn->fArtic = fTrue;
#ifdef DEBUG_ARTIC
	 FOR_EACH_STN(stn, stnlist) {
	    printf("%ld - ", stn->colour);
	    print_prefix(stn->name);
	    putnl();
	 }
#endif
      }
      if (stnStart->colour == 1) {
#ifdef DEBUG_ARTIC
	 printf("%ld components\n",cComponents);
#endif
	 break;
      }
      for (stn = stnlist; stn->colour != stnStart->colour - 1; stn = stn->next) {
	 ASSERT2(stn,"ran out of coloured fixed points");
      }
      stnStart = stn;
   }
   osfree(stn_tab);
#ifdef DEBUG_ARTIC
   FOR_EACH_STN(stn, stnlist) { /* high-light unfixed bits */
      if (stn->status && !fixed(stn)) {
	 print_prefix(stn->name);
	 printf(" [%p] UNFIXED\n", stn);
      }
   }
#endif
}
