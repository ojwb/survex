/* > useful.h
 * Lots of oddments that come in handy generally
 * Copyright (C) 1993-1996 Olly Betts
 */

/* only include once */
#ifndef USEFUL_H
#define USEFUL_H

#include <stdlib.h> /* for Borland C which #defines max() & min() there */
#include <stdio.h>
#include <math.h>
#include "osalloc.h"

/* Macro to allow easy building of macros contain multiple statements, such
 * that the likes of `if (x == y) macro1(x); else x = 2;' works properly  */
#define BLK(X) do {X} while(0)

/* Macro to do nothing, but avoid compiler warnings about empty if bodies &c */
#define NOP (void)0

#if 0
/* These macros work, but we don't need fgetpos/fsetpos - fseek/ftell
 * have equivalent functionality for files < 2Gbytes in size */

/* unix-style libraries may only have ftell/fseek, not fgetpos/fsetpos */
#ifndef HAVE_FGETPOS
/* ftell returns -1L on error; fgetpos returns non-zero on error */
# define fgetpos(FH, P) ((*(P) = ftell(FH)) == -1L)
# define fsetpos(FH, P) fseek(FH, *(P), SEEK_SET)
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
# define strftime(SZ, LENSZ, FMT, LOCALTIME) BLK(strncpy((SZ), asctime((LOCALTIME)), LENSZ);\
                                                 (SZ)[(LENSZ) - 1] = '\0';)
#endif

/* Return max/min of two numbers (if not defined already) */
/* NB Bad news if X or Y has side-effects... */
#ifndef max
# define max(X, Y) ((X) > (Y) ? (X) : (Y))
#endif
#ifndef min
# define min(X, Y) ((X) < (Y) ? (X) : (Y))
#endif

/* Why doesn't ANSI define PI in 'math.h'? Oh well, here's too many dp: */
#ifndef PI /* in case it is defined (eg by DJGPP) */
# ifdef M_PI /* some (eg msc I think) define this */
#  define PI M_PI
# else
#  define PI 3.14159265358979323846264338327950288419716939937510582097494459
# endif
#endif /* !PI */

#define MM_PER_INCH 25.4 /* exact value */
#define METRES_PER_FOOT 0.3048 /* exact value */

/* DJGPP needs these: */

#ifndef EXIT_FAILURE
# define EXIT_FAILURE 1 /* in fact FAILURE_EXIT_STATUS or ? - check! */
#endif /* !EXIT_FAILURE */

#ifndef EXIT_SUCCESS
# define EXIT_SUCCESS 0
#endif /* !EXIT_SUCCESS */

#if 0
/* now resize buffers to cope with arbitrary length filenames */
#ifndef FILENAME_MAX
# define FILENAME_MAX 1024 /* lots (hopefully) */
#endif /* !FILENAME_MAX */
#endif

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
#define fputnl(FH) putc('\n', (FH)) /* print a newline char to a file */
/* print a line followed by a newline char to a file */
#define fputsnl(SZ, FH) BLK(fputs((SZ), (FH)); putc('\n', (FH));)
#define sqrd(X) ((X) * (X))        /* macro to square things */
#define rad(X) ((PI / 180.0) * (X))  /* convert from degrees to radians */
#define deg(X) ((180.0 / PI) * (X))  /* convert from radians to degrees */
#define streq(sz1,sz2) (!(strcmp(sz1, sz2))) /* test equality of strings */

/* macro to just convert argument to a string */
#define STRING(X) _STRING(X)
#define _STRING(X) #X

#include "osdepend.h"
#include "filename.h"
#include "message.h"

/* useful_XXX() are defined in useful.c */
extern void FAR useful_put16(w16, FILE *);
extern void FAR useful_put32(w32, FILE *);
extern w16 FAR useful_get16(FILE *);
extern w32 FAR useful_get32(FILE *);
extern int FAR useful_getline(char *buf, OSSIZE_T len, FILE *fh);

#ifndef WORDS_BIGENDIAN
extern w16 useful_w16;
extern w32 useful_w32;

# define useful_put16(W, FH) BLK(useful_w16 = (W); fwrite(&useful_w16, 2, 1, (FH));)
# define useful_put32(W, FH) BLK(useful_w32 = (W); fwrite(&useful_w32, 4, 1, (FH));)
# define useful_get16(FH) (fread(&useful_w16, 2, 1, (FH)), useful_w16)
# define useful_get32(FH) (fread(&useful_w32, 4, 1, (FH)), useful_w32)
#endif

#define put16(W, FH) useful_put16(W, FH)
#define put32(W, FH) useful_put32(W, FH)
#define get16(FH) useful_get16(FH)
#define get32(FH) useful_get32(FH)
#define getline(SZ, L, FH) useful_getline((SZ), (L), (FH))

#endif /* !USEFUL_H */
