/* > prio.h
 * Headers for printer I/O routines for Survex printer drivers
 * Copyright (C) 1993,1996 Olly Betts
 */

/*
1993.07.12 split from printer drivers
1993.08.15 standardised header
1996.02.10 added pstr typedef (Pascal-style counted string) and prio_putpstr
1998.03.21 added prio_print
*/

#include <stdio.h>

#include "useful.h"

typedef struct {
   size_t len;
   char *str;
} pstr;

extern void prio_open( sz fnmPrn );
extern void prio_close( void );
extern void prio_putc( int ch );
extern void prio_printf( const char *format, ... );
extern void prio_print( const char *str );
extern void prio_putpstr( pstr *p );
