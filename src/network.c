/* > network.c
 * SURVEX Network reduction routines
 * Copyright (C) 1991-1995,1997 Olly Betts
 */

/*
This source file is in pretty bad need of tidying up, and probably needs
splitting up, as it's rather slow to recompile.
*/

/*
1992.07.29 Now free stack elements in unwind_stack( )
1992.10.25 Minor fettling
1993.01.17 dx, etc now subscripted
1993.01.25 PC Compatibility OKed
1993.01.27 Added PRINT_NETBITS option - currently off
1993.02.19 print_prefix(..) -> fprint_prefix(stdout,..)
           altered leg->to & leg->reverse to leg->l.to & leg->l.reverse
           allocate linkrev for reverse leg (to save memory)
           stack used for network reduction is now declared static
           PRINT_NETBITS now tested for 0 / non-0
           finished -> fDone
           fettled indenting
1993.02.23 now calls error() instead of exit()
1993.02.26 stack trailing & in-between travs separately, to save space
           had a stab at some error stats
1993.02.27 now give similar statistics to Sean's for closure errors
1993.02.28 szLink now used to do traverses: \1" - "\2" - "\3
1993.03.06 now output error stats to file
1993.03.11 node->in_network -> node->status
1993.04.06 fixed 4:30am bug
1993.04.07 added path for Error Stat file
1993.04.22 out of memory -> error # 0
1993.04.26 added szLinkEq
1993.04.27 corrected szLinkEq
           tidied up replace_traverses()
1993.04.28 cured 'dreamtime' - 'drunk&stupid' bug
1993.04.30 removed 2 spaces at ends of lines
1993.05.03 solved problem with equate at start of empty traverse
           removed #define print_prefix ... to survex.h
1993.06.04 FALSE -> fFalse
1993.06.16 malloc() -> osmalloc(); free() -> osfree(); syserr() -> fatal()
1993.06.28 addleg() -> addfakeleg()
1993.07.12 fopen( fnm, "wt" ); corrected - mode should be "w"
1993.08.06 fettled a little
1993.08.08 tidied and turned debugging off
1993.08.09 altered data structure (OLD_NODES compiles previous version)
           free fnmErrStat much sooner now
           recoded FOLLOW_TRAVERSE - eliminated all uses of fDone
1993.08.10 fixed typo-bug introduced in the conversion (sigh for 2 hours)
           debugging flags moved to debug.h
           added calls to validate()
1993.08.11 NEW_NODES & POS(stn,d)
           FOR_EACH_STN( stn ) does for loop with stn looping thru' all nodes
1993.08.12 validate() is now a null macro if turned off
           added USED(station,leg) macro
           *_traverse(s) and *_TRAVERSE -> *_trav(s) and *_TRAV
1993.08.16 output files can now be done with extensions
           added EXT_SVX_ERRS
1993.11.03 changed error routines
1993.11.06 extracted messages
1993.11.08 error 5 made fatal & added FATAL5ARGS macro
           only NEW_NODES code left
1993.11.18 check for output file being directory
1993.11.29 error 5 -> 11; FATAL5ARGS -> FATALARGS
1993.11.30 sorted out error vs fatal
1993.12.09 type link -> linkfor
1993.12.17 changed 'Point not fixed errors' to 'Bug in program' errors
           moved fix() to more logical place in replace_*() fns
1994.03.19 added hack labels
           error output all to stderr
1994.04.10 added code to prune and replace lollipops
           BUG() macro introduced
1994.04.11 !fixed() => 3-node
1994.04.12 started to add parallel code
1994.04.13 finished adding parallel code and debugged it
1994.04.14 tidied up debugging code
           bodged triple parallel fix
1994.04.15 tidied triple parallel fix a bit
1994.04.16 added optimize, controlled by -O[LP]*
           fixed hanging loop bug and unattached dumb-bell bug
           tidied up a bit; use BLK(); moved stack* typedefs here
           added netbits.h
1994.04.17 added matrix.h
1994.04.18 added network.h
1994.04.27 removed extra ptrRed declaration; lfErrStat gone
1994.05.07 changed `lolly' to `noose'; sizeof -> ossizeof
           started to add delta-star code
           add fns copy_link and addto_link
1994.05.08 more changes to facilitate covariances
1994.05.10 finished adding delta->star; debugging; added dump_node calls
1994.05.11 bug-fixed delta-star and removed/commented-out debug code
1994.05.12 station list is now partitioned after removing traverses
           stacks store joins only (rest can be worked out)
1994.06.20 added some just-in-case initialisations
           added int argument to warning, error and fatal
1994.08.14 validate.h/debug.h munged
1994.08.15 now print hyp test stat to .err file
1994.08.16 fhErrStat made global so we can solve, add more data and resolve
1994.08.17 #include "readval.h" for TRIM_PREFIX, <stddef.h> for offsetof()
           now free nodes invented for delta-star
1994.08.29 reworked code to use out module
1994.08.31 fputnl()
1994.09.03 pruned lots of commented out dead code; added ASSERT macro
1994.09.08 byte -> uchar
1994.09.11 BAD_DIRN and FOLLOW_TRAV moved to survex.h
1994.09.13 moved ASSERT macro to debug.h
1994.10.01 check for single loop with no fixed points case
1994.10.06 now prints error stats for single leg traverses
           added hook for articulation point code
1994.10.08 added osnew()
1994.10.09 free stations and legs after solution
1994.10.14 now work out totals and bounding box here rather than in survex.c
1994.10.27 fixed replace_trailing_travs so it might work
           stnHi, stnLo -> pfxHi, pfxLo
           pos list now done from here
1994.10.31 added prefix.shape
1994.11.01 fettled formatting
1994.11.06 moved articulation point stuff into matrix.c
           bogus stations are no longer flagged with a different status
1994.11.12 removed #include "artic.h"
1994.11.16 fixed problem with freeing legs, but then looking at them again
1994.11.19 moved fixed point invention from matrix.c to network.c
1994.11.20 tidied
1994.11.24 invent fix at oldest station, not newest
1994.12.03 no longer produce pos list here
           should now throw away unfixed bits and give non-fatal error
1995.04.17 ASSERT->ASSERT2
1996.08.04 H: and V: error values were incorrectly transposed
1997.07.16 added gross error detection code
1997.08.24 covariance code
1998.06.15 fettled
*/

/*
This file would benefit greatly from being written in C++, to avoid all the
yucky divdv( &d, &d, &v ) code.  Oh well, at least it seems to work.
*/

#include <stddef.h>
#include "validate.h"
#include "debug.h"
#include "survex.h"
#include "filename.h"
#include "message.h"
#include "filelist.h"
#include "netbits.h"
#include "matrix.h"
#include "network.h"
#include "readval.h"
#include "out.h"

static void print_var( const var v ) {
#ifdef NO_COVARIANCES
   printf("(%f,%f,%f)", v[0], v[1], v[2]);
#else
   int i;
   for (i = 0; i < 3; i++) printf("(%f,%f,%f)\n", v[i][0], v[i][1], v[i][2]);
#endif
}

#define sqrdd(X) (sqrd((X)[0])+sqrd((X)[1])+sqrd((X)[2]))

typedef struct Stack {
   struct Link *join1, *join2;
   struct Stack *next;
} stack;

#define IS_NOOSE(SR) ((SR)->type == 1)
#define IS_PARALLEL(SR) ((SR)->type == 0)
#define IS_DELTASTAR(SR) ((SR)->type == 2)

typedef struct StackRed {
   struct Link *join1, *join2, *join3;
   /* !HACK! type field is superfluous, can infer info from join values... */
   /* e.g. join3 == NULL for noose, join3 == join1 for ||, otherwise D* */
   int type; /* 1 => noose, 0 => parallel legs, 2 => delta-star */
   struct StackRed *next;
} stackRed;

typedef struct StackTr {
 struct Link     *join1;
 struct StackTr  *next;
} stackTrail;

/* string used between things in traverse printouts eg \1 - \2 - \3 -...*/
static char *szLink  =" - ";
static char *szLinkEq=" = "; /* use this one for equates */

long optimize=27; /* set by commline.c */

#if 0
#define fprint_prefix( FH, NAME ) BLK( (fprint_prefix)( (FH), (NAME) ); fprintf( (FH), " [%p]", (void*)(NAME) ); )
#endif

static stackRed *ptrRed; /* Ptr to TRaverse linked list for C*-*< , -*=*- */
static stack *ptr; /* Ptr to TRaverse linked list for in-between travs */
static stackTrail *ptrTrail; /* Ptr to TRaverse linked list for trail travs*/

static void remove_trailing_travs( void );
static void remove_travs( void );
static void replace_travs( void );
static void replace_trailing_travs( void );
static void remove_subnets( void );
static void replace_subnets( void );

static void remove_trailing_trav( node *stn, uchar i );
static void concatenate_trav( node *stn, uchar i );

static void err_stat( int cLegsTrav, double lenTrav,
                      double eTot, double eTotTheo,
                      double hTot, double hTotTheo,
                      double vTot, double vTotTheo );

extern void solve_network( void /*node *stnlist*/ ) {
 node **pstnPrev, *stn;
 node *stnlistfull;
 if (stnlist==NULL)
  fatal(43,NULL,NULL,0);
 ptr=NULL;
 ptrTrail=NULL;
 ptrRed=NULL;
 dump_network();
 /* If there are no fixed points, invent one.  Do this first to avoid
  * problems with sub-nodes of the invented fix which have been removed.
  * It also means we can fix the "first" station, which makes more sense
  * to the user.
  */
 FOR_EACH_STN( stn )
  if (fixed(stn))
   break;
 if (!stn) {
  node *stnFirst=NULL;
  FOR_EACH_STN( stn )
   if (stn->status)
    stnFirst=stn;
  ASSERT2(stnFirst,"no stations left in net!");
  stn=stnFirst;
  sprintf(szOut,msg(72),sprint_prefix(stn->name));
  out_info(szOut);
  POS(stn,0)=(real)0.0;
  POS(stn,1)=(real)0.0;
  POS(stn,2)=(real)0.0;
  fix(stn);
 }
 remove_trailing_travs();                         validate(); dump_network();
 remove_travs();                                  validate(); dump_network();
 pstnPrev=&stnlistfull; /* do it anyway to suppress warning */
 if (optimize & 16) {
  /* partition station list, so we can ignore stations already removed */
  stn=stnlist;
  stnlist=NULL;
  while (stn) {
   if (stn->status) {
    node *stnNext=stn->next;
    stn->next=stnlist;
    stnlist=stn;
    stn=stnNext;
   } else {
    *pstnPrev=stn;
    pstnPrev=&stn->next;
    stn=stn->next;
   }
  }
 }
 remove_subnets();                                validate(); dump_network();
 solve_matrix();                                  validate(); dump_network();
 replace_subnets();                               validate(); dump_network();
 if (optimize & 16) {
  /* combine partitions to form original station list (order is changed) */
  *pstnPrev=stnlist;
  stnlist=stnlistfull;
 }
 replace_travs();                                 validate(); dump_network();
 replace_trailing_travs();                        validate(); dump_network();
}

static void remove_trailing_travs( void ) {
 node *stn;
 out_current_action(msg(125));
 FOR_EACH_STN( stn ) {
  if (stn->status && shape(stn)==1 && !fixed(stn)) {
   if (stn->leg[0]!=NULL)
    remove_trailing_trav( stn, 0 );
   else if (stn->leg[1]!=NULL)
    remove_trailing_trav( stn, 1 );
   else {
    ASSERT2(stn->leg[2]!=NULL,"Node with shape==1 has no legs...");
    remove_trailing_trav( stn, 2 );
   }
  }
 }
}

static void do_travs( node *stn ) {
 if (stn->leg[0]!=NULL)
  concatenate_trav( stn, 0 );
 if (stn->leg[1]!=NULL)
  concatenate_trav( stn, 1 );
 if (stn->leg[2]!=NULL)
  concatenate_trav( stn, 2 );
}

static void remove_travs( void ) {
 node *stn;
 out_current_action(msg(126));
 FOR_EACH_STN( stn )
  if (stn->status && !unfixed_2_node(stn))
   do_travs(stn);
}

static void remove_trailing_trav( node *stn, uchar i ) {
 uchar j;
 stackTrail *trav;

#if PRINT_NETBITS
 printf("Removed trailing trav ");
#endif
 do {
#if PRINT_NETBITS
  print_prefix(stn->name); printf("<%p>",stn); printf(szLink);
#endif
  stn->status=statRemvd;
  j=reverse_leg(stn->leg[i]);
  stn=stn->leg[i]->l.to;
  i=BAD_DIRN;
  FOLLOW_TRAV( stn, j, i );
 } while (i!=BAD_DIRN && !fixed(stn));

 /* put traverse on stack */
 trav=osnew(stackTrail);
 trav->join1=stn->leg[j];
 trav->next=ptrTrail;
 ptrTrail=trav;

 remove_leg_from_station(j,stn);

#if PRINT_NETBITS
 print_prefix(stn->name); printf("<%p>",stn); putnl();
#endif
}

static void concatenate_trav( node *stn, uchar i ) {
 uchar j;
 stack *trav;
 node *stn2;
 linkfor *newleg, *newleg2;

 stn2=stn->leg[i]->l.to;
 /* Reject single legs as they may be already concatenated traverses */
 if (!unfixed_2_node(stn2))
  return;

 trav=osnew(stack);
 newleg2=(linkfor*)osnew(linkrev);

#if PRINT_NETBITS
 printf("Concatenating trav "); print_prefix(stn->name); printf("<%p>",stn);
#endif

 newleg2->l.to= stn;
 newleg2->l.reverse = i | 64; /* 64 for replacement leg */
 trav->join1=stn->leg[i];

 j=reverse_leg(stn->leg[i]);

 newleg = copy_link(stn->leg[i]);

 stn->leg[i]=newleg;
 i=BAD_DIRN;
 FOLLOW_TRAV(stn2, j, i );
 stn=stn2;

#if PRINT_NETBITS
 printf(szLink); print_prefix(stn->name); printf("<%p>",stn);
#endif

 while (i!=BAD_DIRN && !fixed(stn)) {
  stn->status=statRemvd;
  j=reverse_leg(stn->leg[i]);
  stn2=stn->leg[i]->l.to;

  addto_link(newleg, stn->leg[i]);

  i=BAD_DIRN;
  FOLLOW_TRAV(stn2, j, i );
  stn=stn2;
#if PRINT_NETBITS
  printf(szLink); print_prefix(stn->name); printf("<%p>",stn);
#endif
 }
 trav->join2=stn->leg[j];
 trav->next=ptr;
 ptr=trav;

#if 0 /* !HACK! */
 print_var(newleg->v); putnl();
#endif

#if PRINT_NETBITS
 putchar(' ');
 print_var(newleg->v);

 printf("\nStacked "); print_prefix(trav->end1->name); printf(",%d-",trav->i1);
 print_prefix(trav->end2->name); printf(",%d\n",trav->i2);
#endif

 newleg->l.to     = stn;
 newleg->l.reverse= j | 0x80 | 64; /* 64 for replacement leg */

 stn->leg[j]=newleg2;
}

static void remove_subnets( void ) {
 node *stn,*stn2,*stn3,*stn4;
 int dirn,dirn2,dirn3,dirn4;
 stackRed *trav;
 linkfor *newleg, *newleg2;
 bool fMore=fTrue;

 out_current_action(msg(129));

 while (fMore) {
  fMore=fFalse;
  if (optimize & 1) {
   /*      _
          ( )
           * stn
           |
           * stn2
          /|
    stn4 * * stn3  -->  stn4 *-* stn3
         : :                 : :
   */
   /* NB can have non-fixed 0 nodes */
   FOR_EACH_STN( stn ) {
    if (stn->status && !fixed(stn) && shape(stn)==3) {
     dirn=((stn->leg[1]->l.to==stn)|(stn->leg[0]->l.to==stn)<<1)-1;
     if (dirn<0)
      continue;

     stn2=stn->leg[dirn]->l.to;
     if (fixed(stn2))
      continue;
     ASSERT( shape(stn2)==3 );

     dirn2=reverse_leg(stn->leg[dirn]);
     dirn2=(dirn2+1)%3;
     stn3=stn2->leg[dirn2]->l.to;
     if (stn2==stn3)
      continue; /* dumb-bell - leave alone */

     dirn3=reverse_leg(stn2->leg[dirn2]);

     trav=osnew(stackRed);
     newleg2=(linkfor*)osnew(linkrev);

     newleg = copy_link( stn3->leg[dirn3] );

     dirn2=(dirn2+1)%3;
     stn4=stn2->leg[dirn2]->l.to;
     dirn4=reverse_leg(stn2->leg[dirn2]);
#if 0
     printf("Noose found with stn...stn4 = \n");
     print_prefix(stn->name); putnl();  print_prefix(stn2->name); putnl();
     print_prefix(stn3->name); putnl(); print_prefix(stn4->name); putnl();
#endif

     addto_link(newleg,stn2->leg[dirn2]);

     /* mark stn and stn2 as removed */
     stn->status=statRemvd;
     stn2->status=statRemvd;

     /* stack noose and replace with a leg between stn3 and stn4 */
     trav->join1 = stn3->leg[dirn3];
     newleg->l.to = stn4;
     newleg->l.reverse = dirn4 | 0x80 | 64; /* 64 for replacement leg */

     trav->join2 = stn4->leg[dirn4];
     newleg2->l.to = stn3;
     newleg2->l.reverse = dirn3 | 64; /* 64 for replacement leg */

     stn3->leg[dirn3]=newleg;
     stn4->leg[dirn4]=newleg2;

     trav->next = ptrRed;
     trav->type = 1; /* NOOSE */
     ptrRed = trav;
     fMore=fTrue;
    }
   }
  }

  if (optimize & 2) {
/*printf("replacing parallel legs\n");*/
   FOR_EACH_STN( stn ) {
    /*
       :
       * stn3
       |            :
       * stn        * stn3
      ( )      ->   |
       * stn2       * stn4
       |            :
       * stn4
       :
    */
    if (stn->status && !fixed(stn) && shape(stn)==3) {
     stn2=stn->leg[0]->l.to;
     if (stn2==stn->leg[1]->l.to)
      dirn=2;
     else {
      if (stn2==stn->leg[2]->l.to)
       dirn=1;
      else {
       if (stn->leg[1]->l.to!=stn->leg[2]->l.to)
        continue;
       stn2=stn->leg[1]->l.to;
       dirn=0;
      }
     }

     if (fixed(stn2) || stn==stn2) /* stn==stn2 => noose */
      continue;

     ASSERT( shape(stn2)==3 );

     stn3=stn->leg[dirn]->l.to;
     if (stn3==stn2)
      continue; /* 3 parallel legs (=> nothing else) so leave */

     dirn3=reverse_leg(stn->leg[dirn]);
     dirn2=(0+1+2)-reverse_leg(stn->leg[(dirn+1)%3])
                  -reverse_leg(stn->leg[(dirn+2)%3]);

     stn4=stn2->leg[dirn2]->l.to;
     dirn4=reverse_leg(stn2->leg[dirn2]);

     trav=osnew(stackRed);

     newleg=copy_link(stn->leg[(dirn+1)%3]);
     newleg2=copy_link(stn->leg[(dirn+2)%3]); /* use newleg2 for scratch */
     {
      var sum, prod;
      d temp, temp2;
#if 0 /*ndef NO_COVARIANCES*/
      print_var( newleg->v );
      printf("plus\n");
      print_var( newleg2->v );
      printf("equals\n");
#endif
      addvv(&sum,&newleg->v,&newleg2->v);
      ASSERT2( !fZero(&sum), "loop of zero variance found");
#if 0 /*ndef NO_COVARIANCES*/
      print_var( sum );
#endif
      mulvv(&prod,&newleg->v,&newleg2->v);
      mulvd(&temp,&newleg2->v,&newleg->d);
      mulvd(&temp2,&newleg->v,&newleg2->d);
      adddd(&temp,&temp,&temp2);
/*printf("divdv(&newleg->d,&temp,&sum)\n");*/
      divdv(&newleg->d,&temp,&sum);
/*printf("divvv(&newleg->v,&prod,&sum)\n");*/
      divvv(&newleg->v,&prod,&sum);
     }
     osfree(newleg2);
     newleg2=(linkfor*)osnew(linkrev);

     addto_link(newleg,stn2->leg[dirn2]);
     addto_link(newleg,stn3->leg[dirn3]);

#if 0
     printf("Parallel found with stn...stn4 = \n");
     (dump_node)(stn); (dump_node)(stn2); (dump_node)(stn3); (dump_node)(stn4);
     printf("dirns = %d %d %d %d\n",dirn,dirn2,dirn3,dirn4);
#endif
     ASSERT2( stn3->leg[dirn3]->l.to==stn, "stn3 end of || doesn't recip");
     ASSERT2( stn4->leg[dirn4]->l.to==stn2, "stn4 end of || doesn't recip");
     ASSERT2( stn->leg[(dirn+1)%3]->l.to==stn2 && stn->leg[(dirn+2)%3]->l.to==stn2, "|| legs aren't");

     /* mark stn and stn2 as removed (already discarded triple parallel) */
     /* so stn!=stn4 <=> stn2!=stn3 */
     stn->status=statRemvd;
     stn2->status=statRemvd;

     /* stack parallel and replace with a leg between stn3 and stn4 */
     trav->join1 = stn3->leg[dirn3];
     newleg->l.to = stn4;
     newleg->l.reverse = dirn4 | 0x80 | 64; /* 64 for replacement leg */

     trav->join2 = stn4->leg[dirn4];
     newleg2->l.to = stn3;
     newleg2->l.reverse = dirn3 | 64; /* 64 for replacement leg */

     stn3->leg[dirn3]=newleg;
     stn4->leg[dirn4]=newleg2;

     trav->next = ptrRed;
     trav->type = 0; /* PARALLEL */
     ptrRed = trav;
     fMore=fTrue;
    }
   }
/*printf("done replacing parallel legs\n");*/
  }

  if (optimize & 8) {
   node *stn5, *stn6;
   int dirn5, dirn6, dirn0;
   linkfor *legAB, *legBC, *legCA;
   linkfor *legAZ, *legBZ, *legCZ;
/*printf("replacing delta with star\n");*/
   FOR_EACH_STN( stn ) {
/*    printf("*");*/
    /*
             :
             * stn5            :
             |                 * stn5
             * stn2            |
            / \        ->      O stnZ
       stn *---* stn3         / \
          /     \       stn4 *   * stn6
    stn4 *       * stn6      :   :
         :       :
    */
    if (stn->status && !fixed(stn) && shape(stn)==3) {
     for( dirn0=0 ; ; dirn0++ ) {
      if (dirn0>=3)
       goto nodeltastar; /* continue outer loop */
      dirn=dirn0;
      stn2=stn->leg[dirn]->l.to;
      if (fixed(stn2) || stn2==stn)
       continue;
      dirn2=reverse_leg(stn->leg[dirn]);
      dirn2=(dirn2+1)%3;
      stn3=stn2->leg[dirn2]->l.to;
      if (fixed(stn3) || stn3==stn || stn3==stn2)
       goto nextdirn2;
      dirn3=reverse_leg(stn2->leg[dirn2]);
      dirn3=(dirn3+1)%3;
      if (stn3->leg[dirn3]->l.to==stn) {
       legAB=copy_link(stn->leg[dirn]);
       legBC=copy_link(stn2->leg[dirn2]);
       legCA=copy_link(stn3->leg[dirn3]);
       dirn=(0+1+2)-dirn-reverse_leg(stn3->leg[dirn3]);
       dirn2=(dirn2+1)%3;
       dirn3=(dirn3+1)%3;
      } else {
       if (stn3->leg[(dirn3+1)%3]->l.to==stn) {
        legAB=copy_link(stn->leg[dirn]);
        legBC=copy_link(stn2->leg[dirn2]);
        legCA=copy_link(stn3->leg[(dirn3+1)%3]);
        dirn=(0+1+2)-dirn-reverse_leg(stn3->leg[(dirn3+1)%3]);
        dirn2=(dirn2+1)%3;
        break;
       } else {
        nextdirn2:;
        dirn2=(dirn2+1)%3;
        stn3=stn2->leg[dirn2]->l.to;
        if (fixed(stn3) || stn3==stn || stn3==stn2)
         continue;
        dirn3=reverse_leg(stn2->leg[dirn2]);
        dirn3=(dirn3+1)%3;
        if (stn3->leg[dirn3]->l.to==stn) {
         legAB=copy_link(stn->leg[dirn]);
         legBC=copy_link(stn2->leg[dirn2]);
         legCA=copy_link(stn3->leg[dirn3]);
         dirn=(0+1+2)-dirn-reverse_leg(stn3->leg[dirn3]);
         dirn2=(dirn2+2)%3;
         dirn3=(dirn3+1)%3;
         break;
        } else {
         if (stn3->leg[(dirn3+1)%3]->l.to==stn) {
          legAB=copy_link(stn->leg[dirn]);
          legBC=copy_link(stn2->leg[dirn2]);
          legCA=copy_link(stn3->leg[(dirn3+1)%3]);
          dirn=(0+1+2)-dirn-reverse_leg(stn3->leg[(dirn3+1)%3]);
          dirn2=(dirn2+2)%3;
          break;
         }
        }
       }
      }
     }

     ASSERT( shape(stn2)==3 );
     ASSERT( shape(stn3)==3 );

     stn4=stn->leg[dirn]->l.to;
     stn5=stn2->leg[dirn2]->l.to;
     stn6=stn3->leg[dirn3]->l.to;

     if (stn4==stn2 || stn4==stn3 || stn5==stn3) break;

     dirn4=reverse_leg(stn->leg[dirn]);
     dirn5=reverse_leg(stn2->leg[dirn2]);
     dirn6=reverse_leg(stn3->leg[dirn3]);
#if 0
     printf("delta-star, stn ... stn6 are:\n");
     (dump_node)(stn);
     (dump_node)(stn2);
     (dump_node)(stn3);
     (dump_node)(stn4);
     (dump_node)(stn5);
     (dump_node)(stn6);
#endif
     ASSERT( stn4->leg[dirn4]->l.to==stn );
     ASSERT( stn5->leg[dirn5]->l.to==stn2 );
     ASSERT( stn6->leg[dirn6]->l.to==stn3 );

     trav=osnew(stackRed);
     legAZ=osnew(linkfor);
     legBZ=osnew(linkfor);
     legCZ=osnew(linkfor);

     {
      var sum;
      d temp;
      node *stnZ;
      prefix *nameZ;
      addvv( &sum, &legAB->v, &legBC->v );
      addvv( &sum, &sum, &legCA->v );
      ASSERT2( !fZero(&sum), "loop of zero variance found");

      mulvv( &legAZ->v, &legCA->v, &legAB->v );
      divvv( &legAZ->v, &legAZ->v, &sum );
      mulvv( &legBZ->v, &legAB->v, &legBC->v );
      divvv( &legBZ->v, &legBZ->v, &sum );
      mulvv( &legCZ->v, &legBC->v, &legCA->v );
      divvv( &legCZ->v, &legCZ->v, &sum );

      mulvd( &legAZ->d, &legCA->v, &legAB->d );
      mulvd( &temp,     &legAB->v, &legCA->d );
      subdd( &legAZ->d, &legAZ->d, &temp );
      divdv( &legAZ->d, &legAZ->d, &sum );
      mulvd( &legBZ->d, &legAB->v, &legBC->d );
      mulvd( &temp,     &legBC->v, &legAB->d );
      subdd( &legBZ->d, &legBZ->d, &temp );
      divdv( &legBZ->d, &legBZ->d, &sum );
      mulvd( &legCZ->d, &legBC->v, &legCA->d );
      mulvd( &temp,     &legCA->v, &legBC->d );
      subdd( &legCZ->d, &legCZ->d, &temp );
      divdv( &legCZ->d, &legCZ->d, &sum );

      nameZ=osnew(prefix);
      nameZ->ident = ""; /* root has ident[0]=="\" */
      stnZ=osnew(node);
      stnZ->name=nameZ;
      nameZ->pos=osnew(pos);
      nameZ->pos->shape=3;
      nameZ->stn=stnZ;
      nameZ->up=NULL;
      unfix(stnZ);
      stnZ->status=statInNet;
      stnZ->next=stnlist;
      stnlist=stnZ;
      legAZ->l.to=stnZ;
      legAZ->l.reverse=0 | 0x80 | 64; /* 64 for replacement leg */
      legBZ->l.to=stnZ;
      legBZ->l.reverse=1 | 0x80 | 64; /* 64 for replacement leg */
      legCZ->l.to=stnZ;
      legCZ->l.reverse=2 | 0x80 | 64; /* 64 for replacement leg */
      stnZ->leg[0]=(linkfor*)osnew(linkrev);
      stnZ->leg[1]=(linkfor*)osnew(linkrev);
      stnZ->leg[2]=(linkfor*)osnew(linkrev);
      stnZ->leg[0]->l.to=stn4;
      stnZ->leg[0]->l.reverse=dirn4;
      stnZ->leg[1]->l.to=stn5;
      stnZ->leg[1]->l.reverse=dirn5;
      stnZ->leg[2]->l.to=stn6;
      stnZ->leg[2]->l.reverse=dirn6;
      addto_link( legAZ , stn4->leg[dirn4] );
      addto_link( legBZ , stn5->leg[dirn5] );
      addto_link( legCZ , stn6->leg[dirn6] );
      /* stack stuff */
      trav->join1=stn4->leg[dirn4];
      trav->join2=stn5->leg[dirn5];
      trav->join3=stn6->leg[dirn6];
      trav->next = ptrRed;
      trav->type = 2; /* DELTASTAR */
      ptrRed = trav;
      fMore=fTrue;

      stn->status=statRemvd;
      stn2->status=statRemvd;
      stn3->status=statRemvd;
      stn4->leg[dirn4] = legAZ;
      stn5->leg[dirn5] = legBZ;
      stn6->leg[dirn6] = legCZ;
/*print_prefix(stnZ->name);printf(" %p status %d\n",(void*)stnZ,stnZ->status);*/
     }

    }
    nodeltastar:;
   }
/*printf("done replacing delta with star\n");*/
  }

 }
/* printf("\ndone\n");*/
}

static void replace_subnets( void ) {
   stackRed *ptrOld;
   node *stn, *stn2, *stn3, *stn4;
   int dirn, dirn2, dirn3, dirn4;
   linkfor *leg;
   unsigned long cBogus=0; /* how many invented stations? */

   /* help to catch bad accesses */
   stn = stn2 = stn3 = stn4 = NULL;
   dirn = dirn2 = dirn3 = dirn4 = 0;

   out_current_action(msg(130));

   while (ptrRed != NULL) {
      /*  printf("replace_subnets() type %d\n",ptrRed->type);*/
      if (!IS_DELTASTAR(ptrRed)) {
         leg=ptrRed->join1; leg=leg->l.to->leg[reverse_leg(leg)];
         stn3=leg->l.to; dirn3=reverse_leg(leg);
         leg=ptrRed->join2; leg=leg->l.to->leg[reverse_leg(leg)];
         stn4=leg->l.to; dirn4=reverse_leg(leg);
         ASSERT( !(fixed(stn3)&&!fixed(stn4)) );
         ASSERT( !(!fixed(stn3)&&fixed(stn4)) );
         ASSERT( data_here(stn3->leg[dirn3]) );
      }

      if (IS_NOOSE(ptrRed)) {
         /* noose (hanging-loop) */
         d e;
         linkfor *leg;
         if (fixed(stn3)) {
            /* NB either both or neither fixed */
            leg=stn3->leg[dirn3];
            stn2=ptrRed->join1->l.to;
            dirn2=reverse_leg(ptrRed->join1);

            if (fZero(&leg->v))
               e[0]=e[1]=e[2]=0.0;
            else {
               subdd( &e, &POSD(stn4), &POSD(stn3) );
               subdd( &e, &e, &leg->d );
               divdv( &e, &e, &leg->v );
            }
            if (data_here(ptrRed->join1)) {
               mulvd( &e, &ptrRed->join1->v, &e );
               adddd( &POSD(stn2), &POSD(stn3), &ptrRed->join1->d );
               adddd( &POSD(stn2), &POSD(stn2), &e );
            } else {
               mulvd( &e, &stn2->leg[dirn2]->v, &e );
               subdd( &POSD(stn2), &POSD(stn3), &stn2->leg[dirn2]->d );
               adddd( &POSD(stn2), &POSD(stn2), &e );
            }
            stn2->status=statFixed;
            fix(stn2);
            dirn2 = (dirn2+2)%3; /* point back at stn again */
            stn = stn2->leg[dirn2]->l.to;
#if 0
            printf("Replacing noose with stn...stn4 = \n");
            print_prefix(stn->name); putnl();
            print_prefix(stn2->name); putnl();
            print_prefix(stn3->name); putnl();
            print_prefix(stn4->name); putnl();
#endif
            if (data_here(stn2->leg[dirn2]))
               adddd(&POSD(stn), &POSD(stn2), &stn2->leg[dirn2]->d);
            else
               subdd(&POSD(stn), &POSD(stn2),
                     &stn->leg[reverse_leg(stn2->leg[dirn2])]->d);
            stn->status=statFixed;
            fix(stn);
         } else {
            stn2=ptrRed->join1->l.to;
            dirn2=reverse_leg(ptrRed->join1);
            stn2->status=statInNet;
            dirn2 = (dirn2+2)%3; /* point back at stn again */
            stn = stn2->leg[dirn2]->l.to;
            stn->status=statInNet;
         }

         osfree(stn3->leg[dirn3]);
         stn3->leg[dirn3]=ptrRed->join1;
         osfree(stn4->leg[dirn4]);
         stn4->leg[dirn4]=ptrRed->join2;
      } else if (IS_PARALLEL(ptrRed)) {
         /* parallel legs */
         d e, e2;
         linkfor *leg;
         if (fixed(stn3)) {
            /* NB either both or neither fixed */
            stn=ptrRed->join1->l.to;
            dirn=reverse_leg(ptrRed->join1);

            stn2=ptrRed->join2->l.to;
            dirn2=reverse_leg(ptrRed->join2);

            leg=stn3->leg[dirn3];

            if (fZero(&leg->v))
               e[0]=e[1]=e[2]=0.0;
            else {
               subdd( &e, &POSD(stn4), &POSD(stn3) );
               subdd( &e, &e, &leg->d );
               divdv( &e, &e, &leg->v );
            }

            if (data_here(ptrRed->join1)) {
               leg=ptrRed->join1;
               adddd( &POSD(stn), &POSD(stn3), &leg->d );
            } else {
               leg=stn->leg[dirn];
               subdd( &POSD(stn), &POSD(stn3), &leg->d );
            }
            mulvd( &e2, &leg->v, &e );
            adddd( &POSD(stn), &POSD(stn), &e2 );

            if (data_here(ptrRed->join2)) {
               leg=ptrRed->join2;
               adddd( &POSD(stn2), &POSD(stn4), &leg->d );
            } else {
               leg=stn2->leg[dirn2];
               subdd( &POSD(stn2), &POSD(stn4), &leg->d );
            }
            mulvd( &e2, &leg->v, &e );
            subdd( &POSD(stn2), &POSD(stn2), &e2 );
            stn->status=statFixed;
            stn2->status=statFixed;
            fix(stn);
            fix(stn2);
#if 0
            printf("Replacing parallel with stn...stn4 = \n");
            print_prefix(stn->name); putnl();
            print_prefix(stn2->name); putnl();
            print_prefix(stn3->name); putnl();
            print_prefix(stn4->name); putnl();
#endif
         } else {
            stn=ptrRed->join1->l.to;
            stn->status = statInNet;

            stn2=ptrRed->join2->l.to;
            stn2->status = statInNet;
         }
         osfree(stn3->leg[dirn3]); stn3->leg[dirn3]=ptrRed->join1;
         osfree(stn4->leg[dirn4]); stn4->leg[dirn4]=ptrRed->join2;
      } else if (IS_DELTASTAR(ptrRed)) {
         d e;
         linkfor *leg1, *leg2;
         node *stnZ;
         node *stn[3];
         int dirn[3];
         linkfor *leg[3];
         int i;
         linkfor *legX;

         leg[0]=ptrRed->join1;
         leg[1]=ptrRed->join2;
         leg[2]=ptrRed->join3;

         /* work out ends as we don't bother stacking them */
         legX=leg[0]->l.to->leg[reverse_leg(leg[0])];
         stn[0]=legX->l.to;
         dirn[0]=reverse_leg(legX);
         stnZ=stn[0]->leg[dirn[0]]->l.to;
         stn[1]=stnZ->leg[1]->l.to; dirn[1]=reverse_leg(stnZ->leg[1]);
         stn[2]=stnZ->leg[2]->l.to; dirn[2]=reverse_leg(stnZ->leg[2]);
     /*print_prefix(stnZ->name);printf(" %p status %d\n",(void*)stnZ,stnZ->status);*/

         if (fixed(stnZ)) {
            for (i = 0; i < 3; i++) {
               ASSERT2(fixed(stn[i]), "stn not fixed for D*");
               ASSERT2(data_here(stn[i]->leg[dirn[i]]),
	               "data not on leg for D*");
               ASSERT2(stn[i]->leg[dirn[i]]->l.to == stnZ,
                       "bad sub-network for D*");
            }
            for (i = 0; i < 3; i++) {
               leg2=stn[i]->leg[dirn[i]];
               leg1=copy_link(leg[i]);
               stn2=leg[i]->l.to;
               if (fZero(&leg2->v))
                  e[0]=e[1]=e[2]=0.0;
               else {
                  subdd( &e, &POSD(stnZ), &POSD(stn[i]) );
                  subdd( &e, &e, &leg2->d );
                  divdv( &e, &e, &leg2->v );
                  mulvd( &e, &leg1->v, &e );
               }
               adddd( &POSD(stn2), &POSD(stn[i]), &leg1->d );
               adddd( &POSD(stn2), &POSD(stn2), &e );
               stn2->status=statFixed;
               fix(stn2);
               osfree(leg1);
               osfree(leg2);
               stn[i]->leg[dirn[i]]=leg[i];
               osfree(stnZ->leg[i]);
               stnZ->leg[i]=NULL;
            }
/*printf("---%d %f %f %f %d\n",cBogus,POS(stnZ,0),POS(stnZ,1),POS(stnZ,2),stnZ->status);*/
         } else { /* not fixed case */
            for (i = 0; i < 3; i++) {
               ASSERT2(!fixed(stn[i]), "stn fixed for D*");
               ASSERT2(data_here(stn[i]->leg[dirn[i]]),
                       "data not on leg for D*");
               ASSERT2(stn[i]->leg[dirn[i]]->l.to == stnZ,
                       "bad sub-network for D*");
            }
            for (i = 0; i < 3; i++) {
/*print_prefix(stn[i]->name);printf(" status %d\n",stn[i]->status);*/
               leg2=stn[i]->leg[dirn[i]];
               stn2=leg[i]->l.to;
               stn2->status=statInNet;
               osfree(leg2);
               stn[i]->leg[dirn[i]]=leg[i];
               osfree(stnZ->leg[i]);
               stnZ->leg[i]=NULL;
            }
/*     printf("---%d not fixed %d\n",cBogus,stnZ->status);*/
	 }

	 cBogus++;
      } else {
         BUG("ptrRed has unknown type");
      }

      ptrOld=ptrRed;
      ptrRed=ptrRed->next;
      osfree(ptrOld);
   }

   if (cBogus) {
    int cBogus2=0;
    node **pstnPrev;
    pstnPrev=&stnlist;
    /* this loop could stop when it has found cBogus, but continue for now as
     * it's a useful sanity check
     */
    for( stn=stnlist ; stn /*cBogus*/ ; stn=stn->next ) {
  /*    ASSERT2(stn,"bogus station count is wrong");*/
      if (stn->status && stn->name->up==NULL && stn->name->ident[0]=='\0') {
  /*      printf(":::%d %f %f %f %d\n",cBogus2,POS(stn,0),POS(stn,1),POS(stn,2),stn->status);*/
        *pstnPrev=stn->next;
        osfree(stn->name);
        osfree(stn);
        cBogus2++;
  /*      cBogus--;*/
      } else {
        pstnPrev=&(stn->next);
  /*      if (stn->status==0 && stn->name->up==NULL && stn->name->ident[0]=='\0')
          printf("::: hmm\n");*/
      }
    }
    ASSERT2(cBogus==cBogus2,"bogus station count is wrong");
   }
}

#ifdef BLUNDER_DETECTION
static void do_gross( d e, d v, node *stn1, node *stn2 ) {
   double hsqrd, rsqrd, s, cx, cy, cz;
   double tot;
   int i;
#if 0
printf( "e=( %.2f, %.2f, %.2f )", e[0], e[1], e[2] );
printf( " v=( %.2f, %.2f, %.2f )\n", v[0], v[1], v[2] );
#endif
   hsqrd = sqrd(v[0])+sqrd(v[1]);
   rsqrd = sqrdd(v);
   if (rsqrd == 0.0) return;

   fprint_prefix( stdout, stn1->name );
   fputs( "->", stdout );
   fprint_prefix( stdout, stn2->name );

   cx = v[0] + e[0];
   cy = v[1] + e[1];
   cz = v[2] + e[2];

   s = (e[0]*v[0] + e[1]*v[1] + e[2]*v[2]) / rsqrd;
   tot = 0;
   for ( i = 2; i >= 0; --i )
      tot += sqrd(e[i] - v[i]*s);
   printf( " L: %.2f", tot );

   s = sqrd(cx) + sqrd(cy);
   if (s>0.0) {
      s = hsqrd / s;
      s = s<0.0 ? 0.0 : sqrt(s);
      s = 1 - s;
      tot = sqrd(cx*s) + sqrd(cy*s) + sqrd(e[2]);
      printf( " B: %.2f", tot );
   }

   if (hsqrd>0.0) {
      double nx, ny;
      s = (e[0]*v[1] - e[1]*v[0]) / hsqrd;
      nx = cx - s*v[1];
      ny = cy + s*v[0];
      s = sqrd(nx) + sqrd(ny) + sqrd(cz);
      if (s>0.0) {
         s = rsqrd / s;
         s = s<0.0 ? 0.0 : sqrt(s);
         tot = sqrd(cx - s*nx) + sqrd(cy - s*ny) + sqrd(cz - s*cz);
         printf( " G: %.2f", tot );
      }
   }
   putnl();
}
#endif

static void replace_travs( void ) {
 stack *ptrOld;
 node *stn1,*stn2,*stn3;
 int  i,j,k;
 double eTot,lenTrav,lenTot;
 double eTotTheo;
 double vTot,vTotTheo,hTot,hTotTheo;
 d sc, e;
 bool fEquate; /* used to indicate equates in output */
 int cLegsTrav;
 prefix *nmPrev;
 linkfor *leg;

 out_current_action(msg(127));

 if (!fhErrStat) {
  char *fnmErrStat;
#ifdef NO_EXTENSIONS
  fnmErrStat=UsePth(pthOutput,ERRSTAT_FILE);
#else
  fnmErrStat=AddExt(fnmInput,EXT_SVX_ERRS);
#endif
  if (fDirectory(fnmErrStat))
   fatal(44,wr,fnmErrStat,0);
  else {
   fhErrStat=fopen(fnmErrStat,"w");
   if ( !fhErrStat )
    fatal(47,wr,fnmErrStat,0);
  }
  osfree(fnmErrStat);
 }

 if (!pimgOut) {
  char *fnmImg3D;
#ifdef NO_EXTENSIONS
  fnmImg3D=UsePth(pthOutput,IMAGE_FILE);
#else
  fnmImg3D=AddExt(fnmInput,EXT_SVX_3D);
#endif
  sprintf(szOut,msg(121),fnmImg3D);
  out_current_action(szOut); /* writing .3d file */
  pimgOut=img_open_write( fnmImg3D, szSurveyTitle, !pcs->fAscii );
  if (pimgOut==NULL)
   fatal( img_error(), wr, fnmImg3D, 0 );
  free(fnmImg3D);
 }

 /* First do all the one leg traverses */
 FOR_EACH_STN(stn1) {
  if (stn1->status) {
   int i;
   for( i=0 ; i<=2 ; i++ ) {
    linkfor *leg=stn1->leg[i];
    if (leg && data_here(leg) && !(leg->l.reverse & 64) && !fZero(&leg->v)) {
     if (fixed(stn1)) {
      stn2=leg->l.to;
      fprint_prefix( fhErrStat, stn1->name );
      fputs( szLink, fhErrStat );
      fprint_prefix( fhErrStat, stn2->name );
      img_write_datum( pimgOut, img_MOVE, NULL,
                       POS(stn1,0), POS(stn1,1), POS(stn1,2) );
      img_write_datum( pimgOut, img_LINE, NULL,
                       POS(stn2,0), POS(stn2,1), POS(stn2,2) );
      subdd( &e, &POSD(stn2), &POSD(stn1) );
      subdd( &e, &e, &leg->d );
      eTot = sqrdd( e );
      hTot = sqrd(e[0])+sqrd(e[1]);
      vTot = sqrd(e[2]);
#ifndef NO_COVARIANCES
      /* !HACK! what about covariances? */
      eTotTheo=leg->v[0][0]+leg->v[1][1]+leg->v[2][2];
      hTotTheo=leg->v[0][0]+leg->v[1][1];
      vTotTheo=leg->v[2][2];
#else
      eTotTheo=leg->v[0]+leg->v[1]+leg->v[2];
      hTotTheo=leg->v[0]+leg->v[1];
      vTotTheo=leg->v[2];
#endif
      err_stat( 1, sqrt(sqrdd(leg->d)), eTot, eTotTheo, hTot, hTotTheo,
                 vTot, vTotTheo );
     }
    }
   }
  }
 }

 while (ptr!=NULL) {
  bool fFixed;
  /* work out where traverse should be reconnected */
  leg=ptr->join1; leg=leg->l.to->leg[reverse_leg(leg)];
  stn1=leg->l.to; i=reverse_leg(leg);
  leg=ptr->join2; leg=leg->l.to->leg[reverse_leg(leg)];
  stn2=leg->l.to; j=reverse_leg(leg);
#if PRINT_NETBITS
  printf(" Trav "); print_prefix(stn1->name); printf("<%p>",stn1);
  printf("%s...%s",szLink,szLink); print_prefix(stn2->name);
  printf("<%p>",stn2); putnl();
#endif
/*  ASSERT( fixed(stn1) );
    ASSERT2( fixed(stn2), "stn2 not fixed in replace_travs"); */
  /* calculate scaling factors for error distribution */
  fFixed=fixed(stn1);
  if (fFixed) {
   eTot=0.0;
   hTot=vTot=0.0;
   ASSERT( data_here(stn1->leg[i]) );
   if (fZero(&stn1->leg[i]->v))
    sc[0]=sc[1]=sc[2]=0.0;
   else {
    subdd( &e, &POSD(stn2), &POSD(stn1) );
    subdd( &e, &e, &stn1->leg[i]->d );
    eTot = sqrdd( e );
    hTot = sqrd(e[0])+sqrd(e[1]);
    vTot = sqrd(e[2]);
    divdv( &sc, &e, &stn1->leg[i]->v );
   }
#ifndef NO_COVARIANCES
   /* !HACK! what about covariances? */
   eTotTheo=stn1->leg[i]->v[0][0]+stn1->leg[i]->v[1][1]+stn1->leg[i]->v[2][2];
   hTotTheo=stn1->leg[i]->v[0][0]+stn1->leg[i]->v[1][1];
   vTotTheo=stn1->leg[i]->v[2][2];
#else
   eTotTheo=stn1->leg[i]->v[0]+stn1->leg[i]->v[1]+stn1->leg[i]->v[2];
   hTotTheo=stn1->leg[i]->v[0]+stn1->leg[i]->v[1];
   vTotTheo=stn1->leg[i]->v[2];
#endif
   cLegsTrav=0;
   lenTrav=0.0;
   nmPrev=stn1->name;
   img_write_datum( pimgOut, img_MOVE, NULL,
                    POS(stn1,0),  POS(stn1,1),  POS(stn1,2)  );
  }
  osfree(stn1->leg[i]); stn1->leg[i]=ptr->join1; /* put old link back in */
  osfree(stn2->leg[j]); stn2->leg[j]=ptr->join2; /* and the other end */

  if (fFixed) {
   d err;
   memcpy( &err, &e, sizeof(d) );
#ifdef BLUNDER_DETECTION
   printf( "\n--- e=( %.2f, %.2f, %.2f )\n", e[0], e[1], e[2] );
#endif
   while (fTrue) {
    fEquate=fTrue;
    lenTot=0.0;
    /* get next node in traverse - should have stn3->leg[k]->l.to==stn1 */
    stn3=stn1->leg[i]->l.to; k=reverse_leg(stn1->leg[i]);
    ASSERT2( stn3->leg[k]->l.to==stn1, "reverse leg doesn't reciprocate");

#if 0
    if (!unfixed_2_node(stn3))
     break;
#endif

    if (data_here(stn1->leg[i])) {
     leg=stn1->leg[i];
     adddd( &POSD(stn3), &POSD(stn1), &leg->d );
#ifdef BLUNDER_DETECTION
     do_gross( err, leg->d, stn1, stn3 );
#endif
    } else {
     leg=stn3->leg[k];
     subdd( &POSD(stn3), &POSD(stn1), &leg->d );
#ifdef BLUNDER_DETECTION
     do_gross( err, leg->d, stn3, stn1 );
#endif
    }

    if (stn3==stn2 && k==j)
     break;

    mulvd( &e, &leg->v, &sc );
    adddd( &POSD(stn3), &POSD(stn3), &e );
    if (!fZero(&leg->v))
     fEquate=fFalse;
    lenTot+=sqrdd( leg->d );
    stn3->status=statFixed;
    fix(stn3);
    img_write_datum( pimgOut, img_LINE, NULL,
                     POS(stn3,0),  POS(stn3,1),  POS(stn3,2)  );

    if (nmPrev!=stn3->name && !(fEquate && cLegsTrav==0)) {
     /* (node not part of same stn) && (not equate at start of traverse) */
     fprint_prefix( fhErrStat, nmPrev );
#if PRINT_NAME_PTRS
     fprintf( fhErrStat, "[%p|%p]", nmPrev, stn3->name );
#endif
     fputs( fEquate?szLinkEq:szLink, fhErrStat );
     nmPrev=stn3->name;
#if PRINT_NAME_PTRS
     fprintf( fhErrStat, "[%p]", nmPrev );
#endif
     if (!fEquate) {
      cLegsTrav++;
      lenTrav+=sqrt(lenTot);
     } else
      if (lenTot>0.0) {
#if DEBUG_INVALID
       fprintf(stderr,"lenTot = %8.4f ",lenTot); fprint_prefix(stderr,nmPrev);
       fprintf(stderr," -> "); fprint_prefix( stderr, stn3->name );
#endif
       BUG("during calculation of closure errors");
      }
    } else {
#if SHOW_INTERNAL_LEGS
     fprintf(fhErrStat,"+");
#endif
     if (lenTot>0.0) {
#if DEBUG_INVALID
      fprintf(stderr,"lenTot = %8.4f ",lenTot); fprint_prefix(stderr,nmPrev);
      fprintf(stderr," -> "); fprint_prefix( stderr, stn3->name );
#endif
      BUG("during calculation of closure errors");
     }
    }
    FOLLOW_TRAV(stn3,k,i);
    stn1=stn3;
   } /* endwhile */

   img_write_datum( pimgOut, img_LINE, NULL,
                    POS(stn2,0),  POS(stn2,1),  POS(stn2,2)  );

#if PRINT_NETBITS
   printf("Reached fixed or non-2node station\n");
#endif
#if 0 /* no point testing - this is the exit condn now ... */
   if (stn3!=stn2 || k!=j) {
    print_prefix(stn3->name); printf(",%d NOT ",k);
    print_prefix(stn2->name); printf(",%d\n",j);
   }
#endif
   if (data_here(stn1->leg[i])) {
    if (!fZero(&stn1->leg[i]->v))
     fEquate=fFalse;
    lenTot+=sqrdd( stn1->leg[i]->d );
   } else {
    ASSERT2( data_here(stn2->leg[j]), "data not on either direction of leg");
    if (!fZero(&stn2->leg[j]->v))
     fEquate=fFalse;
    lenTot+=sqrdd( stn2->leg[j]->d );
   }
#if PRINT_NETBITS
   printf("lenTot increased okay\n");
#endif
   if (cLegsTrav) {
    if (stn2->name!=nmPrev) {
     fprint_prefix( fhErrStat, nmPrev );
#if PRINT_NAME_PTRS
     fprintf( fhErrStat, "[%p]", nmPrev );
#endif
     fputs( fEquate?szLinkEq:szLink, fhErrStat );
     if (!fEquate)
      cLegsTrav++;
    }
#if SHOW_INTERNAL_LEGS
    else
     fputc('+',fhErrStat);
#endif
    fprint_prefix( fhErrStat, stn2->name );
#if PRINT_NAME_PTRS
    fprintf( fhErrStat, "[%p]", stn2->name );
#endif
    lenTrav+=sqrt(lenTot);
   }
   if (cLegsTrav)
    err_stat(cLegsTrav,lenTrav,eTot,eTotTheo,hTot,hTotTheo,vTot,vTotTheo);
  }

  ptrOld=ptr;
  ptr=ptr->next;
  osfree(ptrOld);
 }

 /* leave fhErrStat open in case we're asked to close loops again */
#if 0
 fclose(fhErrStat);
#endif
}

static void err_stat( int cLegsTrav, double lenTrav,
                      double eTot, double eTotTheo,
                      double hTot, double hTotTheo,
                      double vTot, double vTotTheo ) {
 fputnl(fhErrStat);
 fprintf( fhErrStat, msg(145),
                          lenTrav,cLegsTrav,sqrt(eTot),sqrt(eTot)/cLegsTrav);
 if (lenTrav>0.0)
  fprintf( fhErrStat, msg(146),100*sqrt(eTot)/lenTrav);
 else
  fputs( msg(147), fhErrStat );
 fputnl(fhErrStat);
 fprintf(fhErrStat,"%f\n",sqrt(eTot/eTotTheo));
 fprintf(fhErrStat,"H: %f V: %f\n",sqrt(hTot/hTotTheo),sqrt(vTot/vTotTheo));
 fputnl(fhErrStat);
}

#if 0
static void deletenode(node *stn) {
 /* release any legs attached - reverse legs will be released since any */
 /* attached nodes aren't attached to fixed points */
 int d;
 for( d=0; d<=2; d++ )
  osfree(stn->leg[d]); /* ignored if NULL */
 /* could delete prefix now... but this (slight) fudge is easier... */
 stn->name->stn=NULL;
 /* and release node itself */
 osfree(stn);
}
#endif

static void replace_trailing_travs( void ) {
 stackTrail *ptrOld;
 node *stn1,*stn2;
 linkfor *leg;
 int  i,j;
 bool fNotAttached=fFalse;

 out_current_action(msg(128));

 while (ptrTrail!=NULL) {
  leg=ptrTrail->join1;
  leg=leg->l.to->leg[reverse_leg(leg)];
  stn1=leg->l.to;
  i=reverse_leg(leg);
#if PRINT_NETBITS
  printf(" Trailing trav "); print_prefix(stn1->name); printf("<%p>",stn1);
  printf("%s...\n",szLink); printf("    attachment stn is at (%f,%f,%f)\n",
  POS(stn1,0), POS(stn1,1), POS(stn1,2) );
#endif
  stn1->leg[i]=ptrTrail->join1;
/*  ASSERT(fixed(stn1)); */
  if (fixed(stn1)) {
   img_write_datum( pimgOut, img_MOVE, NULL,
                    POS(stn1,0),  POS(stn1,1),  POS(stn1,2)  );

   while (i!=BAD_DIRN) {
    stn2=stn1->leg[i]->l.to;
    j=reverse_leg(stn1->leg[i]);
    if (data_here(stn1->leg[i])) {
     adddd( &POSD(stn2), &POSD(stn1), &stn1->leg[i]->d );
#if 0
     printf("Adding leg (%f,%f,%f)\n",
            stn1->leg[i]->d[0],stn1->leg[i]->d[1],stn1->leg[i]->d[2]);
#endif
    } else {
     subdd( &POSD(stn2), &POSD(stn1), &stn2->leg[j]->d );
#if 0
     printf("Subtracting reverse leg (%f,%f,%f)\n",
            stn2->leg[j]->d[0],stn2->leg[j]->d[1],stn2->leg[j]->d[2]);
#endif
    }

    stn2->status=statFixed; /*stn1->status*/
/*   if (()!=statDudEq) */
    fix(stn2);
    img_write_datum( pimgOut, img_LINE, NULL,
                     POS(stn2,0),  POS(stn2,1),  POS(stn2,2)  );
    stn1=stn2;
    i=BAD_DIRN;
    FOLLOW_TRAV(stn2, j, i );
   }
  }

  ptrOld=ptrTrail;
  ptrTrail=ptrTrail->next;
  osfree(ptrOld);
 }

 /* write stations to .3d file and free legs and stations */
 for( stn1=stnlist ; stn1 ; stn1=stn2 ) {
  if (fixed(stn1)) {
   int d;
   img_write_datum( pimgOut, img_LABEL, sprint_prefix(stn1->name),
                    POS(stn1,0), POS(stn1,1), POS(stn1,2) );
   /* update coords of bounding box */
   for ( d=0 ; d<3 ; d++ ) {
    if (POS(stn1,d) < min[d]) { min[d]=POS(stn1,d); pfxLo[d]=stn1->name; }
    if (POS(stn1,d) > max[d]) { max[d]=POS(stn1,d); pfxHi[d]=stn1->name; }
   }
  } else {
   if (!fNotAttached) {
    fNotAttached=fTrue;
    error(45,NULL,NULL,0);
    out_info(msg(71));
   }
   out_info(sprint_prefix(stn1->name));
  }
  stn2=stn1->next;
  for( i=0 ; i<=2 ; i++ ) {
   linkfor *leg, *legRev;
   leg=stn1->leg[i];
   /* only want to think about forwards legs */
   if (leg && data_here(leg)) {
    node *stnB;
    int iB;
    stnB=leg->l.to;
    iB=reverse_leg(leg);
    legRev=stnB->leg[iB];
    ASSERT2(legRev->l.to==stn1,"leg doesn't reciprocate");
    if (fixed(stn1)) {
     /* check not an equating leg */
     if ( !fZero(&leg->v) ) {
      totadj+=sqrt( sqrd( POS(stnB,0) - POS(stn1,0) ) +
                    sqrd( POS(stnB,1) - POS(stn1,1) ) +
                    sqrd( POS(stnB,2) - POS(stn1,2) ) );
      total+=sqrt(sqrdd(leg->d));
      totplan+=sqrt(sqrd(leg->d[0])+sqrd(leg->d[1]));
      totvert+=fabs(leg->d[2]);
     }
    }
    osfree(leg);
    osfree(legRev);
    stn1->leg[i]=NULL;
    stnB->leg[iB]=NULL;
   }
  }
 }
 for( stn1=stnlist ; stn1 ; stn1=stn2 ) {
  stn2=stn1->next;
  stn1->name->stn=NULL;
  osfree(stn1);
 }
 stnlist=NULL;
}
