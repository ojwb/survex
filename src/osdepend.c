/* osdepend.c
 * OS dependent functions
 * Copyright (C) 1993-2003,2004,2005 Olly Betts
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include "whichos.h"
#include "useful.h"
#include "osdepend.h"

#if OS_WIN32
# include <ctype.h> /* needed for isalpha */

/* NB "c:fred" isn't relative. Eg "c:\data\c:fred" won't work */
bool
fAbsoluteFnm(const char *fnm)
{
   /* <drive letter>: or \<path> or /<path>
    * or \\<host>\... or //<host>/... */
   unsigned char ch = (unsigned char)*fnm;
   return ((fnm[1] == ':' && isalpha(ch)) || ch == '\\' || ch == '/');
}

#elif OS_UNIX

bool
fAbsoluteFnm(const char *fnm)
{
   return (fnm[0] == '/');
}

#endif

/* fDirectory( fnm ) returns fTrue if fnm is a directory; fFalse if fnm is a
 * file, doesn't exist, or another error occurs (eg disc not in drive, ...)
 * NB If fnm has a trailing directory separator (e.g. `/' or `/home/olly/'
 * then it's assumed to be a directory even if it doesn't exist (as is an
 * empty string).
 */

#if OS_UNIX || OS_WIN32

# include <sys/types.h>
# include <sys/stat.h>
# include <stdio.h>

bool
fDirectory(const char *fnm)
{
   struct stat buf;
   if (!fnm[0] || fnm[strlen(fnm) - 1] == FNM_SEP_LEV
#ifdef FNM_SEP_LEV2
       || fnm[strlen(fnm) - 1] == FNM_SEP_LEV2
#endif
       ) return 1;
#ifdef HAVE_LSTAT
   /* On Unix, dereference any symlinks we might encounter */
   if (lstat(fnm, &buf) != 0) return 0;
#else
   if (stat(fnm, &buf) != 0) return 0;
#endif
#ifdef S_ISDIR
   /* POSIX way */
   return S_ISDIR(buf.st_mode);
#else
   /* BSD way */
   return ((buf.st_mode & S_IFMT) == S_IFDIR);
#endif
}

#else
# error Unknown OS
#endif
