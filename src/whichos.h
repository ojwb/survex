/* whichos.h
 * Determines which OS Survex will try to compile for
 * Copyright (C) 1993-1995,2002,2003 Olly Betts
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

/* Built-in #define-s that identify compilers: (initals => checked)
 * __arm			(Acorn) Norcroft RISC OS ARM C vsn 4.00 (OJWB)
 *   also __CC_NORCROFT __riscos __acorn
 * unix,UNIX			Unix systems (?)
 * __MSDOS__			Borland C++ (OJWB)
 *       [also __BORLANDC__ (version num) __TURBOC__ (version num)]
 * __TURBOC__			Turbo C
 * __GO32__,unix,MSDOS,__DJGPP__
 *				DJGPP (W) (also GO32, __MSDOS__)
 * _MSDOS, (MSDOS not ANSI)	Microsoft C (OJWB)
 * MC68000, mc68000, SOZOBON, ATARI_ST, TOS|MINIX
 *				Sozobon C (OJWB from MM's docs)
 * MacOSX with apple modified gcc:
 * -D__ppc__ -D__MACH__ -D__APPLE__ -D__APPLE_CC__=932
 */

#ifndef WHICHOS_H
# define WHICHOS_H

/* if OS has been defined, then assume they know what they're up to */
# ifndef OS
/* Okay, let's try to be clever and auto-detect - if OS gets defined more
 * than once, compiler should barf and warn user
 */
#  ifdef __riscos
#   define OS RISCOS
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
#  if defined(__GNUC__) && defined(__APPLE_CC__)
/* Mac OS X is Unix underneath */
#   define OS UNIX
#  endif
#  if (defined(ATARI_ST) || defined(TOS))
#   undef TOS
#   define OS TOS
#  endif
#  if (defined (WIN32) || defined(__WIN32__))
#   undef WIN32
#   define OS WIN32
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
# define WIN32  6
/* Just numbers, not a rating system ;) */

/* One last check, in case nothing worked */
# ifndef OS
#  error Sorry, do not know what OS to compile for - look at whichos.h
# endif
#endif
