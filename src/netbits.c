/* > netbits.c
 * Miscellaneous primitive network routines for Survex
 * Copyright (C) 1992-2001 Olly Betts
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

#if 0
# define DEBUG_INVALID 1
#endif

#include "debug.h"
#include "cavern.h"
#include "filename.h"
#include "message.h"
#include "netbits.h"
#include "datain.h" /* for compile_error */
#include "validate.h" /* for compile_error */

#define THRESHOLD (REAL_EPSILON * 1000) /* 100 was too small */

node *stn_iter = NULL; /* for FOR_EACH_STN */

#ifdef NO_COVARIANCES
static void check_var(/*const*/ var *v) {
   int bad = 0;
   int i;

   for (i = 0; i < 3; i++) {
      char buf[32];
      sprintf(buf, "%6.3f", v[i]);
      if (strstr(buf, "NaN")) printf("*** NaN!!!\n"), bad = 1;
   }
   if (bad) print_var(v);
   return;
}
#else
#define V(A,B) ((*v)[A][B])
static void check_var(/*const*/ var *v) {
   int bad = 0;
   int ok = 0;
   int i, j;
#if DEBUG_INVALID
   real det = 0.0;
#endif

   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) {
	 char buf[32];
	 sprintf(buf, "%6.3f", V(i, j));
	 if (strstr(buf, "NaN")) printf("*** NaN!!!\n"), bad = 1, ok = 1;
	 if (V(i, j) != 0.0) ok = 1;
      }
   }
   if (!ok) return; /* ignore all-zero matrices */

#if DEBUG_INVALID
   for (i = 0; i < 3; i++) {
      det += V(i, 0) * (V((i + 1) % 3, 1) * V((i + 2) % 3, 2) -
			V((i + 1) % 3, 2) * V((i + 2) % 3, 1));
   }

   if (fabs(det) < THRESHOLD)
      printf("*** Singular!!!\n"), bad = 1;
#endif

#if 0
   if (fabs(V(0,1) - V(1,0)) > THRESHOLD ||
       fabs(V(0,2) - V(2,0)) > THRESHOLD ||
       fabs(V(1,2) - V(2,1)) > THRESHOLD)
      printf("*** Not symmetric!!!\n"), bad = 1;
   if (V(0,0) <= 0.0 || V(1,1) <= 0.0 || V(2,2) <= 0.0)
      printf("*** Not positive definite (diag <= 0)!!!\n"), bad = 1;
   if (sqrd(V(0,1)) >= V(0,0)*V(1,1) || sqrd(V(0,2)) >= V(0,0)*V(2,2) ||
       sqrd(V(1,0)) >= V(0,0)*V(1,1) || sqrd(V(2,0)) >= V(0,0)*V(2,2) ||
       sqrd(V(1,2)) >= V(2,2)*V(1,1) || sqrd(V(2,1)) >= V(2,2)*V(1,1))
      printf("*** Not positive definite (off diag^2 >= diag product)!!!\n"), bad = 1;
#endif
   if (bad) print_var(*v);
}
#endif

static void check_d(/*const*/ delta *d) {
   int bad = 0;
   int i;

   for (i = 0; i < 3; i++) {
      char buf[32];
      sprintf(buf, "%6.3f", (*d)[i]);
      if (strstr(buf, "NaN")) printf("*** NaN!!!\n"), bad = 1;
   }

   if (bad) printf("(%4.2f,%4.2f,%4.2f)\n", (*d)[0], (*d)[1], (*d)[2]);
}

/* FIXME: station lists should probably know how long they are... */

/* insert at head of double-linked list */
void
add_stn_to_list(node **list, node *stn) {
   ASSERT(list);
   ASSERT(stn);
#if 0
   printf("add_stn_to_list(%p, [%p] ", list, stn);
   if (stn->name) print_prefix(stn->name);
   printf(")\n");
#endif
   stn->next = *list;
   stn->prev = NULL;
   if (*list) (*list)->prev = stn;
   *list = stn;
}

/* remove from double-linked list */
void
remove_stn_from_list(node **list, node *stn) {
   ASSERT(list);
   ASSERT(stn);
#if 0
   printf("remove_stn_from_list(%p, [%p] ", list, stn);
   if (stn->name) print_prefix(stn->name);
   printf(")\n");
#endif
#if DEBUG_INVALID
     {
	/* check station is actually in this list */
	node *stn_to_remove_is_in_list = *list;
	validate();
	while (stn_to_remove_is_in_list != stn) {
	   ASSERT(stn_to_remove_is_in_list);
	   stn_to_remove_is_in_list = stn_to_remove_is_in_list->next;
	}
     }
#endif
   /* adjust the iterator if it points to the element we're deleting */
   if (stn_iter == stn) stn_iter = stn_iter->next;
   /* need a special case if we're removing the list head */
   if (stn->prev == NULL) {
      *list = stn->next;
      if (*list) (*list)->prev = NULL;
   } else {
      stn->prev->next = stn->next;
      if (stn->next) stn->next->prev = stn->prev;
   }
}

/* Create (uses osmalloc) a forward leg containing the data in leg, or
 * the reversed data in the reverse of leg, if leg doesn't hold data
 */
extern linkfor *
copy_link(linkfor *leg)
{
   linkfor *legOut;
   int d;
   legOut = osnew(linkfor);
   if (data_here(leg)) {
      for (d = 2; d >= 0; d--) legOut->d[d] = leg->d[d];
   } else {
      leg = reverse_leg(leg);
      ASSERT(data_here(leg));
      for (d = 2; d >= 0; d--) legOut->d[d] = -leg->d[d];
   }
#if 1
# ifndef NO_COVARIANCES
   check_var(&(leg->v));
     {
	int i, j;
	for (i = 0; i < 3; i++) {
	   for (j = 0; j < 3; j++) legOut->v[i][j] = leg->v[i][j];
	}
     }
# else
   for (d = 2; d >= 0; d--) legOut->v[d] = leg->v[d];
# endif
#else
   memcpy(legOut->v, leg->v, sizeof(var));
#endif
   return legOut;
}

/* Adds to the forward leg `leg', the data in leg2, or the reversed data
 * in the reverse of leg2, if leg2 doesn't hold data
 */
extern linkfor *
addto_link(linkfor *leg, const linkfor *leg2)
{
   if (data_here(leg2)) {
      adddd(&leg->d, &leg->d, &((linkfor *)leg2)->d);
   } else {
      leg2 = reverse_leg(leg2);
      ASSERT(data_here(leg2));
      subdd(&leg->d, &leg->d, &((linkfor *)leg2)->d);
   }
   addvv(&leg->v, &leg->v, &((linkfor *)leg2)->v);
   return leg;
}

/* Add a leg between existing stations *fr and *to
 * If either node is a three node, then it is split into two
 * and the data structure adjusted as necessary
 */
void
addleg(node *fr, node *to,
       real dx, real dy, real dz,
       real vx, real vy, real vz
#ifndef NO_COVARIANCES
       , real cyz, real czx, real cxy
#endif
       )
{
   cLegs++; /* increment count (first as compiler may do tail recursion) */
   addfakeleg(fr, to, dx, dy, dz, vx, vy, vz
#ifndef NO_COVARIANCES
	      , cyz, czx, cxy
#endif
	      );
}

/* Add a 'fake' leg (not counted) between existing stations *fr and *to
 * If either node is a three node, then it is split into two
 * and the data structure adjusted as necessary
 */
void
addfakeleg(node *fr, node *to,
	   real dx, real dy, real dz,
	   real vx, real vy, real vz
#ifndef NO_COVARIANCES
	   , real cyz, real czx, real cxy
#endif
	   )
{
   int i, j;
   linkfor *leg, *leg2;
   int shape;

   if (fr->name == to->name) {
      /* we have been asked to add a leg with the same node at both ends
       * - give a warning and don't add the leg to the data structure
       */
      /* FIXME: should this be an error? */
      compile_warning(/*Survey leg with same station (`%s') at both ends - typing error?*/50,
		      sprint_prefix(fr->name));
      /* FIXME: inc loop count? */
      return;
   }

   leg = osnew(linkfor);
   leg2 = (linkfor*)osnew(linkrev);

   i = freeleg(&fr);
   j = freeleg(&to);

   leg->l.to = to;
   leg2->l.to = fr;
   leg->d[0] = dx;
   leg->d[1] = dy;
   leg->d[2] = dz;
#ifndef NO_COVARIANCES
   leg->v[0][0] = vx;
   leg->v[1][1] = vy;
   leg->v[2][2] = vz;
   leg->v[0][1] = leg->v[1][0] = cxy;
   leg->v[1][2] = leg->v[2][1] = cyz;
   leg->v[2][0] = leg->v[0][2] = czx;
   check_var(&(leg->v));
#else
   leg->v[0] = vx;
   leg->v[1] = vy;
   leg->v[2] = vz;
#endif
   leg2->l.reverse = i;
   leg->l.reverse = j | FLAG_DATAHERE;

   leg->l.flags = pcs->flags;

   fr->leg[i] = leg;
   to->leg[j] = leg2;

   shape = fr->name->pos->shape + 1;
   if (shape < 1) shape = 1 - shape;
   fr->name->pos->shape = shape;

   shape = to->name->pos->shape + 1;
   if (shape < 1) shape = 1 - shape;
   to->name->pos->shape = shape;
}

char
freeleg(node **stnptr)
{
   node *stn, *oldstn;
   linkfor *leg, *leg2;
#ifndef NO_COVARIANCES
   int i, j;
#endif

   stn = *stnptr;

   if (stn->leg[0] == NULL) return 0; /* leg[0] unused */
   if (stn->leg[1] == NULL) return 1; /* leg[1] unused */
   if (stn->leg[2] == NULL) return 2; /* leg[2] unused */

   /* All legs used, so split node in two */
   oldstn = stn;
   stn = osnew(node);
   leg = osnew(linkfor);
   leg2 = (linkfor*)osnew(linkrev);

   *stnptr = stn;

   add_stn_to_list(&stnlist, stn);
   stn->name = oldstn->name;

   leg->l.to = stn;
   leg->d[0] = leg->d[1] = leg->d[2] = (real)0.0;

#ifndef NO_COVARIANCES
   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) leg->v[i][j] = (real)0.0;
   }
#else
   leg->v[0] = leg->v[1] = leg->v[2] = (real)0.0;
#endif
   leg->l.reverse = 1 | FLAG_DATAHERE;
   leg->l.flags = pcs->flags;

   leg2->l.to = oldstn;
   leg2->l.reverse = 0;

   stn->leg[0] = oldstn->leg[0];
   /* correct reverse leg */
   reverse_leg(stn->leg[0])->l.to = stn;
   stn->leg[1] = leg2;

   oldstn->leg[0] = leg;

   stn->leg[2] = NULL; /* needed as stn->leg[dirn]==NULL indicates unused */

   return(2); /* leg[2] unused */
}

node *
StnFromPfx(prefix *name)
{
   node *stn;
   if (name->stn != NULL) return (name->stn);
   stn = osnew(node);
   stn->name = name;
   if (name->pos == NULL) {
      name->pos = osnew(pos);
#ifdef NEW3DFORMAT
      name->pos->id = 0;
#endif
      name->pos->shape = 0;
      unfix(stn);
   }
   stn->leg[0] = stn->leg[1] = stn->leg[2] = NULL;
   add_stn_to_list(&stnlist, stn);
   name->stn = stn;
   cStns++;
   return stn;
}

extern void
fprint_prefix(FILE *fh, const prefix *ptr)
{
   ASSERT(ptr);
   if (ptr->up != NULL) {
      fprint_prefix(fh, ptr->up);
      if (ptr->up->up != NULL) fputc('.', fh);
      fputs(ptr->ident, fh);
   } else {
      fputc('\\', fh);
   }
}

/* FIXME buffer overflow problems... */
static char *
sprint_prefix_(char *buf, OSSIZE_T len, const prefix *ptr)
{
   size_t i;
   if (ptr->up != NULL) {
      char *p;
      p = sprint_prefix_(buf, len, ptr->up);
      len -= (p - buf);
      buf = p;
      if (ptr->up->up != NULL) {
	 *buf++ = '.';
	 if (len <= 1) { printf("buffer overflow!\n"); exit(1); }
	 len--;
      }
      i = strlen(ptr->ident);
      if (len <= i) { printf("buffer overflow!\n"); exit(1); }
      memcpy(buf, ptr->ident, i);
      buf += i;
   } else {
      if (len <= 1) { printf("buffer overflow!\n"); exit(1); }
      *buf++ = '\\';
      len--;
   }
   *buf = '\0';
   return buf;
}

extern char *
sprint_prefix(const prefix *ptr)
{
   static char buffer[512];
   ASSERT(ptr);
   sprint_prefix_(buffer, sizeof(buffer), ptr);
   return buffer;
}

/* r = ab ; r,a,b are variance matrices */
void
mulvv(var *r, /*const*/ var *a, /*const*/ var *b)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] * (*b)[0];
   (*r)[1] = (*a)[1] * (*b)[1];
   (*r)[2] = (*a)[2] * (*b)[2];
#else
   int i, j, k;
   real tot;

   ASSERT((/*const*/ var *)r != a);
   ASSERT((/*const*/ var *)r != b);

   check_var(a);
   check_var(b);

   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) {
	 tot = 0;
	 for (k = 0; k < 3; k++) {
	    tot += (*a)[i][k] * (*b)[k][j];
	 }
	 (*r)[i][j] = tot;
      }
   }
   check_var(r);
#endif
}

/* r = ab ; r,b delta vectors; a variance matrix */
void
mulvd(delta *r, /*const*/ var *a, /*const*/ delta *b)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] * (*b)[0];
   (*r)[1] = (*a)[1] * (*b)[1];
   (*r)[2] = (*a)[2] * (*b)[2];
#else
   int i, k;
   real tot;

   ASSERT((/*const*/ delta*)r != b);
   check_var(a);
   check_d(b);

   for (i = 0; i < 3; i++) {
      tot = 0;
      for (k = 0; k < 3; k++) tot += (*a)[i][k] * (*b)[k];
      (*r)[i] = tot;
   }
   check_d(r);
#endif
}

/* r = ca ; r,a delta vectors; c real scaling factor  */
void
muldc(delta *r, /*const*/ delta *a, real c) {
   check_d(a);
   (*r)[0] = (*a)[0] * c;
   (*r)[1] = (*a)[1] * c;
   (*r)[2] = (*a)[2] * c;
   check_d(r);
}

/* r = ca ; r,a variance matrices; c real scaling factor  */
void
mulvc(var *r, /*const*/ var *a, real c)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] * c;
   (*r)[1] = (*a)[1] * c;
   (*r)[2] = (*a)[2] * c;
#else
   int i, j;

   check_var(a);
   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) (*r)[i][j] = (*a)[i][j] * c;
   }
   check_var(r);
#endif
}

/* r = a + b ; r,a,b delta vectors */
void
adddd(delta *r, /*const*/ delta *a, /*const*/ delta *b)
{
   check_d(a);
   check_d(b);
   (*r)[0] = (*a)[0] + (*b)[0];
   (*r)[1] = (*a)[1] + (*b)[1];
   (*r)[2] = (*a)[2] + (*b)[2];
   check_d(r);
}

/* r = a - b ; r,a,b delta vectors */
void
subdd(delta *r, /*const*/ delta *a, /*const*/ delta *b) {
   check_d(a);
   check_d(b);
   (*r)[0] = (*a)[0] - (*b)[0];
   (*r)[1] = (*a)[1] - (*b)[1];
   (*r)[2] = (*a)[2] - (*b)[2];
   check_d(r);
}

/* r = a + b ; r,a,b variance matrices */
void
addvv(var *r, /*const*/ var *a, /*const*/ var *b)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] + (*b)[0];
   (*r)[1] = (*a)[1] + (*b)[1];
   (*r)[2] = (*a)[2] + (*b)[2];
#else
   int i, j;

   check_var(a);
   check_var(b);
   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) (*r)[i][j] = (*a)[i][j] + (*b)[i][j];
   }
   check_var(r);
#endif
}

/* r = a - b ; r,a,b variance matrices */
void
subvv(var *r, /*const*/ var *a, /*const*/ var *b)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] - (*b)[0];
   (*r)[1] = (*a)[1] - (*b)[1];
   (*r)[2] = (*a)[2] - (*b)[2];
#else
   int i, j;

   check_var(a);
   check_var(b);
   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) (*r)[i][j] = (*a)[i][j] - (*b)[i][j];
   }
   check_var(r);
#endif
}

/* inv = v^-1 ; inv,v variance matrices */
#ifdef NO_COVARIANCES
extern int
invert_var(var *inv, /*const*/ var *v)
{
   int i;
   for (i = 0; i < 3; i++) {
      if ((*v)[i] < THRESHOLD) return 0; /* matrix is singular */
      (*inv)[i] = 1.0 / (*v)[i];
   }
   return 1;
}
#else
extern int
invert_var(var *inv, /*const*/ var *v)
{
   int i, j;
   real det = 0;

   ASSERT((/*const*/ var *)inv != v);

   check_var(v);
   for (i = 0; i < 3; i++) {
      det += V(i, 0) * (V((i + 1) % 3, 1) * V((i + 2) % 3, 2) -
			V((i + 1) % 3, 2) * V((i + 2) % 3, 1));
   }

   if (fabs(det) < THRESHOLD) {
      /* printf("det=%.20f\n", det); */
      return 0; /* matrix is singular */
   }

   det = 1 / det;

#define B(I,J) ((*v)[(J)%3][(I)%3])
   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) {
         (*inv)[i][j] = det * (B(i+1,j+1)*B(i+2,j+2) - B(i+2,j+1)*B(i+1,j+2));
      }
   }
#undef B

     { /* check that original * inverse = identity matrix */
	var p;
	real d = 0;
	mulvv(&p, v, inv);
	for (i = 0; i < 3; i++) {
	   for (j = 0; j < 3; j++ ) d += fabs(p[i][j] - (real)(i==j));
	}
	if (d > THRESHOLD) {
	   printf("original * inverse=\n");
	   print_var(*v);
	   printf("*\n");
	   print_var(*inv);
	   printf("=\n");
	   print_var(p);
	   BUG("matrix didn't invert");
	}
	check_var(inv);
     }

   return 1;
}
#endif

/* r = (b^-1)a ; r,a delta vectors; b variance matrix */
void
divdv(delta *r, /*const*/ delta *a, /*const*/ var *b)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] / (*b)[0];
   (*r)[1] = (*a)[1] / (*b)[1];
   (*r)[2] = (*a)[2] / (*b)[2];
#else
   var b_inv;
   if (!invert_var(&b_inv, b)) {
      print_var(*b);
      BUG("covariance matrix is singular");
   }
   mulvd(r, &b_inv, a);
#endif
}

/* f = a(b^-1) ; r,a,b variance matrices */
void
divvv(var *r, /*const*/ var *a, /*const*/ var *b)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] / (*b)[0];
   (*r)[1] = (*a)[1] / (*b)[1];
   (*r)[2] = (*a)[2] / (*b)[2];
#else
   var b_inv;
   check_var(a);
   check_var(b);
   if (!invert_var(&b_inv, b)) {
      print_var(*b);
      BUG("covariance matrix is singular");
   }
   mulvv(r, a, &b_inv);
   check_var(r);
#endif
}

bool
fZero(/*const*/ var *v) {
#ifdef NO_COVARIANCES
   /* variance-only version */
   return ((*v)[0] == 0.0 && (*v)[1] == 0.0 && (*v)[2] == 0.0);
#else
   int i, j;

   check_var(v);
   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) if ((*v)[i][j] != 0.0) return fFalse;
   }

   return fTrue;
#endif
}
