/* > new3dout.h
 * Header file for .3dx writing routines
 * Copyright (C) 2000, 2001 Phil Underwood
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef NEW3DOUT_H
# define NEW3DOUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "img.h"
   
struct Twig {
  struct Twig *up, *down, *right;
  struct Prefix *to, *from;
  delta delta;
  int count;
  short int sourceval;
  /* pointers to some random data bits... */
  char *date, *drawings, *tape, *instruments, *source;
};

/* Structures */
typedef struct Twig twig;

extern char *startingdir, *firstfilename;

/* these are the root and current part of the twig structure, respectively */
extern twig *rhizome, *limb;

extern int fUseNewFormat;

void cave_write_stn(node *);
void cave_write_leg(linkfor *);
void cave_write_source(const char *source);
void cave_write_title(const char *, img *);
img *cave_open_write(const char *, const char *);
void cave_close(img *);
int cave_error(void);
/* sets a new current limb */
void create_twig(prefix *pre, const char *fname);
/* returns the active twig of a prefix */
twig *get_twig(prefix *pre);

#ifdef __cplusplus
}
#endif
   
#endif
