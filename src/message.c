/* message.c
 * Fairly general purpose message and error routines
 * Copyright (C) 1993-2003,2004,2005,2006,2007 Olly Betts
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
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

#include "cmdline.h"
#include "whichos.h"
#include "filename.h"
#include "message.h"
#include "osdepend.h"
#include "filelist.h"
#include "debug.h"

#ifdef AVEN
# include "aven.h"
#endif

#ifdef HAVE_SIGNAL
# ifdef HAVE_SETJMP_H
#  include <setjmp.h>
static jmp_buf jmpbufSignal;
#  include <signal.h>
# else
#  undef HAVE_SIGNAL
# endif
#endif

#if (OS==WIN32)
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#elif (OS==MSDOS)
# include <dos.h>
# ifdef __DJGPP__
#  include <dpmi.h>
#  include <go32.h>
#  include <sys/movedata.h>
# endif
#elif (OS==RISCOS)
# include "oslib/wimpreadsy.h"
# include "oslib/territory.h"
#elif (OS==UNIX)
# include <sys/types.h>
# include <sys/stat.h>
#endif

/* For funcs which want to be immune from messing around with different
 * calling conventions */
#ifndef CDECL
# define CDECL
#endif

int msg_warnings = 0; /* keep track of how many warnings we've given */
int msg_errors = 0;   /* and how many (non-fatal) errors */

/* in case osmalloc() fails before appname_copy is set up */
static const char *appname_copy = "anonymous program";

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
void Far *
osmalloc(OSSIZE_T size)
{
   void Far *p;
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
void Far *
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

char Far *
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

static CDECL RETSIGTYPE Far
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
      signal(SIGABRT, report_sig); /* abnormal termination eg abort() */
      signal(SIGFPE,  report_sig); /* arithmetic error eg /0 or overflow */
      signal(SIGILL,  report_sig); /* illegal function image eg illegal instruction */
      signal(SIGSEGV, report_sig); /* illegal storage access eg access outside memory limits */
# ifdef SIGSTAK /* only on RISC OS AFAIK */
      signal(SIGSTAK, report_sig); /* stack overflow */
# endif
      return;
   }

   /* Remove that signal handler to avoid the possibility of an infinite loop.
    */
   signal(sigReceived, SIG_DFL);

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
   int version;
   if (xwimpreadsysinfo_version(&version) != NULL) {
      /* RISC OS 2 or some error (don't care which) */
      return CHARSET_ISO_8859_1;
   }

   /* oddly wimp_VERSION_RO3 is RISC OS 3.1 */
   if (version < wimp_VERSION_RO3) return CHARSET_ISO_8859_1;

   return CHARSET_RISCOS31;
#elif (OS==MSDOS)
#ifdef __DJGPP__
   __dpmi_regs r;
   r.x.ax = 0x6501;
   r.x.bx = 0xffff;
   r.x.dx = 0xffff;
   /* Use DJGPP's transfer buffer (which is at least 2K) */
   r.x.es = __tb >> 4;
   r.x.di = __tb & 0x0f;
   r.x.cx = 2048;
   /* bit 1 is the carry flag */
   if (__dpmi_int(0x21, &r) != -1 && !(r.x.flags & 1)) {
      unsigned short p;
      dosmemget(__tb + 5, 2, &p);
#else
   union REGS r;
   struct SREGS s = { 0 };

   unsigned char buf[48];
   r.x.ax = 0x6501;
   r.x.bx = 0xffff;
   r.x.dx = 0xffff;
   s.es = FP_SEG(buf);
   r.x.di = FP_OFF(buf);
   r.x.cx = 48;
   intdosx(&r, &r, &s);
   if (!r.x.cflag) {
      unsigned short p = buf[5] | (buf[6] << 8);
#endif
      if (p == 437) return CHARSET_DOSCP437;
      if (p == 850) return CHARSET_DOSCP850;
      if (p == 912) return CHARSET_ISO_8859_2;
   }
   return CHARSET_USASCII;
#elif (OS==WIN32)
# ifdef AVEN
#  define CODEPAGE GetACP()
# else
#  define CODEPAGE GetConsoleOutputCP()
# endif
   switch (CODEPAGE) {
    case 1252: return CHARSET_WINCP1252;
    case 1250: return CHARSET_WINCP1250;
    case 850: return CHARSET_DOSCP850;
   }
   return CHARSET_USASCII;
#elif (OS==UNIX)
#if defined(XCAVEROT) || defined(AVEN)
   return CHARSET_ISO_8859_1;
#else
   const char *p = getenv("LC_ALL");
   if (p == NULL || p[0] == '\0') {
      p = getenv("LC_CTYPE");
      if (p == NULL || p[0] == '\0') {
	 p = msg_lang;
      }
   }

   if (p) {
      char *q = strchr(p, '.');
      if (q) p = q + 1;
   }

   if (p) {
      const char *chset = p;
      size_t name_len;

      while (*p != '\0' && *p != '@') p++;

      name_len = p - chset;

      if (name_len) {
	 int only_digit = 1;
	 size_t cnt;

	 for (cnt = 0; cnt < name_len; ++cnt)
	    if (isalpha((unsigned char)chset[cnt])) {
	       only_digit = 0;
	       break;
	    }

	 if (only_digit) goto iso;

	 switch (tolower(chset[0])) {
	  case 'i':
	    if (tolower(chset[1]) == 's' && tolower(chset[2]) == 'o') {
	       chset += 3;
	       iso:
	       if (strncmp(chset, "8859", 4) == 0) {
		  chset += 4;
		  while (chset < p && *chset && !isdigit((unsigned char)*chset))
		     chset++;
		  switch (atoi(chset)) {
		   case 1: return CHARSET_ISO_8859_1;
		   case 2: return CHARSET_ISO_8859_2;
		   case 15: return CHARSET_ISO_8859_15;
		   default: return CHARSET_USASCII;
		  }
	       }
	    }
	    break;
	  case 'u':
	    if (tolower(chset[1]) == 't' && tolower(chset[2]) == 'f') {
	       chset += 3;
	       while (chset < p && *chset && !isdigit((unsigned char)*chset))
		  chset++;
	       switch (atoi(chset)) {
		case 8: return CHARSET_UTF8;
		default: return CHARSET_USASCII;
	       }
	    }
	 }
      }
   }
   return CHARSET_USASCII;
#endif
#else
# error Do not know operating system 'OS'
#endif
}

/* It seems that Swedish and maybe some other scandanavian languages don't
 * transliterate &auml; to ae - but it seems there may be conflicting views
 * on this...
 */
#define umlaut_to_e() 1

/* values <= 127 already dealt with */
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
   case CHARSET_ISO_8859_2:
      if (value >= 0xa0) {
	 int v = 0;
	 switch (value) {
	    case 0xa0: case 0xa4: case 0xa7: case 0xa8: case 0xad: case 0xb0:
	    case 0xb4: case 0xb8: case 0xc1: case 0xc2: case 0xc4: case 0xc7:
	    case 0xc9: case 0xcb: case 0xcd: case 0xce: case 0xd3: case 0xd4:
	    case 0xd6: case 0xd7: case 0xda: case 0xdc: case 0xdd: case 0xdf:
	    case 0xe1: case 0xe2: case 0xe4: case 0xe7: case 0xe9: case 0xeb:
	    case 0xed: case 0xee: case 0xf3: case 0xf4: case 0xf6: case 0xf7:
	    case 0xfa: case 0xfc: case 0xfd:
	       v = value; break;
	    case 0x104: v = '\xa1'; break;
	    case 0x2d8: v = '\xa2'; break;
	    case 0x141: v = '\xa3'; break;
	    case 0x13d: v = '\xa5'; break;
	    case 0x15a: v = '\xa6'; break;
	    case 0x160: v = '\xa9'; break;
	    case 0x15e: v = '\xaa'; break; /* Scedil */
	    case 0x164: v = '\xab'; break;
	    case 0x179: v = '\xac'; break;
	    case 0x17d: v = '\xae'; break;
	    case 0x17b: v = '\xaf'; break;
	    case 0x105: v = '\xb1'; break;
	    case 0x2db: v = '\xb2'; break;
	    case 0x142: v = '\xb3'; break;
	    case 0x13e: v = '\xb5'; break;
	    case 0x15b: v = '\xb6'; break;
	    case 0x2c7: v = '\xb7'; break;
	    case 0x161: v = '\xb9'; break;
	    case 0x15f: v = '\xba'; break; /* scedil */
	    case 0x165: v = '\xbb'; break;
	    case 0x17a: v = '\xbc'; break;
	    case 0x2dd: v = '\xbd'; break;
	    case 0x17e: v = '\xbe'; break;
	    case 0x17c: v = '\xbf'; break;
	    case 0x154: v = '\xc0'; break;
	    case 0x102: v = '\xc3'; break;
	    case 0x139: v = '\xc5'; break;
	    case 0x106: v = '\xc6'; break;
	    case 0x10c: v = '\xc8'; break;
	    case 0x118: v = '\xca'; break;
	    case 0x11a: v = '\xcc'; break;
	    case 0x10e: v = '\xcf'; break;
	    case 0x110: v = '\xd0'; break;
	    case 0x143: v = '\xd1'; break;
	    case 0x147: v = '\xd2'; break;
	    case 0x150: v = '\xd5'; break;
	    case 0x158: v = '\xd8'; break;
	    case 0x16e: v = '\xd9'; break;
	    case 0x170: v = '\xdb'; break;
	    case 0x162: v = '\xde'; break; /* &Tcedil; */
	    case 0x155: v = '\xe0'; break;
	    case 0x103: v = '\xe3'; break;
	    case 0x13a: v = '\xe5'; break;
	    case 0x107: v = '\xe6'; break;
	    case 0x10d: v = '\xe8'; break;
	    case 0x119: v = '\xea'; break;
	    case 0x11b: v = '\xec'; break;
	    case 0x10f: v = '\xef'; break;
	    case 0x111: v = '\xf0'; break;
	    case 0x144: v = '\xf1'; break;
	    case 0x148: v = '\xf2'; break;
	    case 0x151: v = '\xf5'; break;
	    case 0x159: v = '\xf8'; break;
	    case 0x16f: v = '\xf9'; break;
	    case 0x171: v = '\xfb'; break;
	    case 0x163: v = '\xfe'; break; /* tcedil */
	    case 0x2d9: v = '\xff'; break;
	 }
	 if (v == 0) break;
	 value = v;
      }
      *p = value;
      return 1;
   case CHARSET_ISO_8859_15:
      switch (value) {
       case 0xa4: case 0xa6: case 0xb0: case 0xc4:
       case 0xd0: case 0xd4: case 0xd5: case 0xd6:
	 goto donthave;
       case 0x152: value = 0xd4; break; /* &OElig; */
       case 0x153: value = 0xd5; break; /* &oelig; */
#if 0
       case 0x0: value = 0xa4; break; /* euro */
#endif
       case 0x160: value = 0xa6; break; /* Scaron */
       case 0x161: value = 0xb0; break; /* scaron */
       case 0x17d: value = 0xc4; break; /* Zcaron */
       case 0x17e: value = 0xd0; break; /* zcaron */
#if 0
       case 0x0: value = 0xd6; break; /* Ydiersis */
#endif
      }
      if (value < 0x100) {
	 *p = value;
	 return 1;
      }
      donthave:
      break;
#if (OS==RISCOS)
   case CHARSET_RISCOS31:
      /* RISC OS 3.1 (and later) extensions to ISO-8859-1 */
      switch (value) {
       case 0x152: value = 0x9a; break; /* &OElig; */
       case 0x153: value = 0x9b; break; /* &oelig; */
#if 0
       case 0x174: value = 0x81; break; /* &Wcirc; */
       case 0x175: value = 0x82; break; /* &wcirc; */
       case 0x176: value = 0x85; break; /* &Ycirc; */
       case 0x177: value = 0x86; break; /* &ycirc; */
#endif
      }
      if (value < 0x100) {
	 *p = value;
	 return 1;
      }
      break;
#elif (OS==WIN32)
   case CHARSET_WINCP1250:
      /* MS Windows rough equivalent to ISO-8859-2 */
      if (value >= 0x80) {
	 int v = 0;
	 switch (value) {
	    case 0xa0: case 0xa4: case 0xa6: case 0xa7: case 0xa8: case 0xa9:
	    case 0xab: case 0xac: case 0xad: case 0xae: case 0xb0: case 0xb1:
	    case 0xb4: case 0xb5: case 0xb6: case 0xb7: case 0xb8: case 0xbb:
	    case 0xc1: case 0xc2: case 0xc4: case 0xc7: case 0xc9: case 0xcb:
	    case 0xcd: case 0xce: case 0xd3: case 0xd4: case 0xd6: case 0xd7:
	    case 0xda: case 0xdc: case 0xdd: case 0xdf: case 0xe1: case 0xe2:
	    case 0xe4: case 0xe7: case 0xe9: case 0xeb: case 0xed: case 0xee:
	    case 0xf3: case 0xf4: case 0xf6: case 0xf7: case 0xfa: case 0xfc:
	    case 0xfd:
	       v = value; break;
	    case 0x20ac: v = '\x80'; break;
	    case 0x201a: v = '\x82'; break;
	    case 0x201e: v = '\x84'; break;
	    case 0x2026: v = '\x85'; break;
	    case 0x2020: v = '\x86'; break;
	    case 0x2021: v = '\x87'; break;
	    case 0x2030: v = '\x89'; break;
	    case 0x0160: v = '\x8a'; break;
	    case 0x2039: v = '\x8b'; break;
	    case 0x015a: v = '\x8c'; break;
	    case 0x0164: v = '\x8d'; break;
	    case 0x017d: v = '\x8e'; break;
	    case 0x0179: v = '\x8f'; break;
	    case 0x2018: v = '\x91'; break;
	    case 0x2019: v = '\x92'; break;
	    case 0x201c: v = '\x93'; break;
	    case 0x201d: v = '\x94'; break;
	    case 0x2022: v = '\x95'; break;
	    case 0x2013: v = '\x96'; break;
	    case 0x2014: v = '\x97'; break;
	    case 0x2122: v = '\x99'; break;
	    case 0x0161: v = '\x9a'; break;
	    case 0x203a: v = '\x9b'; break;
	    case 0x015b: v = '\x9c'; break;
	    case 0x0165: v = '\x9d'; break;
	    case 0x017e: v = '\x9e'; break;
	    case 0x017a: v = '\x9f'; break;
	    case 0x02c7: v = '\xa1'; break;
	    case 0x02d8: v = '\xa2'; break;
	    case 0x0141: v = '\xa3'; break;
	    case 0x0104: v = '\xa5'; break;
	    case 0x015e: v = '\xaa'; break; /* Scedil */
	    case 0x017b: v = '\xaf'; break;
	    case 0x02db: v = '\xb2'; break;
	    case 0x0142: v = '\xb3'; break;
	    case 0x0105: v = '\xb9'; break;
	    case 0x015f: v = '\xba'; break; /* scedil */
	    case 0x013d: v = '\xbc'; break;
	    case 0x02dd: v = '\xbd'; break;
	    case 0x013e: v = '\xbe'; break;
	    case 0x017c: v = '\xbf'; break;
	    case 0x0154: v = '\xc0'; break;
	    case 0x0102: v = '\xc3'; break;
	    case 0x0139: v = '\xc5'; break;
	    case 0x0106: v = '\xc6'; break;
	    case 0x010c: v = '\xc8'; break;
	    case 0x0118: v = '\xca'; break;
	    case 0x011a: v = '\xcc'; break;
	    case 0x010e: v = '\xcf'; break;
	    case 0x0110: v = '\xd0'; break;
	    case 0x0143: v = '\xd1'; break;
	    case 0x0147: v = '\xd2'; break;
	    case 0x0150: v = '\xd5'; break;
	    case 0x0158: v = '\xd8'; break;
	    case 0x016e: v = '\xd9'; break;
	    case 0x0170: v = '\xdb'; break;
	    case 0x0162: v = '\xde'; break; /* &Tcedil; */
	    case 0x0155: v = '\xe0'; break;
	    case 0x0103: v = '\xe3'; break;
	    case 0x013a: v = '\xe5'; break;
	    case 0x0107: v = '\xe6'; break;
	    case 0x010d: v = '\xe8'; break;
	    case 0x0119: v = '\xea'; break;
	    case 0x011b: v = '\xec'; break;
	    case 0x010f: v = '\xef'; break;
	    case 0x0111: v = '\xf0'; break;
	    case 0x0144: v = '\xf1'; break;
	    case 0x0148: v = '\xf2'; break;
	    case 0x0151: v = '\xf5'; break;
	    case 0x0159: v = '\xf8'; break;
	    case 0x016f: v = '\xf9'; break;
	    case 0x0171: v = '\xfb'; break;
	    case 0x0163: v = '\xfe'; break; /* tcedil */
	    case 0x02d9: v = '\xff'; break;
	 }
	 if (v == 0) break;
	 value = v;
      }
      *p = value;
      return 1;
   case CHARSET_WINCP1252:
      /* MS Windows extensions to ISO-8859-1 */
      switch (value) {
       case 0x152: value = 0x8c; break; /* &OElig; */
       case 0x153: value = 0x9c; break; /* &oelig; */
#if 0
      /* there are a few other obscure ones we don't currently need */
#endif
      }
      if (value < 0x100) {
	 *p = value;
	 return 1;
      }
      break;
#endif
#if (OS==MSDOS)
   case CHARSET_DOSCP437: {
      unsigned char uni2dostab[] = {
	  255, 173, 155, 156,   0, 157,   0,   0,
	    0,   0, 166, 174, 170,   0,   0,   0,
	  248, 241, 253,   0,   0, 230,   0, 250,
	    0,   0, 167, 175, 172, 171,   0, 168,
	    0,   0,   0,   0, 142, 143, 146, 128,
	    0, 144,   0,   0,   0,   0,   0,   0,
	    0, 165,   0,   0,   0,   0, 153,   0,
	    0,   0,   0,   0, 154,   0,   0, 225,
	  133, 160, 131,   0, 132, 134, 145, 135,
	  138, 130, 136, 137, 141, 161, 140, 139,
	    0, 164, 149, 162, 147,   0, 148, 246,
	    0, 151, 163, 150, 129,   0,   0, 152,
      };
      if (value >= 160 && value < 256) {
	 int ch = (int)uni2dostab[value - 160];
	 if (!ch) break;
	 *p = ch;
	 return 1;
      }
#if 0
      switch (value) {
	  case 8359: *p = 158; return 1; /* PESETA SIGN */
	  case 402: *p = 159; return 1; /* LATIN SMALL LETTER F WITH HOOK */
	  case 8976: *p = 169; return 1; /* REVERSED NOT SIGN */
	  case 945: *p = 224; return 1; /* GREEK SMALL LETTER ALPHA */
	  case 915: *p = 226; return 1; /* GREEK CAPITAL LETTER GAMMA */
	  case 960: *p = 227; return 1; /* GREEK SMALL LETTER PI */
	  case 931: *p = 228; return 1; /* GREEK CAPITAL LETTER SIGMA */
	  case 963: *p = 229; return 1; /* GREEK SMALL LETTER SIGMA */
	  case 964: *p = 231; return 1; /* GREEK SMALL LETTER TAU */
	  case 934: *p = 232; return 1; /* GREEK CAPITAL LETTER PHI */
	  case 920: *p = 233; return 1; /* GREEK CAPITAL LETTER THETA */
	  case 937: *p = 234; return 1; /* GREEK CAPITAL LETTER OMEGA */
	  case 948: *p = 235; return 1; /* GREEK SMALL LETTER DELTA */
	  case 8734: *p = 236; return 1; /* INFINITY */
	  case 966: *p = 237; return 1; /* GREEK SMALL LETTER PHI */
	  case 949: *p = 238; return 1; /* GREEK SMALL LETTER EPSILON */
	  case 8745: *p = 239; return 1; /* INTERSECTION */
	  case 8801: *p = 240; return 1; /* IDENTICAL TO */
	  case 8805: *p = 242; return 1; /* GREATER-THAN OR EQUAL TO */
	  case 8804: *p = 243; return 1; /* LESS-THAN OR EQUAL TO */
	  case 8992: *p = 244; return 1; /* TOP HALF INTEGRAL */
	  case 8993: *p = 245; return 1; /* BOTTOM HALF INTEGRAL */
	  case 8776: *p = 247; return 1; /* ALMOST EQUAL TO */
	  case 8729: *p = 249; return 1; /* BULLET OPERATOR */
	  case 8730: *p = 251; return 1; /* SQUARE ROOT */
	  case 8319: *p = 252; return 1; /* SUPERSCRIPT LATIN SMALL LETTER N */
	  case 9632: *p = 254; return 1; /* BLACK SQUARE */
      }
#endif
      break;
   }
#endif
#if (OS==MSDOS || OS==WIN32)
   case CHARSET_DOSCP850: {
      unsigned char uni2dostab[] = {
	 255, 173, 189, 156, 207, 190, 221, 245,
	 249, 184, 166, 174, 170, 240, 169, 238,
	 248, 241, 253, 252, 239, 230, 244, 250,
	 247, 251, 167, 175, 172, 171, 243, 168,
	 183, 181, 182, 199, 142, 143, 146, 128,
	 212, 144, 210, 211, 222, 214, 215, 216,
	 209, 165, 227, 224, 226, 229, 153, 158,
	 157, 235, 233, 234, 154, 237, 232, 225,
	 133, 160, 131, 198, 132, 134, 145, 135,
	 138, 130, 136, 137, 141, 161, 140, 139,
	 208, 164, 149, 162, 147, 228, 148, 246,
	 155, 151, 163, 150, 129, 236, 231, 152
      };
      if (value >= 160 && value < 256) {
	 *p = (int)uni2dostab[value - 160];
	 return 1;
      }
#if 0
      if (value == 305) { /* LATIN SMALL LETTER DOTLESS I */
	 *p = 213;
	 return 1;
      }
      if (value == 402) { /* LATIN SMALL LETTER F WITH HOOK */
	 *p = 159;
	 return 1;
      }
#endif
      break;
   }
#endif
   }
   /* Transliterate characters we can't represent */
#ifdef DEBUG
   fprintf(stderr, "transliterate `%c' 0x%x\n", value, value);
#endif
   switch (value) {
    case 160:
      *p = ' '; return 1;
    case 161 /* ¡ */:
      *p = '!'; return 1;
    case 171 /* « */:
      p[1] = *p = '<'; return 2;
    case 187 /* » */:
      p[1] = *p = '>'; return 2;
    case 191 /* ¿ */:
      *p = '?'; return 1;
    case 192 /* À */: case 193 /* Á */: case 194 /* Â */: case 195 /* Ã */:
      *p = 'A'; return 1;
    case 197 /* Å */:
      p[1] = *p = 'A'; return 2;
    case 196 /* Ä */: /* &Auml; */
      *p = 'A';
      if (!umlaut_to_e()) return 1;
      p[1] = 'E'; return 2;
    case 198 /* Æ */:
      *p = 'A'; p[1] = 'E'; return 2;
    case 199 /* Ç */: case 268: /* &Ccaron; */
      *p = 'C'; return 1;
    case 270: /* &Dcaron; */
      *p = 'D'; return 1;
    case 200 /* È */: case 201 /* É */: case 202 /* Ê */: case 203 /* Ë */:
      *p = 'E'; return 1;
    case 204 /* Ì */: case 205 /* Í */: case 206 /* Î */: case 207 /* Ï */:
      *p = 'I'; return 1;
    case 208 /* Ð */: case 222 /* Þ */:
      *p = 'T'; p[1] = 'H'; return 2;
    case 315: /* &Lacute; */
    case 317: /* &Lcaron; */
      *p = 'L'; return 1;
    case 209 /* Ñ */:
      *p = 'N'; return 1;
    case 210 /* Ò */: case 211 /* Ó */: case 212 /* Ô */: case 213 /* Õ */:
      *p = 'O'; return 1;
    case 214 /* Ö */: /* &Ouml; */ case 0x152: /* &OElig; */
      *p = 'O'; p[1] = 'E'; return 2;
    case 352: /* &Scaron; */
    case 0x15e: /* &Scedil; */
      *p = 'S'; return 1;
    case 0x162: /* &Tcedil; */
    case 0x164: /* &Tcaron; */
      *p = 'T'; return 1;
    case 217 /* Ù */: case 218 /* Ú */: case 219 /* Û */:
      *p = 'U'; return 1;
    case 220 /* Ü */: /* &Uuml; */
      *p = 'U'; p[1] = 'E'; return 2;
    case 221 /* Ý */:
      *p = 'Y'; return 1;
    case 381: /* &Zcaron; */
      *p = 'Z'; return 1;
    case 223 /* ß */:
      p[1] = *p = 's'; return 2;
    case 224 /* à */: case 225 /* á */: case 226 /* â */: case 227 /* ã */:
    case 259: /* &abreve; */
      *p = 'a'; return 1;
    case 228 /* ä */: /* &auml; */ case 230 /* æ */:
      *p = 'a'; p[1] = 'e'; return 2;
    case 229 /* å */:
      p[1] = *p = 'a'; return 2;
    case 231 /* ç */: case 269 /* &ccaron; */:
      *p = 'c'; return 1;
    case 271: /* &dcaron; */
      *p = 'd'; return 1;
    case 232 /* è */: case 233 /* é */: case 234 /* ê */: case 235 /* ë */:
    case 283 /* &ecaron; */:
      *p = 'e'; return 1;
    case 236 /* ì */: case 237 /* í */: case 238 /* î */: case 239 /* ï */:
      *p = 'i'; return 1;
    case 316 /* &lacute; */:
    case 318 /* &lcaron; */:
      *p = 'l'; return 1;
    case 241 /* ñ */: case 328 /* &ncaron; */:
      *p = 'n'; return 1;
    case 345: /* &rcaron; */
      *p = 'r'; return 1;
    case 353: /* &scaron; */
    case 0x15f: /* &scedil; */
      *p = 's'; return 1;
    case 357: /* &tcaron; */
    case 0x163: /* &tcedil; */
      *p = 't'; return 1;
    case 240 /* ð */: case 254 /* þ */:
      *p = 't'; p[1] = 'h'; return 2;
    case 242 /* ò */: case 243 /* ó */: case 244 /* ô */: case 245 /* õ */:
      *p = 'o'; return 1;
    case 246 /* ö */: /* &ouml; */ case 0x153: /* &oelig; */
      *p = 'o'; p[1] = 'e'; return 2;
    case 249 /* ù */: case 250 /* ú */: case 251 /* û */:
    case 367 /* &uring; */:
      *p = 'u'; return 1;
    case 252 /* ü */: /* &uuml; */
      *p = 'u'; p[1] = 'e'; return 2;
    case 253 /* ý */: case 255 /* ÿ */:
      *p = 'y'; return 1;
    case 382: /* &zcaron; */
      *p = 'z'; return 1;
   }
#ifdef DEBUG
   fprintf(stderr, "failed to transliterate\n");
#endif
   return 0;
}

#if (OS==UNIX) && defined(DATADIR) && defined(PACKAGE)
/* Under Unix, we compile in the configured path */
static const char *pth_cfg_files = DATADIR "/" PACKAGE;
#else
/* On other platforms, we fall back on looking in the current directory */
static const char *pth_cfg_files = "";
#endif

static int num_msgs = 0;
static char **msg_array = NULL;

const char *msg_lang = NULL;
const char *msg_lang2 = NULL;

static char **
parse_msgs(int n, unsigned char *p, int charset_code) {
   int i;

   char **msgs = osmalloc(n * sizeof(char *));

   for (i = 0; i < n; i++) {
      unsigned char *to = p;
      int ch;
      msgs[i] = (char *)p;

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
	    /* We assume an N byte UTF-8 code never transliterates to more
	     * than N characters (so we can't transliterate © to (C) or
	     * ® to (R) for example) */
	    to += add_unicode(charset_code, to, ch);
	 }
      }
      *to++ = '\0';
   }
   return msgs;
}

/* This is the name of the default language, which can be set like so:
 * ./configure --enable-defaultlang=fr
 */
#ifdef DEFAULTLANG
/* No point extracting these errors as they won't get used if file opens */
# include "../lib/defaultlang.h"
#else
#define N_DONTEXTRACTMSGS 5
static unsigned char dontextractmsgs[] =
   "Can't open message file `%s' using path `%s'\0"/*1000*/
   "Problem with message file `%s'\0"/*1001*/
   "I don't understand this message file version\0"/*1002*/
   "Message file truncated?\0"/*1003*/
   "Out of memory (couldn't find %lu bytes).\0"/*1004*/;
#endif

static char **dontextract = NULL;

static void
parse_msg_file(int charset_code)
{
   FILE *fh;
   unsigned char header[20];
   int i;
   unsigned len;
   unsigned char *p;
   char *fnm, *s;
   int n;

#ifdef DEBUG
   fprintf(stderr, "parse_msg_file(%d)\n", charset_code);
#endif

   /* sort out messages we need to print if we can't open the message file */
   dontextract = parse_msgs(N_DONTEXTRACTMSGS, dontextractmsgs, charset_code);

   fnm = osstrdup(msg_lang);
   /* trim off charset from stuff like "de_DE.iso8859_1" */
   s = strchr(fnm, '.');
   if (s) *s = '\0';

   fh = fopenWithPthAndExt(pth_cfg_files, fnm, EXT_SVX_MSG, "rb", NULL);

   if (!fh) {
      /* e.g. if 'en_GB' is unknown, see if we know 'en' */
      if (strlen(fnm) > 3 && fnm[2] == '_') {
	 fnm[2] = '\0';
	 fh = fopenWithPthAndExt(pth_cfg_files, fnm, EXT_SVX_MSG, "rb", NULL);
	 if (!fh) fnm[2] = '_'; /* for error reporting */
      }
   }

   if (!fh) {
      fatalerror(/*Can't open message file `%s' using path `%s'*/1000,
		 fnm, pth_cfg_files);
   }

   if (fread(header, 1, 20, fh) < 20 ||
       memcmp(header, "Svx\nMsg\r\n\xfe\xff", 12) != 0) {
      fatalerror(/*Problem with message file `%s'*/1001, fnm);
   }

   if (header[12] != 0)
      fatalerror(/*I don't understand this message file version*/1002);

   n = (header[14] << 8) | header[15];

   len = 0;
   for (i = 16; i < 20; i++) len = (len << 8) | header[i];

   p = osmalloc(len);
   if (fread(p, 1, len, fh) < len)
      fatalerror(/*Message file truncated?*/1003);

   fclose(fh);

#ifdef DEBUG
   fprintf(stderr, "fnm = `%s', n = %d, len = %d\n", fnm, n, len);
#endif
   osfree(fnm);

   msg_array = parse_msgs(n, p, charset_code);
   num_msgs = n;
}

const char *
msg_cfgpth(void)
{
   return pth_cfg_files;
}

const char *
msg_appname(void)
{
   return appname_copy;
}

void
msg_init(char * const *argv)
{
   char *p;
   SVX_ASSERT(argv);

#ifdef HAVE_SIGNAL
   init_signals();
#endif
   /* Point to argv[0] itself so we report a more helpful error if the
    * code to work out the clean appname generates a signal */
   appname_copy = argv[0];
#if (OS == UNIX)
   /* use name as-is on Unix - programs run from path get name as supplied */
   appname_copy = osstrdup(argv[0]);
#else
   /* use the lower-cased leafname on other platforms */
   p = leaf_from_fnm(argv[0]);
   appname_copy = p;
   while (*p) {
      *p = tolower(*p);
      ++p;
   }
#endif

   /* shortcut --version so you can check the version number even when the
    * correct message file can't be found... */
   if (argv[1] && strcmp(argv[1], "--version") == 0) {
      cmdline_version();
      exit(0);
   }
   if (argv[0]) {
#ifdef MACOSX_BUNDLE
      /* If we're being built into a bundle, always look relative to
       * the path to the binary. */
      char * pth = path_from_fnm(argv[0]);
      pth_cfg_files = use_path(pth, "share/survex");
      osfree(pth);
#elif (OS==UNIX) && defined(DATADIR) && defined(PACKAGE)
      bool free_pth = fFalse;
      char *pth = getenv("srcdir");
      if (!pth || !pth[0]) {
	 pth = path_from_fnm(argv[0]);
	 free_pth = fTrue;
      }
      if (pth[0]) {
	 struct stat buf;
#if defined(__GNUC__) && defined(__APPLE_CC__)
	 /* On MacOS X the programs may be installed anywhere, with the
	  * share directory and the binaries in the same directory. */
	 p = use_path(pth, "share/survex/en.msg");
	 if (lstat(p, &buf) == 0 && S_ISREG(buf.st_mode)) {
	    pth_cfg_files = use_path(pth, "share/survex");
	    goto macosx_got_msg;
	 }
	 osfree(p);
#endif
	 /* If we're run with an explicit path, check if "../lib/en.msg"
	  * from the program's path exists, and if so look there for
	  * support files - this allows us to test binaries in the build
	  * tree easily. */
	 p = use_path(pth, "../lib/en.msg");
	 if (lstat(p, &buf) == 0) {
#ifdef S_ISDIR
	    /* POSIX way */
	    if (S_ISREG(buf.st_mode)) {
	       pth_cfg_files = use_path(pth, "../lib");
	    }
#else
	    /* BSD way */
	    if ((buf.st_mode & S_IFMT) == S_IFREG) {
	       pth_cfg_files = use_path(pth, "../lib");
	    }
#endif
	 }
#if defined(__GNUC__) && defined(__APPLE_CC__)
macosx_got_msg:
#endif
	 osfree(p);
      }

      if (free_pth) osfree(pth);
#elif (OS==WIN32)
      DWORD len = 256;
      char *buf = NULL, *modname;
      while (1) {
	  DWORD got;
	  buf = osrealloc(buf, len);
	  got = GetModuleFileName(NULL, buf, len);
	  if (got < len) break;
	  len += len;
      }
      modname = buf;
      /* Strange Win32 nastiness - strip prefix "\\?\" if present */
      if (strncmp(modname, "\\\\?\\", 4) == 0) modname += 4;
      pth_cfg_files = path_from_fnm(modname);
      osfree(buf);
#else
      /* Get the path to the support files from argv[0] */
      pth_cfg_files = path_from_fnm(argv[0]);
#endif
   }

   msg_lang = getenv("SURVEXLANG");
#ifdef DEBUG
   fprintf(stderr, "msg_lang = %p (= \"%s\")\n", msg_lang, msg_lang?msg_lang:"(null)");
#endif

   if (!msg_lang || !*msg_lang) {
      msg_lang = getenv("LC_MESSAGES");
      if (!msg_lang || !*msg_lang) {
	 msg_lang = getenv("LANG");
	 /* Something (AutoCAD?) on Microsoft Windows sets LANG to a number. */
	 if (msg_lang && !isalpha(msg_lang[0])) msg_lang = NULL;
      }
      if (!msg_lang || !*msg_lang) {
#if (OS==WIN32)
	 LCID locid;
#elif (OS==RISCOS)
         territory_t t;
#endif
#ifdef DEFAULTLANG
	 msg_lang = STRING(DEFAULTLANG);
#else
	 msg_lang = "en";
#endif
#if (OS==WIN32)
	 locid = GetUserDefaultLCID();
	 if (locid) {
	    WORD langid = LANGIDFROMLCID(locid);
	    switch (PRIMARYLANGID(langid)) {
/* older mingw compilers don't seem to supply this value */
#ifndef LANG_CATALAN
# define LANG_CATALAN 0x03
#endif
	     case LANG_CATALAN:
	       msg_lang = "ca";
	       break;
	     case LANG_CHINESE:
	       msg_lang = "zh";
	       break;
	     case LANG_ENGLISH:
	       if (SUBLANGID(langid) == SUBLANG_ENGLISH_US)
		  msg_lang = "en_US";
	       else
		  msg_lang = "en";
	       break;
	     case LANG_FRENCH:
	       msg_lang = "fr";
	       break;
	     case LANG_GERMAN:
	       switch (SUBLANGID(langid)) {
		case SUBLANG_GERMAN_SWISS:
		  msg_lang = "de_CH";
		  break;
		case SUBLANG_GERMAN:
		  msg_lang = "de_DE";
		  break;
		default:
		  msg_lang = "de";
	       }
	       break;
	     case LANG_ITALIAN:
	       msg_lang = "it";
	       break;
	     case LANG_PORTUGUESE:
	       if (SUBLANGID(langid) == SUBLANG_PORTUGUESE_BRAZILIAN)
		  msg_lang = "pt_BR";
	       else
		  msg_lang = "pt";
	       break;
	     case LANG_ROMANIAN:
	       msg_lang = "ro";
	       break;
	     case LANG_SLOVAK:
	       msg_lang = "sk";
	       break;
	     case LANG_SPANISH:
	       msg_lang = "es";
	       break;
	    }
	 }
#elif (OS==RISCOS)
         if (!xterritory_number(&t)) switch (t) {
	  case 1: /* UK */
	  case 2: /* Master */
	  case 3: /* Compact */
	  case 17: /* Canada1 */
	  case 19: /* Canada */
	  case 22: /* Ireland */
	    msg_lang = "en";
	    break;
	  case 4: /* Italy */
	    msg_lang = "it";
	    break;
	  case 5: /* Spain (or ca) */
	  case 27: /* Mexico */
	  case 28: /* LatinAm (or pt_BR) */
	    msg_lang = "es";
	    break;
	  case 6: /* France */
	  case 18: /* Canada2 */
	    msg_lang = "fr";
	    break;
	  case 7: /* Germany */
	    msg_lang = "de_DE";
	    break;
	  case 8: /* Portugal */
	    msg_lang = "pt";
	    break;
	  case 23: /* Hong Kong */
	    msg_lang = "zh";
	    break;
	  case 48: /* USA */
	    msg_lang = "en_US";
	    break;
#if 0
	  case 9: /* Esperanto */
	  case 10: /* Greece */
	  case 11: /* Sweden */
	  case 12: /* Finland */
	  case 13: /* Unused */
	  case 14: /* Denmark */
	  case 15: /* Norway */
	  case 16: /* Iceland */
	  case 20: /* Turkey */
	  case 21: /* Arabic */
	  case 24: /* Russia */
	  case 25: /* Russia2 */
	  case 26: /* Israel */
#endif
	 }
#elif (OS==MSDOS)
	   {
	      int country_code;
# ifdef __DJGPP__
	      __dpmi_regs r;
	      r.x.ax = 0x6501;
	      r.x.bx = 0xffff;
	      r.x.dx = 0xffff;
	      /* Use DJGPP's transfer buffer (which is at least 2K) */
	      r.x.es = __tb >> 4;
	      r.x.di = __tb & 0x0f;
	      r.x.cx = 2048;
	      /* bit 1 is the carry flag */
	      if (__dpmi_int(0x21, &r) != -1 && !(r.x.flags & 1)) {
		 unsigned short val;
		 dosmemget(__tb + 3, 2, &val);
		 country_code = val;
# else
	      union REGS r;
	      r.x.ax = 0x3800; /* get current country info */
	      r.x.dx = 0;
	      intdos(&r, &r);
	      if (!r.x.cflag) {
		 country_code = r.x.bx;
# endif
		 /* List of country codes taken from:
		  * http://www.delorie.com/djgpp/doc/rbinter/it/00/14.html */
		 /* The mappings here are guesses at best in most cases.
		  * In a lot of cases we pick a language because we have
		  * a translation in it, rather than because it's the most
		  * widely used or understood in that country. */
		 /* Improvements welcome */
		 switch (country_code) {
		     case 1: /* United States */
		     case 670: /* Saipan / N. Mariana Island */
		     case 671: /* Guam */
		     case 680: /* Palau */
		     case 684: /* American Samoa */
		     case 691: /* Micronesia */
		     case 692: /* Marshall Islands */
			 msg_lang = "en_US";
			 break;
		     case 4: /* Canada (English) */
		     case 27: /* South Africa */
		     case 44: /* United Kingdom */
		     case 61: /* International English / Australia */
		     case 64: /* New Zealand */
		     case 99: /* Asia (English) */
		     case 220: /* Gambia */
		     case 231: /* Liberia */
		     case 232: /* Sierra Leone */
		     case 233: /* Ghana */
		     case 254: /* Kenya */
		     case 256: /* Uganda */
		     case 260: /* Zambia */
		     case 263: /* Zimbabwe */
		     case 264: /* Namibia */
		     case 267: /* Botswana */
		     case 268: /* Swaziland */
		     case 290: /* St. Helena */
		     case 297: /* Aruba */
		     case 350: /* Gibraltar */
		     case 353: /* Ireland */
		     case 356: /* Malta */
		     case 500: /* Falkland Islands */
		     case 501: /* Belize */
		     case 592: /* Guyana */
		     case 672: /* Norfolk Island (Australia) / Christmas Island/Cocos Islands / Antartica */
		     case 673: /* Brunei Darussalam */
		     case 674: /* Nauru */
		     case 675: /* Papua New Guinea */
		     case 676: /* Tonga Islands */
		     case 677: /* Solomon Islands */
		     case 679: /* Fiji */
		     case 682: /* Cook Islands */
		     case 683: /* Niue */
		     case 685: /* Western Samoa */
		     case 686: /* Kiribati */
			 /* I believe only some of these are English speaking... */
		     case 809: /* Antigua and Barbuda / Anguilla / Bahamas / Barbados / Bermuda
				  British Virgin Islands / Cayman Islands / Dominica
				  Dominican Republic / Grenada / Jamaica / Montserra
				  St. Kitts and Nevis / St. Lucia / St. Vincent and Grenadines
				  Trinidad and Tobago / Turks and Caicos */
			 msg_lang = "en";
			 break;
		     case 2: /* Canadian-French */
		     case 32: /* Belgium */ /* maybe */
		     case 33: /* France */
		     case 213: /* Algeria */
		     case 216: /* Tunisia */
		     case 221: /* Senegal */
		     case 223: /* Mali */
		     case 225: /* Ivory Coast */
		     case 226: /* Burkina Faso */
		     case 227: /* Niger */
		     case 228: /* Togo */
		     case 229: /* Benin */
		     case 230: /* Mauritius */
		     case 235: /* Chad */
		     case 236: /* Central African Republic */
		     case 237: /* Cameroon */
		     case 241: /* Gabon */
		     case 242: /* Congo */
		     case 250: /* Rwhanda */
		     case 253: /* Djibouti */
		     case 257: /* Burundi */
		     case 261: /* Madagascar */
		     case 262: /* Reunion Island */
		     case 269: /* Comoros */
		     case 270: /* Mayotte */
		     case 352: /* Luxembourg (or de or ...) */
		     case 508: /* St. Pierre and Miquelon */
		     case 509: /* Haiti */
		     case 590: /* Guadeloupe */
		     case 594: /* French Guiana */
		     case 596: /* Martinique / French Antilles */
		     case 678: /* Vanuatu */
		     case 681: /* Wallis & Futuna */
		     case 687: /* New Caledonia */
		     case 689: /* French Polynesia */
		     case 961: /* Lebanon */
			 msg_lang = "fr";
			 break;
		     case 3: /* Latin America */
		     case 34: /* Spain */
		     case 51: /* Peru */
		     case 52: /* Mexico */
		     case 53: /* Cuba */
		     case 54: /* Argentina */
		     case 56: /* Chile */
		     case 57: /* Columbia */
		     case 58: /* Venezuela */
		     case 63: /* Philippines */
		     case 240: /* Equatorial Guinea */
		     case 502: /* Guatemala */
		     case 503: /* El Salvador */
		     case 504: /* Honduras */
		     case 505: /* Nicraragua */
		     case 506: /* Costa Rica */
		     case 507: /* Panama */
		     case 591: /* Bolivia */
		     case 593: /* Ecuador */
		     case 595: /* Paraguay */
		     case 598: /* Uruguay */
			 msg_lang = "es";
			 break;
		     case 39: /* Italy / San Marino / Vatican City */
			 msg_lang = "it";
			 break;
		     case 41: /* Switzerland / Liechtenstein */ /* or fr or ... */
			 msg_lang = "de_CH";
			 break;
		     case 43: /* Austria (DR DOS 5.0) */
			 msg_lang = "de";
			 break;
		     case 49: /* Germany */
			 msg_lang = "de_DE";
			 break;
		     case 55: /* Brazil (not supported by DR DOS 5.0) */
			 msg_lang = "pt_BR";
			 break;
		     case 238: /* Cape Verde Islands */
		     case 244: /* Angola */
		     case 245: /* Guinea-Bissau */
		     case 259: /* Mozambique */
		     case 351: /* Portugal */
			 msg_lang = "pt";
			 break;
		     case 42: /* Czechoslovakia / Tjekia / Slovakia (not supported by DR DOS 5.0) */
		     case 421: /* Czech Republic / Tjekia (PC DOS 7+) */
		     case 422: /* Slovakia (reported as 421 due to a bug in COUNTRY.SYS) */
			 msg_lang = "sk";
			 break;
		     case 65: /* Singapore */
		     case 86: /* China (MS-DOS 5.0+) */
		     case 88: /* Taiwan (MS-DOS 5.0+) */
		     case 852: /* Hong Kong */
		     case 853: /* Macao */
		     case 886: /* Taiwan (MS-DOS 6.22+) */
			 msg_lang = "zh";
			 break;
#if 0
		     case 7: /* Russia */
		     case 20: /* Egypt */
		     case 30: /* Greece */
		     case 31: /* Netherlands */
		     case 35: /* Bulgaria??? */
		     case 36: /* Hungary (not supported by DR DOS 5.0) */
		     case 38: /* Yugoslavia (not supported by DR DOS 5.0) -- obsolete */
#endif
		     case 40: /* Romania */
			 msg_lang = "ro";
			 break;
#if 0
		     case 45: /* Denmark */
		     case 46: /* Sweden */
		     case 47: /* Norway */
		     case 48: /* Poland (not supported by DR DOS 5.0) */
		     case 60: /* Malaysia */
		     case 62: /* Indonesia / East Timor */
		     case 66: /* Thailand (or Taiwan??? ) */
		     case 81: /* Japan (DR DOS 5.0, MS-DOS 5.0+) */
		     case 82: /* South Korea (DR DOS 5.0) */
		     case 84: /* Vietnam */
		     case 90: /* Turkey (MS-DOS 5.0+) */
		     case 91: /* India */
		     case 92: /* Pakistan */
		     case 93: /* Afghanistan */
		     case 94: /* Sri Lanka */
		     case 98: /* Iran */
		     case 102: /* ??? (Hebrew MS-DOS 5.0) */
		     case 112: /* Belarus */
		     case 200: /* Thailand (PC DOS 6.1+) (reported as 01due to a bug in PC DOS COUNTRY.SYS) */
		     case 212: /* Morocco */
		     case 218: /* Libya */
		     case 222: /* Maruitania */
		     case 224: /* African Guinea */
		     case 234: /* Nigeria */
		     case 239: /* Sao Tome and Principe */
		     case 243: /* Zaire */
		     case 246: /* Diego Garcia */
		     case 247: /* Ascension Isle */
		     case 248: /* Seychelles */
		     case 249: /* Sudan */
		     case 251: /* Ethiopia */
		     case 252: /* Somalia */
		     case 255: /* Tanzania */
		     case 265: /* Malawi */
		     case 266: /* Lesotho */
		     case 298: /* Faroe Islands */
		     case 299: /* Greenland */
		     case 354: /* Iceland */
		     case 355: /* Albania */
		     case 357: /* Cyprus */
		     case 358: /* Finland */
		     case 359: /* Bulgaria */
		     case 370: /* Lithuania (reported as 372 due to a bug in MS-DOS COUNTRY.SYS) */
		     case 371: /* Latvia (reported as 372 due to a bug in MS-DOS COUNTRY.SYS) */
		     case 372: /* Estonia */
		     case 373: /* Moldova */ /* FIXME: similar to Romanian? */
		     case 375: /* ??? (MS-DOS 7.10 / Windows98) */
		     case 380: /* Ukraine */
		     case 381: /* Serbia / Montenegro */
		     case 384: /* Croatia */
		     case 385: /* Croatia (PC DOS 7+) */
		     case 386: /* Slovenia */
		     case 387: /* Bosnia-Herzegovina (Latin) */
		     case 388: /* Bosnia-Herzegovina (Cyrillic) (PC DOS 7+) (reported as 381 due to a bug in PC DOS COUNTRY.SYS) */
		     case 389: /* FYR Macedonia */
		     case 597: /* Suriname (nl) */
		     case 599: /* Netherland Antilles (nl) */
		     case 666: /* Russia??? (PTS-DOS 6.51 KEYB) */
		     case 667: /* Poland??? (PTS-DOS 6.51 KEYB) */
		     case 668: /* Poland??? (Slavic??? ) (PTS-DOS 6.51 KEYB) */
		     case 688: /* Tuvalu */
		     case 690: /* Tokealu */
		     case 711: /* ??? (currency = EA$, code pages 437,737,850,852,855,857) */
		     case 785: /* Arabic (Middle East/Saudi Arabia/etc.) */
		     case 804: /* Ukraine */
		     case 850: /* North Korea */
		     case 855: /* Cambodia */
		     case 856: /* Laos */
		     case 880: /* Bangladesh */
		     case 960: /* Maldives */
		     case 962: /* Jordan */
		     case 963: /* Syria / Syrian Arab Republic */
		     case 964: /* Iraq */
		     case 965: /* Kuwait */
		     case 966: /* Saudi Arabia */
		     case 967: /* Yemen */
		     case 968: /* Oman */
		     case 969: /* Yemen??? (Arabic MS-DOS 5.0) */
		     case 971: /* United Arab Emirates */
		     case 972: /* Israel (Hebrew) (DR DOS 5.0,MS-DOS 5.0+) */
		     case 973: /* Bahrain */
		     case 974: /* Qatar */
		     case 975: /* Bhutan */
		     case 976: /* Mongolia */
		     case 977: /* Nepal */
		     case 995: /* Myanmar (Burma) */
#endif
		 }
	      }
	   }
#endif
      }
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

/* Message may be overwritten by next call
 * (but not in current implementation) */
const char *
msg(int en)
{
   /* NB can't use SVX_ASSERT here! */
   static char badbuf[256];
   if (en >= 1000 && en < 1000 + N_DONTEXTRACTMSGS)
      return dontextract[en - 1000];
   if (!msg_array) {
      if (en != 1)  {
	 sprintf(badbuf, "Message %d requested before msg_array initialised\n",
		 en);
	 return badbuf;
      }
      /* this should be the only other message which can be requested before
       * the message file is opened and read... */
      if (!dontextract) return "Out of memory (couldn't find %lu bytes).";
      return dontextract[(/*Out of memory (couldn't find %lu bytes).*/1004)
			 - 1000];
   }

   if (en < 0 || en >= num_msgs) {
      sprintf(badbuf, "Message %d out of range\n", en);
      return badbuf;
   }

   if (en == 0) {
      const char *p = msg_array[0];
      if (!*p) p = "(C)";
      return p;
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
#ifdef AVEN
   aven_v_report(severity, fnm, line, en, ap);
#else
   if (fnm) {
      fputs(fnm, STDERR);
      if (line) fprintf(STDERR, ":%d", line);
   } else {
      fputs(appname_copy, STDERR);
   }
   fputs(": ", STDERR);

   if (severity == 0) {
      fputs(msg(/*warning*/4), STDERR);
      fputs(": ", STDERR);
   }

   vfprintf(STDERR, msg(en), ap);
   fputnl(STDERR);
#endif

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
