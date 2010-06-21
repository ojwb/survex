/* osdepend.c
 * OS dependent functions
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include "whichos.h"
#include "useful.h"
#include "osdepend.h"

#if (OS==RISCOS)

/* Strange but true: a system variable reference works in a filename with
 * fopen()!
 * Less strange, but just as true: so does a temporary filing system */
bool
fAbsoluteFnm(const char *fnm)
{
   return strchr(fnm, ':') /* filename contains a ':' */
     || (fnm[0] == '-' && strchr(fnm + 1, '-')) /* temp filing system */
     || (fnm[0] == '<' && strchr(fnm + 1, '>')) /* sysvar eg <X$Dir>.File */
     || (fnm[1] == '.' && strchr("$&%@\\", fnm[0]));
   /* root, URD, lib, CSD, previous CSD */
}

#elif ((OS==MSDOS) || (OS==TOS) || (OS==WIN32))
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

#ifdef __DJGPP__
/* DJGPP's ceil and floor are buggy if FP is emulated, so do it ourselves
 * (the problem is stack corruption if memory serves) */
#include <limits.h>
/* These only work for doubles which fit in a long but that's OK for the
 * uses we make of ceil and floor */
double
svx_ceil(double v)
{
   double r;
   if (v < LONG_MIN || v > LONG_MAX) {
      printf("Value out of range in svx_ceil\n");
      abort();
   }
   r = (double)(long)v;
   if (r < v) r += 1.0;
/*   fprintf(stderr,"svx_ceil(%g)=%g\n",v,r); */
   return r;
}

double
svx_floor(double v)
{
   double r;
   if (v < LONG_MIN || v > LONG_MAX) {
      printf("Value out of range in svx_floor\n");
      abort();
   }
   r = (double)(long)v;
   if (r > v) r -= 1.0;
/*   fprintf(stderr,"svx_floor(%g)=%g\n",v,r); */
   return r;
}
#endif

#elif (OS==UNIX)

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

#if ((OS==UNIX) || (OS==MSDOS) || (OS==TOS) || (OS==WIN32))

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
#if (OS == UNIX)
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

#elif (OS==RISCOS)

# include "oslib/osfile.h"

bool
fDirectory(const char *fnm)
{
   int objtype;
   if (!fnm[0] || fnm[strlen(fnm) - 1] == '.') return 1;
   if (xosfile_read((char*)fnm, &objtype, NULL, NULL, NULL, NULL) != NULL)
      return 0;
   /* It's a directory iff (objtype is 2 or 3) */
   /* (3 is an image file (for RISC OS 3 and above) but we probably want */
   /* to treat image files as directories) */
   return (objtype == osfile_IS_DIR || objtype == osfile_IS_IMAGE);
}

#else
# error Unknown OS
#endif
