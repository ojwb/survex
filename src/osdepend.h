/* > osdepend.h
 * Contains commonly required OS dependent bits
 * Copyright (C) 1993,1994,1996,1997 Olly Betts
 */

/* Note to porters: defaults are at the end of the file. Check there first */

/*
1993.05.21 (W) created
1993.05.27 made include once less nasty
1993.06.04 added () to #if
           moved some stuff from main.c
1993.06.07 slight unix fettle
           FNTYPE -> FAR to aid comprehension (by me anyway)
1993.06.10 moved all #s to column one to pander to crap compilers
1993.06.11 corrected unix CLOCKS_PER_SEC
           rearranged so that things default if not defined
1993.07.19 fettled generally
           moved FNM_SEP_xxx top here from filelist.h
           renamed as 'osdepend.h'
1993.07.21 clear screen rather than changing mode (RISC OS)
1993.08.12 fettled a mite
1993.08.15 DOS -> MSDOS
1993.08.16 #define NO_EXTENSIONS under RISC OS
1993.08.17 comment out #define NO_EXTENSIONS for testing
1993.10.24 added SWITCH_SYMBOLS (chars that introduce command line options)
1993.10.26 added HUGE
1993.11.09 removed FPE message to errlist.txt
           UNIX SWITCH_SYMBOLS changed from "/-" to "-"
1993.11.14 HUGE -> HUGE_MODEL because Sun math.h defines HUGE <sigh>
1993.12.08 added prototypes for functions in osdepend.c
1993.12.09 added macros NO_STDPRN, NO_PIPES, NO_PERROR, NO_STRERROR
           moved fDirectory from error.h
           HUGE_MODEL -> Huge
1993.12.15 FAR, HUGE should be #define-d elsewhere if not empty
1994.04.10 added TOS
1994.04.16 moved difftime() macro to useful.h
1994.06.21 added int argument to warning, error and fatal
1994.10.08 sizeof() corrected to ossizeof()
1994.12.03 added FNM_LEV_SEP2 for DJGPP
1996.02.19 sz -> char* ; ostypes.h added
1997.02.01 RISC OS now uses OSLib
1997.02.02 RISC OS fixes
           added our own versions of ceil and floor for DJGPP
1997.02.15 added #include <math.h> before DJGPP ceil/floor kludge
1997.05.11 better OSLib fix
1997.06.05 const added
1997.06.06 fixed for alpha (hopefully)
1998.03.22 autoconf-ed
*/

#ifndef OSDEPEND_H  /* only include once */
# define OSDEPEND_H

/* OSLib's types.h badly pollutes our namespace, so we kludge it off */
# ifndef types_H
#  define types_H
/* and then do the vital bits ourselves */
typedef unsigned int                            bits;
typedef unsigned char                           byte;
#  define UNKNOWN                                 1
# endif

# include "whichos.h"
# include "ostypes.h"

# if (OS==RISCOS)

typedef long w32;
typedef short w16;

/*#  include "kernel.h"*/
#  include "oslib/os.h"
#  include "oslib/fpemulator.h"

/* Take over screen (clear text window/default colours/cls) */
#  define init_screen() BLK( xos_writec(26); xos_writec(20); xos_cls(); )
/* Assume we've got working floating-point iff we can read a version number
 * from the fp software or hardware */
#  define check_fp_ok() BLK( if (xfpemulator_version(NULL)!=NULL) fatal(190,NULL,NULL,0); )
/* BLK( if (!_kernel_fpavailable()) fatal(190,NULL,NULL,0); ) */

/* FNM_SEP_DRV and FNM_SEP_EXT needn't be defined */
#  define FNM_SEP_LEV '.'
#  define FNM_SEP_DRV ':'
#  define FNM_SEP_EXT '/' /* This is what DOS ones are translated to.. */

#if 0
#  define NO_EXTENSIONS /* RISC OS doesn't really have them */
#endif

#  define NO_STDPRN

# elif (OS==MSDOS)

typedef long w32;
typedef short w16;

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

typedef long w32;
typedef short w16;

/* FNM_SEP_DRV and FNM_SEP_EXT needn't be defined */
#  define FNM_SEP_LEV '\\'
#  define FNM_SEP_DRV ':'
#  define FNM_SEP_EXT '.'

# elif (OS==UNIX)

typedef int w32;  /* int is a safer bet -- e.g. Dec Alpha boxes */
typedef short w16;

#  ifndef CLOCKS_PER_SEC
#   define CLOCKS_PER_SEC 1000000 /* nasty hack - true for SunOS anyway */
#  endif /* !CLOCKS_PER_SEC */

/* FNM_SEP_DRV and FNM_SEP_EXT needn't be defined */
#  define FNM_SEP_LEV '/'
/* #  define FNM_SEP_DRV  No equivalent under UNIX */
#  define FNM_SEP_EXT '.'

#  define SWITCH_SYMBOLS "-" /* otherwise '/' causes ambiguity problems */

#  define NO_STDPRN

# elif (OS==AMIGA)

typedef long w32;  /* check */
typedef short w16;

/* FNM_SEP_DRV and FNM_SEP_EXT needn't be defined */
#  define FNM_SEP_LEV '/'
#  define FNM_SEP_DRV ':'
#  define FNM_SEP_EXT '.' /* This is what DOS ones are translated to.. */

#  define NO_STDPRN

# else /* OS==? */
#  error Do not know operating system 'OS'
# endif /* OS==? */

/***************************************************************************/

/* defaults for things that are the same for most OS */

# ifndef FAR
#  define FAR /* nothing for sensible OS */
# endif /* !FAR */

# ifndef Huge
#  define Huge /* nothing for sensible OS */
# endif /* !Huge */

# ifndef init_screen
#  define init_screen() /* ie do nothing special */
# endif /* !init_screen */

# ifndef check_fp_ok
#  define check_fp_ok() /* ie do nothing special */
# endif /* !check_fp_ok */

# ifndef SWITCH_SYMBOLS
#  define SWITCH_SYMBOLS "/-"
# endif

# if 0
#  include <assert.h>
assert(ossizeof(w16)==2);
assert(ossizeof(w32)==4);
# endif

/* prototypes for functions in osdepend.c */
extern bool fAbsoluteFnm( const char *fnm );
extern bool fAmbiguousFnm( const char *fnm );
extern bool fDirectory( const char *fnm );

#endif /* !OSDEPEND_H */
