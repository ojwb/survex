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
/* FIXME: we could pack min and dirn into one ulong which would still allow
 * 2^30 stations.  Also we shouldn't malloc each and every LIV separately */
typedef struct LIV { struct LIV *next; ulong min; uchar dirn; } liv;

/* The goto iter/uniter avoids using recursion which could lead to stack
 * overflow.  Instead we use a linked list which will probably use
 * much less memory on most compilers.
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
	 /* DO: remove stn from stnlist */
	 /* DO: add stn to component list */
	 if (livTos->min <= min) {
	    if (stn->colour <= min) {
	       /* DO: note down leg (<-), remove and replace:
		*                 /\   /        /\
		* [fixed point(s)]  *-*  -> [..]  )
		*                 \/   \        \/
		* DO: start new articulation
		*/
	       stn->fArtic = fTrue;
	    }
	    min = livTos->min;
	 }
	 livTmp = livTos;
	 livTos = livTos->next;
	 osfree(livTmp);
      } else if (stn->leg[i]->l.to->colour < min) {
	 min = stn->leg[i]->l.to->colour;
	 /* FIXME: hmm, min may be reduced on way back out or way in - is
	  * that OK? */
      }
   }
   if (livTos) goto uniter;
   return min;
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
      /* see if this is a fresh component - it may not be, we may be
       * processing the other way from a fixed point cut-line */
      /* FIXME: recode to not use status */
      if (stn->status != statFixed) cComponents++;
#endif

      for (i = 0; i <= 2 && stn->leg[i]; i++) {
	 if (stn->leg[i]->l.to->colour == 0) cUncolouredNeighbours++;
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
	 for (i = 0; i <= 2 && stn->leg[i]; i++) {
	    if (stn->leg[i]->l.to->colour == 0) {
	       ulong colBefore = colour;
	       node *stn2;

	       c++;
	       visit(stn->leg[i]->l.to);
	       n = colour - colBefore;
#ifdef DEBUG_ARTIC
	       printf("visited %lu nodes\n", n);
#endif
	       if (n == 0) continue;
	       /* Solve chunk of net from stn in dirn i up to stations
		* with fArtic set or fixed() true - hmm fixed() test
		* causes problems if an equated fixed point spans 2
		* articulations.
		* Then solve stations up to next set of fArtic points,
		* and repeat until all this bit done.
		*/
	       stn->status = statFixed;
	       stn2 = stn->leg[i]->l.to;
more:
	       solve_matrix(stnlist);
	       FOR_EACH_STN(stn2, stnlist) {
		  if (stn2->fArtic && fixed(stn2)) {
		     int d;
		     for (d = 0; d <= 2 && stn->leg[d]; d++) {
			node *stn3 = stn2->leg[d]->l.to;
			if (!fixed(stn3)) {
			   stn2 = stn3;
			   goto more;
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
#ifdef DEBUG_ARTIC
   FOR_EACH_STN(stn, stnlist) { /* high-light unfixed bits */
      if (stn->status && !fixed(stn)) {
	 print_prefix(stn->name);
	 printf(" [%p] UNFIXED\n", stn);
      }
   }
#endif
}
