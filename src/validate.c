/* > validate.c
 *
 *   Checks that SURVEX's data structures are valid and consistent
 *
 *   NB The checks currently done aren't very comprehensive - more will be
 *    added if bugs require them
 *
 *   Copyright (C) 1993,1994,1996 Olly Betts
 */

/*
1993.08.09 created
1993.08.10 added range checking of positions of fixed stations
           use of fFixed corrected to fixed()
           added 'dump_network'
1993.08.11 changed to work with NEW_NODES stuctures (only :( )
           use POS(stn,d)
           FOR_EACH_STN( stn ) does for loop with stn looping thru' all nodes
1994.04.16 dump_network -> dump_entire_network; dump_network shows nodes
            currently in network
           added netbits.h
1994.04.17 removed fprint_prefix proto
1994.04.18 fettled survex.h
1994.05.10 added checks on leg deltas > MAX_POS and that data is on exactly
            one of leg and reciprocating leg
           added check for innetworkness leaks
           added dump_node
1994.06.20 created validate.h
1994.08.14 added brackets around fn names to prevent macro hassles
1994.11.16 leaf prefixes without station pointers are ok now
1996.04.01 corrected %lf to %g (thanks to gcc for spotting it)
1996.04.12 increased MAX_POS
1996.04.15 moved comment-out check_fixed() here
*/

#include "survex.h"
#include "error.h"
#include "netbits.h"
#include "validate.h"

/* maximum absolute value allowed for a coordinate of a fixed station */
#define MAX_POS 1000000.0

static bool validate_prefix_tree( void );
static bool validate_prefix_subtree( prefix *pfx );

static bool validate_station_list( void );

#if 0
extern void check_fixed(void) {
  /* not a requirement -- we allow hanging sections of survey
   * which get spotted and removed */
  node *stn;
  printf("*** Checking fixed-ness\n");
  FOR_EACH_STN(stn) {
     if (stn->status && !fixed(stn)) {
        printf("*** Station '");
        print_prefix(stn->name);
        printf("' has status %d but isn't fixed\n",stn->status);
     }
  }
}
#endif

extern bool (validate)( void )
 {
 bool fOk=fTrue;
 fOk&=validate_prefix_tree();
 fOk&=validate_station_list();
 if (fOk)
  puts("*** Data structures passed consistency checks");
 return fOk;
 }

static bool validate_prefix_tree( void )
 {
 bool fOk=fTrue;
 if (root->up!=NULL)
  { printf("*** root->up == %p\n",root->up); fOk=fFalse; }
 if (root->right!=NULL)
  { printf("*** root->right == %p\n",root->right); fOk=fFalse; }
 if (root->stn!=NULL)
  { printf("*** root->stn == %p\n",root->stn); fOk=fFalse; }
 if (root->pos!=NULL)
  { printf("*** root->pos == %p\n",root->pos); fOk=fFalse; }
 fOk&=validate_prefix_subtree(root);
 return fOk;
 }

static bool validate_prefix_subtree( prefix *pfx )
 {
 bool fOk=fTrue;
 prefix *pfx2;
 pfx2=pfx->down;
/* this happens now, as nodes are freed after solving */
#if 0
 if (pfx2==NULL)
  {
  if (pfx->stn==NULL)
   {
   printf("*** Leaf prefix '");
   print_prefix(pfx);
   printf("' has no station attached\n");
   fOk=fFalse;
   }
  return fOk;
  }
#endif

 while (pfx2!=NULL)
  {
  if (pfx2->stn!=NULL && pfx2->stn->name!=pfx2)
   {
   printf("*** Prefix '"); print_prefix(pfx2);
   printf("' ->stn->name is '"); print_prefix(pfx2->stn->name);
   printf("'\n");
   fOk=fFalse;
   }
  if (pfx2->up!=pfx)
   {
   printf("*** Prefix '"); print_prefix(pfx2);
   printf("' ->up is '"); print_prefix(pfx);
   printf("'\n");
   fOk=fFalse;
   }
  fOk&=validate_prefix_subtree(pfx2);
  pfx2=pfx2->right;
  }
 return fOk;
 }

static bool validate_station_list( void )
 {
 bool fOk=fTrue;
 node *stn, *stn2;
 int d,d2;
 FOR_EACH_STN( stn )
  {
  for ( d=0 ; d<=2 ; d++ )
   {
   if (stn->leg[d]!=NULL)
    {
    stn2=stn->leg[d]->l.to;
    if (stn->status && !stn2->status)
     {
     printf("*** Station '");
     print_prefix(stn->name);
     printf("' has status %d and connects to '",stn->status);
     print_prefix(stn2->name);
     printf("' which has status %d\n",stn2->status);
     fOk=fFalse;
     }
    d2=reverse_leg(stn->leg[d]);
    if (stn2->leg[d2]==NULL)
     {
     /* this is fine iff stn is at the disconnected end of a fragment */
     if (stn->status!=statRemvd)
      {
      printf("*** Station '");
      print_prefix(stn->name);
      printf("', leg %d doesn't reciprocate from station '",d);
      print_prefix(stn2->name);
      printf("'\n");
      fOk=fFalse;
      }
     }
    else if (stn2->leg[d2]->l.to!=stn)
     {
     /* this is fine iff stn is at the disconnected end of a fragment */
     if (stn->status!=statRemvd)
      {
      printf("*** Station '");
      print_prefix(stn->name);
      printf("', leg %d reciprocates via station '",d);
      print_prefix(stn2->name);
      printf("' to station '");
      print_prefix(stn2->leg[d2]->l.to->name);
      printf("'\n");
      fOk=fFalse;
      }
     }
    else
     {
     if ((data_here(stn->leg[d])!=0) ^ (data_here(stn2->leg[d2])==0))
      {
      printf("*** Station '");
      print_prefix(stn->name);
      printf("', leg %d reciprocates via station '",d);
      print_prefix(stn2->name);
      if (data_here(stn->leg[d]))
       printf("' - data on both legs\n");
      else
       printf("' - data on neither leg\n");
      fOk=fFalse;
      }
     }
    if (data_here(stn->leg[d]))
     {
     int i;
     for( i=0 ; i<3 ; i++ )
      if (fabs(stn->leg[d]->d[i])>MAX_POS)
       {
       printf("*** Station '");
       print_prefix(stn->name);
       printf("', leg %d, d[%d] = %g\n",d,i,(double)(stn->leg[d]->d[i]));
       fOk=fFalse;
       }
     }
    }

   if (fixed(stn))
    {
    if (fabs(POS(stn,0))>MAX_POS || fabs(POS(stn,1))>MAX_POS
                                                || fabs(POS(stn,2))>MAX_POS)
     {
     printf("*** Station '");
     print_prefix(stn->name);
     printf("' fixed at coords (%f,%f,%f)\n",
                                       POS(stn,0), POS(stn,1), POS(stn,2) );
     fOk=fFalse;
     }
    }

   }
  }
 return fOk;
 }

extern void (dump_node)(node *stn)
 {
 int d;
 if (stn->name!=NULL)
  print_prefix(stn->name);
 else
  printf("<null>");
 printf(" stn [%p] name (%p) status %d %sfixed\n",
                          stn, stn->name, stn->status, fixed(stn)?"":"un" );
 for( d=0 ; d<=2 ; d++ )
  {
  if (stn->leg[d]!=NULL)
   printf("  leg %d -> stn [%p]\n",d,stn->leg[d]->l.to);
  }
 }

extern void (dump_entire_network)( void )
 {
 node *stn;
 FOR_EACH_STN( stn )
  dump_node(stn);
 }

extern void (dump_network)( void )
 {
 node *stn;
 int d;
 printf("Nodes with status!=statRemvd\n");
 FOR_EACH_STN( stn )
  {
  if (stn->status)
   {
   printf("stn [%p] name (%p) status %d %sfixed\n",
                          stn, stn->name, stn->status, fixed(stn)?"":"un" );
   for( d=0 ; d<=2 ; d++ )
    {
    if (stn->leg[d]!=NULL)
     printf("  leg %d -> stn [%p]\n",d,stn->leg[d]->l.to);
    }
   }
  }
 }
