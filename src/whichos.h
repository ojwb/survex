/* > whichos.h
 * Determines which OS Survex will try to compile for
 * Copyright (C) 1993-1995 Olly Betts
 */

/*
1993.06.30 LF only eols
           incore filename above -> lower case
1993.07.02 added comment about makefiles
1993.07.19 "common.h" -> "osdepend.h"
1993.08.08 added missing linefeed in comment above
1993.08.12 recoded to try to auto-detect
1993.08.13 __MSDOS__ okay under Borland C++
1993.08.15 should cope if UNIX defined on entry and DOS -> MSDOS
1993.08.16 corrected "indentify" to "identify"; fettled
1993.08.18 changed "don't" to "do not" in #error for GCC
1993.12.09 adjusted 'cos DJGPP defines UNIX and __GO32__
           #undef MSDOS before #define-ing it
1993.12.11 added some support for Microsoft C
1993.12.13 tidied up
1994.03.30 msc defines _MSDOS (and not MSDOS) in ANSI mode
1994.04.10 added TOS
1994.04.27 only include once
1994.06.20 more Norcroft macros
1994.08.26 more Borland macros
1995.10.10 added __DJGPP__ to DJGPP macro list
*/

/* Built-in #define-s that identify compilers: (initals => checked)
 * __arm                       (Acorn) Norcroft RISC OS ARM C vsn 4.00 (OJWB)
 *   also __CC_NORCROFT __riscos __acorn
 * unix,UNIX                   Unix systems (?)
 * __MSDOS__                   Borland C++ (OJWB)
 *       [also __BORLANDC__ (version num) __TURBOC__ (version num)]
 * __TURBOC__  		       Turbo C
 * __GO32__,unix,MSDOS,__DJGPP__ DJGPP (W) (also GO32, __MSDOS__)
 * _MSDOS, (MSDOS not ANSI)    Microsoft C (OJWB)
 * MC68000, mc68000, SOZOBON, ATARI_ST, TOS|MINIX
 *                             Sozobon C (OJWB from MM's docs)
 */

#ifndef WHICHOS_H
# define WHICHOS_H

/* if OS has been defined, then assume they know what they're up to */
# ifndef OS
/* Okay, let's try to be clever and auto-detect - if OS gets defined more
 * than once, compiler should barf and warn user
 */
#  ifdef __arm
#   define OS RISCOS /* RISCiX too? */
#  endif
#  if (defined (_MSDOS) || defined(MSDOS) || defined(__MSDOS__))
#   undef unix /* DJGPP defines this */
#   undef MSDOS
#   define OS MSDOS
#  endif
#  if (defined(unix) || defined(UNIX))
#   undef UNIX /* to stop it causing problems later */
#   define OS UNIX
#  endif
#  if (defined(ATARI_ST) || defined(TOS))
#   undef TOS
#   define OS TOS
#  endif
/* etc ... */
# endif /*!defined(OS)*/

/* predefine OS to be one of these (eg in the make file) to force a compile
 * for an OS/compiler combination not support or that clashes somehow.
 * eg with -DOS=AMIGA to predefine OS as AMIGA
 */
# define RISCOS 1
# define MSDOS  2
# define UNIX   3
# define AMIGA  4
# define TOS    5
/* Just numbers, not a rating system ;) */

/* One last check, in case nothing worked */
# ifndef OS
#  error Sorry, do not know what OS to compile for - look at whichos.h
# endif
#endif
