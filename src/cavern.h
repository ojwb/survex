/* > cavern.h
 * SURVEX Cave surveying software - header file
 * Copyright (C) 1991-2000 Olly Betts
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

#ifndef CAVERN_H
#define CAVERN_H

/* Using covariances increases the memory required somewhat - may be
 * desirable to disable this for small memory machines */

/* #define NO_COVARIANCES 1 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>

#include "useful.h"
#include "img.h"

/* Set EXPLICIT_FIXED_FLAG to 1 to force an explicit fixed flag to be used
 * in each pos struct, rather than using p[0]==UNFIXED_VAL to indicate
 * unfixed-ness.  This may be slightly faster, but uses more memory.
 */
#ifndef EXPLICIT_FIXED_FLAG
# define EXPLICIT_FIXED_FLAG 0
#endif

typedef double real; /* so we can change the precision used easily */
#define HUGE_REAL HUGE_VAL
/* this is big, but not so big we can't negate it safely -- not very nice,
 * and possibly unnecessary - need to recheck the standard */
#define REAL_BIG (DBL_MAX/20.0)
/* RISC OS FP emulation is broken, and the reported epsilon value isn't
 * useful in practice, so we fake it in the makefile */
#ifndef REAL_EPSILON
# define REAL_EPSILON DBL_EPSILON
#endif

#if (!EXPLICIT_FIXED_FLAG)
# define UNFIXED_VAL HUGE_VAL /* if p[0]==UNFIXED_VAL, station is unfixed */
#endif

#define NOT_YET fatalerror(99) /* FIXME: say what's not implemented? */

#define SPECIAL_EOL       0x001
#define SPECIAL_BLANK     0x002
#define SPECIAL_KEYWORD   0x004
#define SPECIAL_COMMENT   0x008
#define SPECIAL_OMIT      0x010
#define SPECIAL_ROOT      0x020
#define SPECIAL_SEPARATOR 0x040
#define SPECIAL_NAMES     0x080
#define SPECIAL_DECIMAL   0x100
#define SPECIAL_MINUS     0x200
#define SPECIAL_PLUS      0x400

#define MAX_NO_OF_SDs 5

extern char *fnm_output_base;
extern int fnm_output_base_is_dir;

/* Types */

typedef enum {
   Q_NULL = -1, Q_DEFAULT, Q_LENGTH, Q_DEPTH,
   Q_DX, Q_DY, Q_DZ, Q_LENGTHOUTPUT,
   Q_COUNT, Q_BEARING, Q_ANGLEOUTPUT,
   Q_GRADIENT, Q_DECLINATION, Q_POS, Q_PLUMB, Q_LEVEL, Q_MAC
} q_quantity;

/* unsigned long to cope with 16-bit int-s */
#define BIT(N) (1UL << (N))
#define BITA(N) (1UL << ((N) - 'a'))

/* masks for quantities which are length and angles respectively */
#define LEN_QMASK (BIT(Q_LENGTH) | BIT(Q_DEPTH) |\
   BIT(Q_DX) | BIT(Q_DY) | BIT(Q_DZ) | BIT(Q_POS) |\
   BIT(Q_COUNT) | BIT(Q_LENGTHOUTPUT))
#define ANG_QMASK (BIT(Q_BEARING) | BIT(Q_GRADIENT) | BIT(Q_PLUMB) |\
   BIT(Q_LEVEL) | BIT(Q_DECLINATION) | BIT(Q_ANGLEOUTPUT))

typedef enum {
   UNITS_NULL = -1, UNITS_METRES, UNITS_FEET, UNITS_YARDS,
   UNITS_DEGS, UNITS_GRADS, UNITS_PERCENT, UNITS_MINUTES, UNITS_MAC
} u_units;
/* if you add/change the order, check factor_tab in commands.c */

/* enumeration of field types */
typedef enum {
   End = 0, Fr, To, Tape, Comp, Clino, BackComp, BackClino,
   FrDepth, ToDepth, Dx, Dy, Dz, FrCount, ToCount, Dr,
#ifdef SVX_MULTILINEDATA
   Next, Back,
#endif
   Ignore, IgnoreAll
} datum;

/* assert(IgnoreAll<32); */
/* Dr, Comp, Dz give CYLPOL style */
/* Cope with any combination which gives enough info ??? */
/* typedef enum {NORMAL,CARTES,DIVING,TOPFIL,CYLPOL} style; */

/* type of a function to read in some data style */
#ifdef SVX_MULTILINEDATA
typedef int (*style)(int);
#else
typedef int (*style)(void);
#endif

/* Structures */

/* station name */
typedef struct Prefix {
   struct Prefix *up, *down, *right;
   struct Node *stn;
   struct Pos *pos;
   char *ident;
   const char *filename;
   unsigned int line;
} prefix;

/* variance */
#ifdef NO_COVARIANCES
typedef real var[3];
#else
typedef real var[3][3];
#endif

/* position or length vector */
typedef real d[3];

/* stuff stored for both forward & reverse legs */
typedef struct {
   struct Node *to;
   /* bits 0..1 = reverse leg number; bit7 is fFullLeg */
   /* bit6 = fReplacementLeg (by reduction rules) */
   uchar reverse;
} linkcommon;

/* reverse leg - deltas & vars stored on other dirn */
typedef struct LinkRev {
   linkcommon l;
} linkrev;

/* forward leg - deltas & vars stored here */
typedef struct Link {
   linkcommon l;
   d d; /* Delta */
   var v; /* Variances */
} linkfor;

/* node - like a station, except several nodes are used to represent a
 * station with more than 3 legs connected to it
 */
typedef struct Node {
   struct Prefix *name;
   struct Link *leg[3];
   struct Node *prev, *next;
   long colour;
} node;

/* station position */
typedef struct Pos {
   d p; /* Position */
   int shape;
#if EXPLICIT_FIXED_FLAG
   uchar fFixed; /* flag indicating if station is a fixed point */
#endif
} pos;

#define STYLE_DEFAULT -1
#define STYLE_UNKNOWN 0
#define STYLE_NORMAL  1
#define STYLE_DIVING  2
/*#define STYLE_MAX     2*/

/*
typedef struct Inst {
   real zero, scale, units;
} inst;
*/

/* various settings preserved by *BEGIN and *END */
typedef struct Settings {
   int Truncate;
/*   bool f0Eq;*/
   bool f90Up;
   enum {OFF, LOWER, UPPER} Case;
   style Style;
   prefix *Prefix;
   prefix *tag; /* used to check BEGIN/END tags match */
   short *Translate; /* if short is >= 16 bits, which ANSI requires */
   real Var[Q_MAC];
   real z[Q_MAC];
   real sc[Q_MAC];
   real units[Q_MAC];   
   datum *ordering;
   int begin_lineno; /* 0 means no block started in this file */
   struct Settings *next;
} settings;

/* global variables */
extern settings *pcs;
extern prefix *root;
extern node *stnlist;
extern unsigned long optimize;

extern char *survey_title;
extern int survey_title_len;

extern bool fExplicitTitle;
extern long cLegs, cStns, cComponents;
extern FILE *fhErrStat;
/* extern FILE *fhPosList; */
extern img *pimgOut;
extern char out_buf[];
#ifndef NO_PERCENTAGE
extern bool fPercent;
#endif
extern real totadj, total, totplan, totvert;
extern real min[3], max[3];
extern prefix *pfxHi[3], *pfxLo[3];
extern bool fAscii;

/* macros */

#define POS(S, D) ((S)->name->pos->p[(D)])
#define POSD(S) ((S)->name->pos->p)
#define USED(S, L) ((S)->leg[(L)] != NULL)

#define FLAG_DATAHERE 128
#define FLAG_REPLACEMENTLEG 64

#define data_here(L) ((L)->l.reverse & FLAG_DATAHERE)
#define reverse_leg_dirn(L) ((L)->l.reverse & 0x03)
#define reverse_leg(L) ((L)->l.to->leg[((L)->l.reverse & 0x03)])

#if EXPLICIT_FIXED_FLAG
# define pfx_fixed(N) ((N)->pos->fFixed)
# define pos_fixed(P) ((P)->fFixed)
# define fix(S) (S)->name->pos->fFixed = (char)fTrue
# define fixpos(P) (P)->fFixed = (char)fTrue
# define unfix(S) (S)->name->pos->fFixed = (char)fFalse
#else
# define pfx_fixed(N) ((N)->pos->p[0] != UNFIXED_VAL)
# define pos_fixed(P) ((P)->p[0] != UNFIXED_VAL)
# define fix(S) NOP
# define fixpos(P) NOP
# define unfix(S) POS((S), 0) = UNFIXED_VAL
#endif
#define fixed(S) pfx_fixed((S)->name)

/* macros/fns for special chars */

#define isEol(c)    (pcs->Translate[(c)] & SPECIAL_EOL)
#define isBlank(c)  (pcs->Translate[(c)] & SPECIAL_BLANK)
#define isKeywd(c)  (pcs->Translate[(c)] & SPECIAL_KEYWORD)
#define isComm(c)   (pcs->Translate[(c)] & SPECIAL_COMMENT)
#define isOmit(c)   (pcs->Translate[(c)] & SPECIAL_OMIT)
#define isRoot(c)   (pcs->Translate[(c)] & SPECIAL_ROOT)
#define isSep(c)    (pcs->Translate[(c)] & SPECIAL_SEPARATOR)
#define isNames(c)  (pcs->Translate[(c)] & SPECIAL_NAMES)
#define isDecimal(c) (pcs->Translate[(c)] & SPECIAL_DECIMAL)
#define isMinus(c)  (pcs->Translate[(c)] & SPECIAL_MINUS)
#define isPlus(c)   (pcs->Translate[(c)] & SPECIAL_PLUS)

#define isSign(c)   (pcs->Translate[(c)] & (SPECIAL_PLUS | SPECIAL_MINUS))
#define isData(c)   (pcs->Translate[(c)] & (SPECIAL_OMIT | SPECIAL_ROOT|\
   SPECIAL_SEPARATOR | SPECIAL_NAMES | SPECIAL_DECIMAL | SPECIAL_PLUS |\
   SPECIAL_MINUS))

#endif /* CAVERN_H */
