/* > debug.h
 * SURVEX debugging info control macros
 * Copyright (C) 1993-1996 Olly Betts
 */

/*
1993.08.10 created to house all debugging info control macros
1993.08.11 turned it all off, since the program is now bug-free :|
1993.08.12 added DEBUG_MATRIX
           added DUMP_NETWORK. This & VALIDATE work by defining null macros
1993.08.15 fettled
1993.08.22 turned all debugging off
1993.09.21 (W)added DEBUG_COPYING_ENTRIES for release
1993.10.24 (W)added DEBUG_MALLOC [at some point in past]
1993.11.03 added DEBUG_MATRIX_BUILD to control some #if0-ed code in matrix.c
           removed DEBUG_COPYING_ENTRIES (from matrix.c)
1993.11.06 added DEBUG_INVALID
1993.11.08 added FATAL5ARGS
1993.11.30 FATAL5ARGS -> FATALARGS
1994.04.10 added BUG() macro
1994.04.13 debugging parallel stuff
1994.04.16 can now force calls with (validate)() and (dump_network)()
1994.05.10 added dump_node
1994.06.20 added int argument to warning, error and fatal
           created validate.h
1994.08.14 moved survex.exe specific stuff to validate.h
           include only once
1994.09.13 moved ASSERT macro here from network.c
1995.04.15 ASSERT -> ASSERT2; ASSERT now just condition, which is stringized
1996.02.16 (W)ASSERT & ASSERT2 L -> Line to stop BC whinge !?
1996.02.20 VALIDATE on
1996.02.22 and off again
1996.05.06 ASSERT() and ASSERT2() now use STRING() to include __LINE__ in msg
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
# define FATALARGS(X,Y) X,Y
#else
# define FATALARGS(X,Y) NULL,NULL
#endif

/* macro to report detected bug */
#define BUG(SZ) fatal(11,FATALARGS(wr,(SZ)),0)

/* assert macro, which calls BUG() if it fails */
#define ASSERT(E) if(E){}else BUG(__FILE__":"STRING(__LINE__)": assert("#E") failed")

/* assert macro, which calls BUG() if it fails */
#define ASSERT2(E,M) if(E){}else BUG(__FILE__":"STRING(__LINE__)": assert("#E") failed - "M)

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
