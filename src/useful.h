/* > useful.h
 * Lots of oddments that come in handy generally
 * Copyright (C) 1993-1996 Olly Betts
 */

/*
1993.05.12 (W) added FNTYPE for different DOS memory models (non-ANSI)
1993.05.27 moved GCC fgetpos/fsetpos hack here from survex.h
           also modified hack - it might actually work now!
1993.06.04 FALSE -> fFalse
1993.06.07 #define SEEK_SET, SEEK_CUR and SEEK_END for unix-style libs
           FNTYPE -> FAR to aid comprehension
1993.06.08 corrected FEET_PER_METRE to METRES_PER_FOOT
1993.06.10 slight fettle
           added deg to complement rad
1993.06.16 invented NO_FGETPOS as GCC wasn't really appropriate
1993.06.28 (MF) corrected fsetpos() hack
           added some precautionary brackets to fgetpos() hack
1993.07.02 added CLK_TCK/CLOCKS_PER_SEC fix for TurboC
           added NO_STRFTIME hack
1993.07.17 #if 0-ed out fgetpos/fsetpos hack to discourage use
1993.07.19 "common.h" -> "osdepend.h"
1993.07.20 added w32 and w16 for 16 and 32 bit words
1993.08.10 now include stdio.h to get FILE defined
1993.08.11 improved NO_FGETPOS comment
           added #include <stdlib.h> to keep BC quiet
1993.08.15 standardised header & fettled
1993.10.26 tidied
1993.11.18 added fDirectory
1993.12.09 rearranged so osdepend.c compiles; moved fDirectory to osdepend.h
1993.12.10 if HUGE_VAL not defined define it the same as HUGE
1994.03.19 added strcmpi for DJGPP (and anyone else who needs it)
1994.04.16 added BLK() macro
1994.05.16 moved difftime() macro from osdepend.h
1994.06.03 fixed typo in value of PI (in 12st decimal place)
1994.06.09 added missing `;' to end of strcmpi prototype
1994.06.18 fettled incorrect indentation
1994.08.31 added fputnl
1994.09.08 byte -> uchar
1994.09.21 added fputsnl
1994.11.19 added LITTLE_ENDIAN stuff for probable speed up
1995.03.16 added fix for libraries which typedef uint, etc themselves
1995.03.24 NO_STRFTIME now won't overflow buffer
1995.10.11 fixed useful_getline to take buffer length
1996.02.19 fixed typo (UNIT -> UINT)
	   fixed useful_getline prototype
	   ostypes.h added
1996.04.01 strcmpi -> stricmp
1996.04.02 removed spurious < which actually gave compilable code (erk)
1996.04.04 added NOP macro
1996.05.06 added STRING macro
1998.03.22 changed to use autoconf's WORDS_BIGENDIAN
*/

/* only include once */
#ifndef USEFUL_H
#define USEFUL_H

#include <stdlib.h> /* for Borland C which #defines max() & min() there */
#include <stdio.h>
#include <math.h>
#include "osalloc.h"

/* Macro to allow easy building of macros contain multiple statements, such
 * that the likes of `if (x==y) macro1(x); else x=2;' works properly
 */
#define BLK(X) do { X } while(0)

/* Macro to do nothing, but avoid compiler warnings about empty if bodies &c */
#define NOP (void)0

#if 0
/* These macros work, but we don't need fgetpos/fsetpos - fseek/ftell
 * have equivalent functionality for files < 2Gbytes in size
 */

/* unix-style libraries may only have ftell/fseek, not fgetpos/fsetpos */
#ifndef HAVE_FGETPOS
/* ftell returns -1L on error; fgetpos returns non-zero on error */
# define fgetpos(FH,P) ((*(P)=ftell(FH))==-1L)
# define fsetpos(FH,P) fseek(FH,*(P),SEEK_SET)
  typedef long int fpos_t;
#endif
#endif

/* deals with Borland TurboC & maybe others */
#if (!defined(CLOCKS_PER_SEC) && defined(CLK_TCK))
# define CLOCKS_PER_SEC CLK_TCK
#endif

/* Deals with TurboC & maybe others. The format string is ignored,
 * and some default used instead.
 */
/* still something of a !HACK! */
#ifndef HAVE_STRFTIME
# define strftime(SZ,LENSZ,FMT,LOCALTIME) BLK(strncpy((SZ),asctime((LOCALTIME)),LENSZ);(SZ)[(LENSZ)-1]='\0';)
#endif

/* Return max/min of two numbers (if not defined already) */
/* NB Bad news if X or Y has side-effects... */
#ifndef max
# define max(X,Y) ( (X)>(Y) ? (X) : (Y) )
#endif /* !max */
#ifndef min
# define min(X,Y) ( (X)<(Y) ? (X) : (Y) )
#endif /* !min */

/* Why doesn't ANSI define PI in 'math.h'? Oh well, here's too many dp: */
#ifndef PI /* in case it is defined (eg by DJGPP) */
# ifdef M_PI /* some (eg msc I think) define this */
#  define PI M_PI
# else
#  define PI 3.14159265358979323846264338327950288419716939937510582097494459
# endif
#endif /* !PI */

#define MM_PER_INCH     25.4   /* exact value */
#define METRES_PER_FOOT 0.3048 /* exact value */

/* DJGPP needs these: */

#ifndef EXIT_FAILURE
# define EXIT_FAILURE 1 /* in fact FAILURE_EXIT_STATUS or ? - check! */
#endif /* !EXIT_FAILURE */

#ifndef EXIT_SUCCESS
# define EXIT_SUCCESS 0
#endif /* !EXIT_SUCCESS */

#ifndef FILENAME_MAX
# define FILENAME_MAX 1024 /* lots (hopefully) */
#endif /* !FILENAME_MAX */

#ifndef SEEK_SET
# define SEEK_SET 0
# define SEEK_CUR 1
# define SEEK_END 2
#endif /* !SEEK_SET */

/* Older UNIX libraries and DJGPP libraries have HUGE instead of HUGE_VAL */
#ifndef HUGE_VAL
# ifdef HUGE
#  define HUGE_VAL HUGE
# else
#  error Neither HUGE_VAL nor HUGE is defined
# endif
#endif

#include "ostypes.h"

#define putnl() putchar('\n')    /* print a newline char */
#define fputnl(FH) fputc('\n',(FH)) /* print a newline char to a file */
/* print a line followed by a newline char to a file */
#define fputsnl(SZ,FH) BLK(fputs((SZ),(FH));fputc('\n',(FH));)
#define sqrd(X) ((X)*(X))        /* macro to square things */
#define rad(X) ((PI/180.0)*(X))  /* convert from degrees to radians */
#define deg(X) ((180.0/PI)*(X))  /* convert from radians to degrees */
#define streq(sz1,sz2) (!(strcmp(sz1,sz2))) /* test equality of strings */

/* macro to just convert argument to a string */
#define STRING(X) _STRING(X)
#define _STRING(X) #X

#include "osdepend.h"
#include "filename.h"
#include "message.h"

/* useful_XXX() are defined in useful.c */
extern void  FAR useful_put16( w16, FILE* );
extern void  FAR useful_put32( w32, FILE* );
extern w16   FAR useful_get16( FILE* );
extern w32   FAR useful_get32( FILE* );
extern int   FAR useful_getline( char *buf, OSSIZE_T len, FILE *fh );

#ifndef WORDS_BIGENDIAN
extern w16 useful_w16;
extern w32 useful_w32;

# define useful_put16(W,FH) BLK(useful_w16=(W);fwrite(&useful_w16,2,1,(FH));)
# define useful_put32(W,FH) BLK(useful_w32=(W);fwrite(&useful_w32,4,1,(FH));)
# define useful_get16(FH) (fread(&useful_w16,2,1,(FH)),useful_w16)
# define useful_get32(FH) (fread(&useful_w32,4,1,(FH)),useful_w32)
#endif

#define put16( W, FH ) useful_put16( W, FH )
#define put32( W, FH ) useful_put32( W, FH )
#define get16( FH ) useful_get16( FH )
#define get32( FH ) useful_get32( FH )
#define getline( SZ, L, FH ) useful_getline((SZ),(L),(FH))

#endif /* !USEFUL_H */
