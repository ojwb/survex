/* > prio.c
 * Printer I/O routines for Survex printer drivers
 * Copyright (C) 1993-1997 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "filename.h"
#include "message.h"
#include "useful.h"
#include "osdepend.h"
#include "prio.h"

#if (OS==MSDOS)
/* Make DJGPP v2 use register struct-s compatible with v1 and BorlandC */
# if defined(__DJGPP__) && (__DJGPP__ >= 2)
#  define _NAIVE_DOS_REGS
# endif
# include <dos.h>
#endif

static FILE *fhPrn;

#ifdef HAVE_POPEN
static bool fPipeOpen = fFalse;
#endif

extern void
prio_open(const char *fnmPrn)
{
#ifdef HAVE_POPEN
   if (*fnmPrn == '|') {
      fnmPrn++; /* skip '|' */
      fhPrn = popen(fnmPrn, "w");
      if (!fhPrn) fatalerror(/*Couldn't open pipe: `%s'*/17, fnmPrn);
      fPipeOpen = fTrue;
      return;
   }
#endif
   fhPrn = safe_fopen(fnmPrn, "wb");
#if (OS==MSDOS)
   { /* For DOS, check if we're talking directly to a device, and force
      * "raw mode" if we are, so that ^Z ^S ^Q ^C don't get filtered */
      union REGS in, out;
      in.x.ax = 0x4400; /* get device info */
      in.x.bx = fileno(fhPrn);
      intdos(&in, &out);
      /* check call worked && file is a device */
      if (!out.x.cflag && (out.h.dl & 0x80)) {
         in.x.ax = 0x4401; /* set device info */
         in.x.dx = out.h.dl | 0x20; /* force binary mode */
         intdos(&in, &out);
      }
   }
#endif
}

static void
prio_writeerror(void)
{
   fatalerror(/*Error writing printer output*/87);
}

extern void
prio_close(void)
{
#ifdef HAVE_POPEN
   if (fPipeOpen) {
      /* pclose gives return code from program or -1, so only 0 means OK */
      if (pclose(fhPrn) == -1) prio_writeerror();
      return;
   }
#endif
   if (fclose(fhPrn) == EOF) prio_writeerror();
}

extern void
prio_putc(int ch)
{
   /* fputc() returns EOF on write error */
   if (fputc(ch, fhPrn) == EOF) fatalerror(87);
}

extern void
prio_printf(const char *format, ...)
{
   int result;
   va_list args;
   va_start(args, format);
   result = vfprintf(fhPrn, format, args);
   va_end(args);
   if (result < 0) prio_writeerror();
}

extern void
prio_print(const char *str)
{
   if (fputs(str, fhPrn) < 0) prio_writeerror();
}

extern void
prio_putpstr(const pstr *p)
{
   prio_putbuf(p->str, p->len);
}

extern void
prio_putbuf(const void *buf, size_t len)
{
   /* fwrite() returns # of members successfuly written */
   if (fwrite(buf, 1, len, fhPrn) != len) prio_writeerror();
}
