/* > debug.h
 * SURVEX debugging info control macros
 * Copyright (C) 1993-1996,2001 Olly Betts
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*#define DEBUG_INVALID 1*/

#ifndef DEBUG_H
#define DEBUG_H
#include "useful.h"

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

/* macro to turn on|off extra info given for fatal(5,...) (bug in proggy) */
#ifdef DEBUG_INVALID
# define DEBUG_XTRA(X) X
#else
# define DEBUG_XTRA(X)
#endif

/* macro to report detected bug */
#define BUG(SZ) BLK(DEBUG_XTRA(fputsnl((SZ), STDERR);) fatalerror(11);)

/* assert macro, which calls BUG() if it fails */
#define ASSERT(E) if (E) {} else BUG(__FILE__":"STRING(__LINE__)": assert("#E") failed")

/* assert macro, which calls BUG() if it fails */
#define ASSERT2(E, M) if (E) {} else BUG(__FILE__":"STRING(__LINE__)": assert("#E") failed - "M)

/* error.c */

/* show amounts of memory allocated, etc */
#define DEBUG_MALLOC 0

/* datain.c */

/* general debugging info */
#define DEBUG_DATAIN 0
/* more (older) debugging info */
#define DEBUG_DATAIN_1 0

/* network.c */

/* send error statistics to stdout instead of to a file (for debugging) */
#define ERRSTATS_TO_STDOUT 0
/* print info generally useful for debugging */
#define PRINT_NETBITS 0
/* puts '+' for legs 'inside' big (>3) nodes */
#define SHOW_INTERNAL_LEGS 0

/* matrix.c */

/* print the station table built during matrix construction */
#define PRINT_STN_TAB 0
/* print out the matrices */
#define PRINT_MATRICES 0
/* display the status of all nodes */
#define DEBUG_DISPLAY_STATUS 0
/* display info on all the stations */
#define DEBUG_DISPLAY_STN_INFO 0
/* display info about where we are in algorithm */
#define DEBUG_MATRIX 0
/* print out bumf as matrix is built from network */
#define DEBUG_MATRIX_BUILD 0

/* cavern.c */

/* display lots of info as statistics are calculated */
#define DEBUG_STATS 0
#endif
