/* > debug.h
 * SURVEX debugging info control macros
 * Copyright (C) 1993-1996 Olly Betts
 */

#ifndef DEBUG_H
#define DEBUG_H
#include "useful.h"

/* turn periodic calls to validate() checks on/off */
#define VALIDATE 0

/* turn on dumping of network data structure (lots of output) */
#define DUMP_NETWORK 0

/* elaborate if data structure becomes invalid */
#ifndef DEBUG_INVALID
# define DEBUG_INVALID 0
#endif

/* macro to turn on|off extra info given for fatal(5,...) (bug in proggy) */
#if DEBUG_INVALID
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
/* print pointer values of names in replace_traverses() */
#define PRINT_NAME_PTRS 0

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

/* survex.c */

/* display lots of info as statistics are calculated */
#define DEBUG_STATS 0
#endif
