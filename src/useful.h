/* > useful.h
 * Lots of oddments that come in handy generally
 * Copyright (C) 1993-2001 Olly Betts
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
/* FIXME: still something of a hack */
#ifndef HAVE_STRFTIME
# define strftime(S, L, F, T) \
 BLK(strncpy((S), asctime((T)), L); (S)[(L) - 1] = '\0';)
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
# define EXIT_FAILURE 1 /* FIXME: in fact FAILURE_EXIT_STATUS or ? check! */
#endif /* !EXIT_FAILURE */

#ifndef EXIT_SUCCESS
# define EXIT_SUCCESS 0
#endif /* !EXIT_SUCCESS */

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
#define radius(X, Y) sqrt(sqrd((double)(X)) + sqrd((double)(Y))) /* euclidean distance */
#define rad(X) ((PI / 180.0) * (X))  /* convert from degrees to radians */
#define deg(X) ((180.0 / PI) * (X))  /* convert from radians to degrees */

/* macro to convert argument to a string literal */
#define STRING(X) _STRING(X)
#define _STRING(X) #X

#include "osdepend.h"
#include "filename.h"
#include "message.h"

#ifndef WORDS_BIGENDIAN
extern INT16_T useful_w16;
extern INT32_T useful_w32;

# define put16(W, FH) BLK(INT16_T w = (W); fwrite(&w, 2, 1, (FH));)
# define put32(W, FH) BLK(INT32_T w = (W); fwrite(&w, 4, 1, (FH));)
# define get16(FH) (fread(&useful_w16, 2, 1, (FH)), useful_w16)
# define get32(FH) (fread(&useful_w32, 4, 1, (FH)), useful_w32)
#else
void FAR useful_put16(INT16_T, FILE *);
void FAR useful_put32(INT32_T, FILE *);
INT16_T FAR useful_get16(FILE *);
INT32_T FAR useful_get32(FILE *);

# define put16(W, FH) useful_put16(W, FH)
# define put32(W, FH) useful_put32(W, FH)
# define get16(FH) useful_get16(FH)
# define get32(FH) useful_get32(FH)
#endif

#endif /* !USEFUL_H */
