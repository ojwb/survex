/* > prio.h
 * Headers for printer I/O routines for Survex printer drivers
 * Copyright (C) 1993,1996 Olly Betts
 */

#include <stdio.h>

#include "useful.h"

typedef struct {
   size_t len;
   char *str;
} pstr;

extern void prio_open(const char *fnmPrn);
extern void prio_close(void);
extern void prio_putc(int ch);
extern void prio_printf(const char *format, ...);
extern void prio_print(const char *str);
extern void prio_putpstr(const pstr *p);
extern void prio_putbuf(const void *buf, size_t len);
