/* > matrix.c
 * Matrix building and solving routines
 * Copyright (C) 1993-1996 Olly Betts
 */

/*
1993.01.25 PC Compatibility OKed
1993.01.27 Turned off PRINT_STN_TAB
1993.01.28 Slightly altered wording of messages
1993.02.19 print_prefix(..) -> fprint_prefix(stdout,..)
           altered leg->to & leg->reverse to leg->l.to & leg->l.reverse
           debugging code now uses #if rather than #ifdef
           indenting fettled
1993.02.23 removed duplicate prototype for find_stn_in_tab()
1993.03.10 added experimental 'equated but not fixed' code
1993.03.11 node->in_network -> node->status
1993.04.08 removed crap code
           error.h
1993.04.22 out of memory -> error # 0
1993.05.03 removed #define print_prefix ... to survex.h
1993.06.04 added commented-out code to look at matrix use
           added code to invent fixed points
           FALSE -> fFalse
1993.06.16 malloc() -> osmalloc(); free() -> osfree(); syserr() -> fatal()
1993.08.06 fettled and commented out debugging code added
1993.08.08 fettled
1993.08.09 altered data structure (OLD_NODES compiles previous version)
1993.08.10 debugging flags moved to debug.h
           solve matrix for z,y then x so fixed point stuff is okay
1993.08.11 NEW_NODES & POS(stn,d)
           FOR_EACH_STN( stn ) does for loop with stn looping thru' all nodes
           fixed bug in listing of diagonal elements of matrix
           improved putting fixed points in matrix for NEW_NODES case
1993.08.12 added DEBUG_MATRIX for even more debug code
           added USED(station,leg) macro
1993.08.15 added starts of code for almost halving MATRIX size (WHOLE_MATRIX)
1993.08.19 now deal with fixed points as matrix is built (faster, smaller
            matrix, and matrix is symmetric positive definite)
           removed spurious close comment in debug code
           fixed a bug in new code
1993.08.20 turned off new code as it still doesn't work
1993.08.21 fixed bug (dirn was unsigned char and so >= 0 always true)
           fixed bug with adding leg to fixed point
           fixed bug (dirn instead of d to look up coords of fixed points)
1993.08.22 turned off new code as it still doesn't work
1993.09.21 (W)#ifed a bit of stray debugging with DEBUG_COPYING_ENTRIES
1993.09.23 (IH)M uses huge ptr arithmetic
1993.10.23 fixed but in NEW_MX code and turned it on
1993.10.26 eliminated naughty reference to OS==MSDOS
1993.10.27 real *M -> real FAR *M in print_matrix
           removed (long) cast from osmalloc() for M as size_t should sort it
1993.10.28 added NEW_GE (NB code doesn't (yet) give right answers)
1993.11.02 added CHOLESKI which solves using Choleski factorisation (works)
1993.11.03 removed NEW_GE - GE at best 4 flops less calculation than CHOLESKI
           removed non-NEW_MX as NEW_MX works fine and is faster & smaller
           removed non-CHOLESKI code and non-WHOLE_MATRIX
           added DEBUG_MATRIX_BUILD to control some #if0-ed code
           removed DEBUG_COPYING_ENTRIES and code controlled
1993.11.03 changed error routines
1993.11.07 extracted messages
1993.11.08 added FATAL5ARGS() macro
           only NEW_NODES code left
1993.11.10 removed "matrix.c" reference
1993.11.11 now say "Solving one equation" if appropriate
1993.11.14 HUGE -> HUGE_MODEL because Sun math.h defines HUGE <sigh>
1993.11.29 sizeof -> ossizeof, and added OSSIZE_T cast to M osmalloc calc
           error 5 -> 11; FATAL5ARGS -> FATALARGS
           message 72 + '\n' + message 73 -> message 73
1993.11.30 made M definition one line because of DOS
           added OSSIZE_T cast to M definition
           sorted out error vs fatal
1993.12.09 HUGE_MODEL -> Huge
1993.12.17 added code to deal with EXPLICIT_FIXED_FLAG
1994.01.05 removed blank line from 'not all attached to fixed pts' message
1994.03.13 added code to deal with case of no unfixed points
1994.03.18 fixed invented-fix-of-equated-station bug
           added some more debugging code
1994.03.19 moved # to first column
           error output all to stderr
1994.04.10 introduced BUG() macro
1994.04.17 debugging; added SOR code
1994.04.18 fettling survex.h
1994.04.23 fettled sor()
1994.05.11 added #ifdef SOR
1994.06.20 added int argument to warning, error and fatal
1994.08.16 fixed osfree of non-osmalloc-ed stn_tab
1994.08.25 added %age to choleski()
1994.08.29 reworked code to use out module
1994.08.31 fputnl() ; turned on PRINT_MATRICES
1994.09.03 turned off PRINT_MATRICES; fettled
1994.09.13 if (E) BUG(M); -> ASSERT(!E,M)
1994.09.20 global buffer szOut created
1994.11.01 "estimate" of number of stations after reduction is now exact
1994.11.06 commented out fixed point inventing code
1994.11.06 merged in artic.h
>>1994.10.06 created
>>1994.10.27 added component code
>>1994.10.28 fettled debugging code out unless DEBUG_ARTIC def'd
>>1994.11.06 more major fettling
1994.11.19 killed invented fix code
           hacked into shape some more
1994.11.22 removed LOTS and replaced with liv linked stack
1994.11.25 fixed a bug and turned off debug code
1994.11.26 corrected component count
1994.12.03 turned off remaining debug info
1995.01.15 fixed bug in articulation stuff
1995.03.31 minor tweaks
1995.04.15 ASSERT -> ASSERT2
1996.02.20 solve_matrix() should take void parameter, not empty list
1996.04.03 fixed a warning
*/

#include "debug.h"
#include "survex.h"
#include "error.h"
#include "netbits.h"
#include "matrix.h"
#include "out.h"

/* #define DEBUG_ARTIC */

#undef PRINT_MATRICES
#define PRINT_MATRICES 0

#undef DEBUG_MATRIX_BUILD
#define DEBUG_MATRIX_BUILD 0

#undef DEBUG_MATRIX
#define DEBUG_MATRIX 0

#if PRINT_MATRICES
static void print_matrix( real FAR *M, real *B );
#endif

static void choleski( real FAR *M, real *B );
#ifdef SOR
static void sor( real FAR *M, real *B );
#endif

/* for M(row,col) col must be <= row, so Y<=X */
# define M(X,Y) ((real Huge *)M)[((((OSSIZE_T)(X))*((X)+1))>>1)+(Y) ]
              /* +(Y>X?0*printf("row<col (line %d)\n",__LINE__):0) */
/*#define M_(X,Y) ((real Huge *)M)[((((OSSIZE_T)(Y))*((Y)+1))>>1)+(X) ]*/

static int    find_stn_in_tab( node *stn );
static int add_stn_to_tab( node *stn );
static void build_matrix(long n, prefix **stn_tab);

static long n_stn_tab;

static prefix **stn_tab; /* pos ** instead? */
static long n;

static ulong colour;

/* list item visit */
typedef struct LIV { struct LIV *next; ulong min; uchar dirn; } liv;

/* The goto iter/uniter avoids using recursion which could lead to stack
 * overflow.  Instead we use a linked list or array which will probably use
 * much less memory on most compilers.  Particularly true in visit_count().
 */

static ulong visit( node *stn ) {
  ulong min;
  int i;
  liv *livTos=NULL, *livTmp;
iter:
  colour++;
  stn->colour=colour;
  min=colour;
  for( i=0 ; i<=2 ; i++ ) {
    if (stn->leg[i]) {
      if (stn->leg[i]->l.to->colour==0) {
        livTmp=osnew(liv);
        livTmp->next=livTos;
        livTos=livTmp;
        livTos->min=min;
        livTos->dirn=reverse_leg(stn->leg[i]);
        stn=stn->leg[i]->l.to;
        goto iter;
uniter:
        i=reverse_leg(stn->leg[livTos->dirn]);
        stn=stn->leg[livTos->dirn]->l.to;
        if (!(min<livTos->min)) {
          if (min>=stn->colour)
            stn->fArtic=fTrue;
          min=livTos->min;
        }
        livTmp=livTos;
        livTos=livTos->next;
        osfree(livTmp);
      } else {
        if ( stn->leg[i]->l.to->colour < min )
          min=stn->leg[i]->l.to->colour;
      }
    }
  }
  if (livTos)
    goto uniter;
  return min;
}

static void visit_count( node *stn, ulong max ) {
  int i;
  int sp=0;
  node *stn2;
  uchar *j_s;
  stn->status=statFixed;
  if (fixed(stn))
    return;
  add_stn_to_tab(stn);
  if (stn->fArtic)
    return;
  j_s=(uchar*)osmalloc((OSSIZE_T)max);
iter:
  for( i=0 ; i<=2 ; i++ ) {
    if (stn->leg[i]) {
      stn2=stn->leg[i]->l.to;
      if (fixed(stn2)) {
        stn2->status=statFixed;
      } else if (stn2->status!=statFixed) {
        add_stn_to_tab(stn2);
        stn2->status=statFixed;
        if (!stn2->fArtic) {
          ASSERT2(sp<max,"dirn stack too small in visit_count");
          j_s[sp]=reverse_leg(stn->leg[i]);
          sp++;
          stn=stn2;
          goto iter;
uniter:
          sp--;
          i=reverse_leg(stn->leg[j_s[sp]]);
          stn=stn->leg[j_s[sp]]->l.to;
        }
      }
    }
  }
  if (sp>0)
    goto uniter;
  osfree(j_s);
  return;
}

extern void solve_matrix(void) {
  node *stn, *stnStart;
  int c,i;
#ifdef DEBUG_ARTIC
  ulong cFixed;
#endif
  stn_tab=NULL; /* so we can just osfree it */
  /* find articulation points and components */
  cComponents=0;
  colour=1;
  stnStart=NULL;
  FOR_EACH_STN(stn) {
    stn->fArtic=fFalse;
    if (fixed(stn)) {
      stnStart=stn;
      stn->colour=colour++;
    } else {
      stn->colour=0;
    }
  }
  ASSERT2(stnStart,"no fixed points!");
#ifdef DEBUG_ARTIC
  cFixed=colour-1;
#endif
   for(;;) {
    int cColouredNeighbours=0,cNeighbours=0;
    stn=stnStart;
    /* see if this is a fresh component */
    if (stn->status!=statFixed)
      cComponents++;
    for( i=0 ; i<=2 ; i++ ) {
      if (stn->leg[i]) {
        cNeighbours++;
        if (stn->leg[i]->l.to->colour) {
          cColouredNeighbours++;
          stn->leg[i]->l.to->status=statFixed;
        }
      }
    }
#ifdef DEBUG_ARTIC
    print_prefix(stn->name);
    printf(" [%p] is root of component %ld\n",stn,cComponents);
    dump_node(stn);
    printf(" and colour = %d/%d\n",stn->colour,cFixed);
#endif
#ifdef DEBUG_ARTIC
    if (cNeighbours==0) {
      printf("0-node\n");
    }
#endif
    if (cColouredNeighbours!=cNeighbours) {
/*    stn->colour=colour; */
      c=0;
      for( i=0 ; i<=2 ; i++ ) {
        if (stn->leg[i]) {
          if (stn->leg[i]->l.to->colour==0) {
            ulong colBefore=colour;
            c++;
            visit(stn->leg[i]->l.to);
            n=colour-colBefore;
#ifdef DEBUG_ARTIC
            printf("visited %lu nodes\n",n);
#endif
            if (n==0)
              continue;
            if (0) {
#if 0
            if (n==1) {
              /* special case for n==1 for speed !HACK! */
              osfree(stn_tab);
 /* we just need n to be a reasonable estimate >= the number of stations
  * left after reduction. If memory is plentiful, we can be crass.
  */
              stn_tab=osmalloc(n*ossizeof(prefix*));
              /* Solve chunk of net from stn in dirn i up to stations
               * with fArtic set or fixed() true.
               * Then solve stations up to next set of fArtic points,
               * and repeat until all this bit done.
               */
              {
                node *stn2;
                stn->status=statFixed;
                stn2=stn->leg[i]->l.to;
more0:
                n_stn_tab=0;
                visit_count(stn2,n);

                printf("visit_count returned okay n_stn_tab=%d\n",n_stn_tab);

                build_matrix(n_stn_tab,stn_tab);
                FOR_EACH_STN(stn2) {
                  if (stn2->fArtic && fixed(stn2)) {
                    int d;
                    for ( d=0 ; d<=2 ; d++ ) {
                      if (USED(stn2,d)) {
                        node *stn3=stn2->leg[d]->l.to;
                        if (!fixed(stn3)) {
                          stn2=stn3;
                          printf("more !!! ");
                          print_prefix(stn3->name);
                          putnl();
                          goto more0;
                        }
                      }
                    }
                    printf("unartic -- ");
                    print_prefix(stn2->name);
                    putnl();
                    stn2->fArtic=fFalse;
                  }
                }
              }
#endif
            } else {
              osfree(stn_tab);
 /* we just need n to be a reasonable estimate >= the number of stations
  * left after reduction. If memory is plentiful, we can be crass.
  */
              stn_tab=osmalloc((OSSIZE_T)(n*ossizeof(prefix*)));
              /* Solve chunk of net from stn in dirn i up to stations
               * with fArtic set or fixed() true.
               * Then solve stations up to next set of fArtic points,
               * and repeat until all this bit done.
               */
              {
                node *stn2;
                stn->status=statFixed;
                stn2=stn->leg[i]->l.to;
more:
                n_stn_tab=0;
                visit_count(stn2,n);
#ifdef DEBUG_ARTIC
                printf("visit_count returned okay\n");
#endif
                build_matrix(n_stn_tab,stn_tab);
                FOR_EACH_STN(stn2) {
                  if (stn2->fArtic && fixed(stn2)) {
                    int d;
                    for ( d=0 ; d<=2 ; d++ ) {
                      if (USED(stn2,d)) {
                        node *stn3=stn2->leg[d]->l.to;
                        if (!fixed(stn3)) {
                          stn2=stn3;
                          goto more;
                        }
                      }
                    }
                    stn2->fArtic=fFalse;
                  }
                }
              }
            }
          }
        }
      }
      /* Special case to check if start station is an articulation point
       * which it is iff we have to colour from it in more than one dirn
       */
      if (c>1)
        stn->fArtic=fTrue;
#if 0
      FOR_EACH_STN(stn) {
        printf("%ld - ",stn->colour);
        print_prefix(stn->name);
        putnl();
      }
#endif
    }
    if (stnStart->colour==1) {
#ifdef DEBUG_ARTIC
      printf("%ld components\n",cComponents);
#endif
      break;
    }
    for( stn=stnlist ; stn->colour!=stnStart->colour-1 ; stn=stn->next ) {
      ASSERT2(stn,"ran out of coloured fixed points");
    }
    stnStart=stn;
  }
  osfree(stn_tab);
#if 0
  FOR_EACH_STN(stn) { /* high-light unfixed bits */
    if (stn->status && !fixed(stn)) {
      printf("UNFIXED (status %d) [%p]\n",stn->status,stn);
      print_prefix(stn->name);
    }
  }
#endif
}

static void build_matrix(long n, prefix **stn_tab) {
  real FAR *M;
  real *B;
  node *stn;
  int  row,col;
  int  d;
  real e,a;
#ifdef DEBUG_ARTIC
  printf("# stations %lu\n",n);
  {
    int m;
    for( m=0 ; m<n ; m++ ) {
      print_prefix(stn_tab[m]);
      putnl();
    }
  }
#endif
  if (n==0) {
    out_info(msg(74)); /* no matrix */
    return;
  }
  /* release any unused entries in stn_tab (warning: n==0 is bad news) */
/*  stn_tab=osrealloc(stn_tab,n*ossizeof(prefix*)); */
  /* (OSSIZE_T) cast may be needed if n>=181 */
  M=osmalloc((OSSIZE_T)((((OSSIZE_T)n*(n+1))>>1))*ossizeof(real));
  B=osmalloc((OSSIZE_T)(n*ossizeof(real)));

  out_info("");
  if (n==1)
    out_info(msg(78));
  else {
    sprintf(szOut,msg(75),n);
    out_info(szOut);
  }

  for(d=2;d>=0;d--) {
    sprintf(szOut,msg(76),(char)('x'+d));
    out_current_action(szOut);
    /* Initialise M and B to zero */
    for( row=n-1 ; row>=0 ; row-- ) {
      B[row]=(real)0.0;
      for( col=row ; col>=0 ; col-- )
        M(row,col)=(real)0.0; /* might be best to zero "linearly" */
    }
    /* Construct matrix - Go thru' stn list & add all forward legs to M */
    /* (so each leg goes on exactly once) */
    FOR_EACH_STN( stn ) {
#if DEBUG_MATRIX_BUILD
     print_prefix(stn->name);
     printf(" status? %d, used: %d artic %d colour %d\n",stn->status,
      ( !!stn->leg[2])<<2 | !!stn->leg[1]<<1 | !!stn->leg[0], stn->fArtic, stn->colour );
     {
      int dirn;
      for(dirn=0;dirn<=2;dirn++)
       if (USED(stn,dirn)) {
        printf("Leg %d, vx=%f, reverse=%d, to ",dirn, stn->leg[dirn]->v[0],
               stn->leg[dirn]->l.reverse);
        print_prefix(stn->leg[dirn]->l.to->name);putnl();
       }
      putnl();
     }
#endif /* DEBUG_MATRIX_BUILD */
     if (stn->status==statFixed) {
      int f,t;
      int dirn;
/*      print_prefix(stn->name); putnl(); */
      if (fixed(stn)) {
       for( dirn=2 ; dirn>=0 ; dirn-- )
        if ( USED(stn,dirn) && data_here(stn->leg[dirn]) ) {
#if DEBUG_MATRIX_BUILD
         print_prefix(stn->name); printf(" - ");
         print_prefix(stn->leg[dirn]->l.to->name); putnl();
#endif /* DEBUG_MATRIX_BUILD */
         if (!(fixed(stn->leg[dirn]->l.to)) &&
             stn->leg[dirn]->l.to->status==statFixed) {
          t=find_stn_in_tab(stn->leg[dirn]->l.to);
	  ASSERT2(0,"fix covariances code!");
          e=stn->leg[dirn]->v[d][d]; /* !HACK! covariances */
          if (e!=(real)0.0) /* not an equate */ {
           e=((real)1.0)/e;
           a=e*stn->leg[dirn]->d[d];
           M(t,t)+=e;
           B[t]+=e*POS(stn,d)+a;
#if DEBUG_MATRIX_BUILD
           printf("--- Dealing with stn fixed at %lf\n",POS(stn,d));
#endif /* DEBUG_MATRIX_BUILD */
          }
         }
        }
      } else {
       f=find_stn_in_tab(stn);
       for( dirn=2 ; dirn >= 0 ; dirn-- )
        if ( USED(stn,dirn) && data_here(stn->leg[dirn]) ) {
	 ASSERT2(0,"fix covariances code!");
         e=stn->leg[dirn]->v[d][d]; /* !HACK! covariances */
         if (fixed(stn->leg[dirn]->l.to)) {
          if (e!=(real)0.0) /* Ignore equated nodes */ {
           e=((real)1.0)/e;
           a=e*stn->leg[dirn]->d[d];
           M(f,f)+=e;
           B[f]+=e*POS(stn->leg[dirn]->l.to,d)-a;
          }
         } else {
          if (stn->leg[dirn]->l.to->status==statFixed) {
           t=find_stn_in_tab(stn->leg[dirn]->l.to);
#if DEBUG_MATRIX
           printf("Leg %d to %d, var %f, delta %f\n",f,t,e,stn->leg[dirn]->d[d]);
#endif
           if (e!=(real)0.0 && t!=f) /* Ignore equated nodes & lollipops */ {
            e=((real)1.0)/e;
            a=e*stn->leg[(dirn)]->d[d];
            M(f,f)+=e; B[f]-=a;
            M(t,t)+=e; B[t]+=a;
            if (f<t) M(t,f)-=e; else M(f,t)-=e;
           }
          }
         }
        }
      }
     }
    }

#if PRINT_MATRICES
    print_matrix( M,B ); /* 'ave a look! */
#endif

#ifdef SOR
    if (optimize & 4) /* defined in network.c, set in commline.c */
     sor(M,B);
    else
#endif
     choleski(M,B);

    {
     int m;
     for( m=n-1 ; m>=0 ; m-- )
      stn_tab[m]->pos->p[d]=B[m];
#if EXPLICIT_FIXED_FLAG
/* broken code? !HACK! */
     for( m=n-1 ; m>=0 ; m-- )
      fix(stn_tab[m]->stn);
#endif
    }
  }
  osfree(B);
  osfree(M);
}

static int find_stn_in_tab( node *stn ) {
 int i=0;
 pos *pos;
 pos=stn->name->pos;
 while (stn_tab[i]->pos!=pos)
  if (++i==n_stn_tab) {
#if DEBUG_INVALID
   fputs("Station ",stderr); fprint_prefix(stderr,stn->name);
   fputs(" not in table\n\n",stderr);
   for( i=0 ; i<n_stn_tab ; i++ )
    { fprintf(stderr,"%3d: ",i); fprint_prefix(stderr,stn_tab[i]); fputnl(stderr); }
#endif
#if 0
   print_prefix(stn->name);
   printf(" status? %d, used: %d artic %d colour %d\n",stn->status,
    ( !!stn->leg[2])<<2 | !!stn->leg[1]<<1 | !!stn->leg[0], stn->fArtic, stn->colour );
#endif
   fatal(11,NULL,NULL,0);
  }
 return i;
}

static int add_stn_to_tab( node *stn ) {
  int i;
  pos *pos;
  pos=stn->name->pos;
  for( i=0 ; i<n_stn_tab ; i++ ) {
    if (stn_tab[i]->pos==pos)
      return i;
  }
  stn_tab[n_stn_tab++]=stn->name;
  return i;
}

/* Solve MX=B for X by Choleski factorisation */
/* Note M must be symmetric positive definite */
/* routine is entitled to scribble on M and B if it wishes */
static void choleski( real FAR *M, real *B ) {
 int i,j,k;
 long n=n_stn_tab;
 real V;
#ifndef NO_PERCENTAGE
 ulong flopsTot, flops=0, temp=0;
#define do_percent(N) BLK(flops+=(N);out_set_percentage((int)((100.0*flops)/flopsTot));)
#endif

 /* calc as double so we don't overflow a ulong with intermediate results */
 flopsTot = (ulong)(n*(2.0*n*n+9.0*n-5.0)/6.0);
 /* 3*n*(n-1)/2 + n*(n-1)*(n-2)/3 + n*(n-1)/2 + n + n*(n-1)/2; */
/* n*(9*n-5 + 2*n*n )/6 ; */

 for( j=1 ; j<n ; j++ ) {
  for( i=0 ; i<j ; i++ ) {
   V=(real)0.0;
   for( k=0 ; k<i ; k++ )
    V+=M(i,k)*M(j,k)*M(k,k);
   M(j,i) = ( M(j,i)-V ) / M(i,i);
  }
  V=(real)0.0;
  for( k=0 ; k<j ; k++ )
   V-=M(j,k)*M(j,k)*M(k,k);
  M(j,j)+=V; /* may be best to add M() last for numerical reasons too */
#ifndef NO_PERCENTAGE
  if (fPercent) {
   temp+=((ulong)j+j)+1ul; /* avoid multiplies */
   do_percent(temp);
  }
#endif
 }

 /* Multiply x by L inverse */
 for( i=0 ; i<n-1 ; i++ ) {
  for( j=i+1 ; j<n ; j++ )
   B[j]-=M(j,i)*B[i];
 }

#ifndef NO_PERCENTAGE
 if (fPercent) {
  temp=(ulong)n*(n-1ul)/2ul; /* needed again lower down */
  do_percent(temp);
 }
#endif

 /* Multiply x by D inverse */
 for( i=0 ; i<n ; i++ )
  B[i]/=M(i,i);

#ifndef NO_PERCENTAGE
 if (fPercent)
  do_percent((ulong)n);
#endif

 /* Multiply x by (L transpose) inverse */
 for( i=n-1 ; i>0 ; i-- ) {
  for( j=i-1 ; j>=0 ; j-- )
   B[j]-=M(i,j)*B[i];
 }

#ifndef NO_PERCENTAGE
 if (fPercent)
  do_percent(temp);
# undef do_percent
#endif

/* printf("\n%ld/%ld\n\n",flops,flopsTot); */
}

#ifdef SOR
/* factor to use for SOR (must have 1 <= SOR_factor < 2) */
#define SOR_factor 1.95 /* 1.95 */

/* Solve MX=B for X by SOR of Gauss-Siedel */
/* routine is entitled to scribble on M and B if it wishes */
static void sor( real FAR *M, real *B )
 {
 real t,x,delta,threshold,t2;
 int row,col;
 real *X;
 long it=0;

 X=osmalloc(n*ossizeof(real));

 threshold=0.00001;

printf("reciprocating diagonal\n");

 /* munge diagonal so we can multiply rather than divide */
 for ( row=n-1 ; row>=0 ; row-- )
  {
  M(row,row) = 1/M(row,row);
  X[row]=0.0;
  }

printf("starting iteration\n");

 do {
  it++;/*printf("*");*/
  t=0.0;
  for( row=0 ; row<n ; row++ )
   {
   x=B[row];
   for( col=0 ; col<row ; col++ )
    x-=M(row,col)*X[col];
   for( col++ ; col<n ; col++ )
    x-=M(col,row)*X[col];
   x*=M(row,row);
   delta=(x-X[row])*SOR_factor;
   X[row] += delta;
   t2=fabs(delta);
   if (t2>t) t=t2;
   }
  } while (t>=threshold && it<100000);

 if (t>=threshold)
  {
  fprintf(stderr,"*not* converged after %ld iterations\n",it);
  BUG("iteration stinks");
  }

printf("%ld iterations\n",it);

#if 0
 putnl();
 for ( row=n-1 ; row>=0 ; row-- )
  {
  t=0.0;
  for ( col=0 ; col<row ; col++ )
   t+=M(row,col)*X[col];
  t+=X[row]/M(row,row);
  for ( col=row+1 ; col<n ; col++ )
   t+=M(col,row)*X[col];
  printf("[ %lf %lf ]\n",t,B[row]);
  }
#endif

 for ( row=n-1 ; row>=0 ; row-- )
  B[row]=X[row];

 osfree(X);
printf("\ndone\n");

 }
#endif

#if PRINT_MATRICES
static void print_matrix( real FAR *M, real *B ) {
 int row,col;
 int n=n_stn_tab;
 printf("Matrix, M and vector, B:\n");
 for( row=0 ; row<n ; row++ ) {
  for( col=0 ; col<=row ; col++ )
   printf("%6.2f\t",M(row,col));
  for( ; col<=n ; col++ )
   printf(" \t");
  printf("\t%6.2f\n",B[row]);
 }
 putnl();
 return;
}
#endif
