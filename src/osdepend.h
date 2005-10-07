/* osdepend.h
 * Contains commonly required OS dependent bits
 * Copyright (C) 1993-2003,2004 Olly Betts
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

# include "whichos.h"
# include "ostypes.h"

# if (OS==WIN32)

/* FNM_SEP_DRV and FNM_SEP_EXT and FNM_SEP_LEV2 needn't be defined */
#  define FNM_SEP_LEV '\\'
#  define FNM_SEP_LEV2 '/'
#  define FNM_SEP_DRV ':'
#  define FNM_SEP_EXT '.'

#  ifdef __TURBOC__
#   include <time.h>
#   ifndef CLOCKS_PER_SEC
#    define CLOCKS_PER_SEC CLK_TCK
#   endif
#  endif

# elif (OS==UNIX)

/* FNM_SEP_DRV and FNM_SEP_EXT needn't be defined */
#  define FNM_SEP_LEV '/'
/* #  define FNM_SEP_DRV  No equivalent under UNIX */
#  define FNM_SEP_EXT '.'

#  define NO_STDPRN

# else /* OS==? */
#  error Do not know operating system 'OS'
# endif /* OS==? */

/***************************************************************************/

/* Use "Far" rather than "FAR" to avoid colliding with windows headers
 * which may "#define FAR far" and "#define far" or something similar.
 *
 * Use "Huge" to avoid colliding with "HUGE" which is a pre-ANSI name for
 * "HUGE_VAL"
 */
# ifdef HAVE_FAR_POINTERS
#  define Far far
#  define Huge huge
# else
/* just lose these on a sensible OS */
#  define Far
#  define Huge
# endif /* !Huge */

/* defaults for things that are the same for most OS */

# ifndef init_screen
#  define init_screen() /* ie do nothing special */
# endif /* !init_screen */

/* prototypes for functions in osdepend.c */
bool fAbsoluteFnm(const char *fnm);
bool fDirectory(const char *fnm);

# ifndef HAVE_DIFFTIME
#  define difftime(A, B) ((B)-(A))
# endif

#endif /* !OSDEPEND_H */
