/* > prio.c
 * Printer I/O routines for Survex printer drivers
 * Copyright (C) 1993-1997 Olly Betts
 */

/*
1993.07.12 split from printer drivers
1993.07.18 corrected fnm -> fnmPrn in Unix pipe code
1993.08.13 fettled header
1993.08.15 DOS -> MSDOS
1993.10.15 (W) fixed DOS printer bug by making check for LPT case insensitive
1993.10.24 added remark: strcmpi() not ANSI
1993.11.03 changed error routines
1993.11.05 changed error numbers so we can use same error list as survex
1993.11.16 corrected puts to wr in UNIX pipe code & changed message
1993.11.18 check if output file is directory
1993.11.21 corrected isDirectory to fDirectory
1993.11.29 error 10->24
1993.11.30 sorted out error vs fatal
1993.12.09 removed explicit OS references--use NO_PIPES and NO_STDPRN macros
1994.01.05 fixed Wook bug: strcmpi(a,b) should read strcmpi(a,b)==0
1994.06.02 removed stdprn hack - replaced with a call to ioctl
1994.06.05 ioctl -> intdos call as msc has that too
1994.06.20 added int argument to warning, error and fatal
1994.08.10 fixed # of args to fatal in pipe code
1995.03.25 fettled
1995.12.19 patched intdos() calls for DJGPPv2
1996.02.10 added prio_putpstr
1996.02.19 fixed so intdos works with DJGPPv2 *and* BorlandC
1996.05.06 attempted to fix very dim Norcroft warning
1997.01.23 fixed warning from Norcroft Cv4
*/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "error.h"
#include "useful.h"
#include "osdepend.h"
#include "prio.h"

#if (OS==MSDOS)
/* Make DJGPP v2 use register struct-s compatible with v1 and BorlandC */
# if defined(__DJGPP__)
#  if (__DJGPP__ >= 2)
#   define _NAIVE_DOS_REGS
#  endif
# endif
# include <dos.h>
#endif

static FILE *fhPrn;

#ifndef NO_PIPES
static bool fPipeOpen=fFalse;
#endif

extern void prio_open( sz fnmPrn ) {
#ifndef NO_PIPES
   if (*fnmPrn=='|') {
      fhPrn=popen(fnmPrn+1,"w"); /* fnmPrn+1 to skip '|' */
      if (!fhPrn)
         fatal(17,wr,fnmPrn,0);
      fPipeOpen=fTrue;
      return;
   }
#endif
   if (fDirectory(fnmPrn))
      fatal(44,wr,fnmPrn,0);
   fhPrn=fopen(fnmPrn,"wb");
   if (!fhPrn)
      fatal(24,wr,fnmPrn,0);
#if (OS==MSDOS)
   { /* For DOS, check if we're talking directly to a device, and force
      * "raw mode" if we are, so that ^Z ^S ^Q ^C don't get filtered */
      union REGS in, out;
      in.x.ax=0x4400; /* get device info */
      in.x.bx=fileno(fhPrn);
      intdos(&in,&out);
      /* check call worked && file is a device */
      if (!out.x.cflag && (out.h.dl & 0x80)) {
         in.x.ax=0x4401; /* set device info */
         in.x.dx=out.h.dl | 0x20; /* force binary mode */
         intdos(&in,&out);
      }
   }
#endif
}

extern void prio_close( void ) {
#ifndef NO_PIPES
   if (fPipeOpen) {
      pclose(fhPrn);
      return;
   }
#endif
   fclose(fhPrn);
}

extern void prio_putc( int ch ) {
   /* fputc() returns EOF on write error */
   if (fputc( ch, fhPrn )==EOF)
      fatal(87,NULL,NULL,0);
}

extern void prio_printf( const char * format, ... ) {
   int result;
   va_list args;
   va_start(args,format);
   result=vfprintf( fhPrn, format, args );
   va_end(args);
   if (result<0)
      fatal(87,NULL,NULL,0);
}

extern void prio_putpstr( pstr *p ) {
   /* fwrite() returns # of members successfuly written */
   if (fwrite( p->str, 1, p->len, fhPrn ) != p->len)
      fatal(87,NULL,NULL,0);
}
