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

/* insert at head of double-linked list */
void add_stn_to_list(node **list, node *stn);

/* remove from double-linked list */
void remove_stn_from_list(node **list, node *stn);

/* one node must only use leg[0] */
#define one_node(S) (!(S)->leg[1] && (S)->leg[0])

/* two node must only use leg[0] and leg[1] */
#define two_node(S) (!(S)->leg[2] && (S)->leg[1])

/* three node iff it uses leg[2] */
#define three_node(S) ((S)->leg[2])

/* NB FOR_EACH_STN() can't be nested - but it's hard to police as we can't
 * easily set stn_iter to NULL if the loop is exited with break */

/* Need stn_iter so we can adjust iterator if the stn it points to is deleted */
extern node *stn_iter;
#define FOR_EACH_STN(S,L) \
 for (stn_iter = (L); ((S) = stn_iter);\
 stn_iter = ((S) == stn_iter) ? stn_iter->next : stn_iter)

#define print_prefix(N) fprint_prefix(stdout, (N))

char *sprint_prefix(prefix *ptr);
void fprint_prefix(FILE *fh, prefix *ptr);

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
bool fZero(const var *v);

#ifdef NO_COVARIANCES
# define print_var(V) printf("(%f,%f,%f)", (V)[0], (V)[1], (V)[2])
#else
# define print_var(V) \
printf("/%4.2f,%4.2f,%4.2f\\\n|%4.2f,%4.2f,%4.2f|\n\\%4.2f,%4.2f,%4.2f/\n",\
(V)[0][0], (V)[0][1], (V)[0][2],\
(V)[1][0], (V)[1][1], (V)[1][2],\
(V)[2][0], (V)[2][1], (V)[2][2])
#endif

