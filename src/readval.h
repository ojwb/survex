/* > readval.h
 * Header file for routines to read a datum from the current input file
 * Copyright (C) 1991-1994 Olly Betts
 */

/*
1994.04.16 created
1994.05.09 moved TRIM_PREFIX from readval.c
1994.11.13 removed read_numeric_UD and fDefault argument from read_numeric
*/

#define TRIM_PREFIX

extern prefix *read_prefix( bool fOmit );
extern real    read_numeric( bool fOmit );
