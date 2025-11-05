/* matrix.c
 * Matrix building and solving routines
 * Copyright (C) 1993-2003,2010,2013,2024 Olly Betts
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

/*#define SOR 1*/

#if 0
# define DEBUG_INVALID 1
#endif

#include <config.h>

#include "debug.h"
#include "cavern.h"
#include "filename.h"
#include "message.h"
#include "netbits.h"
#include "matrix.h"
#include "osalloc.h"
#include "out.h"

#undef PRINT_MATRICES
#define PRINT_MATRICES 0

#undef DEBUG_MATRIX_BUILD
#define DEBUG_MATRIX_BUILD 0

#undef DEBUG_MATRIX
#define DEBUG_MATRIX 0

#if PRINT_MATRICES
static void print_matrix(real *M, real *B, long n);
#endif

static void choleski(real *M, real *B, long n);

#ifdef SOR
static void sor(real *M, real *B, long n);
#endif

/* for M(row, col) col must be <= row, so Y <= X */
# define M(X, Y) ((real *)M)[((((size_t)(X)) * ((X) + 1)) >> 1) + (Y)]
	      /* +(Y>X?0*printf("row<col (line %d)\n",__LINE__):0) */
/*#define M_(X, Y) ((real *)M)[((((size_t)(Y)) * ((Y) + 1)) >> 1) + (X)]*/

static void set_row(node *stn, int row_number) {
    // We store the matrix row/column index in stn->colour for quick and easy
    // lookup when copying out the solved station coordinates.
    stn->colour = row_number;
    for (int d = 0; d < 3; d++) {
	linkfor *leg = stn->leg[d];
	if (!leg) break;
	node *to = leg->l.to;
	if (to->colour < 0 && stn->name->pos == to->name->pos) {
	    set_row(to, row_number);
	}
    }
}

#ifdef NO_COVARIANCES
# define FACTOR 1
#else
# define FACTOR 3
#endif

/* Find positions for a subset of the reduced network by solving a matrix
 * equation.
 *
 * list is a non-empty linked list of unfixed stations to solve for.
 *
 * As a pre-condition, all stations in list must have a negative value for
 * stn->colour.  This can be ensured by the caller (which avoids having to
 * make an extra pass over the list just to set the colours suitably).
 */
extern void
solve_matrix(node *list)
{
   // Assign a matrix row/column index to each group of stations with the same
   // pos.
   //
   // We also set listend to the last station in the list while doing so, which
   // we use after solving to splice list into fixedlist.
   node *listend = NULL;
   size_t n = 0;
   for (node *stn = list; stn; stn = stn->next) {
      listend = stn;
      if (stn->colour < 0) {
	  set_row(stn, n++);
      }
   }
   SVX_ASSERT(n > 0);

   // Array to map from row/column index to pos.  We fill this in as we build
   // the matrix, and use it to know where to copy the solved station
   // coordinates to.
   pos **stn_tab = osmalloc(n * sizeof(pos*));

   real *M = osmalloc((((n * FACTOR * (n * FACTOR + 1)) >> 1)) * sizeof(real));
   real *B = osmalloc(n * FACTOR * sizeof(real));

   if (n == 1)
      out_current_action(msg(/*Solving one equation*/78));
   else
      out_current_action1(msg(/*Solving %d simultaneous equations*/75), (int)n);

#ifdef NO_COVARIANCES
   int dim = 2;
#else
   int dim = 0; /* Collapse loop to a single iteration. */
#endif
   for ( ; dim >= 0; dim--) {
      /* Initialise M and B to zero - zeroing "linearly" will minimise
       * paging when the matrix is large */
      {
	 int end = n * FACTOR;
	 for (int row = 0; row < end; row++) B[row] = (real)0.0;
	 end = ((size_t)n * FACTOR * (n * FACTOR + 1)) >> 1;
	 for (int row = 0; row < end; row++) M[row] = (real)0.0;
      }

      /* Construct matrix by going through the stn list.
       *
       * All legs between two fixed stations can be ignored here.
       *
       * Other legs we want to add exactly once to M.  To achieve this we
       * want to:
       *
       * - add forward legs between two unfixed stations,
       *
       * - add legs from unfixed stations to fixed stations (we do them from
       *   the unfixed end so we don't need to detect when we're at a fixed
       *   point cut line and determine which side we're currently dealing
       *   with).
       *
       * To implement this, we only look at legs from unfixed stations and add
       * a leg if to a fixed station, or to an unfixed station and it's a
       * forward leg.
       */
      for (node *stn = list; stn; stn = stn->next) {
	 if (dim == 0) {
	     stn_tab[stn->colour] = stn->name->pos;
	 }

#ifdef NO_COVARIANCES
	 real e;
#else
	 svar e;
	 delta a;
#endif
#if DEBUG_MATRIX_BUILD
	 print_prefix(stn->name);
	 printf(" used: %d colour %ld\n",
		(!!stn->leg[2]) << 2 | (!!stn -> leg[1]) << 1 | (!!stn->leg[0]),
		stn->colour);

	 for (int dirn = 0; dirn <= 2 && stn->leg[dirn]; dirn++) {
	    printf("Leg %d, vx=%f, reverse=%d, to ", dirn,
		   stn->leg[dirn]->v[0], stn->leg[dirn]->l.reverse);
	    print_prefix(stn->leg[dirn]->l.to->name);
	    putnl();
	 }
	 putnl();
#endif /* DEBUG_MATRIX_BUILD */

	 int f = stn->colour;
	 SVX_ASSERT(f >= 0);
	 {
	    for (int dirn = 0; dirn <= 2 && stn->leg[dirn]; dirn++) {
	       linkfor *leg = stn->leg[dirn];
	       node *to = leg->l.to;
	       if (fixed(to)) {
		  bool fRev = !data_here(leg);
		  if (fRev) leg = reverse_leg(leg);
		  /* Ignore equated nodes */
#ifdef NO_COVARIANCES
		  e = leg->v[dim];
		  if (e != (real)0.0) {
		     e = ((real)1.0) / e;
		     M(f,f) += e;
		     B[f] += e * POS(to, dim);
		     if (fRev) {
			B[f] += leg->d[dim];
		     } else {
			B[f] -= leg->d[dim];
		     }
		  }
#else
		  if (invert_svar(&e, &leg->v)) {
		     if (fRev) {
			adddd(&a, &POSD(to), &leg->d);
		     } else {
			subdd(&a, &POSD(to), &leg->d);
		     }
		     delta b;
		     mulsd(&b, &e, &a);
		     for (int i = 0; i < 3; i++) {
			M(f * FACTOR + i, f * FACTOR + i) += e[i];
			B[f * FACTOR + i] += b[i];
		     }
		     M(f * FACTOR + 1, f * FACTOR) += e[3];
		     M(f * FACTOR + 2, f * FACTOR) += e[4];
		     M(f * FACTOR + 2, f * FACTOR + 1) += e[5];
		  }
#endif
	       } else if (data_here(leg) &&
			  (leg->l.reverse & FLAG_ARTICULATION) == 0) {
		  /* forward leg, unfixed -> unfixed */
		  int t = to->colour;
		  SVX_ASSERT(t >= 0);
#if DEBUG_MATRIX
# ifdef NO_COVARIANCES
		  printf("Leg %d to %d, var %f, delta %f\n", f, t, e,
			 leg->d[dim]);
# else
		  printf("Leg %d to %d, var (%f, %f, %f; %f, %f, %f), "
			 "delta %f\n", f, t, e[0], e[1], e[2], e[3], e[4], e[5],
			 leg->d[dim]);
# endif
#endif
		  /* Ignore equated nodes & lollipops */
#ifdef NO_COVARIANCES
		  e = leg->v[dim];
		  if (t != f && e != (real)0.0) {
		     e = ((real)1.0) / e;
		     M(f,f) += e;
		     M(t,t) += e;
		     if (f < t) M(t,f) -= e; else M(f,t) -= e;
		     real a = e * leg->d[dim];
		     B[f] -= a;
		     B[t] += a;
		  }
#else
		  if (t != f && invert_svar(&e, &leg->v)) {
		     mulsd(&a, &e, &leg->d);
		     for (int i = 0; i < 3; i++) {
			M(f * FACTOR + i, f * FACTOR + i) += e[i];
			M(t * FACTOR + i, t * FACTOR + i) += e[i];
			if (f < t)
			   M(t * FACTOR + i, f * FACTOR + i) -= e[i];
			else
			   M(f * FACTOR + i, t * FACTOR + i) -= e[i];
			B[f * FACTOR + i] -= a[i];
			B[t * FACTOR + i] += a[i];
		     }
		     M(f * FACTOR + 1, f * FACTOR) += e[3];
		     M(t * FACTOR + 1, t * FACTOR) += e[3];
		     M(f * FACTOR + 2, f * FACTOR) += e[4];
		     M(t * FACTOR + 2, t * FACTOR) += e[4];
		     M(f * FACTOR + 2, f * FACTOR + 1) += e[5];
		     M(t * FACTOR + 2, t * FACTOR + 1) += e[5];
		     if (f < t) {
			M(t * FACTOR + 1, f * FACTOR) -= e[3];
			M(t * FACTOR, f * FACTOR + 1) -= e[3];
			M(t * FACTOR + 2, f * FACTOR) -= e[4];
			M(t * FACTOR, f * FACTOR + 2) -= e[4];
			M(t * FACTOR + 2, f * FACTOR + 1) -= e[5];
			M(t * FACTOR + 1, f * FACTOR + 2) -= e[5];
		     } else {
			M(f * FACTOR + 1, t * FACTOR) -= e[3];
			M(f * FACTOR, t * FACTOR + 1) -= e[3];
			M(f * FACTOR + 2, t * FACTOR) -= e[4];
			M(f * FACTOR, t * FACTOR + 2) -= e[4];
			M(f * FACTOR + 2, t * FACTOR + 1) -= e[5];
			M(f * FACTOR + 1, t * FACTOR + 2) -= e[5];
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
	 for (int m = (int)(n - 1); m >= 0; m--) {
#ifdef NO_COVARIANCES
	    stn_tab[m]->p[dim] = B[m];
	    if (dim == 0) {
	       SVX_ASSERT2(pos_fixed(stn_tab[m]),
		       "setting station coordinates didn't mark pos as fixed");
	    }
#else
	    for (int i = 0; i < 3; i++) {
	       stn_tab[m]->p[i] = B[m * FACTOR + i];
	    }
	    SVX_ASSERT2(pos_fixed(stn_tab[m]),
		    "setting station coordinates didn't mark pos as fixed");
#endif
	 }
      }
   }

   // Put the solved stations back on fixedlist.
   listend->next = fixedlist;
   if (fixedlist) fixedlist->prev = listend;
   fixedlist = list;

   free(B);
   free(M);
   free(stn_tab);

#if DEBUG_MATRIX
   for (node *stn = list; stn; stn = stn->next) {
      printf("(%8.2f, %8.2f, %8.2f ) ", POS(stn, 0), POS(stn, 1), POS(stn, 2));
      print_prefix(stn->name);
      putnl();
   }
#endif
}

/* Solve MX=B for X by first factoring M into LDL'.  This is a modified form
 * of Choleski factorisation - the original Choleski factorisation is LL',
 * but this modified version has the advantage of avoiding O(n) square root
 * calculations.
 */
/* Note M must be symmetric positive definite */
/* routine is entitled to scribble on M and B if it wishes */
static void
choleski(real *M, real *B, long n)
{
   for (int j = 1; j < n; j++) {
      real V;
      for (int i = 0; i < j; i++) {
	 V = (real)0.0;
	 for (int k = 0; k < i; k++) V += M(i,k) * M(j,k) * M(k,k);
	 M(j,i) = (M(j,i) - V) / M(i,i);
      }
      V = (real)0.0;
      for (int k = 0; k < j; k++) V += M(j,k) * M(j,k) * M(k,k);
      M(j,j) -= V; /* may be best to add M() last for numerical reasons too */
   }

   /* Multiply x by L inverse */
   for (int i = 0; i < n - 1; i++) {
      for (int j = i + 1; j < n; j++) {
	 B[j] -= M(j,i) * B[i];
      }
   }

   /* Multiply x by D inverse */
   for (int i = 0; i < n; i++) {
      B[i] /= M(i,i);
   }

   /* Multiply x by (L transpose) inverse */
   for (int i = (int)(n - 1); i > 0; i--) {
      for (int j = i - 1; j >= 0; j--) {
	 B[j] -= M(i,j) * B[i];
      }
   }

   /* printf("\n%ld/%ld\n\n",flops,flopsTot); */
}

#ifdef SOR
/* factor to use for SOR (must have 1 <= SOR_factor < 2) */
#define SOR_factor 1.93 /* 1.95 */

/* Solve MX=B for X by SOR of Gauss-Siedel */
/* routine is entitled to scribble on M and B if it wishes */
static void
sor(real *M, real *B, long n)
{
   long it = 0;

   real *X = osmalloc(n * sizeof(real));

   const real threshold = 0.00001;

   printf("reciprocating diagonal\n"); /* TRANSLATE */

   /* munge diagonal so we can multiply rather than divide */
   for (int row = n - 1; row >= 0; row--) {
      M(row,row) = 1 / M(row,row);
      X[row] = 0;
   }

   printf("starting iteration\n"); /* TRANSLATE */

   real t;
   do {
      /*printf("*");*/
      it++;
      t = 0.0;
      for (int row = 0; row < n; row++) {
	 real x = B[row];
	 int col;
	 for (col = 0; col < row; col++) x -= M(row,col) * X[col];
	 for (col++; col < n; col++) x -= M(col,row) * X[col];
	 x *= M(row,row);
	 real sor_delta = (x - X[row]) * SOR_factor;
	 X[row] += sor_delta;
	 real t2 = fabs(sor_delta);
	 if (t2 > t) t = t2;
      }
      printf("% 6ld: %8.6f\n", it, t);
   } while (t >= threshold && it < 100000);

   if (t >= threshold) {
      fprintf(stderr, "*not* converged after %ld iterations\n", it);
      BUG("iteration stinks");
   }

   printf("%ld iterations\n", it); /* TRANSLATE */

#if 0
   putnl();
   for (int row = n - 1; row >= 0; row--) {
      t = 0.0;
      for (int col = 0; col < row; col++) t += M(row, col) * X[col];
      t += X[row] / M(row, row);
      for (col = row + 1; col < n; col++)
	 t += M(col, row) * X[col];
      printf("[ %f %f ]\n", t, B[row]);
   }
#endif

   for (int row = n - 1; row >= 0; row--) B[row] = X[row];

   free(X);
   printf("\ndone\n"); /* TRANSLATE */
}
#endif

#if PRINT_MATRICES
static void
print_matrix(real *M, real *B, long n)
{
   printf("Matrix, M and vector, B:\n");
   for (long row = 0; row < n; row++) {
      long col;
      for (col = 0; col <= row; col++) printf("%6.2f\t", M(row, col));
      for (; col <= n; col++) printf(" \t");
      printf("\t%6.2f\n", B[row]);
   }
   putnl();
   return;
}
#endif
