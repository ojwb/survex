/* > matrix.c
 * Matrix building and solving routines
 * Copyright (C) 1993-1998 Olly Betts
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

#undef PRINT_MATRICES
#define PRINT_MATRICES 0

#undef DEBUG_MATRIX_BUILD
#define DEBUG_MATRIX_BUILD 0

#undef DEBUG_MATRIX
#define DEBUG_MATRIX 0

#if PRINT_MATRICES
static void print_matrix(real FAR *M, real *B);
#endif

static void choleski(
#ifdef NO_COVARIANCES
		     real FAR *M, real *B
#else
		     var FAR *M, d *B
#endif
		     );

#ifdef SOR
static void sor(real FAR *M, real *B);
#endif

#ifdef NO_COVARIANCES
/* for M(row, col) col must be <= row, so Y <= X */
# define M(X, Y) ((real Huge *)M)[((((OSSIZE_T)(X)) * ((X) + 1)) >> 1) + (Y)]
              /* +(Y>X?0*printf("row<col (line %d)\n",__LINE__):0) */
/*#define M_(X, Y) ((real Huge *)M)[((((OSSIZE_T)(Y)) * ((Y) + 1)) >> 1) + (X)]*/
#else
# define M(X, Y) ((var Huge *)M)[((((OSSIZE_T)(X)) * ((X) + 1)) >> 1) + (Y)]
#endif

static int find_stn_in_tab(node *stn);
static int add_stn_to_tab(node *stn);
static void build_matrix(long n, prefix **stn_tab);

static long n_stn_tab;

static prefix **stn_tab; /* FIXME: use pos ** instead */
static long n;

#if 0
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
   for (i = 0; i <= 2; i++) {
      if (stn->leg[i]) {
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

extern void
solve_matrix(void)
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
      /* FIXME: need to count components to get correct loop count */
      /* see if this is a fresh component */
      if (stn->status != statFixed) cComponents++;
#endif

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
#else
extern void
solve_matrix(void)
{
   node *stn;
   n = 0;
   FOR_EACH_STN(stn, stnlist) {
      if (!fixed(stn)) n++;
   }
   if (n == 0) return;

   /* we just need n to be a reasonable estimate >= the number
    * of stations left after reduction. If memory is
    * plentiful, we can be crass.
    */
   stn_tab = osmalloc((OSSIZE_T)(n*ossizeof(prefix*)));
   
   FOR_EACH_STN(stn, stnlist) {
      if (!fixed(stn)) add_stn_to_tab(stn);
   }

   build_matrix(n_stn_tab, stn_tab);
}
#endif

#ifdef NO_COVARIANCES
# define B_ZERO (real)0.0
# define M_ZERO (real)0.0
# define MELT_TYPE real
# define BELT_TYPE real
# define MELT_COPY(A, B) ((A) = (B))
# define BELT_COPY(A, B) ((A) = (B))
#else
# define B_ZERO d_zero
# define M_ZERO var_zero
# define MELT_TYPE var
# define BELT_TYPE d
# define MELT_COPY(A, B) (memcpy((A), (B), sizeof(var)))
# define BELT_COPY(A, B) (memcpy((A), (B), sizeof(d)))
#endif

static void
build_matrix(long n, prefix **stn_tab)
{
#ifdef NO_COVARIANCES
   real FAR *M;
   real *B;
   real e;
#else
   var FAR *M;
   d *B;
   d d_zero = { 0.0, 0.0, 0.0 };
   var var_zero = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
   var e;
   d a;
#endif
   node *stn;
   int row, col;
   int dim;

#ifdef DEBUG_ARTIC
   printf("# stations %lu\n",n);
   if (n) {
      int m;
      for (m = 0; m < n; m++) {
	 print_prefix(stn_tab[m]);
	 putnl();
      }
   }
#endif

   if (n == 0) {
      out_puts(msg(/*Network solved by reduction - no simultaneous equations to solve.*/74));
      return;
   }
   /* release any unused entries in stn_tab (warning: n==0 is bad news) */
   /*  stn_tab=osrealloc(stn_tab,n*ossizeof(prefix*)); */
   /* (OSSIZE_T) cast may be needed if n>=181 */
   M = osmalloc((OSSIZE_T)((((OSSIZE_T)n * (n + 1)) >> 1)) * ossizeof(MELT_TYPE));
   B = osmalloc((OSSIZE_T)(n * ossizeof(BELT_TYPE)));

   out_puts("");
   if (n == 1)
      out_puts(msg(/*Solving one equation*/78));
   else {
      out_printf((msg(/*Solving %d simultaneous equations*/75), n));
   }

#ifdef NO_COVARIANCES
   dim = 2;
#else
   dim = 0; /* fudge next loop for now */
#endif
   for ( ; dim >= 0; dim--) {
      char buf[256];
      sprintf(buf, msg(/*Solving to find %c coordinates*/76), (char)('x'+dim));
      out_current_action(buf);
      /* Initialise M and B to zero */
      /* FIXME might be best to zero "linearly" */
      for (row = (int)(n - 1); row >= 0; row--) {
	 BELT_COPY(B[row], B_ZERO);
	 for (col = row; col >= 0; col--) MELT_COPY(M(row,col), M_ZERO);
      }

      /* Construct matrix - Go thru' stn list & add all forward legs to M */
      /* (so each leg goes on exactly once) */
      FOR_EACH_STN(stn, stnlist) {
#if DEBUG_MATRIX_BUILD
	 int dirn;

	 print_prefix(stn->name);
	 printf(" used: %d artic %d colour %d\n",
		(!!stn->leg[2]) << 2 | (!!stn -> leg[1]) << 1 | (!!stn->leg[0]),
		stn->fArtic, stn->colour);

	 for (dirn = 0; dirn <= 2; dirn++)
 	    if (USED(stn,dirn)) {
	       printf("Leg %d, vx=%f, reverse=%d, to ", dirn,
		      stn->leg[dirn]->v[0], stn->leg[dirn]->l.reverse);
	       print_prefix(stn->leg[dirn]->l.to->name);
	       putnl();
	    }
	 putnl();
#endif /* DEBUG_MATRIX_BUILD */
	 int f, t;
	 int dirn;
/*         print_prefix(stn->name); putnl(); */
	 if (fixed(stn)) {
	    for (dirn = 2; dirn >= 0; dirn--)
	       if (USED(stn,dirn) && data_here(stn->leg[dirn])) {
#if DEBUG_MATRIX_BUILD
		  print_prefix(stn->name);
		  printf(" - ");
		  print_prefix(stn->leg[dirn]->l.to->name);
		  putnl();
#endif /* DEBUG_MATRIX_BUILD */
		  if (!fixed(stn->leg[dirn]->l.to)) {
		     t = find_stn_in_tab(stn->leg[dirn]->l.to);
#ifdef NO_COVARIANCES
		     e = stn->leg[dirn]->v[dim];
		     if (e != (real)0.0) {
			/* not an equate */
			e = ((real)1.0) / e;
			M(t,t) += e;
			B[t] += e * (POS(stn, dim) + stn->leg[dirn]->d[dim]);
#if DEBUG_MATRIX_BUILD
			printf("--- Dealing with stn fixed at %lf\n",
			       POS(stn, dim));
#endif /* DEBUG_MATRIX_BUILD */
		     }
#else
		     if (invert_var( &e, &stn->leg[dirn]->v )) {
			/* not an equate */
			d b;
			addvv(&M(t,t), &M(t,t), &e);
			adddd(&a, &POSD(stn), &stn->leg[dirn]->d);
			mulvd(&b, &e, &a);
			adddd(&B[t], &B[t], &b);
#if DEBUG_MATRIX_BUILD
			printf("--- Dealing with stn fixed at (%lf, %lf, %lf)\n",
			       POS(stn, 0), POS(stn, 1), POS(stn, 2));
#endif /* DEBUG_MATRIX_BUILD */
		     }
#endif
		  }
	       }
	 } else {
	    f = find_stn_in_tab(stn);
	    for (dirn = 2; dirn >= 0; dirn--)
	       if (USED(stn,dirn) && data_here(stn->leg[dirn])) {
		  if (fixed(stn->leg[dirn]->l.to)) {
		     /* Ignore equated nodes */
#ifdef NO_COVARIANCES
		     e = stn->leg[dirn]->v[dim];
		     if (e != (real)0.0) {
			e = ((real)1.0) / e;
			M(f,f) += e;
			B[f] += e * (POS(stn->leg[dirn]->l.to,dim)
				     - stn->leg[dirn]->d[dim]);
		     }
#else
		     if (invert_var(&e, &stn->leg[dirn]->v)) {
			d b;
			addvv(&M(f, f), &M(f, f), &e);
			subdd(&a, &POSD(stn->leg[dirn]->l.to),
			      &stn->leg[dirn]->d);
			mulvd(&b, &e, &a);
			adddd(&B[f], &B[f], &b);
		     }
#endif
		  } else {
		     t = find_stn_in_tab(stn->leg[dirn]->l.to);
#if DEBUG_MATRIX
		     printf("Leg %d to %d, var %f, delta %f\n", f, t, e,
			    stn->leg[dirn]->d[dim]);
#endif
		     /* Ignore equated nodes & lollipops */
#ifdef NO_COVARIANCES
		     e = stn->leg[dirn]->v[dim];
		     if (t != f && e != (real)0.0) {
			real a;
			e = ((real)1.0) / e;
			M(f,f) += e;
			M(t,t) += e;
			if (f < t) M(t,f) -= e; else M(f,t) -= e;
			a = e * stn->leg[(dirn)]->d[dim];
			B[f] -= a;
			B[t] += a;
		     }
#else
		     if (t != f && invert_var( &e, &stn->leg[dirn]->v )) {
			var *p;
			mulvd(&a, &e, &stn->leg[(dirn)]->d);
			addvv(&M(f,f), &M(f,f), &e);
			addvv(&M(t,t), &M(t,t), &e);
			if (f < t) p = &M(t,f); else p = &M(f,t);
			subvv(p, p, &e);
			subdd(&B[f], &B[f], &a);
			adddd(&B[t], &B[t], &a);
		     }
#endif
		  }
	       }
	 }
      }

#if PRINT_MATRICES
      print_matrix(M, B); /* 'ave a look! */
#endif

#ifdef SOR
      /* defined in network.c, may be altered by -z<letters> on command line */
      if (optimize & BITA('i'))
	 sor(M,B);
      else
#endif
	 choleski(M,B);

      {
	 int m;
	 for (m = (int)(n - 1); m >= 0; m--) {
#ifdef NO_COVARIANCES
	    stn_tab[m]->pos->p[dim] = B[m];
	    if (dim == 0) {
	       ASSERT2(pfx_fixed(stn_tab[m]),
		       "setting station coordinates didn't mark pos as fixed");
	       ASSERT2(fixed(stn_tab[m]->stn),
		       "setting station coordinates didn't mark stn as fixed");
	    }
#else
	    BELT_COPY(stn_tab[m]->pos->p, B[m]);
	    ASSERT2(pfx_fixed(stn_tab[m]),
		    "setting station coordinates didn't mark pos as fixed");
	    ASSERT2(fixed(stn_tab[m]->stn),
		    "setting station coordinates didn't mark stn as fixed");
#endif
	 }
#if EXPLICIT_FIXED_FLAG
/* broken code? !HACK! */
	 for(m = n - 1; m >= 0; m--) fix(stn_tab[m]->stn);
#endif
      }
   }
   osfree(B);
   osfree(M);
}

static int
find_stn_in_tab(node *stn)
{
   int i = 0;
   pos *pos;

   pos = stn->name->pos;
   while (stn_tab[i]->pos != pos)
      if (++i == n_stn_tab) {
#if DEBUG_INVALID
	 fputs("Station ", stderr);
	 fprint_prefix(stderr,stn->name);
	 fputs(" not in table\n\n",stderr);
	 for (i = 0; i < n_stn_tab; i++) {
	    fprintf(stderr,"%3d: ",i);
	    fprint_prefix(stderr,stn_tab[i]);
	    fputnl(stderr);
	 }
#endif
#if 0
	 print_prefix(stn->name);
	 printf(" used: %d artic %d colour %d\n",
		(!!stn->leg[2])<<2 | (!!stn->leg[1])<<1 | (!!stn->leg[0]),
		stn->fArtic, stn->colour );
#endif
	 fatalerror(/*Bug in program detected! Please report this to the authors*/11);
      }
   return i;
}

static int
add_stn_to_tab(node *stn)
{
   int i;
   pos *pos;

   pos = stn->name->pos;
   for (i = 0; i < n_stn_tab; i++) {
      if (stn_tab[i]->pos == pos) return i;
   }
   stn_tab[n_stn_tab++] = stn->name;
   return i;
}

/* Solve MX=B for X by Choleski factorisation */
/* Note M must be symmetric positive definite */
/* routine is entitled to scribble on M and B if it wishes */
static void
choleski(
#ifdef NO_COVARIANCES
	 real FAR *M, real *B
#else
	 var FAR *M, d *B
#endif
	 )
{
   int i, j, k;
   long n = n_stn_tab;
#ifndef NO_PERCENTAGE
   ulong flopsTot, flops = 0, temp = 0;
#define do_percent(N) BLK(flops += (N); out_set_percentage((int)((100.0 * flops) / flopsTot));)
#endif

   /* calc as double so we don't overflow a ulong with intermediate results */
   flopsTot = (ulong)(n * (2.0 * n * n + 9.0 * n - 5.0) / 6.0);
   /* 3*n*(n-1)/2 + n*(n-1)*(n-2)/3 + n*(n-1)/2 + n + n*(n-1)/2; */
   /* n*(9*n-5 + 2*n*n )/6 ; */

#ifdef MATRIX_PRE_INVERT_DIAGONAL
   /* More efficient if we keep an array of all the elts on the
    * diagonal pre-inverted to use later
    * This saves us O(n^2) matrix inversions
    */
   var I[n]; /* FIXME must be dynamically allocated of course... */
   if (!invert_var(&I[0], &M(0,0))) BUG("Can't invert matrix diagonal");
#endif

   for (j = 1; j < n; j++) {
#ifdef NO_COVARIANCES
      real V;
      for (i = 0; i < j; i++) {
         V = (real)0.0;
	 for (k = 0; k < i; k++) V += M(i,k) * M(j,k) * M(k,k);
	 M(j,i) = (M(j,i) - V) / M(i,i);
      }
      V = (real)0.0;
      for (k = 0; k < j; k++) V += M(j,k) * M(j,k) * M(k,k);
      M(j,j) -= V; /* may be best to add M() last for numerical reasons too */
#else
      var V;
      var var_zero = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
      var t1, t2;
      for (i = 0; i < j; i++) {
	 MELT_COPY(V, M_ZERO);
	 for (k = 0; k < i; k++) {
	    mulvv(&t1, &M(i,k), &M(j,k));
	    mulvv(&t2, &t1, &M(k,k));
	    addvv(&V, &V, &t2);
	 }
	 subvv(&V, &M(j,i), &V);
#ifndef MATRIX_PRE_INVERT_DIAGONAL
	 divvv(&M(j,i), &V, &M(i,i));
#else
	 mulvv(&M(j,i), &V, &I[i]);
#endif
      }
      MELT_COPY(V, M_ZERO);
      for (k = 0; k < j; k++) {
         mulvv(&t1, &M(j,k), &M(k,k)); /* NB operand order matters for matrices */
	 mulvv(&t2, &t1, &M(j,k));
	 addvv(&V, &V, &t2);
      }
      subvv(&M(j,j), &M(j,j), &V); /* may be best to add M() last for numerical reasons too */
#ifdef MATRIX_PRE_INVERT_DIAGONAL
      if (!invert_var(&I[j], &M(j,j))) BUG("Can't invert matrix diagonal");
#endif
#endif

#ifndef NO_PERCENTAGE
      if (fPercent) {
	 temp += ((ulong)j + j) + 1ul; /* avoid multiplies */
	 do_percent(temp);
      }
#endif
   }

   /* Multiply x by L inverse */
   for (i = 0; i < n - 1; i++) {
      for (j = i + 1; j < n; j++) {
#ifdef NO_COVARIANCES
	 B[j] -= M(j,i) * B[i];
#else
         d tmp;
         mulvd(&tmp, &M(j,i), &B[i]);
         subdd(&B[j], &B[j], &tmp);
#endif
      }
   }

#ifndef NO_PERCENTAGE
   if (fPercent) {
      temp = (ulong)n * (n - 1ul) / 2ul; /* needed again lower down */
      do_percent(temp);
   }
#endif

   /* Multiply x by D inverse */
   for (i = 0; i < n; i++) {
#ifdef NO_COVARIANCES
      B[i] /= M(i,i);
#else
      d t;
      BELT_COPY(t, B[i]);
#ifndef MATRIX_PRE_INVERT_DIAGONAL
      divdv(&B[i], &t, &M(i,i));
#else
      muldv(&B[i], &t, &I[i]);
#endif
#endif
   }

#ifdef MATRIX_PRE_INVERT_DIAGONAL
   release(I); /*FIXME*/
#endif

#ifndef NO_PERCENTAGE
   if (fPercent) do_percent((ulong)n);
#endif

   /* Multiply x by (L transpose) inverse */
   for (i = (int)(n - 1); i > 0; i--) {
      for (j = i - 1; j >= 0; j--) {
#ifdef NO_COVARIANCES
  	 B[j] -= M(i,j) * B[i];
#else
         d tmp;
         mulvd(&tmp, &M(i,j), &B[i]);
         subdd(&B[j], &B[j], &tmp);
#endif
      }
   }

#ifndef NO_PERCENTAGE
   if (fPercent) do_percent(temp);
# undef do_percent
#endif

   /* printf("\n%ld/%ld\n\n",flops,flopsTot); */
}

#ifdef SOR
/* factor to use for SOR (must have 1 <= SOR_factor < 2) */
#define SOR_factor 1.95 /* 1.95 */

/* Solve MX=B for X by SOR of Gauss-Siedel */
/* routine is entitled to scribble on M and B if it wishes */
static void
sor(real FAR *M, real *B)
{
   real t, x, delta, threshold, t2;
   int row, col;
   real *X;
   long it = 0;

   X = osmalloc(n * ossizeof(real));

   threshold = 0.00001;

   printf("reciprocating diagonal\n"); /*FIXME to msg file */

   /* munge diagonal so we can multiply rather than divide */
   for (row = n - 1; row >= 0; row--) {
      M(row,row) = 1 / M(row,row);
      X[row] = 0.0;
   }

   printf("starting iteration\n"); /*FIXME*/

   do {
      /*printf("*");*/
      it++;
      t = 0.0;
      for (row = 0; row < n; row++) {
	 x = B[row];
	 for (col = 0; col < row; col++) x -= M(row,col) * X[col];
	 for (col++; col < n; col++) x -= M(col,row) * X[col];
	 x *= M(row,row);
	 delta = (x - X[row]) * SOR_factor;
	 X[row] += delta;
	 t2 = fabs(delta);
	 if (t2 > t) t = t2;
      }
   } while (t >= threshold && it < 100000);

   if (t >= threshold) {
      fprintf(stderr, "*not* converged after %ld iterations\n", it);
      BUG("iteration stinks");
   }

   printf("%ld iterations\n", it); /*FIXME*/

#if 0
   putnl();
   for (row = n - 1; row >= 0; row--) {
      t = 0.0;
      for (col = 0; col < row; col++) t += M(row, col) * X[col];
      t += X[row] / M(row, row);
      for (col = row + 1; col < n; col++)
	 t += M(col, row) * X[col];
      printf("[ %lf %lf ]\n", t, B[row]);
   }
#endif

   for (row = n - 1; row >= 0; row--) B[row] = X[row];

   osfree(X);
   printf("\ndone\n"); /*FIXME*/
}
#endif

#if PRINT_MATRICES
static void
print_matrix(real FAR *M, real *B)
{
   int row, col;
   int n = n_stn_tab;
   printf("Matrix, M and vector, B:\n");
   for (row = 0; row < n; row++) {
      for (col = 0; col <= row; col++) printf("%6.2f\t", M(row, col));
      for (; col <= n; col++) printf(" \t");
      printf("\t%6.2f\n", B[row]);
   }
   putnl();
   return;
}
#endif
