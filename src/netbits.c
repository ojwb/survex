/* > netbits.c
 * Miscellaneous primitive network routines for Survex
 * Copyright (C) 1992-1997 Olly Betts
 */

/*
1992.07.27 First written
1993.01.25 PC Compatibility OKed
1993.02.19 altered leg->to & leg->reverse to leg->l.to & leg->l.reverse
           allocate linkrev for reverse leg (to save memory)
1993.03.11 node->in_network -> node->status
1993.04.08 "error.h"
1993.04.22 out of memory error -> error # 0
1993.06.16 malloc() -> osmalloc(); syserr() -> fatal()
1993.06.28 split addleg() into addleg() and addfakeleg()
           fettled indentation
1993.08.06 fixed Austria93 bug (but maybe not the Waddo 'magnifying glass'
            bug - tho' I haven't been able to reproduce it!)
1993.08.09 altered data structure (OLD_NODES compiles previous version)
           simplified StnFromPfx()
1993.08.10 warn legs with same station at both ends
           addleg() calls addfakeleg()
           introduced unfix()
1993.08.11 NEW_NODES
1993.08.13 fettled header
1993.11.03 changed error routines
1993.11.06 corrected puts to wr in commented out code
1993.11.08 FATAL5ARGS()
           only NEW_NODES code left
1993.11.11 fiddled with a bit
1993.12.01 probably time cLegs was long
1993.12.09 type link -> linkfor
1994.03.19 all error output -> stderr
1994.04.16 moved [sf]print_prefix to here from survex.c; created netbits.h
1994.04.18 fettling survex.h
1994.04.27 cLegs,cStns now in survex.h
1994.05.07 added copy_link and addto_link
           added mulXX, divXX, addXX, fZero
1994.05.08 fettled
1994.06.21 added int argument to warning, error and fatal
1994.06.30 tweaked code in case compiler can do tail recursion
1994.08.31 fputnl()
1994.09.02 all direct output fettled to use out
1994.09.03 fettled
1994.09.08 byte -> uchar
1994.09.20 global szOut buffer created
1994.10.08 StnFromPfx copes with prefixes with a pos but no stn
           osnew() created; fettled
1994.10.31 added prefix.shape
1995.10.11 fettled layout
1997.08.22 added covariances
1998.03.21 fixed up to compile cleanly on Linux
1998.04.28 invert_var now external and returns 0 if matrix is singular
1998.04.29 added subvv
*/

#include "debug.h"
#include "survex.h"
#include "filename.h"
#include "message.h"
#include "netbits.h"

/* Create (uses osmalloc) a forward leg containing the data in leg, or
 * the reversed data in the reverse of leg, if leg doesn't hold data
 */
extern linkfor *copy_link( linkfor *leg ) {
   linkfor *legOut;
   int d;
   legOut=osnew(linkfor);
   if (data_here(leg)) {
      for (d=2;d>=0;d--)
         legOut->d[d] = leg->d[d];
   } else {
      leg=leg->l.to->leg[reverse_leg(leg)];
      for (d=2;d>=0;d--)
         legOut->d[d] = -leg->d[d];
   }
#if 1
# ifndef NO_COVARIANCES
     {
	int i,j;
	for ( i = 0; i < 3 ; i++ ) {
	   for ( j = 0; j < 3 ; j++ ) {
	      legOut->v[i][j] = leg->v[i][j];
	   }
	}
     }
# else
   for (d=2;d>=0;d--)
      legOut->v[d] = leg->v[d];
# endif
#else
   memcpy( legOut->v, leg->v, sizeof (var));
#endif
   return legOut;
}

/* Adds to the forward leg `leg', the data in leg2, or the reversed data
 * in the reverse of leg2, if leg2 doesn't hold data
 */
extern linkfor *addto_link( linkfor *leg, linkfor *leg2 ) {
   if (data_here(leg2)) {
      adddd( &leg->d, &leg->d, &leg2->d );
   } else {
      leg2=leg2->l.to->leg[reverse_leg(leg2)];
      subdd( &leg->d, &leg->d, &leg2->d );
   }
   addvv( &leg->v, &leg->v, &leg2->v );
   return leg;
}

/* Add a leg between existing stations *fr and *to
 * If either node is a three node, then it is split into two
 * and the data structure adjusted as necessary
 */
void addleg( node *fr, node *to,
             real dx, real dy, real dz,
             real vx, real vy, real vz
#ifndef NO_COVARIANCES
	     , real cyz, real czx, real cxy
#endif
	    ) {
   cLegs++; /* increment count (first as compiler may do tail recursion) */
   addfakeleg(fr,to,dx,dy,dz,vx,vy,vz
#ifndef NO_COVARIANCES
	      ,cyz,czx,cxy
#endif
	      );
}

/* Add a 'fake' leg (not counted) between existing stations *fr and *to
 * If either node is a three node, then it is split into two
 * and the data structure adjusted as necessary
 */
void addfakeleg( node *fr, node *to,
		 real dx, real dy, real dz,
		 real vx, real vy, real vz
#ifndef NO_COVARIANCES
		 , real cyz, real czx, real cxy
#endif
		) {
   uchar i,j;
   linkfor *leg, *leg2;

   if (fr->name!=to->name) {
      leg=osnew(linkfor);
      leg2=(linkfor*)osnew(linkrev);

      i=freeleg(&fr);
      j=freeleg(&to);

      leg->l.to=to;
      leg2->l.to=fr;
      leg->d[0]=dx;leg->d[1]=dy;leg->d[2]=dz;
#ifndef NO_COVARIANCES
      leg->v[0][0]=vx;leg->v[1][1]=vy;leg->v[2][2]=vz;
      leg->v[0][1]=leg->v[1][0]=cxy;
      leg->v[1][2]=leg->v[2][1]=cyz;
      leg->v[2][0]=leg->v[0][2]=czx;
#else
      leg->v[0]=vx;leg->v[1]=vy;leg->v[2]=vz;
#endif
      leg2->l.reverse=i;
      leg->l.reverse=(j | 0x80);

      fr->leg[i]=leg;
      to->leg[j]=leg2;

      fr->name->pos->shape++;
      to->name->pos->shape++;

      return;
   }

   /* we have been asked to add a leg with the same node at both ends
    * - give a warning and don't add the leg to the data structure
    */
   warning(50,wr,sprint_prefix(fr->name),0);
}

char freeleg(node **stnptr) {
   node *stn, *oldstn;
   linkfor *leg, *leg2;
   int i, j;

   stn=*stnptr;

   if (stn->leg[0]==NULL)
      return 0; /* leg[0] unused */
   if (stn->leg[1]==NULL)
      return 1; /* leg[1] unused */
   if (stn->leg[2]==NULL)
      return 2; /* leg[2] unused */

   /* All legs used, so split node in two */
   oldstn=stn;
   stn=osnew(node);
   leg=osnew(linkfor);
   leg2=(linkfor*)osnew(linkrev);

   *stnptr=stn;

   stn->next=stnlist;
   stnlist=stn;
   stn->name=oldstn->name;
   stn->status=oldstn->status;

   leg->l.to=stn;
   leg->d[0]=leg->d[1]=leg->d[2]=(real)0.0;

#ifndef NO_COVARIANCES
   for ( i = 0; i < 3 ; i++ ) {
      for ( j = 0; j < 3 ; j++ ) {
	 leg->v[i][j] = (real)0.0;
      }
   }
#else
   leg->v[0]=leg->v[1]=leg->v[2]=(real)0.0;
#endif
   leg->l.reverse=1 | 0x80;

   leg2->l.to=oldstn;
   leg2->l.reverse=0;

   stn->leg[0]=oldstn->leg[0];
   /* correct reverse leg */
   stn->leg[0]->l.to->leg[reverse_leg(stn->leg[0])]->l.to=stn;
   stn->leg[1]=leg2;

   oldstn->leg[0]=leg;

   stn->leg[2]=NULL; /* needed as stn->leg[dirn]==NULL indicates unused */

   return(2); /* leg[2] unused */
}

node *StnFromPfx( prefix *name ) {
   node *stn;
   if (name->stn!=NULL)
      return (name->stn);
   stn=osnew(node);
   stn->name=name;
   if (name->pos==NULL) {
      name->pos=osnew(pos);
      name->pos->shape=0;
      unfix(stn);
   }
   stn->status=statInNet;
   stn->leg[0]=NULL;
   stn->leg[1]=NULL;
   stn->leg[2]=NULL;
   stn->next=stnlist;
   stnlist=stn;
   name->stn=stn;
   cStns++;
   return stn;
}

extern void fprint_prefix( FILE *fh, prefix *ptr ) {
   if (ptr->up!=NULL) {
      fprint_prefix( fh, ptr->up );
      if (ptr->up->up!=NULL)
         fputc('.',fh);
      fputs( ptr->ident, fh );
   } else {
      fputc('\\',fh);
   }
}

static char *sprint_prefix_( char *sz, prefix *ptr ) {
   int i;
   if (ptr->up != NULL) {
      sz = sprint_prefix_( sz, ptr->up );
      if (ptr->up->up != NULL)
         *sz++ = '.';
      i = strlen( ptr->ident );
      memcpy( sz, ptr->ident, i );
      sz += i;
   } else {
      *sz++ = '\\';
   }
   *sz = '\0';
   return sz;
}

extern char *sprint_prefix( prefix *ptr ) {
   static char sz[512];
   sprint_prefix_( sz, ptr );
   return sz;
}

#ifndef NO_COVARIANCES
static void print_var( const var *a ) {
   printf( "/ %4.2f, %4.2f, %4.2f \\\n", (*a)[0][0], (*a)[0][1], (*a)[0][2] );
   printf( "| %4.2f, %4.2f, %4.2f |\n", (*a)[1][0], (*a)[1][1], (*a)[1][2] );
   printf( "\\ %4.2f, %4.2f, %4.2f /\n", (*a)[2][0], (*a)[2][1], (*a)[2][2] );
}
#endif
		 
/* r = ab ; r,a,b are variance matrices */
void mulvv( var *r, const var *a, const var *b ) {
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] * (*b)[0];
   (*r)[1] = (*a)[1] * (*b)[1];
   (*r)[2] = (*a)[2] * (*b)[2];
#else
   int i, j, k;
   real tot;

   for ( i = 0; i < 3; i++ ) {
      for ( j = 0; j < 3; j++ ) {
	 tot = 0;
	 for ( k = 0; k < 3; k++ ) {
	    tot += (*a)[i][k] * (*b)[k][j];
	 }
	 (*r)[i][j] = tot;
      }
   }
#endif
}

/* r = ab ; r,b delta vectors; a variance matrix */
void mulvd( d *r, var *a, d *b ) {
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] * (*b)[0];
   (*r)[1] = (*a)[1] * (*b)[1];
   (*r)[2] = (*a)[2] * (*b)[2];
#else
   int i, k;
   real tot;

   for ( i = 0; i < 3; i++ ) {
      tot = 0;
      for ( k = 0; k < 3; k++ ) {
	 tot += (*a)[i][k] * (*b)[k];
      }
      (*r)[i] = tot;
   }
#endif
}

/* r = a + b ; r,a,b delta vectors */
void adddd( d *r, d *a, d *b ) {
   (*r)[0] = (*a)[0] + (*b)[0];
   (*r)[1] = (*a)[1] + (*b)[1];
   (*r)[2] = (*a)[2] + (*b)[2];
}

/* r = a - b ; r,a,b delta vectors */
void subdd( d *r, d *a, d *b ) {
   (*r)[0] = (*a)[0] - (*b)[0];
   (*r)[1] = (*a)[1] - (*b)[1];
   (*r)[2] = (*a)[2] - (*b)[2];
}

/* r = a + b ; r,a,b variance matrices */
void addvv( var *r, var *a, var *b ) {
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] + (*b)[0];
   (*r)[1] = (*a)[1] + (*b)[1];
   (*r)[2] = (*a)[2] + (*b)[2];
#else
   int i,j;

   for ( i = 0; i < 3; i++ )
     for ( j = 0; j < 3; j++ )
       (*r)[i][j] = (*a)[i][j] + (*b)[i][j];
#endif
}

/* r = a - b ; r,a,b variance matrices */
void subvv( var *r, var *a, var *b ) {
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] - (*b)[0];
   (*r)[1] = (*a)[1] - (*b)[1];
   (*r)[2] = (*a)[2] - (*b)[2];
#else
   int i,j;

   for ( i = 0; i < 3; i++ )
     for ( j = 0; j < 3; j++ )
       (*r)[i][j] = (*a)[i][j] - (*b)[i][j];
#endif
}

#ifndef NO_COVARIANCES
/* inv = v^-1 ; inv,v variance matrices */
extern int invert_var( var *inv, const var *v ) {
   int i,j;
   real det = 0;

   for ( i = 0; i < 3; i++ ) {
      /* !HACK! this is just wrong... */
/*      det += (*v)[i][0] * (*v)[(i+1)%3][1] * (*v)[(i+2)%3][2];*/
      det += (*v)[i][0] * ( (*v)[(i+1)%3][1] * (*v)[(i+2)%3][2]
			   - (*v)[(i+1)%3][2] * (*v)[(i+2)%3][1] );
/*      if (i != 1) det += part; else det -= part; */
   }

   if (fabs(det) < 1E-10) {
      printf("det=%f\n",det);
      return 0; /* matrix is singular !HACK! use epsilon */
   }

   det = 1/det;

#define B(I,J) ((*v)[(J)%3][(I)%3])
   for ( i = 0; i < 3; i++ ) {
      for ( j = 0; j < 3; j++ ) {
         (*inv)[i][j] = det * (B(i+1,j+1)*B(i+2,j+2) - B(i+2,j+1)*B(i+1,j+2));
/*         if ((i+j) % 2) (*inv)[i][j] = -((*inv)[i][j]);*/
      }
   }
#undef B
   
     {
	var p;
	real d = 0;
	mulvv( &p, v, inv );
	for ( i = 0; i < 3; i++ ) {
	   for ( j = 0; j < 3; j++ ) {
	      d += fabs(p[i][j] - (real)(i==j));
	   }
	}
	if (d > 1E-5) {
	   printf("original * inverse=\n");
	   print_var( v );
	   printf("*\n");
	   print_var( inv );
	   printf("=\n");
	   print_var( &p );
	   BUG("matrix didn't invert");
	}
     }

   return 1;
}
#endif

/* r = (b^-1)a ; r,a delta vectors; b variance matrix */
void divdv( d *r, d *a, var *b ) {
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] / (*b)[0];
   (*r)[1] = (*a)[1] / (*b)[1];
   (*r)[2] = (*a)[2] / (*b)[2];
#else
   var b_inv;
   if (!invert_var( &b_inv, b )) {
      print_var( b );
      BUG("covariance matrix is singular");
   }
   mulvd( r, &b_inv, a );
#endif
}

/* f = a(b^-1) ; r,a,b variance matrices */
void divvv( var *r, var *a, var *b ) {
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] / (*b)[0];
   (*r)[1] = (*a)[1] / (*b)[1];
   (*r)[2] = (*a)[2] / (*b)[2];
#else
   var b_inv;
   if (!invert_var( &b_inv, b )) {
      print_var( b );
      BUG("covariance matrix is singular");
   }
   mulvv( r, a, &b_inv );
#endif
}

bool fZero( var *v ) {
#ifdef NO_COVARIANCES
   /* variance-only version */
   return ( (*v)[0]==0.0 && (*v)[1]==0.0 && (*v)[2]==0.0 );
#else
   int i,j;

   for ( i = 0; i < 3; i++ )
      for ( j = 0; j < 3; j++ )
	 if ((*v)[i][j] != 0.0) return fFalse;

   return fTrue;
#endif
}
