/* > listpos.c
 * SURVEX Cave surveying software: stuff to do with stn position output
 * Copyright (C) 1991-1995 Olly Betts
 */

/*
1993.02.18 split off from c.Main
1993.02.22 not a lot
1993.03.04 minor fettle to make source clearer
1993.04.08 added "error.h"
1993.04.22 out of memory -> error # 0
1993.04.26 now produce stn pos list in file
1993.06.05 minor fettle
1993.06.06 moved # directives to start in column 1 for crap C compilers
1993.06.16 malloc() -> osmalloc(); realloc() -> osrealloc()
           syserr() -> fatal()
1993.07.12 fopen( fnm, "wt" ); corrected - mode should be "w"
1993.08.09 altered data structure (OLD_NODES compiles previous version)
           fettled in changes to listpos.h (long since defunct)
           fettled a little
1993.08.11 NEW_NODES & POS(stn,d)
1993.08.15 changed prefix->stn->name to prefix (should always be the same)
1993.08.16 changed print_node_stats to send output to a stream
           output files can now be done with extensions
           added EXT_SVX_POS; suppressed 2 warnings
1993.11.03 changed error routines
1993.11.18 check for output file being directory
1993.11.19 bug-fix of directory code
1993.11.30 sorted out error vs fatal
1994.04.16 added netbits.h
1994.04.17 removed fprint_prefix proto
1994.04.18 created listpos.h
1994.04.27 fhPosList now static; lfPosList gone
1994.06.21 added int argument to warning, error and fatal
1994.08.27 modified so output goes through out_* functions
1994.08.29 fixed typo
1994.08.31 fputnl() ; extracted explicit "node" and "nodes" strings
1994.09.20 global buffer szOut created
1994.09.21 fputsnl introduced
1994.10.08 sizeof() corrected to ossizeof()
1994.10.24 turned off .pos file and node stats for now
1994.10.27 modified listpos -- now called from network.c
1994.10.31 p_pos no longer needs prefix->stn
           put node_stats back in
1994.10.13 fettled
1994.12.03 fhPosList is now a static here
           Only fixed stations go in the .pos file
           Added warning for unused fixed points
           Node-stats don't included unattached bits
1995.11.22 added Leandro's patch (#ifdef-ed as it'll confuse diffpos)
*/

#define PRINT_STN_POS_LIST 1
#define NODESTAT 1

#include "survex.h"
#include "filename.h"
#include "message.h"
#include "filelist.h"
#include "netbits.h"
#include "listpos.h"
#include "out.h"

static void p_pos( prefix* );
#if NODESTAT
static void node_stats(prefix *from);
static void node_stat(prefix *p);
#endif

#if NODESTAT
static int* cOrder;
static int  icOrderMac;
#endif

static FILE *fhPosList;

extern void list_pos( prefix *from ) {
 prefix *p;
 char *fnmPosList;
# ifdef NO_EXTENSIONS
 fnmPosList = UsePth( pthOutput, POSLIST_FILE );
# else
 fnmPosList = AddExt( fnmInput, EXT_SVX_POS );
# endif
 if (fDirectory(fnmPosList))
  fatal(44,wr,fnmPosList,0);
 else {
  fhPosList=fopen(fnmPosList,"w");
  if ( !fhPosList )
   fatal(47,wr,fnmPosList,0);
 }
 osfree(fnmPosList);
 fputs(msg(195),fhPosList); /* Output headings line */
 fputnl(fhPosList);
 p_pos(from);
 p=from->down;
 p_pos(p);
 while (p!=from) {
  if (p->down!=NULL)
   p=p->down;
  else {
   if (p->right!=NULL)
    p=p->right;
   else {
    do {
     p=p->up;
    } while (p->right==NULL && p!=from);
    if (p!=from)
     p=p->right;
   }
  }
  p_pos(p);
 }
 fclose(fhPosList);
}

void p_pos(prefix *p) {
#if PRINT_STN_POS_LIST
 /* check it's not just a stem node, and that the station is fixed */
 if (p->pos!=NULL && pfx_fixed(p)) {
#ifdef PRINT_STN_ORDER
  /* NB this patch (from Leandro Dybal Bertoni <LEANDRO@trieste.fapesp.br>)
   * will confuse diffpos a lot */
  fprintf(fhPosList,"%2d (%8.2f, %8.2f, %8.2f ) ",
          p->pos->shape, p->pos->p[0], p->pos->p[1], p->pos->p[2] );
#else
  fprintf(fhPosList,"(%8.2f, %8.2f, %8.2f ) ",
          p->pos->p[0], p->pos->p[1], p->pos->p[2] );
#endif
  fprint_prefix( fhPosList, p );
  fprintf(fhPosList,"\n");
 }
#endif
}

#if NODESTAT
extern void node_stats( prefix *from ) {
 prefix *p;
 icOrderMac=8; /* Start value */
 cOrder=osmalloc(icOrderMac*ossizeof(int));
 {
  int c;
  for( c=0; c<icOrderMac; c++ )
   cOrder[c]=0;
 }
 node_stat(from);
 p=from->down;
 node_stat(p);
 while (p!=from) {
  if (p->down!=NULL)
   p=p->down;
  else {
   if (p->right!=NULL)
    p=p->right;
   else {
    do {
     p=p->up;
    } while (p->right==NULL && p!=from);
    if (p!=from)
     p=p->right;
   }
  }
  node_stat(p);
 }
}

static void node_stat(prefix *p) {
 if (p && p->pos && pfx_fixed(p)) {
  int order;
  order=p->pos->shape;
  if (order==0)
   warning(73,wr,sprint_prefix(p),0); /* unused fixed pt */
  if (order>=icOrderMac) {
   int c;
   c=icOrderMac;
   do {
    c=c<<1;
   } while (c<=order);
   cOrder=osrealloc(cOrder,c*ossizeof(int));
   for( ; icOrderMac<c; icOrderMac++ )
    cOrder[icOrderMac]=0;
  }
  cOrder[order]++;
  /* printf(" order %d",order); */
 }
}
#endif

extern void print_node_stats( FILE *fh ) {
#if NODESTAT
 int c;
 node_stats(root);
 for( c=0; c<icOrderMac; c++)
  if (cOrder[c]>0) {
   sprintf( out_buf, "%4d %d-%s.", cOrder[c], c, msg(cOrder[c]==1 ? 176:177) );
   out_info(out_buf);
   fputsnl(out_buf,fh);
  }
#endif
}
