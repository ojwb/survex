/* > matrix.c
 * Matrix building and solving routines
 * Copyright (C) 1993-1999 Olly Betts
 */

/*#define SOR 1*/

#if 0
# define DEBUG_INVALID 1
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
static void print_matrix(real FAR *M, real *B, long n);
#endif

static void choleski(real FAR *M, real *B, long n);

#ifdef SOR
static void sor(real FAR *M, real *B, long n);
#endif

/* for M(row, col) col must be <= row, so Y <= X */
# define M(X, Y) ((real Huge *)M)[((((OSSIZE_T)(X)) * ((X) + 1)) >> 1) + (Y)]
              /* +(Y>X?0*printf("row<col (line %d)\n",__LINE__):0) */
/*#define M_(X, Y) ((real Huge *)M)[((((OSSIZE_T)(Y)) * ((Y) + 1)) >> 1) + (X)]*/

static int find_stn_in_tab(node *stn);
static int add_stn_to_tab(node *stn);
static void build_matrix(node *list, long n, pos **stn_tab);

static long n_stn_tab;

static pos **stn_tab;

extern void
solve_matrix(node *list)
{
   node *stn;
   long n = 0;
   FOR_EACH_STN(stn, list) {
      if (!fixed(stn)) n++;
   }
   if (n == 0) return;

   /* we just need n to be a reasonable estimate >= the number
    * of stations left after reduction. If memory is
    * plentiful, we can be crass.
    */
   stn_tab = osmalloc((OSSIZE_T)(n*ossizeof(pos*)));
   
   FOR_EACH_STN(stn, list) {
      if (!fixed(stn)) add_stn_to_tab(stn);
   }

   /* FIXME: release any unused entries in stn_tab ? */
   /* stn_tab = osrealloc(stn_tab, n_stn_tab * ossizeof(pos*)); */
   
   build_matrix(list, n_stn_tab, stn_tab);
   FOR_EACH_STN(stn, list) {
      printf("(%8.2f, %8.2f, %8.2f ) ", POS(stn, 0), POS(stn, 1), POS(stn, 2));
      print_prefix(stn->name);
      putnl();
   }
}

#ifdef NO_COVARIANCES
# define FACTOR 1
#else
# define FACTOR 3
#endif

static void
build_matrix(node *list, long n, pos **stn_tab)
{
   real FAR *M;
   real *B;
   int dim;

   if (n == 0) {
      out_puts(msg(/*Network solved by reduction - no simultaneous equations to solve.*/74));
      return;
   }
   /* (OSSIZE_T) cast may be needed if n>=181 */
   M = osmalloc((OSSIZE_T)((((OSSIZE_T)n * FACTOR * (n * FACTOR + 1)) >> 1)) * ossizeof(real));
   B = osmalloc((OSSIZE_T)(n * FACTOR * ossizeof(real)));

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
      node *stn;
      int row;

      sprintf(buf, msg(/*Solving to find %c coordinates*/76), (char)('x'+dim));
      out_current_action(buf);
      /* Initialise M and B to zero */
      /* FIXME: might be best to zero "linearly" */
      for (row = (int)(n * FACTOR - 1); row >= 0; row--) {
	 int col;
	 B[row] = (real)0.0;
	 for (col = row; col >= 0; col--) M(row,col) = (real)0.0;
      }

      /* Construct matrix - Go thru' stn list & add all forward legs to M */
      /* (so each leg goes on exactly once) */
      FOR_EACH_STN(stn, list) {
#ifdef NO_COVARIANCES
	 real e;
#else
	 var e;
	 d a;
#endif
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
		     if (invert_var(&e, &stn->leg[dirn]->v)) {
			/* not an equate */
			d b;
			int i, j;
			adddd(&a, &POSD(stn), &stn->leg[dirn]->d);
			mulvd(&b, &e, &a);
			for (i = 0; i < 3; i++) {
			   for (j = 0; j <= i; j++) {
			      M(t * FACTOR + i, t * FACTOR + j) += e[i][j];
			   }
			   B[t * FACTOR + i] += b[i];
			}
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
			int i, j;
			subdd(&a, &POSD(stn->leg[dirn]->l.to), &stn->leg[dirn]->d);
			mulvd(&b, &e, &a);
			for (i = 0; i < 3; i++) {
			   for (j = 0; j <= i; j++) {
			      M(f * FACTOR + i, f * FACTOR + j) += e[i][j];
			   }
			   B[f * FACTOR + i] += b[i];
			}
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
			int i, j;
			mulvd(&a, &e, &stn->leg[(dirn)]->d);
			for (i = 0; i < 3; i++) {
			   for (j = 0; j <= i; j++) {
			      M(f * FACTOR + i, f * FACTOR + j) += e[i][j];
			      M(t * FACTOR + i, t * FACTOR + j) += e[i][j];
			   }
			   for (j = 0; j < 3; j++) {
			      if (f < t) 
				 M(t * FACTOR + i, f * FACTOR + j) -= e[i][j];
			      else 
				 M(f * FACTOR + i, t * FACTOR + j) -= e[i][j];
			   }
			   B[f * FACTOR + i] -= a[i];
			   B[t * FACTOR + i] += a[i];
			}
		     }
#endif
		  }
	       }
	 }
      }

#if PRINT_MATRICES
      print_matrix(M, B, n * FACTOR); /* 'ave a look! */
#endif

#ifdef SOR
      /* defined in network.c, may be altered by -z<letters> on command line */
      if (optimize & BITA('i'))
	 sor(M, B, n * FACTOR);
      else
#endif
	 choleski(M, B, n * FACTOR);

      {
	 int m;
	 for (m = (int)(n - 1); m >= 0; m--) {
#ifdef NO_COVARIANCES
	    stn_tab[m]->p[dim] = B[m];
	    if (dim == 0) {
	       ASSERT2(pos_fixed(stn_tab[m]),
		       "setting station coordinates didn't mark pos as fixed");
	    }
#else
	    int i;
	    for (i = 0; i < 3; i++) {
	       stn_tab[m]->p[i] = B[m * FACTOR + i];
	    }
	    ASSERT2(pos_fixed(stn_tab[m]),
		    "setting station coordinates didn't mark pos as fixed");
#endif
	 }
#if EXPLICIT_FIXED_FLAG
	 for (m = n - 1; m >= 0; m--) fixpos(stn_tab[m]);
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
   while (stn_tab[i] != pos)
      if (++i == n_stn_tab) {
#if DEBUG_INVALID
	 fputs("Station ", stderr);
	 fprint_prefix(stderr,stn->name);
	 fputs(" not in table\n\n",stderr);
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
      if (stn_tab[i] == pos) return i;
   }
   stn_tab[n_stn_tab++] = pos;
   return i;
}

/* Solve MX=B for X by Choleski factorisation - modified Choleski actually
 * since we factor into LDL' while Choleski is just LL'
 */
/* Note M must be symmetric positive definite */
/* routine is entitled to scribble on M and B if it wishes */
static void
choleski(real FAR *M, real *B, long n)
{
   int i, j, k;
#ifndef NO_PERCENTAGE
   ulong flopsTot, flops = 0, temp = 0;
#define do_percent(N) BLK(flops += (N); out_set_percentage((int)((100.0 * flops) / flopsTot));)
#endif

   /* calc as double so we don't overflow a ulong with intermediate results */
   flopsTot = (ulong)(n * (2.0 * n * n + 9.0 * n - 5.0) / 6.0);
   /* 3*n*(n-1)/2 + n*(n-1)*(n-2)/3 + n*(n-1)/2 + n + n*(n-1)/2; */
   /* n*(9*n-5 + 2*n*n )/6 ; */

   for (j = 1; j < n; j++) {
      real V;
      for (i = 0; i < j; i++) {
         V = (real)0.0;
	 for (k = 0; k < i; k++) V += M(i,k) * M(j,k) * M(k,k);
	 M(j,i) = (M(j,i) - V) / M(i,i);
      }
      V = (real)0.0;
      for (k = 0; k < j; k++) V += M(j,k) * M(j,k) * M(k,k);
      M(j,j) -= V; /* may be best to add M() last for numerical reasons too */

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
	 B[j] -= M(j,i) * B[i];
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
      B[i] /= M(i,i);
   }

#ifndef NO_PERCENTAGE
   if (fPercent) do_percent((ulong)n);
#endif

   /* Multiply x by (L transpose) inverse */
   for (i = (int)(n - 1); i > 0; i--) {
      for (j = i - 1; j >= 0; j--) {
  	 B[j] -= M(i,j) * B[i];
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
#define SOR_factor 1.93 /* 1.95 */

/* Solve MX=B for X by SOR of Gauss-Siedel */
/* routine is entitled to scribble on M and B if it wishes */
static void
sor(real FAR *M, real *B, long n)
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
      X[row] = 0;
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
      printf("% 6d: %8.6f\n", it, t);
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
print_matrix(real FAR *M, real *B, long n)
{
   long row, col;
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
