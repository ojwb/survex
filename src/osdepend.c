/* > osdepend.c
 * OS dependent functions
 * Copyright (C) 1993-1997 Olly Betts
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include "whichos.h"
#include "useful.h"
#include "osdepend.h"

#if (OS==RISCOS)

/* strange but true, a system variable reference works in a filename with fopen */
/* (less strange, but as true, so does a temporary filing system) */
bool
fAbsoluteFnm(const char *fnm)
{
   return strchr(fnm, ':') /* filename contains a ':' */
     || (fnm[0] == '-' && strchr(fnm + 1, '-')) /* temporary filing system */
     || (fnm[0] == '<' && strchr(fnm + 1, '>')) /* System var eg <My$Dir>.File */
     || (fnm[1] == '.' && strchr("$&%@\\", fnm[0]));
   /* root, URD, lib, CSD, previous CSD */
}

#elif ((OS==MSDOS) || (OS==TOS) || (OS==WIN32))
# include <ctype.h> /* needed for isalpha */

/* NB "c:fred" isn't relative. Eg "c:\data\c:fred" won't work */
bool
fAbsoluteFnm(const char *fnm)
{
   /* <drive letter>: or \<path> or /<path> or \\<host>\... or //<host>/... */
   unsigned char ch = *(unsigned const char*)fnm;
   return ((fnm[1] == ':' && isalpha(ch)) || ch == '\\' || ch == '/');
}

#ifdef __DJGPP__
/* DJGPP's ceil and floor are bugged if FP is emulated, so do it ourselves */
/* (the problem is stack corruption if memory serves) */
#include <limits.h>
/* These only work for doubles which fit in a long but that's OK for the
 * uses we make of ceil and floor */
double svx_ceil(double v) {
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

double svx_floor( double v ) {
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

/* FIXME handle the shell feature: ~olly/fred or ~/fred ?
 * use $HOME and getpwnam() (or something like that) */
bool
fAbsoluteFnm(const char *fnm)
{
   return (fnm[0] == '/');
}

#endif

/* fDirectory( fnm ) returns fTrue if fnm is a directory; fFalse if fnm is a
 * file, doesn't exist, or another error occurs (eg disc not in drive, ...)
 */

#if ((OS==UNIX) || (OS==MSDOS) || (OS==TOS) || (OS==WIN32))

# include <sys/types.h>
# include <sys/stat.h>
# include <stdio.h>

bool
fDirectory(const char *fnm)
{
   struct stat buf;
   if (stat(fnm, &buf) != 0) return 0;
   return ((buf.st_mode & S_IFMT) == S_IFDIR);
}

#elif (OS==RISCOS)

# include "oslib/osfile.h"

bool
fDirectory(const char *fnm)
{
   int objtype;
   /* it's a directory iff objtype is 2 or 3 */
   /* (3 is an image file (for RISC OS 3 and above) but we probably want */
   /* to treat image files as directories) */
   if (xosfile_read((char*)fnm, &objtype, NULL, NULL, NULL, NULL) != NULL)
      return fFalse;
   return (objtype == osfile_IS_DIR || objtype == osfile_IS_IMAGE);
}

#else
# error Unknown OS
#endif
