/* > survex.h
 * SURVEX Cave surveying software - header file
 * Copyright (C) 1991,1992,1993,1994,1995,1996,1997 Olly Betts
 */

/*
1992.10.26 Amended isXXXX() macros to work for isXXXX(ch=getc());
1993.01.14 link->dx .. ->dz changed to link->d[0] .. ->d[2]
           same for link->vx etc; & node->x .. to node->p[0] ..
1993.01.25 PC Compatibility OKed
1993.02.17 changed #ifdef RISCOS to #if OS=...
1993.02.18 #define shape(X) ((int)NST[(X)->used & 3]) -> ( ... &7])
1993.02.19 altered leg->to & leg->reverse to leg->l.to & leg->l.reverse
           added putnl() macro to do putchar('\n');
1993.02.23 added #define UNIX 3 and fettled whitespace
1993.02.26 added struct stackTrail
1993.02.27 added sqrd() macro
1993.03.11 node->in_network -> node->status
1993.04.05 (w) added really nasty MAX_FILEPATH_LEN hack
1993.04.06 added #ifndef xxx #define xxx stuff to help GCC cope
1993.04.07 #error Don't ... -> #error Do not ... for GCC's happiness
1993.04.21 pulled out non-survex specific bits to "useful.h"
1993.04.22 commenting improved
           added #define EXT_SVX_DATA
           isxxxx() macros -> isXxxx() to avoid possible collision with
             future extensions to C
           removed really nasty MAX_FILEPATH_LEN macro
1993.04.25 AMIGA-ed
1993.05.02 created svxmacro.h
1993.05.03 settings.Ascii -> settings.fAscii
           moved #define print_prefix ... to here
1993.05.03 (W) GCC define introduced
           (W) stuff removed to filelist.h
1993.05.27 moved GCC fgetpos/fsetpos hack to useful.h
1993.06.16 changed syserr() to fatal()
1993.08.06 fettled a tad
1993.08.09 altered data structure (OLD_NODES compiles previous version)
1993.08.10 removed DEBUG_1 to debug.h
           added EXPLICIT_FIXED_FLAG option to make data smaller & faster
1993.08.11 NEW_NODES added
           FOR_EACH_STN( stn ) does for loop with stn looping thru' all nodes
1993.08.12 fettled
           merged in svxmacro.h at end - changes for svxmacro.h:
>>1993.05.02 split from "survex.h"
>>1993.07.12 added dummy do ... while (0) to ident_cpy() macro
>>           removed surplus semi-colons and {}s from macro defns
>>           minor fettles
>>1993.07.16 changed shape() to use a shifted constant rather than a table
>>1993.08.06 hacked fix and fixed in bloody bug blatting blitzkrieg
>>           NB function version ***NOT*** changed
>>1993.08.08 removed 'fix fix' as it didn't :(
>>1993.08.09 altered data structure (OLD_NODES compiles previous version)
>>1993.08.10 added EXPLICIT_FIXED_FLAG option to make data smaller & faster
>>1993.08.11 NEW_NODES & POS
>>1993.08.12 corrected unfix() defn for EXPLICIT_FIXED_FLAG
>>           added USED(station,leg) macro
1993.08.12 ident_cpy now counts down (faster?)
1993.08.15 ident_cpy move to readval.c and uses memcpy
           fettled
1993.11.03 changed error routines
1993.11.08 only NEW_NODES code left
           removed MAX_LINE_LEN (no longer used)
1993.11.10 added HUGE_REAL
           added isSign() and isData()
1993.11.10 cs.Style is now a function pointer
1993.12.09 type link -> linkfor
1993.12.17 EXPLICIT_FIXED_FLAG is now turned off _unless_ defined already
1994.03.15 moved typedef ... Mode; to commline.c
1994.03.16 mods to allow column ordering to be varied
1994.03.18 added IGNORE and IGNOREALL to datum list
1994.03.21 increased MAX_NO_OF_SDS to 5
1994.04.13 added stackRed
1994.04.16 moved macros print_prefix and FOR_EACH_STN to netbits.h
           moved stack* typedefs to network.c
1994.04.18 fettled
1994.04.21 increased IDENT_LEN to 12
1994.04.27 added more externs
1994.05.12 added fPercent
1994.06.21 added int argument to warning, error and fatal
1994.06.26 cs -> pcs
1994.06.27 added settings.tag
1994.08.16 fhErrStat made global so we can solve, add more data and resolve
1994.08.17 added statBogus
1994.09.08 byte -> uchar
1994.09.11 moved BAD_DIRN and FOLLOW_TRAV here
1994.09.20 global temporary buffer szOut created
1994.10.01 pimgOut added
1994.10.06 fReplacementLeg added as bit 6 of reverse in linkcommon
           added node.colour for articulation stuff
1994.10.14 made net stats global
1994.10.18 now include <float.h>
1994.10.27 stnHi, stnLo -> pfxHi, pfxLo ; made fhPosList global
1994.10.28 added cComponents
1994.10.30 fettled and improved comments
1994.10.31 added shape
1994.11.08 added STYLE_* and cs.iStyle
1994.11.09 added inst struct
1994.11.12 finished rejigging *calibrate, *data, *sd and *units
1994.11.13 okay, not quite
1994.12.03 fhPosList needn't be global
           added pfx_fixed()
1995.02.21 added BIT() to fix 16-bit int problems
1996.03.23 settings.Translate is now a pointer, and point one element in
1996.04.04 NOP introduced
1996.04.15 SPECIAL_POINT -> SPECIAL_DECIMAL
1997.08.22 v[] -> var[] and added covariances too
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>

#include "useful.h"
#include "img.h"

#ifndef SURVEX_H
#define SURVEX_H

/* Max number of significant chars in each level of survey station name */
#define IDENT_LEN 12

/* set this to 1 to force an explicit fixed flag to be used in each pos
 * struct, rather than using p[0]==UNFIXED_VAL to indicate unfixed-ness.
 * This is maybe slightly faster, but uses more memory (definitely).
 */
#ifndef EXPLICIT_FIXED_FLAG
# define EXPLICIT_FIXED_FLAG 0
#endif

typedef double real; /* so we can change the precision used easily */
#define HUGE_REAL HUGE_VAL
/* this is big, but not so big we can't negate it safely -- not very nice */
#define REAL_BIG (DBL_MAX/20.0)

#if (!EXPLICIT_FIXED_FLAG)
# define UNFIXED_VAL HUGE_VAL /* if p[0]==UNFIXED_VAL, station is unfixed */
#endif

#define NOT_YET fatal(99,NULL,NULL,0)

#define COMMAND_FILE_COMMENT_CHAR ';'

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

#define TAB  9
#define CR  13
#define LF  10

#define MAX_NO_OF_SDs 5

#define COMMAND_BUFFER_LEN   256

/* Types */

/* one part of a station name */
typedef char id[IDENT_LEN];

typedef enum { Q_NULL=-1, Q_LENGTH, Q_DEPTH,
  Q_DX, Q_DY, Q_DZ, Q_LENGTHOUTPUT,
  Q_COUNT, Q_BEARING, Q_ANGLEOUTPUT,
  Q_GRADIENT, Q_DECLINATION, Q_POS, Q_PLUMB, Q_LEVEL, Q_MAC } q_quantity;

#define BIT(N) (1UL<<(N)) /* unsigned long to cope with 16-bit int-s */

/* masks for quantities which are length and angles respectively */
#define LEN_QMASK (BIT(Q_LENGTH)|BIT(Q_DEPTH)|\
  BIT(Q_DX)|BIT(Q_DY)|BIT(Q_DZ)|BIT(Q_POS)|\
  BIT(Q_COUNT)|BIT(Q_LENGTHOUTPUT))
#define ANG_QMASK (BIT(Q_BEARING)|BIT(Q_GRADIENT)|BIT(Q_PLUMB)|BIT(Q_LEVEL)|\
  BIT(Q_DECLINATION)|BIT(Q_ANGLEOUTPUT))

typedef enum { UNITS_NULL=-1, UNITS_METRES, UNITS_FEET, UNITS_YARDS,
  UNITS_DEGS, UNITS_GRADS, UNITS_PERCENT, UNITS_MINUTES, UNITS_MAC } u_units;
/* if you add/change the order, check factor_tab in commands.c */

/* enumeration of field types */
typedef enum { End=0, Fr, To, Tape, Comp, Clino, BackComp, BackClino,
  FrDepth, ToDepth, dx, dy, dz, FrCount, ToCount, dr,
  Ignore, IgnoreAll } datum;
/* assert(IgnoreAll<32); */
/* dr, Comp, dz give CYLPOL style */
/* Cope with any combination which gives enough info ??? */
/* typedef enum {NORMAL,CARTES,DIVING,TOPFIL,CYLPOL} style; */

/* type of a function to read in some data style */
typedef void (*style)(void);

/* Structures */

/* station name */
typedef struct Prefix {
 struct Prefix *up,*down,*right;
 struct Node   *stn;
 struct Pos    *pos;
 id             ident;
} prefix;

/* variance */
typedef real var[3][3];

/* postion or length vector */
typedef real d[3];

/* stuff stored for both forward & reverse legs */
typedef struct {
 struct Node *to;
 /* bits 0..1 = reverse leg number; bit7 is fFullLeg */
 /* bit6 = fReplacementLeg (by reduction rules) */
 uchar        reverse;
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
 struct Link   *leg[3];
 struct Node   *next;
 uchar status; /* possible values are statXXXXX (defd. below) */
 uchar fArtic; /* Is this an articulation point? */
 unsigned long colour;
} node;

/* station position */
typedef struct Pos {
 d          p; /* Position */
 int        shape;
#if EXPLICIT_FIXED_FLAG
 uchar      fFixed; /* flag indicating if station is a fixed point */
#endif
} pos;

#define statRemvd 0 /* removed during network reduction */
#define statInNet 1 /* in network */
#define statFixed 2 /* connected to a fixed point (& in network) */
#if 0
#define statDudEq 3 /* stn only equated, and being ignored */
#define statBogus 4 /* eg stn invented and discarded for delta-star */
#endif

#define STYLE_UNKNOWN 0
#define STYLE_NORMAL  1
#define STYLE_DIVING  2
#define STYLE_MAX     2

/*
typedef struct Inst {
  real zero, scale, units;
} inst;
*/

/* various settings preserved by *BEGIN and *END */
typedef struct Settings {
 uchar                    Unique;
 bool                     fAscii;
 bool                     f90Up;
 enum {OFF,LOWER,UPPER}   Case;
 style                    Style;
 prefix                   *Prefix;
 prefix                   *tag; /* used to check BEGIN/END tags match */
 short                    *Translate; /* if short is >= 16 bits, which ANSI requires */
 real                     Var[Q_MAC];
 real                     z[Q_MAC];
 real                     sc[Q_MAC];
 real                     units[Q_MAC];

 /* need to add a flag to indicate if this ordering is inherited or not
  * so we know whether an ancestor will want it */
 datum *ordering;
 bool fOrderingFreeable; /* was ordering malloc-ed for this level */
/* bool fTranslateFreeable; now infered from parent block */ /* was Translate malloc-ed for this level */

 struct Settings         *next;
 /* plus some more - maybe! */
} settings;

/* global variables */
extern settings *pcs;
extern prefix *root;
extern node *stnlist;
extern long optimize;
extern char szSurveyTitle[];
extern bool fExplicitTitle;
extern sz pthOutput, fnmInput;
extern long cLegs,cStns,cComponents;
extern FILE *fhErrStat;
/* extern FILE *fhPosList; */
extern img *pimgOut;
extern char szOut[];
#ifndef NO_PERCENTAGE
extern bool fPercent;
#endif
extern real  totadj,total,totplan,totvert;
extern real  min[3],max[3];
extern prefix *pfxHi[3],*pfxLo[3];

/* macros */

#define POS(S,D) ((S)->name->pos->p[(D)])
#define POSD(S) ((S)->name->pos->p)
#define USED(S,L) ((S)->leg[(L)]!=NULL)

/* Macro to neaten code for following along traverses of unfixed 2-nodes */
#define FOLLOW_TRAV(S,I,O) BLK(\
 if ((S)->leg[ ((I)+1)%3 ]!=NULL)\
  { if ((S)->leg[((I)+2)%3]==NULL) (O) = ((I)+1)%3; }\
 else\
  { if ((S)->leg[((I)+2)%3]!=NULL) (O) = ((I)+2)%3; } )
#define BAD_DIRN 255 /* used as a bad direction (ie not 0, 1, or 2) */

#define data_here(L)     ((L)->l.reverse & 0x80)
#define reverse_leg(L)   ((L)->l.reverse & 0x03)

#if EXPLICIT_FIXED_FLAG
# define pfx_fixed(N)     ((N)->pos->fFixed)
# define fix(S)           (S)->name->pos->fFixed=(char)fTrue
# define unfix(S)         (S)->name->pos->fFixed=(char)fFalse
#else
# define pfx_fixed(N)     ((N)->pos->p[0]!=UNFIXED_VAL)
# define fix(S)           NOP
# define unfix(S)         POS((S),0)=UNFIXED_VAL
#endif
#define fixed(S)         pfx_fixed((S)->name)

#define shape(S)         ( ((S)->leg[0]!=NULL)+\
                           ((S)->leg[1]!=NULL)+\
                           ((S)->leg[2]!=NULL) )
#define unfixed_2_node(S) ( (shape((S))==2) && !fixed((S)) )
#define remove_leg_from_station(L,S) (S)->leg[(L)]=NULL

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

#define isSign(c)   (pcs->Translate[(c)] & (SPECIAL_PLUS|SPECIAL_MINUS) )
#define isData(c)   (pcs->Translate[(c)] & (SPECIAL_OMIT|SPECIAL_ROOT|\
 SPECIAL_SEPARATOR|SPECIAL_NAMES|SPECIAL_DECIMAL|SPECIAL_PLUS|SPECIAL_MINUS) )

#endif /* SURVEX_H */
