/* > osdepend.h
 * Contains commonly required OS dependent bits
 * Copyright (C) 1993-2000 Olly Betts
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

/* Note to porters: defaults are at the end of the file. Check there first */

#ifndef OSDEPEND_H  /* only include once */
# define OSDEPEND_H

/* OSLib's types.h badly pollutes our namespace, so we kludge it off
 * and then do the vital bits ourselves */
# ifndef types_H
#  define types_H
typedef unsigned int bits;
typedef unsigned char byte;
#  define UNKNOWN 1
# endif

# include "whichos.h"
# include "ostypes.h"

# if (OS==RISCOS)

/*#  include "kernel.h"*/
#  include "oslib/os.h"
#  include "oslib/fpemulator.h"

/* Take over screen (clear text window/default colours/cls) */
#  define init_screen() BLK(xos_writec(26); xos_writec(20); xos_cls();)

/* FNM_SEP_DRV and FNM_SEP_EXT needn't be defined */
#  define FNM_SEP_LEV '.'
#  define FNM_SEP_DRV ':'
/* "." in DOS filenames is translated to "/" by DOSFS and some other software.
 * We follow this defacto convention */
#  define FNM_SEP_EXT '/'

#  define NO_STDPRN

/* Phil's 3dx code uses hypot() - euclidean distance */
#  define hypot(X, Y, Z) \
	sqrt(sqrd((double)(X)) + sqrd((double)(Y)) + sqrd((double)(Z))

# elif (OS==MSDOS) || (OS==WIN32)

/* FNM_SEP_DRV and FNM_SEP_EXT and FNM_SEP_LEV2 needn't be defined */
#  define FNM_SEP_LEV '\\'
#  define FNM_SEP_LEV2 '/'
#  define FNM_SEP_DRV ':'
#  define FNM_SEP_EXT '.'

#  ifdef __DJGPP__
#   include <math.h>
#   ifdef ceil
#    undef ceil
#   endif
#   ifdef floor
#    undef floor
#   endif
/* DJGPP's ceil and floor are bugged if FP is emulated, so do it ourselves */
#   define ceil(X) svx_ceil((X))
#   define floor(X) svx_floor((X))
extern double svx_ceil(double);
extern double svx_floor(double);
#  endif

# elif (OS==TOS)

/* FNM_SEP_DRV and FNM_SEP_EXT needn't be defined */
#  define FNM_SEP_LEV '\\'
#  define FNM_SEP_DRV ':'
#  define FNM_SEP_EXT '.'

# elif (OS==UNIX)

#  ifndef CLOCKS_PER_SEC
#   define CLOCKS_PER_SEC 1000000 /* nasty hack - true for SunOS anyway */
#  endif /* !CLOCKS_PER_SEC */

/* FNM_SEP_DRV and FNM_SEP_EXT needn't be defined */
#  define FNM_SEP_LEV '/'
/* #  define FNM_SEP_DRV  No equivalent under UNIX */
#  define FNM_SEP_EXT '.'

#  define NO_STDPRN

# elif (OS==AMIGA)

/* FNM_SEP_DRV and FNM_SEP_EXT needn't be defined */
#  define FNM_SEP_LEV '/'
#  define FNM_SEP_DRV ':'
#  define FNM_SEP_EXT '.'

#  define NO_STDPRN

# else /* OS==? */
#  error Do not know operating system 'OS'
# endif /* OS==? */

/***************************************************************************/

# ifdef HAVE_FAR_POINTERS
#  define FAR far
#  define Huge huge
# else
/* just lose these on a sensible OS */
#  define FAR
#  define Huge
# endif /* !Huge */

/* defaults for things that are the same for most OS */

# ifndef init_screen
#  define init_screen() /* ie do nothing special */
# endif /* !init_screen */

/* prototypes for functions in osdepend.c */
extern bool fAbsoluteFnm(const char *fnm);
extern bool fDirectory(const char *fnm);

# ifndef HAVE_DIFFTIME
#  define difftime(A, B) ((B)-(A))
# endif

#endif /* !OSDEPEND_H */
