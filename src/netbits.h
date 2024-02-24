/* netbits.h
 * Header file for miscellaneous primitive network routines for Survex
 * Copyright (C) 1994,1997,1998,2001,2006,2015 Olly Betts
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

#include <stdbool.h>

void clear_last_leg(void);

node *StnFromPfx(prefix *name);

linkfor *copy_link(linkfor *leg);
linkfor *addto_link(linkfor *leg, const linkfor *leg2);

void addlegbyname(prefix *fr_name, prefix *to_name, bool fToFirst,
		  real dx, real dy, real dz, real vx, real vy, real vz
#ifndef NO_COVARIANCES
		  , real cyz, real czx, real cxy
#endif
		  );

void process_equate(prefix *name1, prefix *name2);

void addfakeleg(node *fr, node *to,
		real dx, real dy, real dz, real vx, real vy, real vz
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
 for (stn_iter = (L); ((S) = stn_iter) != NULL;\
 stn_iter = ((S) == stn_iter) ? stn_iter->next : stn_iter)

#define print_prefix(N) fprint_prefix(stdout, (N))

char *sprint_prefix(const prefix *ptr);
void fprint_prefix(FILE *fh, const prefix *ptr);

/* r = ab ; r,a,b are variance matrices */
void mulss(var *r, /*const*/ svar *a, /*const*/ svar *b);

#ifdef NO_COVARIANCES
/* In the NO_COVARIANCES case, v and s are the same so we only need one
 * version. */
# define smulvs(R,A,B) mulss(R,A,B)
#else
/* r = ab ; r,a,b are variance matrices */
void smulvs(svar *r, /*const*/ var *a, /*const*/ svar *b);
#endif

/* r = ab ; r,b delta vectors; a variance matrix */
void mulsd(delta *r, /*const*/ svar *a, /*const*/ delta *b);

/* r = ca ; r,a variance matrices; c real scaling factor  */
void mulsc(svar *r, /*const*/ svar *a, real c);

/* r = a + b ; r,a,b delta vectors */
void adddd(delta *r, /*const*/ delta *a, /*const*/ delta *b);

/* r = a - b ; r,a,b delta vectors */
void subdd(delta *r, /*const*/ delta *a, /*const*/ delta *b);

/* r = a + b ; r,a,b variance matrices */
void addss(svar *r, /*const*/ svar *a, /*const*/ svar *b);

/* r = a - b ; r,a,b variance matrices */
void subss(svar *r, /*const*/ svar *a, /*const*/ svar *b);

/* r = (b^-1)a ; r,a delta vectors; b variance matrix */
void divds(delta *r, /*const*/ delta *a, /*const*/ svar *b);

/* inv = v^-1 ; inv,v variance matrices */
int invert_svar(svar *inv, /*const*/ svar *v);

/* Is v zero? */
bool fZeros(/*const*/ svar *v);

#define PR "%8.6f"

#ifdef NO_COVARIANCES
# define print_var(V) printf("("PR","PR","PR")\n", (V)[0], (V)[1], (V)[2])
# define print_svar(V) printf("("PR","PR","PR")\n", (V)[0], (V)[1], (V)[2])
#else
# define print_var(V) \
printf("/"PR","PR","PR"\\\n|"PR","PR","PR"|\n\\"PR","PR","PR"/\n",\
(V)[0][0], (V)[0][1], (V)[0][2],\
(V)[1][0], (V)[1][1], (V)[1][2],\
(V)[2][0], (V)[2][1], (V)[2][2])
# define print_svar(V) \
printf("/"PR","PR","PR"\\\n:"PR","PR","PR":\n\\"PR","PR","PR"/\n",\
(V)[0], (V)[3], (V)[4],\
(V)[3], (V)[1], (V)[5],\
(V)[4], (V)[5], (V)[2])
#endif

#define print_d(D) printf("("PR","PR","PR")", (D)[0], (D)[1], (D)[2])
