/* > message.c
 * Fairly general purpose message and error routines
 * Copyright (C) 1993-2001 Olly Betts
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

/*#define DEBUG 1*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <locale.h>

#include "whichos.h"
#include "filename.h"
#include "message.h"
#include "osdepend.h"
#include "filelist.h"
#include "debug.h"

#ifdef HAVE_SIGNAL
# ifdef HAVE_SETJMP
#  include <setjmp.h>
static jmp_buf jmpbufSignal;
#  include <signal.h>
# else
#  undef HAVE_SIGNAL
# endif
#endif

/* This is the name of the default language.  Add -DDEFAULTLANG to CFLAGS
 * e.g. with `CFLAGS="-DDEFAULTLANG=fr" ./configure'
 */
#ifndef DEFAULTLANG
# define DEFAULTLANG "en"
#endif

/* For funcs which want to be immune from messing around with different
 * calling conventions */
#ifndef CDECL
# define CDECL
#endif

int msg_warnings = 0; /* keep track of how many warnings we've given */
int msg_errors = 0;   /* and how many (non-fatal) errors */

/* in case osmalloc() fails before szAppNameCopy is set up */
static const char *szAppNameCopy = "anonymous program";

/* error code for failed osmalloc and osrealloc calls */
static void
outofmem(OSSIZE_T size)
{
   fatalerror(/*Out of memory (couldn't find %lu bytes).*/1,
              (unsigned long)size);
}

#ifdef TOMBSTONES
#define TOMBSTONE_SIZE 16
static const char tombstone[TOMBSTONE_SIZE] = "012345\xfftombstone";
#endif

/* malloc with error catching if it fails. Also allows us to write special
 * versions easily eg for DOS EMS or MS Windows.
 */
void FAR *
osmalloc(OSSIZE_T size)
{
   void FAR *p;
#ifdef TOMBSTONES
   size += TOMBSTONE_SIZE * 2;
   p = malloc(size);
#else
   p = xosmalloc(size);
#endif
   if (p == NULL) outofmem(size);
#ifdef TOMBSTONES
   printf("osmalloc truep=%p truesize=%d\n", p, size);
   memcpy(p, tombstone, TOMBSTONE_SIZE);
   memcpy(p + size - TOMBSTONE_SIZE, tombstone, TOMBSTONE_SIZE);
   *(size_t *)p = size;
   p += TOMBSTONE_SIZE;
#endif
   return p;
}

/* realloc with error catching if it fails. */
void FAR *
osrealloc(void *p, OSSIZE_T size)
{
   /* some pre-ANSI realloc implementations don't cope with a NULL pointer */
   if (p == NULL) {
      p = xosmalloc(size);
   } else {
#ifdef TOMBSTONES
      int true_size;
      size += TOMBSTONE_SIZE * 2;
      p -= TOMBSTONE_SIZE;
      true_size = *(size_t *)p;
      printf("osrealloc (in truep=%p truesize=%d)\n", p, true_size);
      if (memcmp(p + sizeof(size_t), tombstone + sizeof(size_t),
		 TOMBSTONE_SIZE - sizeof(size_t)) != 0) {
	 printf("start tombstone for block %p, size %d corrupted!",
		p + TOMBSTONE_SIZE, true_size - TOMBSTONE_SIZE * 2);
      }
      if (memcmp(p + true_size - TOMBSTONE_SIZE, tombstone,
		 TOMBSTONE_SIZE) != 0) {
	 printf("end tombstone for block %p, size %d corrupted!",
		p + TOMBSTONE_SIZE, true_size - TOMBSTONE_SIZE * 2);
      }
      p = realloc(p, size);
      if (p == NULL) outofmem(size);
      printf("osrealloc truep=%p truesize=%d\n", p, size);
      memcpy(p, tombstone, TOMBSTONE_SIZE);
      memcpy(p + size - TOMBSTONE_SIZE, tombstone, TOMBSTONE_SIZE);
      *(size_t *)p = size;
      p += TOMBSTONE_SIZE;
#else
      p = xosrealloc(p, size);
#endif
   }
   if (p == NULL) outofmem(size);
   return p;
}

void FAR *
osstrdup(const char *str)
{
   char *p;
   OSSIZE_T len;
   len = strlen(str) + 1;
   p = osmalloc(len);
   memcpy(p, str, len);
   return p;
}

/* osfree is usually just a macro in osalloc.h */
#ifdef TOMBSTONES
void
osfree(void *p)
{
   int true_size;
   if (!p) return;
   p -= TOMBSTONE_SIZE;
   true_size = *(size_t *)p;
   printf("osfree truep=%p truesize=%d\n", p, true_size);
   if (memcmp(p + sizeof(size_t), tombstone + sizeof(size_t),
	      TOMBSTONE_SIZE - sizeof(size_t)) != 0) {
      printf("start tombstone for block %p, size %d corrupted!",
	     p + TOMBSTONE_SIZE, true_size - TOMBSTONE_SIZE * 2);
   }
   if (memcmp(p + true_size - TOMBSTONE_SIZE, tombstone,
	      TOMBSTONE_SIZE) != 0) {
      printf("end tombstone for block %p, size %d corrupted!",
	     p + TOMBSTONE_SIZE, true_size - TOMBSTONE_SIZE * 2);
   }
   free(p);
}
#endif

#ifdef HAVE_SIGNAL

static int sigReceived;

/* for systems not using autoconf, assume the signal handler returns void
 * unless specified elsewhere */
#ifndef RETSIGTYPE
# define RETSIGTYPE void
#endif

static CDECL RETSIGTYPE FAR
report_sig(int sig)
{
   sigReceived = sig;
   longjmp(jmpbufSignal, 1);
}

static void
init_signals(void)
{
   int en;
   if (!setjmp(jmpbufSignal)) {
#if 1 /* disable these to get a core dump */
      signal(SIGABRT, report_sig); /* abnormal termination eg abort() */
      signal(SIGFPE,  report_sig); /* arithmetic error eg /0 or overflow */
      signal(SIGILL,  report_sig); /* illegal function image eg illegal instruction */
      signal(SIGSEGV, report_sig); /* illegal storage access eg access outside memory limits */
#endif
# ifdef SIGSTAK /* only on RISC OS AFAIK */
      signal(SIGSTAK, report_sig); /* stack overflow */
# endif
      return;
   }

   switch (sigReceived) {
      case SIGABRT: en = /*Abnormal termination*/90; break;
      case SIGFPE:  en = /*Arithmetic error*/91; break;
      case SIGILL:  en = /*Illegal instruction*/92; break;
      case SIGSEGV: en = /*Bad memory access*/94; break;
# ifdef SIGSTAK
      case SIGSTAK: en = /*Stack overflow*/96; break;
# endif
      default:      en = /*Unknown signal received*/97; break;
   }
   fputsnl(msg(en), STDERR);

   /* Any of the signals we catch indicates a bug */
   fatalerror(/*Bug in program detected! Please report this to the authors*/11);

   exit(EXIT_FAILURE);
}
#endif

static int
default_charset(void)
{
#if (OS==RISCOS)
   /* RISCOS 3.1 and above CHARSET_RISCOS31 (ISO_8859_1 + extras in 128-159)
    * RISCOS < 3.1 is ISO_8859_1 */
 
   if (xwimpreadsysinfo_version(&version) != NULL) {
      /* RISC OS 2 or some error (don't care which) */
      return CHARSET_ISO_8859_1;
   }

   /* oddly wimp_VERSION_RO3 is RISC OS 3.1 */
   if (version < wimp_VERSION_RO3) return CHARSET_ISO_8859_1;
      
   return CHARSET_RISCOS31;
#elif (OS==MSDOS)
   return CHARSET_DOSCP850;
#else
   /* FIXME: assume ISO_8859_1 for now */
   return CHARSET_ISO_8859_1;
#endif
}

#if (OS==MSDOS)
static int
xlate_dos_cp850(int unicode)
{
   switch (unicode) {
#include "uni2dos.h"
   }
   return 0;
}
#endif

static int
add_unicode(int charset, unsigned char *p, int value)
{
#ifdef DEBUG
   fprintf(stderr, "add_unicode(%d, %p, %d)\n", charset, p, value);
#endif
   if (value == 0) return 0;
   switch (charset) {
   case CHARSET_USASCII:
      if (value < 0x80) {
	 *p = value;
	 return 1;
      }
      break;
   case CHARSET_ISO_8859_1:
      if (value < 0x100) {
	 *p = value;
	 return 1;
      }
      break;
#if (OS==RISCOS)
   case CHARSET_RISCOS31:
      /* RISC OS 3.1 (and later) extensions to ISO-8859-1 */
      switch (value) {
       case 0x152: value = 0x9a; break; /* &OElig; */
       case 0x153: value = 0x9b; break; /* &oelig; */
       case 0x174: value = 0x81; break; /* &Wcirc; */
       case 0x175: value = 0x82; break; /* &wcirc; */
       case 0x176: value = 0x85; break; /* &Ycirc; */
       case 0x177: value = 0x86; break; /* &ycirc; */
      }
      if (value < 0x100) {
	 *p = value;
	 return 1;
      }
      break;
#endif
#if (OS==MSDOS)
   case CHARSET_DOSCP850:
      value = xlate_dos_cp850(value);
      if (value) {
	 *p = value;
	 return 1;
      }
      break;
#endif
   }
   return 0;
}

/* fall back on looking in the current directory */
static const char *pth_cfg_files = "";

static int num_msgs = 0;
static char **msg_array = NULL;

const char *msg_lang = NULL;
const char *msg_lang2 = NULL;

static void
parse_msg_file(int charset_code)
{
   FILE *fh;
   unsigned char header[20];
   int i;
   unsigned len;
   unsigned char *p;
   char *fnm, *s;

#ifdef DEBUG
   fprintf(stderr, "parse_msg_file(%d)\n", charset_code);
#endif

   fnm = osstrdup(msg_lang);
   /* trim off charset from stuff like "de_DE.iso8859_1" */
   s = strchr(fnm, '.');
   if (s) *s = '\0';

   fh = fopenWithPthAndExt(pth_cfg_files, fnm, EXT_SVX_MSG, "rb", NULL);

   if (!fh) {
      /* e.g. if 'en-COCKNEY' is unknown, see if we know 'en' */
      if (strlen(fnm) > 3 && fnm[2] == '-') {
	 fnm[2] = '\0';
	 fh = fopenWithPthAndExt(pth_cfg_files, fnm, EXT_SVX_MSG, "rb", NULL);
	 if (!fh) fnm[2] = '-'; /* for error reporting */
      }
   }

   if (!fh) {
      /* no point extracting this error as it won't get used if file opens */
      fprintf(STDERR, "Can't open message file `%s' using path `%s'\n",
	      fnm, pth_cfg_files);
      exit(EXIT_FAILURE);
   }

   if (fread(header, 1, 20, fh) < 20 ||
       memcmp(header, "Svx\nMsg\r\n\xfe\xff", 12) != 0) {
      /* no point extracting this error as it won't get used if file opens */
      fprintf(STDERR, "Problem with message file `%s'\n", fnm);
      exit(EXIT_FAILURE);
   }

   if (header[12] != 0) {
      /* no point extracting this error as it won't get used if file opens */
      fprintf(STDERR, "I don't understand this message file version\n");
      exit(EXIT_FAILURE);
   }

   num_msgs = (header[14] << 8) | header[15];

   len = 0;
   for (i = 16; i < 20; i++) len = (len << 8) | header[i];

   p = osmalloc(len);
   if (fread(p, 1, len, fh) < len) {
      /* no point extracting this error - translation will never be used */
      fprintf(STDERR, "Message file truncated?\n");
      exit(EXIT_FAILURE);
   }
   fclose(fh);

#ifdef DEBUG
   fprintf(stderr, "fnm = `%s', num_msgs = %d, len = %d\n", fnm, num_msgs, len);
#endif
   osfree(fnm);

   msg_array = osmalloc(sizeof(char *) * num_msgs);

   for (i = 0; i < num_msgs; i++) {
      unsigned char *to = p;
      int ch;
      msg_array[i] = (char *)p;

      /* If we want UTF8 anyway, we just need to find the start of each
       * message */
      if (charset_code == CHARSET_UTF8) {
	 p += strlen((char *)p) + 1;
	 continue;
      }

      while ((ch = *p++) != 0) {
	 /* A byte in the range 0x80-0xbf or 0xf0-0xff isn't valid in
	  * this state, (0xf0-0xfd mean values > 0xffff) so treat as
	  * literal and try to resync so we cope better when fed
	  * non-utf-8 data.  Similarly we abandon a multibyte sequence
	  * if we hit an invalid character. */
	 if (ch >= 0xc0 && ch < 0xf0) {
	    int ch1 = *p;
	    if ((ch1 & 0xc0) != 0x80) goto resync;

	    if (ch < 0xe0) {
	       /* 2 byte sequence */
	       ch = ((ch & 0x1f) << 6) | (ch1 & 0x3f);
	       p++;
	    } else {
	       /* 3 byte sequence */
	       int ch2 = p[1];
	       if ((ch2 & 0xc0) != 0x80) goto resync;
	       ch = ((ch & 0x1f) << 12) | ((ch1 & 0x3f) << 6) | (ch2 & 0x3f);
	       p += 2;
	    }
	 }

         resync:

	 if (ch < 127) {
	    *to++ = (char)ch;
	 } else {
            /* FIXME: this rather assumes a 2 byte UTF-8 code never
             * transliterates to more than 2 characters */
	    to += add_unicode(charset_code, to, ch);
	 }
      }
      *to++ = '\0';
   }
}

const char *
msg_cfgpth(void)
{
   return pth_cfg_files;
}

void
msg_init(const char *argv0)
{
   char *p;

#ifdef HAVE_SIGNAL
   init_signals();
#endif
   /* Point to argv0 itself so we get the app name correct if osstrdup()
    * generates a signal */
   szAppNameCopy = argv0;
   szAppNameCopy = osstrdup(argv0);

   /* Look for env. var. "SURVEXHOME" or the like */
   p = getenv("SURVEXHOME");
   if (p && *p) {
      pth_cfg_files = osstrdup(p);
#if (OS==UNIX) && defined(DATADIR) && defined(PACKAGE)
   } else {
      /* under Unix, we compile in the configured path */
      pth_cfg_files = DATADIR "/" PACKAGE;
#else
   } else if (argv0) {
      /* else try the path on argv[0] */
      pth_cfg_files = path_from_fnm(argv0);
#endif
   }

   msg_lang = getenv("SURVEXLANG");
#ifdef DEBUG
   fprintf(stderr, "lang = %p (= \"%s\")\n", lang, lang?lang:"(null)");
#endif

   if (!msg_lang || !*msg_lang) {
      msg_lang = getenv("LANG");
      if (!msg_lang || !*msg_lang) msg_lang = DEFAULTLANG;
   }
#ifdef DEBUG
   fprintf(stderr, "msg_lang = %p (= \"%s\")\n", msg_lang, msg_lang?msg_lang:"(null)");
#endif

   /* On Mandrake LANG defaults to C */
   if (strcmp(msg_lang, "C") == 0) msg_lang = "en";

   msg_lang = osstrdup(msg_lang);

   /* Convert en-us to en_US, etc */
   p = strchr(msg_lang, '-');
   if (p) {
      *p++ = '_';
      while (*p) {
	 *p = toupper(*p);
	 p++;
      }
   }

   p = strchr(msg_lang, '_');
   if (p) {
      *p = '\0';
      msg_lang2 = osstrdup(msg_lang);
      *p = '_';
   }

#ifdef LC_MESSAGES
   /* try to setlocale() appropriately too */
   if (!setlocale(LC_MESSAGES, msg_lang)) {
      if (msg_lang2) setlocale(LC_MESSAGES, msg_lang2);
   }
#endif

   select_charset(default_charset());
}

/* message may be overwritten by next call
 * (but not in current implementation) */
const char *
msg(int en)
{
   /* NB can't use ASSERT here! */
   static char badbuf[256];
   if (!msg_array) {
      if (en != 1)  {
	 sprintf(badbuf, "Message %d requested before msg_array initialised\n", en);
	 return badbuf;
      }
      /* this should be the only message which can be requested before
       * the message file is opened and read... */
      return "Out of memory (couldn't find %ul bytes).\n";
   }

   if (en < 0 || en >= num_msgs) {
      sprintf(badbuf, "Message %d out of range\n", en);
      return badbuf;
   }

   return msg_array[en];
}

/* returns persistent copy of message */
const char *
msgPerm(int en)
{
   return msg(en);
}

void
v_report(int severity, const char *fnm, int line, int en, va_list ap)
{
   if (fnm) {
      fputs(fnm, STDERR);
      if (line) fprintf(STDERR, ":%d", line);
   } else {
      fputs(szAppNameCopy, STDERR);
   }
   fputs(": ", STDERR);

   if (severity == 0) {
      fputs(msg(/*warning*/4), STDERR);
      fputs(": ", STDERR);
   }

   vfprintf(STDERR, msg(en), ap);
   fputnl(STDERR);

   switch (severity) {
    case 0:
      msg_warnings++;
      break;
    case 1:
      msg_errors++;
      if (msg_errors == 50)
	 fatalerror_in_file(fnm, 0, /*Too many errors - giving up*/19);
      break;
    case 2:
      exit(EXIT_FAILURE);
   }
}

void
warning(int en, ...)
{
   va_list ap;
   va_start(ap, en);
   v_report(0, NULL, 0, en, ap);
   va_end(ap);
}

void
error(int en, ...)
{
   va_list ap;
   va_start(ap, en);
   v_report(1, NULL, 0, en, ap);
   va_end(ap);
}

void
fatalerror(int en, ...)
{
   va_list ap;
   va_start(ap, en);
   v_report(2, NULL, 0, en, ap);
   va_end(ap);
}

void
warning_in_file(const char *fnm, int line, int en, ...)
{
   va_list ap;
   va_start(ap, en);
   v_report(0, fnm, line, en, ap);
   va_end(ap);
}

void
error_in_file(const char *fnm, int line, int en, ...)
{
   va_list ap;
   va_start(ap, en);
   v_report(1, fnm, line, en, ap);
   va_end(ap);
}

void
fatalerror_in_file(const char *fnm, int line, int en, ...)
{
   va_list ap;
   va_start(ap, en);
   v_report(2, fnm, line, en, ap);
   va_end(ap);
}

/* Code to support switching character set at runtime (e.g. for a printer
 * driver to support different character sets on screen and on the printer)
 */
typedef struct charset_li {
   struct charset_li *next;
   int code;
   char **msg_array;
} charset_li;

static charset_li *charset_head = NULL;

static int charset = CHARSET_BAD;

int
select_charset(int charset_code)
{
   int old_charset = charset;
   charset_li *p;

#ifdef DEBUG
   fprintf(stderr, "select_charset(%d), old charset = %d\n", charset_code,
           charset);
#endif

   charset = charset_code;

   /* check if we've already parsed messages for new charset */
   for (p = charset_head; p; p = p->next) {
#ifdef DEBUG
      printf("%p: code %d msg_array %p\n", p, p->code, p->msg_array);
#endif
      if (p->code == charset) {
         msg_array = p->msg_array;
         return old_charset;
      }
   }

   /* nope, got to reparse message file */
   parse_msg_file(charset_code);

   /* add to list */
   p = osnew(charset_li);
   p->code = charset;
   p->msg_array = msg_array;
   p->next = charset_head;
   charset_head = p;

   return old_charset;
}
