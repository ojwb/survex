/* > version.h
 * Contains version number of release, and minimum version of message file
 * Copyright (C) 1992-1997 Olly Betts
 */

/*
1993.05.12 (W) had to remove innermost "" from README_FILE to compile with BC
1993.05.23 Version corrected to 0.10
1993.05.27 replaced innermost ""; error was missing #include "filelist.h"
           now include only once
1993.05.28 added #include "filelist.h" to make sure things work
1993.06.05 incremented version to 0.11 - 'development' to 'beta-test' in msg
1993.06.07 0.12 now
1993.06.10 fettled
1993.06.19 0.20
1993.06.28 0.21
1993.07.01 0.22
1993.07.02 0.23
1993.08.08 0.24
1993.08.09 0.24a
1993.08.11 0.24b
1993.08.12 0.24c
1993.08.13 0.24d
1993.08.14 0.24e
1993.08.15 0.24f
1993.08.16 0.25
1993.08.17 0.25a
1993.08.19 0.25b
1993.08.21 0.26
1993.08.22 0.30
1993.10.22 0.30a [symmetric matrix produced]
1993.11.02 0.32 [choleski factorisation used to solve matrix]
1993.11.08 moved DOOMMESS to errlist.txt
1993.11.12 0.32a [to Wookey]
1993.11.17 0.32b [PRINTPCL does 300 dpi plots under DOS; UNIX fixed]
1993.11.21 0.32c [to Wookey & Oxford bod]
1993.12.02 0.32d [to Wookey]
1993.12.07 0.32e [to BillP, MarkF (& Devon ?)]
1993.12.07 0.32f
1993.12.09 0.32g
1993.12.12 0.40 [Release]
1993.12.13 increased MESSAGE_VERSION_MIN to 0.40 (1st release vsn with it in)
1994.01.05 0.41 (MESSAGE_VERSION_MIN too) [Relase]
1993.03.?? 0.41a [to Jens]
1994.03.15 0.41c [to Wookey]
1994.03.16 0.41d
1994.03.19 0.41e
1994.03.21 0.41f [to Wookey]
1994.03.23 0.41g
1994.03.30 0.41h
1994.04.08 0.41i (msg too)
1994.04.14 0.42 (msg too)
1994.04.18 0.43
1994.04.22 0.50 (msg too) [release]
1994.05.11 0.50a [to Wookey]
1994.05.12 0.50b [to Wookey]
1994.05.13 0.50c [to Wookey]
           0.51 [release]
1994.05.16 actually changed to 0.51
1994.06.02 0.51a [printps -> michael lake]
1994.06.09 0.51b
1994.06.20 0.51c
1994.06.27 0.51d [BEGIN/END added]
1994.07.08 0.52 [release]
1994.08.28 0.52a
1994.09.13 0.53 [release]
1994.09.20 0.53a
1994.09.22 0.54 [release]
1994.10.30 0.54a [test release]
1994.11.26 0.54b (msg too)
1994.12.13 0.54c
1994.12.21 0.54d (same as 0.54c, but svx054c.zip seems to be 0.54b - sigh)
           min message version fudged to 0.54 so old caverot will work
1995.02.01 0.54e
1995.02.22 0.60
1995.03.25 added THIS_YEAR
1995.04.20 0.61
1995.12.18 0.63
1996.02.09 year->1996
1996.04.09 0.69 (0.70 test release)
1997.01.16 0.71
1997.03.22 0.73
*/

#ifndef VERSION_H /* include once only */
#define VERSION_H

#define VERSION "0.73"

/* MESSAGE_VERSION_MIN <= msg(0) <= VERSION */
/* change MESSAGE_VERSION_MIN to be the current setting of VERSION when a
 * non-backward compatible change is made to messages.txt
 */
#define MESSAGE_VERSION_MIN "0.73"

/* THIS_YEAR is used for copyright strings, e.g. "1991-"THIS_YEAR */

#define THIS_YEAR "1998"

#endif /* !VERSION_H */
