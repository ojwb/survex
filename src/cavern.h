/* cavern.h
 * SURVEX Cave surveying software - header file
 * Copyright (C) 1991-2003,2005,2006,2010,2013,2014,2015,2016,2019,2021 Olly Betts
 * Copyright (C) 2004 Simeon Warner
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
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

#ifdef HAVE_PROJ_H
/* Work around broken check in proj.h:
 * https://github.com/OSGeo/PROJ/issues/1523
 */
# ifndef PROJ_H
#  include <proj.h>
# endif
#endif
#define ACCEPT_USE_OF_DEPRECATED_PROJ_API_H 1
#include <proj_api.h>

#include "img_hosted.h"
#include "useful.h"

/* Set EXPLICIT_FIXED_FLAG to 1 to force an explicit fixed flag to be used
 * in each pos struct, rather than using p[0]==UNFIXED_VAL to indicate
 * unfixed-ness.  This may be slightly faster, but uses more memory.
 */
#ifndef EXPLICIT_FIXED_FLAG
# define EXPLICIT_FIXED_FLAG 0
#endif

typedef double real; /* so we can change the precision used easily */
#define HUGE_REAL HUGE_VAL
#define REAL_EPSILON DBL_EPSILON

#if (!EXPLICIT_FIXED_FLAG)
# define UNFIXED_VAL HUGE_VAL /* if p[0]==UNFIXED_VAL, station is unfixed */
#endif

#define SPECIAL_EOL		0x0001
#define SPECIAL_BLANK		0x0002
#define SPECIAL_KEYWORD		0x0004
#define SPECIAL_COMMENT		0x0008
#define SPECIAL_OMIT		0x0010
#ifndef NO_DEPRECATED
#define SPECIAL_ROOT		0x0020
#endif
#define SPECIAL_SEPARATOR	0x0040
#define SPECIAL_NAMES		0x0080
#define SPECIAL_DECIMAL		0x0100
#define SPECIAL_MINUS		0x0200
#define SPECIAL_PLUS		0x0400
#define SPECIAL_OPEN		0x0800
#define SPECIAL_CLOSE		0x1000

extern char *fnm_output_base;
extern int fnm_output_base_is_dir;

extern bool fExportUsed;

extern int current_days_since_1900;

/* Types */

typedef enum {
   Q_NULL = -1, Q_DEFAULT, Q_POS, Q_PLUMB, Q_LEVEL,
   Q_GRADIENT, Q_BACKGRADIENT, Q_BEARING, Q_BACKBEARING,
   Q_LENGTH, Q_BACKLENGTH, Q_DEPTH, Q_DX, Q_DY, Q_DZ, Q_COUNT, Q_DECLINATION,
   Q_LEFT, Q_RIGHT, Q_UP, Q_DOWN,
   Q_MAC
} q_quantity;

typedef enum {
   INFER_NULL = -1, INFER_EQUATES, INFER_EXPORTS, INFER_PLUMBS, INFER_SUBSURVEYS
} infer_what;

/* unsigned long to cope with 16-bit int-s */
#define BIT(N) (1UL << (N))
#define BITA(N) (1UL << ((N) - 'a'))

#define TSTBIT(W, N) (((W)>>(N))&1)

/* masks for quantities which are length and angles respectively */
#define LEN_QMASK (BIT(Q_LENGTH) | BIT(Q_BACKLENGTH) | BIT(Q_DEPTH) |\
   BIT(Q_DX) | BIT(Q_DY) | BIT(Q_DZ) | BIT(Q_POS) | BIT(Q_COUNT) |\
   BIT(Q_LEFT) | BIT(Q_RIGHT) | BIT(Q_UP) | BIT(Q_DOWN))
#define ANG_QMASK (BIT(Q_BEARING) | BIT(Q_BACKBEARING) |\
   BIT(Q_GRADIENT) | BIT(Q_BACKGRADIENT) | BIT(Q_PLUMB) | BIT(Q_LEVEL) |\
   BIT(Q_DECLINATION))

/* if you add/change the order, check factor_tab in commands.c */
typedef enum {
   UNITS_NULL = -1, UNITS_METRES, UNITS_FEET, UNITS_YARDS, UNITS_INCHES,
   UNITS_DEGS, UNITS_QUADRANTS, UNITS_GRADS, UNITS_PERCENT, UNITS_MINUTES,
   UNITS_MAC, UNITS_DEPRECATED_ALIAS_FOR_GRADS, UNITS_FEETINCHES
} u_units;

/* don't reorder these values!  They need to match with img.h too */
typedef enum {
   FLAGS_NOT = -2, FLAGS_UNKNOWN = -1, FLAGS_SURFACE, FLAGS_DUPLICATE,
   FLAGS_SPLAY,
#if 0
   /* underground, but through rock (e.g. radiolocation).  Want to hide from
    * plots by default (so not cave) but don't want to include in surface
    * triangulation nets (so not surface) */
   FLAGS_SKELETAL, /* FIXME */
#endif
   /* Don't need to match img.h: */
   FLAGS_ANON_ONE_END,
   FLAGS_IMPLICIT_SPLAY,
   FLAGS_STYLE_BIT0, FLAGS_STYLE_BIT1, FLAGS_STYLE_BIT2
} flags;

/* flags are currently stored in an unsigned char */
typedef int compiletimeassert_flags0[FLAGS_STYLE_BIT2 <= 7 ? 1 : -1];

/* Mask to AND with to get bits to pass to img library. */
#define FLAGS_MASK \
    (BIT(FLAGS_SURFACE) | BIT(FLAGS_DUPLICATE) | BIT(FLAGS_SPLAY))

typedef int compiletimeassert_flags1[BIT(FLAGS_SURFACE) == img_FLAG_SURFACE ? 1 : -1];
typedef int compiletimeassert_flags2[BIT(FLAGS_DUPLICATE) == img_FLAG_DUPLICATE ? 1 : -1];
typedef int compiletimeassert_flags3[BIT(FLAGS_SPLAY) == img_FLAG_SPLAY ? 1 : -1];

typedef enum {
   /* Don't reorder these values!  They need to match with img.h too. */
   SFLAGS_SURFACE = 0, SFLAGS_UNDERGROUND, SFLAGS_ENTRANCE, SFLAGS_EXPORTED,
   SFLAGS_FIXED, SFLAGS_ANON, SFLAGS_WALL,
   /* These values don't need to match img.h, but mustn't clash. */
   SFLAGS_USED = 11,
   SFLAGS_SOLVED = 12, SFLAGS_SUSPECTTYPO = 13, SFLAGS_SURVEY = 14, SFLAGS_PREFIX_ENTERED = 15
} sflags;

/* Mask to AND with to get bits to pass to img library. */
#define SFLAGS_MASK (BIT(SFLAGS_SURFACE) | BIT(SFLAGS_UNDERGROUND) |\
	BIT(SFLAGS_ENTRANCE) | BIT(SFLAGS_EXPORTED) | BIT(SFLAGS_FIXED) |\
	BIT(SFLAGS_ANON) | BIT(SFLAGS_WALL))

typedef int compiletimeassert_sflags1[BIT(SFLAGS_SURFACE) == img_SFLAG_SURFACE ? 1 : -1];
typedef int compiletimeassert_sflags2[BIT(SFLAGS_UNDERGROUND) == img_SFLAG_UNDERGROUND ? 1 : -1];
typedef int compiletimeassert_sflags3[BIT(SFLAGS_ENTRANCE) == img_SFLAG_ENTRANCE ? 1 : -1];
typedef int compiletimeassert_sflags4[BIT(SFLAGS_EXPORTED) == img_SFLAG_EXPORTED ? 1 : -1];
typedef int compiletimeassert_sflags5[BIT(SFLAGS_FIXED) == img_SFLAG_FIXED ? 1 : -1];
typedef int compiletimeassert_sflags6[BIT(SFLAGS_ANON) == img_SFLAG_ANON ? 1 : -1];
typedef int compiletimeassert_sflags7[BIT(SFLAGS_WALL) == img_SFLAG_WALL ? 1 : -1];

/* enumeration of field types */
typedef enum {
   End = 0, Tape, Comp, Clino, BackTape, BackComp, BackClino,
   Left, Right, Up, Down,
   FrDepth, ToDepth, Dx, Dy, Dz, FrCount, ToCount,
   /* Up to here are readings are allowed multiple values
    * and have slot in the value[] array in datain.c.
    * (Depth, DepthChange, and Count can have multiple
    * readings, but are actually handled using tokens
    * above rather than as themselves).
    *
    * Fr must be the first reading after this comment!
    */
   Fr, To, Station, Depth, DepthChange, Count, Dir,
   Newline, IgnoreAllAndNewLine, Ignore, IgnoreAll,
   /* IgnoreAll must be the last reading before this comment!
    *
    * Readings after this comment are only used in datain.c
    * so can have enum values >= 32 because we only use a
    * bitmask for those readings used in commands.c.
    */
   CompassDATComp, CompassDATClino, CompassDATBackComp, CompassDATBackClino,
   CompassDATLeft, CompassDATRight, CompassDATUp, CompassDATDown,
   CompassDATFlags
} reading;

/* if IgnoreAll is >= 32, the compiler will choke on this */
typedef char compiletimeassert_reading[IgnoreAll < 32 ? 1 : -1];

/* position or length vector */
typedef real delta[3];

/* variance */
#ifdef NO_COVARIANCES
typedef real var[3];
typedef var svar;
#else
typedef real var[3][3];
typedef real svar[6];
#endif

/* station name */
typedef struct Prefix {
   struct Prefix *up, *down, *right;
   struct Node *stn;
   struct Pos *pos;
   const char *ident;
   const char *filename;
   unsigned int line;
   /* If (min_export == 0) then max_export is max # levels above is this
    * prefix is used (and so needs to be exported) (0 == parent only).
    * If (min_export > 0) then max_export is max # levels above this
    * prefix has been exported, and min_export is how far down the exports
    * have got (if min_export > 1 after a run, this prefix hasn't been
    * exported from below enough).
    * If INFER_EXPORTS is active when a station is encountered, we
    * set min_export = USHRT_MAX and max_export gets set as usual.  Then at
    * the end of the run, we also mark stations with min_export == USHRT_MAX
    * and max_export > 0 as exported. */
   unsigned short max_export, min_export;
   /* stn flags - e.g. surface, underground, entrance
    * also suspecttypo and survey */
   unsigned short sflags;
   short shape;
} prefix;

/* survey metadata */
typedef struct Meta_data {
    size_t ref_count;
    /* Days since 1900 for start and end date of survey, or -1 if undated. */
    int days1, days2;
} meta_data;

/* stuff stored for both forward & reverse legs */
typedef struct {
   struct Node *to;
   /* bits 0..1 = reverse leg number; bit7 is fFullLeg */
   /* bit6 = fReplacementLeg (by reduction rules) */
   /* bit5 = articulation leg (i.e. carries no error) */
   unsigned char reverse;
   /* flags - e.g. surface, duplicate survey
    * only used if (FLAG_DATAHERE & !(FLAG_REPLACEMENTLEG|FLAG_FAKE))
    * This could be only in linkfor, but this is actually more space
    * efficient.
    */
   unsigned char flags;
} linkcommon;

#define FLAG_DATAHERE 0x80
#define FLAG_REPLACEMENTLEG 0x40
#define FLAG_ARTICULATION 0x20
#define FLAG_FAKE 0x10 /* an equate or leg inside an sdfix */
#define MASK_REVERSEDIRN 0x03

/* reverse leg - deltas & vars stored on other dirn */
typedef struct LinkRev {
   linkcommon l;
} linkrev;

/* forward leg - deltas & vars stored here */
typedef struct Link {
   linkcommon l;
   delta d; /* Delta */
   svar v; /* Variances */
   meta_data *meta;
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
   delta p; /* Position */
#if EXPLICIT_FIXED_FLAG
   unsigned char fFixed; /* flag indicating if station is a fixed point */
#endif
} pos;

/*
typedef struct Inst {
   real zero, scale, units;
} inst;
*/

/* Survey data styles */
#define STYLE_NORMAL     0
#define STYLE_DIVING     1
#define STYLE_CARTESIAN  2
#define STYLE_CYLPOLAR   3
#define STYLE_NOSURVEY   4
#define STYLE_PASSAGE    5
#define STYLE_IGNORE     6

typedef int compiletimeassert_style1[STYLE_NORMAL == img_STYLE_NORMAL ? 1 : -1];
typedef int compiletimeassert_style2[STYLE_DIVING == img_STYLE_DIVING ? 1 : -1];
typedef int compiletimeassert_style3[STYLE_CARTESIAN == img_STYLE_CARTESIAN ? 1 : -1];
typedef int compiletimeassert_style4[STYLE_CYLPOLAR == img_STYLE_CYLPOLAR ? 1 : -1];
typedef int compiletimeassert_style5[STYLE_NOSURVEY == img_STYLE_NOSURVEY ? 1 : -1];

/* various settings preserved by *BEGIN and *END */
typedef struct Settings {
   struct Settings *next;
   unsigned int Truncate;
   bool f_clino_percent;
   bool f_backclino_percent;
   bool f_bearing_quadrants;
   bool f_backbearing_quadrants;
   bool dash_for_anon_wall_station;
   unsigned long len_footinches; /* argument that this could be an array of Q_MAC, but bitbashing is easier??? */
   unsigned char infer;
   enum {OFF, LOWER, UPPER} Case;
   int style;
   prefix *Prefix;
   prefix *begin_survey; /* used to check BEGIN and END match */
   short *Translate; /* if short is >= 16 bits, which ANSI requires */
   real Var[Q_MAC];
   real z[Q_MAC];
   real sc[Q_MAC];
   real units[Q_MAC];
   const reading *ordering;
   int begin_lineno; /* 0 means no block started in this file */
   int flags;
   projPJ proj;
   /* Location at which we calculate the declination if
    * z[Q_DECLINATION] == HUGE_REAL. */
   real dec_x, dec_y, dec_z;
   /* Cached auto-declination, or HUGE_REAL for no cached value.  Only
    * meaningful if date1 != -1.
    */
   real declination;
   /* Grid convergence. */
   real convergence;
   meta_data * meta;
} settings;

/* global variables */
extern settings *pcs;
extern prefix *root;
extern prefix *anon_list;
extern node *stnlist;
extern unsigned long optimize;
extern projPJ proj_out;
extern char * proj_str_out;

extern char *survey_title;
extern int survey_title_len;

extern bool fExplicitTitle;
extern long cLegs, cStns, cComponents;
extern FILE *fhErrStat;
extern img *pimg;
extern real totadj, total, totplan, totvert;
extern real min[3], max[3];
extern prefix *pfxHi[3], *pfxLo[3];
extern bool fQuiet; /* just show brief summary + errors */
extern bool fMute; /* just show errors */
extern bool fSuppress; /* only output 3d file */

/* macros */

#define POS(S, D) ((S)->name->pos->p[(D)])
#define POSD(S) ((S)->name->pos->p)

#define data_here(L) ((L)->l.reverse & FLAG_DATAHERE)
#define reverse_leg_dirn(L) ((L)->l.reverse & MASK_REVERSEDIRN)
#define reverse_leg(L) ((L)->l.to->leg[reverse_leg_dirn(L)])

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

/* macros for special chars */

#define isEol(c)    (pcs->Translate[(c)] & SPECIAL_EOL)
#define isBlank(c)  (pcs->Translate[(c)] & SPECIAL_BLANK)
#define isKeywd(c)  (pcs->Translate[(c)] & SPECIAL_KEYWORD)
#define isComm(c)   (pcs->Translate[(c)] & SPECIAL_COMMENT)
#define isOmit(c)   (pcs->Translate[(c)] & SPECIAL_OMIT)
#ifndef NO_DEPRECATED
#define isRoot(c)   (pcs->Translate[(c)] & SPECIAL_ROOT)
#endif
#define isSep(c)    (pcs->Translate[(c)] & SPECIAL_SEPARATOR)
#define isNames(c)  (pcs->Translate[(c)] & SPECIAL_NAMES)
#define isDecimal(c) (pcs->Translate[(c)] & SPECIAL_DECIMAL)
#define isMinus(c)  (pcs->Translate[(c)] & SPECIAL_MINUS)
#define isPlus(c)   (pcs->Translate[(c)] & SPECIAL_PLUS)
#define isOpen(c)   (pcs->Translate[(c)] & SPECIAL_OPEN)
#define isClose(c)  (pcs->Translate[(c)] & SPECIAL_CLOSE)

#define isSign(c)   (pcs->Translate[(c)] & (SPECIAL_PLUS | SPECIAL_MINUS))
#define isData(c)   (pcs->Translate[(c)] & (SPECIAL_OMIT | SPECIAL_ROOT|\
   SPECIAL_SEPARATOR | SPECIAL_NAMES | SPECIAL_DECIMAL | SPECIAL_PLUS |\
   SPECIAL_MINUS))

typedef struct nosurveylink {
   node *fr, *to;
   int flags;
   meta_data *meta;
   struct nosurveylink *next;
} nosurveylink;

extern nosurveylink *nosurveyhead;

typedef struct lrud {
    struct lrud * next;
    prefix *stn;
    meta_data *meta;
    real l, r, u, d;
} lrud;

typedef struct lrudlist {
    lrud * tube;
    struct lrudlist * next;
} lrudlist;

extern lrudlist * model;

extern lrud ** next_lrud;

#endif /* CAVERN_H */
