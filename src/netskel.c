/* > netskel.c
 * Survex network reduction - remove trailing traverses and concatenate
 * traverses between junctions
 * Copyright (C) 1991-2000 Olly Betts
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

/*#define BLUNDER_DETECTION 1*/

#if 0
#define DEBUG_INVALID 1
#define VALIDATE 1
#define DUMP_NETWORK 1
#endif

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "validate.h"
#include "debug.h"
#include "cavern.h"
#include "filename.h"
#include "message.h"
#include "filelist.h"
#include "netartic.h"
#include "netbits.h"
#include "netskel.h"
#include "network.h"
#include "out.h"

#define sqrdd(X) (sqrd((X)[0]) + sqrd((X)[1]) + sqrd((X)[2]))

typedef struct Stack {
   struct Link *join1, *join2;
   struct Stack *next;
} stack;

typedef struct StackTr {
   struct Link *join1;
   struct StackTr *next;
} stackTrail;

/* string used between things in traverse printouts eg \1 - \2 - \3 -...*/
static const char *szLink = " - ";
static const char *szLinkEq = " = "; /* use this one for equates */

#if 0
#define fprint_prefix(FH, NAME) BLK((fprint_prefix)((FH), (NAME));\
                                    fprintf((FH), " [%p]", (void*)(NAME)); )
#endif

static stack *ptr; /* Ptr to TRaverse linked list for in-between travs */
static stackTrail *ptrTrail; /* Ptr to TRaverse linked list for trail travs*/

static void remove_trailing_travs(void);
static void remove_travs(void);
static void replace_travs(void);
static void replace_trailing_travs(void);

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
   dump_network();

   /* If there are no fixed points, invent one.  Do this first to
    * avoid problems with sub-nodes of the invented fix which have
    * been removed.  It also means we can fix the "first" station,
    * which makes more sense to the user. */
   FOR_EACH_STN(stn, stnlist)
      if (fixed(stn)) break;
   
   if (!stn) {
      node *stnFirst = NULL;
      
      if (!first_solve) {
	 /* We've had a *solve and all the new survey since then is hanging,
	  * so don't invent a fixed point but complain instead */
	 /* Let replace_trailing_travs() do the work for us... */
	 remove_trailing_travs();
	 replace_trailing_travs();
	 return;	 
      }
      
      /* New stations are pushed onto the head of the list, so the
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

   first_solve = 0;

   remove_trailing_travs();
   validate(); dump_network();
   remove_travs();
   validate(); dump_network();
   remove_subnets();
   validate(); dump_network();
   articulate();
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
      if (fixed(stn) || three_node(stn)) {
	 int d;
	 for (d = 0; d <= 2; d++) {
	    linkfor *leg = stn->leg[d];
	    if (leg && !(leg->l.reverse & FLAG_REPLACEMENTLEG))
	       concatenate_trav(stn, d);
	 }
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
   ASSERT(j == 0 || j == 1);

   newleg = copy_link(stn->leg[i]);

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
   newleg->l.reverse = j | FLAG_DATAHERE | FLAG_REPLACEMENTLEG;

   newleg2->l.to->leg[reverse_leg_dirn(newleg2)] = newleg;
   /* i.e. stn->leg[i] = newleg; with original stn and i */

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
   d e, sc;
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

	    lenTot = sqrdd(leg->d);
	    
	    add_stn_to_list(&stnlist, stn3);

	    if (!fZero(&leg->v)) {
	       fEquate = fFalse;
	       mulvd(&e, &leg->v, &sc);
	       adddd(&POSD(stn3), &POSD(stn3), &e);
	    }
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
	 /* FIXME: better to handle the last leg in a traverse with the
	  * same code as all the others - i.e. in loop above */
	 if (data_here(stn1->leg[i])) {
	    if (!fZero(&stn1->leg[i]->v)) fEquate = fFalse;
	    lenTot = sqrdd(stn1->leg[i]->d);
	 } else {
	    ASSERT2(data_here(stn2->leg[j]),
		    "data not on either direction of leg");
	    if (!fZero(&stn2->leg[j]->v)) fEquate = fFalse;
	    lenTot = sqrdd(stn2->leg[j]->d);
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
	 cComponents++; /* adjust component count */
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
