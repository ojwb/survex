/* > osdepend.c
 * OS dependent functions
 * Copyright (C) 1993-1997 Olly Betts
 */

/*
1993.12.08 extracted from error.c
1993.12.09 improved a bit; moved fDirectory to here from useful.c
1993.12.12 added #include <ctype.h> for DOS
1993.12.13 only #include <ctype.h> for DOS
1994.04.10 added TOS
1994.05.17 added SunOS fDirectory fix from Andy Holtsbery
1994.09.08 RISC OS fDirectory now uses _kernel_* rather than os_* function
1994.12.10 fAmbiguous turned off for now
1995.03.25 killed warning
1996.02.19 sz -> char*
1996.04.03 fixed warning
1997.01.31 RISC OS now uses OSLib
1997.02.02 tweaks to get RISC OS working again
           added our own versions of ceil and floor for DJGPP
1997.02.15 turned on DJGPP ceil/floor kludge
1997.06.05 const added
*/

#include <string.h>
#include "whichos.h"
#include "useful.h"
#include "osdepend.h"

#if (OS==RISCOS)

bool fAbsoluteFnm( const char *fnm ) {
 return strchr(fnm,':') /* filename contains a ':' */
     || (fnm[0]=='-' && strchr(fnm+1,'-')) /* temporary filing system */
     || (fnm[0]=='<' && strchr(fnm+1,'>')) /* System var eg <My$Dir>.File */
     || (fnm[1]=='.' && strchr("$&%@\\",fnm[0]));
                                     /* root, URD, lib, CSD, previous CSD */
}

bool fAmbiguousFnm( const char *fnm ) {
   fnm=fnm;
   return fFalse;
#if 0
   return (fnm[0]=='^' && fnm[1]=='.') /* Parent directory */
   /* System var could be here too */
#endif
}

#elif ((OS==MSDOS) || (OS==TOS))

# include <ctype.h> /* needed for isalpha */

/* NB for Wooks: "c:fred" isn't relative. Eg "c:\data\c:fred" won't work */

bool fAbsoluteFnm( const char *fnm ) {
 return (fnm[1]==':' && isalpha(fnm[0]) ) /* <drive letter>: */
     || (fnm[0]=='\\');                   /* Root */
}

/* DOS treats .\fred as ambiguous, so we should */
bool fAmbiguousFnm( const char *fnm ) {
 fnm=fnm;
 return fFalse;
#if 0
 return (strncmp(fnm,"..\\",3)==0)        /* Parent directory */
     || (fnm[0]=='.' && fnm[1]=='\\');    /* Currently Selected Directory */
#endif
}

#ifdef __DJGPP__
#if 0
double svx_ceil( double v ) {
   return ceil(v);
}

double svx_floor( double v ) {
   return floor(v);
}
#else
#include <limits.h>
/* These only work for doubles which fit in a long but that's OK for the
 * uses we make of ceil and floor */
double svx_ceil( double v ) {
   double r;
   if (v<LONG_MIN || v>LONG_MAX) {
      putchar('\b');
      return v;
   }   
   r = (double)(long)v;
   if (r<v) r = r + 1.0;
/*   fprintf(stderr,"svx_ceil(%g)=%g\n",v,r); */
   return r;
}

double svx_floor( double v ) {
   double r;
   if (v<LONG_MIN || v>LONG_MAX) {
      putchar('\b');
      return v;
   }   
   r = (double)(long)v;
   if (r>v) r = r - 1.0;
/*   fprintf(stderr,"svx_floor(%g)=%g\n",v,r); */
   return r;
}
#endif
#endif

#elif (OS==UNIX)

/* ~olly/fred or ~/fred is a shell feature--could handle it (use $HOME) */
bool fAbsoluteFnm( const char *fnm ) {
   return (fnm[0]=='/')                    /* Root */
#if 0
          || (fnm[0]=='.' && fnm[1]=='/')    /* Currently Selected Directory */
#endif
          ;
}

bool fAmbiguousFnm( const char *fnm ) {
 return fFalse;
#if 0
 return (strncmp(fnm,"../",3)==0); /* Parent directory */
#endif
}

#endif

/* fDirectory( fnm ) returns fTrue if fnm is a directory; fFalse if fnm is a
 * file, doesn't exist, or another error occurs (eg disc not in drive, ...)
 */

#if ((OS==UNIX) || (OS==MSDOS) || (OS==TOS))

# include <sys/types.h>
# include <sys/stat.h>
# include <stdio.h>

bool fDirectory( const char *fnm ) {
 struct stat buf;
 stat( fnm, &buf );
 return ((buf.st_mode & S_IFMT) == S_IFDIR);
}

#elif (OS==RISCOS)

# include "oslib/osfile.h"

bool fDirectory( const char *fnm ) {
 int objtype;
 /* it's a directory iff objtype is 2 or 3 */
 /* (3 is an image file (for RISC OS 3 and above) but we probably want */
 /* to treat image files as directories) */
 if (xosfile_read( fnm, &objtype, NULL, NULL, NULL, NULL ) != NULL)
    return fFalse;
 return (objtype == osfile_IS_DIR || objtype == osfile_IS_IMAGE);
}

#else
# error Unknown OS
#endif
