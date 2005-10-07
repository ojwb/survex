/* prio.c
 * Printer I/O routines for Survex printer drivers
 * Copyright (C) 1993-1997 Olly Betts
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
      if (ferror(fhPrn) || pclose(fhPrn)) prio_writeerror();
      return;
   }
#endif
   if (ferror(fhPrn) || fclose(fhPrn) == EOF) prio_writeerror();
}

extern void
prio_putc(int ch)
{
   /* putc() returns EOF on write error */
   if (putc(ch, fhPrn) == EOF) fatalerror(87);
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
