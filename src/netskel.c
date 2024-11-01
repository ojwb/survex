/* netskel.c
 * Survex network reduction - remove trailing traverses and concatenate
 * traverses between junctions
 * Copyright (C) 1991-2024 Olly Betts
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

/* #define BLUNDER_DETECTION */

#if 0
#define DEBUG_INVALID 1
#define VALIDATE 1
#define DUMP_NETWORK 1
#endif

#include <config.h>

#include "validate.h"
#include "debug.h"
#include "cavern.h"
#include "commands.h"
#include "datain.h"
#include "filename.h"
#include "message.h"
#include "filelist.h"
#include "img_hosted.h"
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
static void write_passage_models(void);

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

   /* We can't average across solving to fix positions. */
   clear_last_leg();

   if (stnlist == NULL) {
      if (first_solve) fatalerror(/*No survey data*/43);
      /* We've had a *solve followed by another *solve (or the implicit
       * *solve at the end of the data.  Don't moan about that. */
      return;
   }
   ptr = NULL;
   ptrTrail = NULL;
   dump_network();

   if (first_solve && !pcs->proj_str && !proj_str_out) {
      /* If we haven't already solved to find some station positions, and
       * there's no specified coordinate system, then check if there are any
       * fixed points, and if there aren't, invent one at (0,0,0).
       *
       * We do this first so the solving part is just like the standard case -
       * this avoid problems, such as sub-nodes of the invented fix having been
       * removed.  It also means we can fix the "first" station, which makes
       * more sense to the user. */
      for (stn = stnlist; stn; stn = stn->next)
	 if (fixed(stn)) break;

      if (!stn) {
	/* If we've had a *solve and all the new survey since then is hanging,
	 * we don't want to invent a fixed point.  We want to complain but
	 * the easiest way to is just to continue processing and let
	 * articulate() catch this condition as it will any hanging survey
	 * data. */
	 node *stnFirst = NULL;

	 /* New stations are pushed onto the head of the list, so the
	  * first station added is the last in the list. */
	 for (stn = stnlist; stn; stn = stn->next) {
	     /* Prefer a station with legs attached when choosing one to fix
	      * so that if there's a hanging station on a nosurvey leg we pick
	      * the main clump of survey data. */
	     if (stnFirst && !stnFirst->leg[0]) continue;
	     stnFirst = stn;
	 }

	 /* If we've got nosurvey legs, then the station we find to fix could
	  * have no real legs attached. */
	 SVX_ASSERT2(nosurveyhead || stnFirst->leg[0],
		     "no fixed stns, but we've got a zero node!");
	 SVX_ASSERT2(stnFirst, "no stations left in net!");
	 stn = stnFirst;
	 compile_diagnostic_pfx(DIAG_INFO, stn->name,
				/*Survey has no fixed points. Therefore I’ve fixed %s at (0,0,0)*/72,
				sprint_prefix(stn->name));
	 POS(stn,0) = (real)0.0;
	 POS(stn,1) = (real)0.0;
	 POS(stn,2) = (real)0.0;
      }
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

   /* Now write out any passage models. */
   write_passage_models();
}

static void
remove_trailing_travs(void)
{
   node *stn;
   /* TRANSLATORS: In French, Eric chose to use the terminology used by
    * toporobot: "sequence" for the English "traverse", which makes sense
    * (although toporobot actually uses this term to mean something more
    * specific).  Feel free to follow this lead if you can't think of a better
    * term - these messages mostly indicate how processing is progressing.
    *
    * A trailing traverse is a dead end back to a junction. */
   out_current_action(msg(/*Removing trailing traverses*/125));
   FOR_EACH_STN(stn, stnlist) {
      if (one_node(stn) && !fixed(stn)) {
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
	    print_prefix(stn2->name); printf("<%p>",stn2); fputs(szLink, stdout);
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
	    i = (three_node(stn2)) ? 2 : 1;
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
   /* TRANSLATORS: In French, Eric chose to use the terminology used by
    * toporobot: "sequence" for the English "traverse", which makes sense
    * (although toporobot actually uses this term to mean something more
    * specific).  Feel free to follow this lead if you can't think of a better
    * term - these messages mostly indicate how processing is progressing. */
   out_current_action(msg(/*Concatenating traverses*/126));
   FOR_EACH_STN(stn, stnlist) {
      if (three_node(stn) || fixed(stn)) {
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
   /* If the traverse is already a single leg there's nothing to do (this
    * may also be an already-replaced traverse).
    */
   if (!two_node(stn2) || fixed(stn2)) return;

   trav = osnew(stack);
   newleg2 = (linkfor*)osnew(linkrev);

#if PRINT_NETBITS
   printf("Concatenating trav "); print_prefix(stn->name); printf("<%p>",stn);
#endif

   newleg2->l.to = stn;
   newleg2->l.reverse = i | FLAG_REPLACEMENTLEG;
   trav->join1 = stn->leg[i];

   j = reverse_leg_dirn(stn->leg[i]);
   SVX_ASSERT(j == 0 || j == 1);

   newleg = copy_link(stn->leg[i]);

   while (1) {
      stn = stn2;

#if PRINT_NETBITS
      fputs(szLink, stdout); print_prefix(stn->name); printf("<%p>",stn);
#endif

      /* Check if we've reached the end of this traverse. */
      if (!two_node(stn) || fixed(stn)) break;

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
   print_var(&(newleg->v));
   printf("\nStacked ");
   print_prefix(newleg2->l.to->name);
   printf(",%d-", reverse_leg_dirn(newleg2));
   print_prefix(stn->name);
   printf(",%d\n", j);
#endif
}

#ifdef BLUNDER_DETECTION
/* expected_error is actually squared... */
/* only called if fhErrStat != NULL */
static void
do_gross(delta e, delta v, node *stn1, node *stn2, double expected_error)
{
   double hsqrd, rsqrd, s, cx, cy, cz;
   double tot;
   int i;
   int output = 0;
   prefix *name1 = stn1->name, *name2 = stn2->name;

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
      fprintf(fhErrStat, " L: %.2f", sqrt(tot));
      /* checked - works */
      fprintf(fhErrStat, " (%.2fm -> %.2fm)", sqrt(sqrdd(v)), sqrt(sqrdd(v)) * (1 - s));
      output = 1;
   }

   s = sqrd(cx) + sqrd(cy);
   if (s > 0.0) {
      s = hsqrd / s;
      SVX_ASSERT(s >= 0.0);
      s = sqrt(s);
      s = 1 - s;
      tot = sqrd(cx * s) + sqrd(cy * s) + sqrd(e[2]);
      if (tot <= expected_error) {
	 double newval, oldval;
	 if (!output) {
	    fprint_prefix(fhErrStat, name1);
	    fputs("->", fhErrStat);
	    fprint_prefix(fhErrStat, name2);
	 }
	 fprintf(fhErrStat, " B: %.2f", sqrt(tot));
	 /* checked - works */
	 newval = deg(atan2(cx, cy));
	 if (newval < 0) newval += 360;
	 oldval = deg(atan2(v[0], v[1]));
	 if (oldval < 0) oldval += 360;
	 fprintf(fhErrStat, " (%.2fdeg -> %.2fdeg)", oldval, newval);
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
	 SVX_ASSERT(s >= 0);
	 s = sqrt(s);
	 tot = sqrd(cx - s * nx) + sqrd(cy - s * ny) + sqrd(cz - s * cz);
	 if (tot <= expected_error) {
	    if (!output) {
	       fprint_prefix(fhErrStat, name1);
	       fputs("->", fhErrStat);
	       fprint_prefix(fhErrStat, name2);
	    }
	    fprintf(fhErrStat, " G: %.2f", sqrt(tot));
	    /* checked - works */
	    fprintf(fhErrStat, " (%.2fdeg -> %.2fdeg)",
		    deg(atan2(v[2], sqrt(v[0] * v[0] + v[1] * v[1]))),
		    deg(atan2(cz, sqrt(nx * nx + ny * ny))));
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
   double eTot = 0, lenTrav = 0, lenTot;
   double eTotTheo = 0;
   double vTot = 0, vTotTheo = 0, hTot = 0, hTotTheo = 0;
   delta e, sc;
   bool fEquate; /* used to indicate equates in output */
   int cLegsTrav = 0;
   bool fArtic;

    /* TRANSLATORS: In French, Eric chose to use the terminology used by
     * toporobot: "sequence" for the English "traverse", which makes sense
     * (although toporobot actually uses this term to mean something more
     * specific).  Feel free to follow this lead if you can't think of a better
     * term - these messages mostly indicate how processing is progressing. */
   out_current_action(msg(/*Calculating traverses*/127));

   if (!fhErrStat && !fSuppress)
      fhErrStat = safe_fopen_with_ext(fnm_output_base, EXT_SVX_ERRS, "w");

   if (!pimg) {
      char *fnm = add_ext(fnm_output_base, EXT_SVX_3D);
      filename_register_output(fnm);
      pimg = img_open_write_cs(fnm, s_str(&survey_title), proj_str_out,
			       img_FFLAG_SEPARATOR(output_separator));
      if (!pimg) fatalerror(img_error(), fnm);
      osfree(fnm);
   }

   /* First do all the one leg traverses */
   for (stn1 = stnlist; stn1; stn1 = stn1->next) {
#if PRINT_NETBITS
      printf("One leg traverses from ");
      print_prefix(stn1->name);
      printf(" [%p]\n", stn1);
#endif
      for (i = 0; i <= 2; i++) {
	 linkfor *leg = stn1->leg[i];
	 if (leg && data_here(leg) &&
	     !(leg->l.reverse & (FLAG_REPLACEMENTLEG | FLAG_FAKE))) {
	    SVX_ASSERT(fixed(stn1));
	    SVX_ASSERT(!fZeros(&leg->v));

	    stn2 = leg->l.to;
	    if (TSTBIT(leg->l.flags, FLAGS_SURFACE)) {
	       stn1->name->sflags |= BIT(SFLAGS_SURFACE);
	       stn2->name->sflags |= BIT(SFLAGS_SURFACE);
	    } else {
	       stn1->name->sflags |= BIT(SFLAGS_UNDERGROUND);
	       stn2->name->sflags |= BIT(SFLAGS_UNDERGROUND);
	    }
	    img_write_item(pimg, img_MOVE, 0, NULL,
			   POS(stn1, 0), POS(stn1, 1), POS(stn1, 2));
	    if (leg->meta) {
		pimg->days1 = leg->meta->days1;
		pimg->days2 = leg->meta->days2;
	    } else {
		pimg->days1 = pimg->days2 = -1;
	    }
	    pimg->style = (leg->l.flags >> FLAGS_STYLE_BIT0) & 0x07;
	    img_write_item(pimg, img_LINE, leg->l.flags & FLAGS_MASK,
			   sprint_prefix(stn1->name->up),
			   POS(stn2, 0), POS(stn2, 1), POS(stn2, 2));
	    if (!(leg->l.reverse & FLAG_ARTICULATION)) {
#ifdef BLUNDER_DETECTION
	       delta err;
	       int do_blunder;
#else
	       if (fhErrStat) {
		  fprint_prefix(fhErrStat, stn1->name);
		  fputs(szLink, fhErrStat);
		  fprint_prefix(fhErrStat, stn2->name);
	       }
#endif
	       subdd(&e, &POSD(stn2), &POSD(stn1));
	       subdd(&e, &e, &leg->d);
	       if (fhErrStat) {
		  eTot = sqrdd(e);
		  hTot = sqrd(e[0]) + sqrd(e[1]);
		  vTot = sqrd(e[2]);
#ifndef NO_COVARIANCES
		  /* FIXME: what about covariances? */
		  hTotTheo = leg->v[0] + leg->v[1];
		  vTotTheo = leg->v[2];
		  eTotTheo = hTotTheo + vTotTheo;
#else
		  hTotTheo = leg->v[0] + leg->v[1];
		  vTotTheo = leg->v[2];
		  eTotTheo = hTotTheo + vTotTheo;
#endif
#ifdef BLUNDER_DETECTION
		  memcpy(&err, &e, sizeof(delta));
		  do_blunder = (eTot > eTotTheo);
		  fputs("\ntraverse ", fhErrStat);
		  fprint_prefix(fhErrStat, stn1->name);
		  fputs("->", fhErrStat);
		  fprint_prefix(fhErrStat, stn2->name);
		  fprintf(fhErrStat, " e=(%.2f, %.2f, %.2f) mag=%.2f %s\n",
			  e[0], e[1], e[2], sqrt(eTot),
			  (do_blunder ? "suspect:" : "OK"));
		  if (do_blunder)
		     do_gross(err, leg->d, stn1, stn2, eTotTheo);
#endif
		  err_stat(1, sqrt(sqrdd(leg->d)), eTot, eTotTheo,
			   hTot, hTotTheo, vTot, vTotTheo);
	       }
	    }
	 }
      }
   }

   while (ptr != NULL) {
      /* work out where traverse should be reconnected */
      linkfor *leg = ptr->join1;
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
      printf("<%p>[%d]%s...%s", stn1, i, szLink, szLink);
      print_prefix(stn2->name);
      printf("<%p>[%d]\n", stn2, j);
#endif

      if (!fixed(stn1)) {
	  SVX_ASSERT(!fixed(stn2));
	  goto skip_hanging_traverse;
      }
      SVX_ASSERT(fixed(stn2));

      /* calculate scaling factors for error distribution */
      eTot = 0.0;
      hTot = vTot = 0.0;
      SVX_ASSERT(data_here(stn1->leg[i]));
      if (fZeros(&stn1->leg[i]->v)) {
	 sc[0] = sc[1] = sc[2] = 0.0;
      } else {
	 subdd(&e, &POSD(stn2), &POSD(stn1));
	 subdd(&e, &e, &stn1->leg[i]->d);
	 eTot = sqrdd(e);
	 hTot = sqrd(e[0]) + sqrd(e[1]);
	 vTot = sqrd(e[2]);
	 divds(&sc, &e, &stn1->leg[i]->v);
      }
#ifndef NO_COVARIANCES
      /* FIXME: what about covariances? */
      hTotTheo = stn1->leg[i]->v[0] + stn1->leg[i]->v[1];
      vTotTheo = stn1->leg[i]->v[2];
#else
      hTotTheo = stn1->leg[i]->v[0] + stn1->leg[i]->v[1];
      vTotTheo = stn1->leg[i]->v[2];
#endif
      eTotTheo = hTotTheo + vTotTheo;
      cLegsTrav = 0;
      lenTrav = 0.0;
      img_write_item(pimg, img_MOVE, 0, NULL,
		     POS(stn1, 0), POS(stn1, 1), POS(stn1, 2));

      fArtic = stn1->leg[i]->l.reverse & FLAG_ARTICULATION;
      osfree(stn1->leg[i]);
      stn1->leg[i] = ptr->join1; /* put old link back in */

      osfree(stn2->leg[j]);
      stn2->leg[j] = ptr->join2; /* and the other end */

#ifdef BLUNDER_DETECTION
      delta err;
      int do_blunder;
      memcpy(&err, &e, sizeof(delta));
      do_blunder = (eTot > eTotTheo);
      if (fhErrStat && !fArtic) {
	 fputs("\ntraverse ", fhErrStat);
	 fprint_prefix(fhErrStat, stn1->name);
	 fputs("->", fhErrStat);
	 fprint_prefix(fhErrStat, stn2->name);
	 fprintf(fhErrStat, " e=(%.2f, %.2f, %.2f) mag=%.2f %s\n",
		 e[0], e[1], e[2], sqrt(eTot),
		 (do_blunder ? "suspect:" : "OK"));
      }
#endif
      while (true) {
	 int reached_end;
	 prefix *leg_pfx;

	 fEquate = true;
	 /* get next node in traverse
	  * should have stn3->leg[k]->l.to == stn1 */
	 stn3 = stn1->leg[i]->l.to;
	 k = reverse_leg_dirn(stn1->leg[i]);
	 SVX_ASSERT2(stn3->leg[k]->l.to == stn1,
		 "reverse leg doesn't reciprocate");

	 reached_end = (stn3 == stn2 && k == j);

	 if (data_here(stn1->leg[i])) {
	    leg_pfx = stn1->name->up;
	    leg = stn1->leg[i];
#ifdef BLUNDER_DETECTION
	    if (do_blunder && fhErrStat)
	       do_gross(err, leg->d, stn1, stn3, eTotTheo);
#endif
	    if (!reached_end)
	       adddd(&POSD(stn3), &POSD(stn1), &leg->d);
	 } else {
	    leg_pfx = stn3->name->up;
	    leg = stn3->leg[k];
#ifdef BLUNDER_DETECTION
	    if (do_blunder && fhErrStat)
	       do_gross(err, leg->d, stn1, stn3, eTotTheo);
#endif
	    if (!reached_end)
	       subdd(&POSD(stn3), &POSD(stn1), &leg->d);
	 }

	 lenTot = sqrdd(leg->d);

	 if (!fZeros(&leg->v)) fEquate = false;
	 if (!reached_end) {
	    add_stn_to_list(&stnlist, stn3);
	    if (!fEquate) {
	       mulsd(&e, &leg->v, &sc);
	       adddd(&POSD(stn3), &POSD(stn3), &e);
	    }
	 }

	 if (!(leg->l.reverse & (FLAG_REPLACEMENTLEG | FLAG_FAKE))) {
	     if (TSTBIT(leg->l.flags, FLAGS_SURFACE)) {
		stn1->name->sflags |= BIT(SFLAGS_SURFACE);
		stn3->name->sflags |= BIT(SFLAGS_SURFACE);
	     } else {
		stn1->name->sflags |= BIT(SFLAGS_UNDERGROUND);
		stn3->name->sflags |= BIT(SFLAGS_UNDERGROUND);
	     }

	    SVX_ASSERT(!fEquate);
	    SVX_ASSERT(!fZeros(&leg->v));
	    if (leg->meta) {
		pimg->days1 = leg->meta->days1;
		pimg->days2 = leg->meta->days2;
	    } else {
		pimg->days1 = pimg->days2 = -1;
	    }
	    pimg->style = (leg->l.flags >> FLAGS_STYLE_BIT0) & 0x07;
	    img_write_item(pimg, img_LINE, leg->l.flags & FLAGS_MASK,
			   sprint_prefix(leg_pfx),
			   POS(stn3, 0), POS(stn3, 1), POS(stn3, 2));
	 }

	 /* FIXME: equate at the start of a traverse treated specially
	  * - what about equates at end? */
	 if (stn1->name != stn3->name && !(fEquate && cLegsTrav == 0)) {
	    /* (node not part of same stn) &&
	     * (not equate at start of traverse) */
#ifndef BLUNDER_DETECTION
	    if (fhErrStat && !fArtic) {
	       if (!prefix_ident(stn1->name)) {
		  /* FIXME: not ideal */
		  fputs("<fixed point>", fhErrStat);
	       } else {
		  fprint_prefix(fhErrStat, stn1->name);
	       }
	       fputs(fEquate ? szLinkEq : szLink, fhErrStat);
	       if (reached_end) {
		  if (!prefix_ident(stn3->name)) {
		     /* FIXME: not ideal */
		     fputs("<fixed point>", fhErrStat);
		  } else {
		     fprint_prefix(fhErrStat, stn3->name);
		  }
	       }
	    }
#endif
	    if (!fEquate) {
	       cLegsTrav++;
	       lenTrav += sqrt(lenTot);
	    }
	 } else {
#if SHOW_INTERNAL_LEGS
	    if (fhErrStat && !fArtic) fprintf(fhErrStat, "+");
#endif
	    if (lenTot > 0.0) {
#if DEBUG_INVALID
	       fprintf(stderr, "lenTot = %8.4f ", lenTot);
	       fprint_prefix(stderr, stn1->name);
	       fprintf(stderr, " -> ");
	       fprint_prefix(stderr, stn3->name);
#endif
	       BUG("during calculation of closure errors");
	    }
	 }
	 if (reached_end) break;

	 i = k ^ 1; /* flip direction for other leg of 2 node */

	 stn1 = stn3;
      } /* endwhile */

      if (cLegsTrav && !fArtic && fhErrStat)
	 err_stat(cLegsTrav, lenTrav, eTot, eTotTheo,
		  hTot, hTotTheo, vTot, vTotTheo);

skip_hanging_traverse:
      ptrOld = ptr;
      ptr = ptr->next;
      osfree(ptrOld);
   }

   /* Leave fhErrStat open in case we're asked to close loops again... */
}

static void
err_stat(int cLegsTrav, double lenTrav,
	 double eTot, double eTotTheo,
	 double hTot, double hTotTheo,
	 double vTot, double vTotTheo)
{
   double E = sqrt(eTot / eTotTheo);
   double H = sqrt(hTot / hTotTheo);
   double V = sqrt(vTot / vTotTheo);
   if (!fSuppress) {
      double sqrt_eTot = sqrt(eTot);
      fputnl(fhErrStat);
      fprintf(fhErrStat, msg(/*Original length %6.2fm (%3d legs), moved %6.2fm (%5.2fm/leg). */145),
	      lenTrav, cLegsTrav, sqrt_eTot, sqrt_eTot / cLegsTrav);
      if (lenTrav > 0.0) {
	 fprintf(fhErrStat, msg(/*Error %6.2f%%*/146), 100 * sqrt_eTot / lenTrav);
      } else {
	 /* TRANSLATORS: Here N/A means "Not Applicable" -- it means the
	  * traverse has zero length, so error per metre is meaningless.
	  *
	  * There should be 4 spaces between "Error" and "N/A" so that it lines
	  * up with the numbers in the message above. */
	 fputs(msg(/*Error    N/A*/147), fhErrStat);
      }
      fputnl(fhErrStat);
      fprintf(fhErrStat, "%f\n", E);
      fprintf(fhErrStat, "H: %f V: %f\n", H, V);
      fputnl(fhErrStat);
   }
   img_write_errors(pimg, cLegsTrav, lenTrav, E, H, V);
}

static void
replace_trailing_travs(void)
{
   stackTrail *ptrOld;
   node *stn1, *stn2;
   linkfor *leg;
   int i;

   /* TRANSLATORS: In French, Eric chose to use the terminology used by
    * toporobot: "sequence" for the English "traverse", which makes sense
    * (although toporobot actually uses this term to mean something more
    * specific).  Feel free to follow this lead if you can't think of a better
    * term - these messages mostly indicate how processing is progressing.
    *
    * A trailing traverse is a dead end back to a junction. */
   out_current_action(msg(/*Calculating trailing traverses*/128));

   while (ptrTrail != NULL) {
      leg = ptrTrail->join1;
      leg = reverse_leg(leg);
      stn1 = leg->l.to;
      if (!fixed(stn1)) {
	  // This happens in a component which wasn't attached to fixed points.
	  goto skip;
      }
      i = reverse_leg_dirn(leg);
#if PRINT_NETBITS
      printf(" Trailing trav ");
      print_prefix(stn1->name);
      printf("<%p>", stn1);
      printf("%s...\n", szLink);
      printf("    attachment stn is at (%f, %f, %f)\n",
	     POS(stn1, 0), POS(stn1, 1), POS(stn1, 2));
#endif
      /* We may have swapped the links round when we removed the leg.  If
       * we did then stn1->leg[i] will be in use.  The link we swapped
       * with is the first free leg */
      if (stn1->leg[i]) {
	 /* j is the direction to swap with */
	 int j = (stn1->leg[1]) ? 2 : 1;
	 /* change the other direction of leg i to use leg j */
	 reverse_leg(stn1->leg[i])->l.reverse += j - i;
	 stn1->leg[j] = stn1->leg[i];
      }
      stn1->leg[i] = ptrTrail->join1;
      img_write_item(pimg, img_MOVE, 0, NULL,
		     POS(stn1, 0), POS(stn1, 1), POS(stn1, 2));

      while (1) {
	 prefix *leg_pfx;
	 int j;

	 leg = stn1->leg[i];
	 stn2 = leg->l.to;
	 j = reverse_leg_dirn(leg);
	 if (data_here(leg)) {
	    leg_pfx = stn1->name->up;
	    adddd(&POSD(stn2), &POSD(stn1), &leg->d);
#if 0
	    printf("Adding leg (%f, %f, %f)\n", leg->d[0], leg->d[1], leg->d[2]);
#endif
	 } else {
	    leg_pfx = stn2->name->up;
	    leg = stn2->leg[j];
	    subdd(&POSD(stn2), &POSD(stn1), &leg->d);
#if 0
	    printf("Subtracting reverse leg (%f, %f, %f)\n", leg->d[0], leg->d[1], leg->d[2]);
#endif
	 }

	 add_stn_to_list(&stnlist, stn2);
	 if (!(leg->l.reverse & (FLAG_REPLACEMENTLEG | FLAG_FAKE))) {
	     if (TSTBIT(leg->l.flags, FLAGS_SURFACE)) {
		stn1->name->sflags |= BIT(SFLAGS_SURFACE);
		stn2->name->sflags |= BIT(SFLAGS_SURFACE);
	     } else {
		stn1->name->sflags |= BIT(SFLAGS_UNDERGROUND);
		stn2->name->sflags |= BIT(SFLAGS_UNDERGROUND);
	     }
	 }
	 if (!(leg->l.reverse & (FLAG_REPLACEMENTLEG | FLAG_FAKE))) {
	    SVX_ASSERT(!fZeros(&leg->v));
	    if (leg->meta) {
		pimg->days1 = leg->meta->days1;
		pimg->days2 = leg->meta->days2;
	    } else {
		pimg->days1 = pimg->days2 = -1;
	    }
	    pimg->style = (leg->l.flags >> FLAGS_STYLE_BIT0) & 0x07;
	    img_write_item(pimg, img_LINE, leg->l.flags & FLAGS_MASK,
			   sprint_prefix(leg_pfx),
			   POS(stn2, 0), POS(stn2, 1), POS(stn2, 2));
	 }

	 /* stop if not 2 node */
	 if (!two_node(stn2)) break;

	 stn1 = stn2;
	 i = j ^ 1; /* flip direction for other leg of 2 node */
      }

skip:
      ptrOld = ptrTrail;
      ptrTrail = ptrTrail->next;
      osfree(ptrOld);
   }

   /* write out connections with no survey data */
   while (nosurveyhead) {
      nosurveylink *p = nosurveyhead;
      if (!fixed(p->fr) || !fixed(p->to)) {
	  goto skip_nosurvey;
      }
      if (TSTBIT(p->flags, FLAGS_SURFACE)) {
	 p->fr->name->sflags |= BIT(SFLAGS_SURFACE);
	 p->to->name->sflags |= BIT(SFLAGS_SURFACE);
      } else {
	 p->fr->name->sflags |= BIT(SFLAGS_UNDERGROUND);
	 p->to->name->sflags |= BIT(SFLAGS_UNDERGROUND);
      }
      img_write_item(pimg, img_MOVE, 0, NULL,
		     POS(p->fr, 0), POS(p->fr, 1), POS(p->fr, 2));
      if (p->meta) {
	  pimg->days1 = p->meta->days1;
	  pimg->days2 = p->meta->days2;
      } else {
	  pimg->days1 = pimg->days2 = -1;
      }
      pimg->style = img_STYLE_NOSURVEY;
      img_write_item(pimg, img_LINE, (p->flags & FLAGS_MASK),
		     sprint_prefix(p->fr->name->up),
		     POS(p->to, 0), POS(p->to, 1), POS(p->to, 2));
skip_nosurvey:
      nosurveyhead = p->next;
      osfree(p);
   }

   /* write stations to .3d file and free legs and stations */
   for (stn1 = stnlist; stn1; stn1 = stn1->next) {
      int d;
      SVX_ASSERT(fixed(stn1));
      if (stn1->name->stn == stn1) {
	 int sf = stn1->name->sflags;
	 /* take care of unused fixed points */
	 if (!TSTBIT(sf, SFLAGS_SOLVED)) {
	    const char * label = NULL;
	    if (TSTBIT(sf, SFLAGS_ANON)) {
	       label = "";
	    } else if (prefix_ident(stn1->name)) {
	       label = sprint_prefix(stn1->name);
	    }
	    if (label) {
	       /* Set flag to stop station being rewritten after *solve. */
	       stn1->name->sflags = sf | BIT(SFLAGS_SOLVED);
	       sf &= SFLAGS_MASK;
	       if (stn1->name->max_export) sf |= BIT(SFLAGS_EXPORTED);
	       img_write_item(pimg, img_LABEL, sf, label,
			      POS(stn1, 0), POS(stn1, 1), POS(stn1, 2));
	    }
	 }
      }
      /* update coords of bounding box, ignoring the base positions
       * of points fixed with error estimates and only counting stations
       * in underground surveys.
       *
       * NB We don't set SFLAGS_UNDERGROUND for the anchor station for
       * a point fixed with error estimates, so this test will exclude
       * those too, which is what we want.
       */
      if (TSTBIT(stn1->name->sflags, SFLAGS_UNDERGROUND)) {
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

	 /* Range without anonymous stations at offset 3. */
	 if (!TSTBIT(stn1->name->sflags, SFLAGS_ANON)) {
	    for (d = 0; d < 3; d++) {
	       if (POS(stn1, d) < min[d + 3]) {
		  min[d + 3] = POS(stn1, d);
		  pfxLo[d + 3] = stn1->name;
	       }
	       if (POS(stn1, d) > max[d + 3]) {
		  max[d + 3] = POS(stn1, d);
		  pfxHi[d + 3] = stn1->name;
	       }
	    }
	 }
      }

      d = stn1->name->shape;
      if (d <= 1 && !TSTBIT(stn1->name->sflags, SFLAGS_USED)) {
	 bool unused_fixed_point = false;
	 if (d == 0) {
	    /* Unused fixed point without error estimates */
	    unused_fixed_point = true;
	 } else if (stn1->leg[0]) {
	    prefix *pfx = stn1->leg[0]->l.to->name;
	    if (!prefix_ident(pfx) && !TSTBIT(pfx->sflags, SFLAGS_ANON)) {
	       /* Unused fixed point with error estimates */
	       unused_fixed_point = true;
	    }
	 }
	 if (unused_fixed_point) {
	    /* TRANSLATORS: fixed survey station that is not part of any survey
	     */
	    warning_in_file(stn1->name->filename, stn1->name->line,
		    /*Unused fixed point “%s”*/73, sprint_prefix(stn1->name));
	 }
      }

      /* For stations fixed with error estimates, we need to ignore the leg to
       * the "real" fixed point in the node stats.
       */
      if (stn1->leg[0] && !prefix_ident(stn1->leg[0]->l.to->name) &&
	  !TSTBIT(stn1->leg[0]->l.to->name->sflags, SFLAGS_ANON))
	 stn1->name->shape--;

      for (i = 0; i <= 2; i++) {
	 leg = stn1->leg[i];
	 /* only want to think about forwards legs */
	 if (leg && data_here(leg)) {
	    linkfor *legRev;
	    node *stnB;
	    int iB;
	    stnB = leg->l.to;
	    iB = reverse_leg_dirn(leg);
	    legRev = stnB->leg[iB];
	    SVX_ASSERT2(legRev->l.to == stn1, "leg doesn't reciprocate");
	    SVX_ASSERT(fixed(stn1));
	    if (!(leg->l.flags &
		  (BIT(FLAGS_DUPLICATE)|BIT(FLAGS_SPLAY)|
		   BIT(FLAGS_SURFACE)))) {
	       /* check not an equating leg, or one inside an sdfix point */
	       if (!(leg->l.reverse & (FLAG_REPLACEMENTLEG | FLAG_FAKE))) {
		  totadj += sqrt(sqrd(POS(stnB, 0) - POS(stn1, 0)) +
				 sqrd(POS(stnB, 1) - POS(stn1, 1)) +
				 sqrd(POS(stnB, 2) - POS(stn1, 2)));
		  total += sqrt(sqrdd(leg->d));
		  totplan += hypot(leg->d[0], leg->d[1]);
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

static void
write_passage_models(void)
{
   lrudlist * psg = model;
   while (psg) {
      lrudlist * oldp;
      lrud * xsect = psg->tube;
      int xflags = 0;
      while (xsect) {
	 lrud *oldx;
	 prefix *pfx;
	 const char *name;
	 pimg->l = xsect->l;
	 pimg->r = xsect->r;
	 pimg->u = xsect->u;
	 pimg->d = xsect->d;
	 if (xsect->meta) {
	     pimg->days1 = xsect->meta->days1;
	     pimg->days2 = xsect->meta->days2;
	 } else {
	     pimg->days1 = pimg->days2 = -1;
	 }

	 pfx = xsect->stn;
	 name = sprint_prefix(pfx);
	 oldx = xsect;
	 xsect = xsect->next;
	 osfree(oldx);

	 if (!pfx->pos) {
	     /* TRANSLATORS: e.g. the user specifies a passage cross-section at
	      * station "entrance.27", but there is no station "entrance.27" in
	      * the centre-line. */
	     compile_diagnostic_pfx(DIAG_ERR, pfx,
				    /*Cross section specified at non-existent station “%s”*/83,
				    name);
	 } else {
	     if (xsect == NULL) xflags = img_XFLAG_END;
	     img_write_item(pimg, img_XSECT, xflags, name, 0, 0, 0);
	 }
      }
      oldp = psg;
      psg = psg->next;
      osfree(oldp);
   }
   model = NULL;
}

node *
find_non_anon_stn(node *stn)
{
    if (TSTBIT(stn->name->sflags, SFLAGS_ANON)) {
	/* An anonymous stations must be at the end of a trailing traverse
	 * (since the same anonymous station can't be referred to more
	 * than once), and trailing traverses have been removed at this
	 * point.
	 *
	 * However, we may remove a hanging trailing traverse back to an
	 * anonymous station.  It's not helpful to fail to point to a
	 * station in such a case so we look through the list of trailing
	 * traverses to find the one which would reattach to this station
	 * and report a station from that traverse instead.
	 */
	for (stackTrail* p = ptrTrail; p; p = p->next) {
	    linkfor *leg = ptrTrail->join1;
	    if (reverse_leg(leg)->l.to == stn) {
		return leg->l.to;
	    }
	}
    }
    return stn;
}
