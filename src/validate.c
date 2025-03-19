/* validate.c
 *
 *   Checks that SURVEX's data structures are valid and consistent
 *
 *   NB The checks currently done aren't very comprehensive - more will be
 *    added if bugs require them
 *
 *   Copyright (C) 1993,1994,1996,2000,2001,2024 Olly Betts
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

#include <config.h>

#include "cavern.h"
#include "filename.h"
#include "message.h"
#include "netbits.h"
#include "validate.h"

/* Maximum absolute value of a coordinate of a fixed station for validate()
 * to allow, in metres.
 *
 * The Northing of the equator in southern hemisphere UTM zones is 10000000m,
 * which seems to be the largest coordinate in any common CRS.  We add an
 * extra 100km to allow for extending such a zone North to cover a cave system
 * which straddles the equator.
 */
#define MAX_POS 10100000.0

static bool validate_prefix_tree(void);
static bool validate_prefix_subtree(prefix *pfx);

static bool validate_station_list(void);

#undef validate
extern bool
validate(void)
{
   bool fOk = true;
   if (!validate_prefix_tree()) fOk = false;
   if (!validate_station_list()) fOk = false;
   if (fOk) puts("*** Data structures passed consistency checks");
   else puts("*** Data structures FAILED consistency checks");
   return fOk;
}

static bool
validate_prefix_tree(void)
{
   bool fOk = true;
   if (root->up != NULL) {
      printf("*** root->up == %p\n", root->up);
      fOk = false;
   }
   if (root->right != NULL) {
      printf("*** root->right == %p\n", root->right);
      fOk = false;
   }
   if (root->stn != NULL) {
      printf("*** root->stn == %p\n", root->stn);
      fOk = false;
   }
   if (root->pos != NULL) {
      printf("*** root->pos == %p\n", root->pos);
      fOk = false;
   }
   fOk &= validate_prefix_subtree(root);
   return fOk;
}

static bool
validate_prefix_subtree(prefix *pfx)
{
   bool fOk = true;
   prefix *pfx2;
   pfx2 = pfx->down;
   /* this happens now, as nodes are freed after solving */
#if 0
   if (pfx2 == NULL) {
      if (pfx->stn == NULL) {
	 printf("*** Leaf prefix '");
	 print_prefix(pfx);
	 printf("' has no station attached\n");
	 fOk = false;
      }
      return fOk;
   }
#endif

   while (pfx2 != NULL) {
      if (pfx2->stn != NULL && pfx2->stn->name != pfx2) {
	 printf("*** Prefix '");
	 print_prefix(pfx2);
	 printf("' ->stn->name is '");
	 print_prefix(pfx2->stn->name);
	 printf("'\n");
	 fOk = false;
      }
      if (pfx2->up != pfx) {
	 printf("*** Prefix '");
	 print_prefix(pfx2);
	 printf("' ->up is '");
	 print_prefix(pfx);
	 printf("'\n");
	 fOk = false;
      }
      fOk &= validate_prefix_subtree(pfx2);
      pfx2 = pfx2->right;
   }
   return fOk;
}

static bool
validate_station_list(void)
{
   bool fOk = true;
   node *stn2;
   int d, d2;

   SVX_ASSERT(!stnlist || !stnlist->prev);
   /* NB: don't use FOR_EACH_STN as it isn't reentrant at present */
   for (int fixed = 0; fixed < 2; ++fixed) {
      for (node *stn = fixed ? fixedlist : stnlist; stn; stn = stn->next) {
	 if (fixed(stn) != fixed) {
	     printf("*** Station '");
	     print_prefix(stn->name);
	     printf("' is %sfixed but on %sfixed list\n",
		    fixed ? "un" : "", fixed ? "" : "un");
	     fOk = false;
	 }

	 bool fGap = false;
#if 0
	 printf("V [%p]<-[%p]->[%p] ", stn->prev, stn, stn->next); print_prefix(stn->name); putnl();
#endif
	 SVX_ASSERT(stn->prev == NULL || stn->prev->next == stn);
	 SVX_ASSERT(stn->next == NULL || stn->next->prev == stn);
	 for (d = 0; d <= 2; d++) {
	    if (!stn->leg[d]) {
	       fGap = true;
	    } else {
	       if (fGap) {
		  printf("*** Station '");
		  print_prefix(stn->name);
		  printf("', leg %d is used, but an earlier leg isn't\n", d);
		  fOk = false;
	       }
	       stn2 = stn->leg[d]->l.to;
	       SVX_ASSERT(stn2);
	       d2 = reverse_leg_dirn(stn->leg[d]);
	       if (stn2->leg[d2] == NULL) {
		  /* fine iff stn is at the disconnected end of a fragment */
		  node *s;
		  /* NB: don't use FOR_EACH_STN as it isn't reentrant at present */
		  for (s = stnlist; s; s = s->next) if (s == stn) break;
		  if (!s) for (s = fixedlist; s; s = s->next) if (s == stn) break;
		  if (s) {
		     printf("*** Station '");
		     print_prefix(stn->name);
		     printf("', leg %d doesn't reciprocate from station '", d);
		     print_prefix(stn2->name);
		     printf("'\n");
		     fOk = false;
		  }
	       } else if (stn2->leg[d2]->l.to == NULL) {
		  printf("*** Station '");
		  print_prefix(stn2->name);
		  printf("' [%p], leg %d points to NULL\n", stn2, d2);
		  fOk = false;
	       } else if (stn2->leg[d2]->l.to!=stn) {
		  /* fine iff stn is at the disconnected end of a fragment */
		  node *s;
		  /* NB: don't use FOR_EACH_STN as it isn't reentrant at present */
		  for (s = stnlist; s; s = s->next) if (s == stn) break;
		  if (!s) for (s = fixedlist; s; s = s->next) if (s == stn) break;
		  if (s) {
		     printf("*** Station '");
		     print_prefix(stn->name);
		     printf("' [%p], leg %d reciprocates via station '", stn, d);
		     print_prefix(stn2->name);
		     printf("' to station '");
		     print_prefix(stn2->leg[d2]->l.to->name);
		     printf("'\n");
		     fOk = false;
		  }
	       } else if ((data_here(stn->leg[d]) != 0) ^
			  (data_here(stn2->leg[d2]) == 0)) {
		  printf("*** Station '");
		  print_prefix(stn->name);
		  printf("' [%p], leg %d reciprocates via station '", stn, d);
		  print_prefix(stn2->name);
		  if (data_here(stn->leg[d]))
		     printf("' - data on both legs\n");
		  else
		     printf("' - data on neither leg\n");
		  fOk = false;
	       }
	       if (data_here(stn->leg[d])) {
		  int i;
		  for (i = 0; i < 3; i++)
		     if (fabs(stn->leg[d]->d[i]) > MAX_POS) {
			printf("*** Station '");
			print_prefix(stn->name);
			printf("', leg %d, d[%d] = %g\n",
			       d, i, (double)(stn->leg[d]->d[i]));
			fOk = false;
		     }
	       }
	    }

	    if (fixed(stn)) {
	       if (fabs(POS(stn, 0)) > MAX_POS ||
		   fabs(POS(stn, 1)) > MAX_POS ||
		   fabs(POS(stn, 2)) > MAX_POS) {
		  printf("*** Station '");
		  print_prefix(stn->name);
		  printf("' fixed at coords (%f,%f,%f)\n",
			 POS(stn, 0), POS(stn, 1), POS(stn, 2) );
		  fOk = false;
	       }
	    }
	 }
      }
   }
   return fOk;
}

#undef dump_node
extern void
dump_node(node *stn)
{
    printf("stn [%p]", stn);
    if (stn->name) {
	if (stn->name->stn == stn) {
	    printf("<->");
	} else {
	    printf("-->");
	}
	printf("name ");
	print_prefix(stn->name);
	printf(" (%p)", stn->name);
    } else {
	printf("-->NULL");
    }

    printf(" colour %ld", stn->colour);
    if (fixed(stn)) {
	printf(" FIXED\n");
    } else {
	putnl();
    }

    for (int d = 0; d <= 2; d++) {
	linkfor *leg = stn->leg[d];
	if (leg) {
	    const char* type;
	    if (data_here(leg)) {
		type = fZeros(&leg->v) ? "=>" : "->";
	    } else {
		type = fZeros(&reverse_leg(leg)->v) ? "<=" : "<-";
	    }
	    printf("  leg #%d %s stn [%p] ", d, type, leg->l.to);
	    print_prefix(leg->l.to->name);
	    printf(" | rev %d\n", reverse_leg_dirn(leg));
	}
    }
}

/* This doesn't cover removed stations - might be nice to have
 * dump_entire_network() which iterates prefix tree */
#undef dump_network
extern void
dump_network(void)
{
   node *stn;
   /* NB: don't use FOR_EACH_STN as it isn't reentrant at present */
   for (stn = fixedlist; stn; stn = stn->next) dump_node(stn);
   for (stn = stnlist; stn; stn = stn->next) dump_node(stn);
}
