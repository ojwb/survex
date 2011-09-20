/* osdepend.h
 * Contains commonly required OS dependent bits
 * Copyright (C) 1993-2003,2004,2005,2010 Olly Betts
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

#ifndef OSDEPEND_H  /* only include once */
# define OSDEPEND_H

# include "whichos.h"
# include "ostypes.h"

# if OS_WIN32

/* FNM_SEP_DRV and FNM_SEP_EXT and FNM_SEP_LEV2 needn't be defined */
#  define FNM_SEP_LEV '\\'
#  define FNM_SEP_LEV2 '/'
#  define FNM_SEP_DRV ':'
#  define FNM_SEP_EXT '.'

# elif OS_UNIX

/* FNM_SEP_DRV and FNM_SEP_EXT needn't be defined */
#  define FNM_SEP_LEV '/'
/* #  define FNM_SEP_DRV  No equivalent under UNIX */
#  define FNM_SEP_EXT '.'

# else
#  error Do not know what to do for this operating system
# endif

/***************************************************************************/

/* prototypes for functions in osdepend.c */
bool fAbsoluteFnm(const char *fnm);
bool fDirectory(const char *fnm);

# ifndef HAVE_DIFFTIME
#  define difftime(A, B) ((B)-(A))
# endif

#endif /* !OSDEPEND_H */
