/* debug.h
 * SURVEX debugging info control macros
 * Copyright (C) 1993-1996,2001,2002 Olly Betts
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

/*#define DEBUG_INVALID 1*/

#ifndef DEBUG_H
#define DEBUG_H
#include "useful.h"
#include "message.h" /* for fatalerror() */

/* turn periodic calls to validate() checks on/off */
#ifndef VALIDATE
# define VALIDATE 0
#endif

/* turn on dumping of network data structure (lots of output) */
#ifndef DUMP_NETWORK
# define DUMP_NETWORK 0
#endif

/* elaborate if data structure becomes invalid */
#ifndef DEBUG_INVALID
# define DEBUG_INVALID 0
#endif

/* macro to report detected bug */
#ifdef DEBUG_INVALID
# define BUG(M) BLK(fputsnl(__FILE__":"STRING(__LINE__)": "M, STDERR);\
 fatalerror(/*Bug in program detected! Please report this to the authors*/11);)
#else
# define BUG(M) fatalerror(/*Bug in program detected! Please report this to the authors*/11)
#endif

/* assert macro, which calls BUG() if it fails */
#define SVX_ASSERT(E) if (E) {} else BUG("assert("#E") failed")

/* assert macro, which calls BUG() if it fails */
#define SVX_ASSERT2(E, M) if (E) {} else BUG("assert("#E") failed - "M)

/* datain.c */

/* general debugging info */
#define DEBUG_DATAIN 0
/* more (older) debugging info */
#define DEBUG_DATAIN_1 0

/* network.c */

/* print info generally useful for debugging */
#define PRINT_NETBITS 0
/* puts '+' for legs 'inside' big (>3) nodes */
#define SHOW_INTERNAL_LEGS 0

/* matrix.c */

/* print out the matrices */
#define PRINT_MATRICES 0
/* display info about where we are in algorithm */
#define DEBUG_MATRIX 0
/* print out bumf as matrix is built from network */
#define DEBUG_MATRIX_BUILD 0

#endif
