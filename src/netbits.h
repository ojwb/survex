/* > netbits.h
 * Header file for miscellaneous primitive network routines for Survex
 * Copyright (C) 1994,1997,1998 Olly Betts
 */

extern node *StnFromPfx(prefix *name);

extern linkfor *copy_link(linkfor *leg);
extern linkfor *addto_link(linkfor *leg, linkfor *leg2);

extern char freeleg(node **stnptr);

extern void addleg(node *fr, node *to,
		   real dx, real dy, real dz,
		   real vx, real vy, real vz
#ifndef NO_COVARIANCES
		   , real cyz, real czx, real cxy
#endif
		   );

extern void addfakeleg(node *fr, node *to,
		       real dx, real dy, real dz,
		       real vx, real vy, real vz
#ifndef NO_COVARIANCES
		       , real cyz, real czx, real cxy
#endif
		       );

#define FOR_EACH_STN(S) for((S) = stnlist; (S) != NULL; (S) = (S)->next)
#define print_prefix(N) fprint_prefix(stdout, (N))

char *sprint_prefix(prefix *ptr);
void fprint_prefix(FILE *fh, prefix *ptr);

/* extern for debugging */
/* extern void print_var(var *a); */

/* r = ab ; r,a,b are variance matrices */
void mulvv(var *r, const var *a, const var *b);

/* r = ab ; r,b delta vectors; a variance matrix */
void mulvd(d *r, var *a, d *b);

/* r = a + b ; r,a,b delta vectors */
void adddd(d *r, d *a, d *b);

/* r = a - b ; r,a,b delta vectors */
void subdd(d *r, d *a, d *b);

/* r = a + b ; r,a,b variance matrices */
void addvv(var *r, var *a, var *b);

/* r = a - b ; r,a,b variance matrices */
void subvv(var *r, var *a, var *b);

/* r = a(b^-1) ; r,a delta vectors; b variance matrix */
void divdv(d *r, d *a, var *b);

/* r = a(b^-1) ; r,a,b variance matrices */
void divvv(var *r, var *a, var *b);

#ifndef NO_COVARIANCES
/* inv = v^-1 ; inv,v variance matrices */
int invert_var(var *inv, const var *v);
#endif

/* Is v zero? */
bool fZero(var *v);
