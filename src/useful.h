/* useful.h
 * Lots of oddments that come in handy generally
 * Copyright (C) 1993-2003,2004,2010,2011,2014 Olly Betts
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

/* only include once */
#ifndef USEFUL_H
#define USEFUL_H

#ifndef PACKAGE
# error config.h must be included first in each C/C++ source file
#endif

#include <stdlib.h> /* for Borland C which #defines max() & min() there */
#include <stdio.h>
#include <math.h>

/* Macro to allow easy building of macros contain multiple statements, such
 * that the likes of “if (x == y) macro1(x); else x = 2;” works properly  */
#define BLK(X) do {X} while(0)

/* Macro to do nothing, but avoid compiler warnings about empty if bodies &c */
#define NOP (void)0

/* In C++ code, #include<algorithm> and use std::max and std::min instead. */
#ifndef __cplusplus
/* Return max/min of two numbers. */
/* May be defined already (e.g. by Borland C in stdlib.h) */
/* NB Bad news if X or Y has side-effects... */
# ifndef max
#  define max(X, Y) ((X) > (Y) ? (X) : (Y))
# endif
# ifndef min
#  define min(X, Y) ((X) < (Y) ? (X) : (Y))
# endif
#endif

/* M_PI, etc may be defined in math.h */
#ifndef M_PI
# ifdef PI /* MSVC defines PI IIRC */
#  define M_PI PI
# else
#  define M_PI 3.14159265358979323846264338327950288419716939937510582097494459
# endif
#endif
#ifndef M_PI_2
# define M_PI_2 (M_PI / 2.0)
#endif
#ifndef M_PI_4
# define M_PI_4 (M_PI / 4.0)
#endif

#define MM_PER_INCH 25.4 /* exact value */
#define METRES_PER_FOOT 0.3048 /* exact value */
#define POINTS_PER_INCH	72.0
#define POINTS_PER_MM (POINTS_PER_INCH / MM_PER_INCH)

#define putnl() putchar('\n')    /* print a newline char */
#define fputnl(FH) PUTC('\n', (FH)) /* print a newline char to a file */
/* print a line followed by a newline char to a file */
#define fputsnl(SZ, FH) BLK(fputs((SZ), (FH)); PUTC('\n', (FH));)
#define sqrd(X) ((X) * (X))        /* macro to square things */

/* 2D Euclidean distance */
#ifndef HAVE_HYPOT
# define hypot(X, Y) sqrt(sqrd((double)(X)) + sqrd((double)(Y)))
#endif
#define rad(X) ((M_PI / 180.0) * (X))  /* convert from degrees to radians */
#define deg(X) ((180.0 / M_PI) * (X))  /* convert from radians to degrees */

/* macro to convert argument to a string literal */
#define STRING(X) STRING_(X)
#define STRING_(X) #X

#endif /* !USEFUL_H */
