/* > netbits.h
 * Header file for miscellaneous primitive network routines for Survex
 * Copyright (C) 1994,1997 Olly Betts
 */

/*
1994.04.16 created
1994.05.07 added copy_link and addto_link
1997.08.22 added covariances
*/

extern node *StnFromPfx( prefix *name );

extern linkfor *copy_link( linkfor *leg );
extern linkfor *addto_link( linkfor *leg, linkfor *leg2 );

extern char freeleg( node **stnptr );

extern void addleg( node *fr, node *to,
                    real dx, real dy, real dz,
                    real vx, real vy, real vz,
                    real cyz, real czx, real cxy );

extern void addfakeleg( node *fr, node *to,
		        real dx, real dy, real dz,
		        real vx, real vy, real vz,
		        real cyz, real czx, real cxy );

#define FOR_EACH_STN(S) for( (S)=stnlist ; (S)!=NULL ; (S)=(S)->next )
#define print_prefix(N) fprint_prefix( stdout, (N) )

extern char *sprint_prefix( prefix *ptr );
extern void fprint_prefix( FILE *fh, prefix *ptr );

/* r = ab ; r,a,b are variance matrices */
void mulvv( var *r, var *a, var *b );

/* r = ab ; r,b delta vectors; a variance matrix */
void mulvd( d *r, var *a, d *b );

/* r = a + b ; r,a,b delta vectors */
void adddd( d *r, d *a, d *b );

/* r = a - b ; r,a,b delta vectors */
void subdd( d *r, d *a, d *b );

/* r = a + b ; r,a,b variance matrices */
void addvv( var *r, var *a, var *b );

/* r = a(b^-1) ; r,a delta vectors; b variance matrix */
void divdv( d *r, d *a, var *b );

/* r = a(b^-1) ; r,a,b variance matrices */
void divvv( var *r, var *a, var *b );

/* Is v zero? */
bool fZero( var *v );
