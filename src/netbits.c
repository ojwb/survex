/* > netbits.c
 * Miscellaneous primitive network routines for Survex
 * Copyright (C) 1992-1997 Olly Betts
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "debug.h"
#include "cavern.h"
#include "filename.h"
#include "message.h"
#include "netbits.h"
#include "datain.h" /* for compile_error */

node *stn_iter = NULL; /* for FOR_EACH_STN */

/* FIXME: station lists should know how long they are... */

/* insert at head of double-linked list */
void
add_stn_to_list(node **list, node *stn) {
   ASSERT(list);
   ASSERT(stn);
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
   /* adjust the iterator if it points to the element we're deleting */
   if (stn_iter == stn) stn_iter = stn_iter->next;
   /* need a special case if we're removing the list head */
   if (stn->prev == NULL) {
      *list = stn->next;
      if (*list) (*list)->prev = NULL;
   } else {
      stn->prev->next = stn->next;
      stn->next->prev = stn->prev;
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
      for (d = 2; d >= 0; d--) legOut->d[d] = -leg->d[d];
   }
#if 1
# ifndef NO_COVARIANCES
     {
	int i,j;
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
addto_link(linkfor *leg, linkfor *leg2)
{
   if (data_here(leg2)) {
      adddd(&leg->d, &leg->d, &leg2->d);
   } else {
      leg2 = reverse_leg(leg2);
      subdd(&leg->d, &leg->d, &leg2->d);
   }
   addvv(&leg->v, &leg->v, &leg2->v);
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
   uchar i, j;
   linkfor *leg, *leg2;

   if (fr->name == to->name) {
      /* we have been asked to add a leg with the same node at both ends
       * - give a warning and don't add the leg to the data structure
       */
      compile_warning(/*Survey leg with same station ('%s') at both ends - typing error?*/50,
		      sprint_prefix(fr->name));
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
#else
   leg->v[0] = vx;
   leg->v[1] = vy;
   leg->v[2] = vz;
#endif
   leg2->l.reverse = i;
   leg->l.reverse = (j | 0x80);

   fr->leg[i] = leg;
   to->leg[j] = leg2;

   fr->name->pos->shape++;
   to->name->pos->shape++;
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
   leg->l.reverse = (1 | 0x80);

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
fprint_prefix(FILE *fh, prefix *ptr)
{
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
sprint_prefix_(char *buf, OSSIZE_T len, prefix *ptr)
{
   int i;
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
sprint_prefix(prefix *ptr)
{
   static char buffer[512];
   sprint_prefix_(buffer, sizeof(buffer), ptr);
   return buffer;
}

/* r = ab ; r,a,b are variance matrices */
void
mulvv(var *r, const var *a, const var *b)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] * (*b)[0];
   (*r)[1] = (*a)[1] * (*b)[1];
   (*r)[2] = (*a)[2] * (*b)[2];
#else
   int i, j, k;
   real tot;

   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) {
	 tot = 0;
	 for (k = 0; k < 3; k++) {
	    tot += (*a)[i][k] * (*b)[k][j];
	 }
	 (*r)[i][j] = tot;
      }
   }
#endif
}

/* r = ab ; r,b delta vectors; a variance matrix */
void
mulvd(d *r, var *a, d *b)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] * (*b)[0];
   (*r)[1] = (*a)[1] * (*b)[1];
   (*r)[2] = (*a)[2] * (*b)[2];
#else
   int i, k;
   real tot;

   for (i = 0; i < 3; i++) {
      tot = 0;
      for (k = 0; k < 3; k++) tot += (*a)[i][k] * (*b)[k];
      (*r)[i] = tot;
   }
#endif
}

/* r = a + b ; r,a,b delta vectors */
void
adddd(d *r, d *a, d *b)
{
   (*r)[0] = (*a)[0] + (*b)[0];
   (*r)[1] = (*a)[1] + (*b)[1];
   (*r)[2] = (*a)[2] + (*b)[2];
}

/* r = a - b ; r,a,b delta vectors */
void
subdd(d *r, d *a, d *b) {
   (*r)[0] = (*a)[0] - (*b)[0];
   (*r)[1] = (*a)[1] - (*b)[1];
   (*r)[2] = (*a)[2] - (*b)[2];
}

/* r = a + b ; r,a,b variance matrices */
void
addvv(var *r, var *a, var *b)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] + (*b)[0];
   (*r)[1] = (*a)[1] + (*b)[1];
   (*r)[2] = (*a)[2] + (*b)[2];
#else
   int i, j;

   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) (*r)[i][j] = (*a)[i][j] + (*b)[i][j];
   }
#endif
}

/* r = a - b ; r,a,b variance matrices */
void
subvv(var *r, var *a, var *b)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] - (*b)[0];
   (*r)[1] = (*a)[1] - (*b)[1];
   (*r)[2] = (*a)[2] - (*b)[2];
#else
   int i, j;

   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) (*r)[i][j] = (*a)[i][j] - (*b)[i][j];
   }
#endif
}

#ifndef NO_COVARIANCES
/* inv = v^-1 ; inv,v variance matrices */
extern int
invert_var(var *inv, const var *v)
{
   int i, j;
   real det = 0;

   for (i = 0; i < 3; i++) {
      det += (*v)[i][0] * ((*v)[(i + 1) % 3][1] * (*v)[(i + 2) % 3][2] -
			   (*v)[(i + 1) % 3][2] * (*v)[(i + 2) % 3][1]);
   }

   if (fabs(det) < 1E-10) {
      printf("det=%.20f\n", det);
      return 0; /* matrix is singular - FIXME use epsilon */
   }

   det = 1 / det;

#define B(I,J) ((*v)[(J)%3][(I)%3])
   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) {
         (*inv)[i][j] = det * (B(i+1,j+1)*B(i+2,j+2) - B(i+2,j+1)*B(i+1,j+2));
/*         if ((i+j) % 2) (*inv)[i][j] = -((*inv)[i][j]);*/
      }
   }
#undef B

     {
	var p;
	real d = 0;
	mulvv( &p, v, inv );
	for (i = 0; i < 3; i++) {
	   for (j = 0; j < 3; j++ ) d += fabs(p[i][j] - (real)(i==j));
	}
	if (d > 1E-5) {
	   printf("original * inverse=\n");
	   print_var(*v);
	   printf("*\n");
	   print_var(*inv);
	   printf("=\n");
	   print_var(p);
	   BUG("matrix didn't invert");
	}
     }

   return 1;
}
#endif

/* r = (b^-1)a ; r,a delta vectors; b variance matrix */
void
divdv(d *r, d *a, var *b)
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
divvv(var *r, var *a, var *b)
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
   mulvv(r, a, &b_inv);
#endif
}

bool
fZero(var *v) {
#ifdef NO_COVARIANCES
   /* variance-only version */
   return ((*v)[0] == 0.0 && (*v)[1] == 0.0 && (*v)[2] == 0.0);
#else
   int i, j;

   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) if ((*v)[i][j] != 0.0) return fFalse;
   }

   return fTrue;
#endif
}
