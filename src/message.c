/* message.c
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

#ifdef AVEN
#include "aven.h"
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
#include <windows.h>
#endif

#if (OS==RISCOS)
#include "oslib/wimpreadsy.h"
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

char FAR *
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
   int version;
   if (xwimpreadsysinfo_version(&version) != NULL) {
      /* RISC OS 2 or some error (don't care which) */
      return CHARSET_ISO_8859_1;
   }

   /* oddly wimp_VERSION_RO3 is RISC OS 3.1 */
   if (version < wimp_VERSION_RO3) return CHARSET_ISO_8859_1;

   return CHARSET_RISCOS31;
#elif (OS==MSDOS)
   return CHARSET_DOSCP850;
#elif (OS==WIN32)
# ifdef AVEN
#  define CODEPAGE GetACP()
# else
#  define CODEPAGE GetConsoleOutputCP()
# endif
   switch (CODEPAGE) {
    case 1252: return CHARSET_WINCP1252;
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
	    if (isalpha(chset[cnt])) {
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
		  while (chset < p && *chset && !isdigit(*chset)) chset++;
		  switch (atoi(chset)) {
		   case 1: return CHARSET_ISO_8859_1;
		   case 15: return CHARSET_ISO_8859_15;
		   default: return CHARSET_USASCII;		   
		  }
	       }
	    }
	    break;
	  case 'u':
	    if (tolower(chset[1]) == 't' && tolower(chset[2]) == 'f') {
	       chset += 3;
	       while (chset < p && *chset && !isdigit(*chset)) chset++;
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

#if (OS==MSDOS || OS==WIN32)
static int
xlate_dos_cp850(int unicode)
{
   switch (unicode) {
#include "uni2dos.h"
   }
   return 0;
}
#endif

/* It seems that Swedish and maybe some other scandanavian languages don't
 * transliterate &auml; to ae - but it seems there may be conflicting views
 * on this...
 */
#define umlaut_to_e() 1

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
   case CHARSET_ISO_8859_15:
      switch (value) {
       case 0xa4: case 0xa6: case 0xb0: case 0xc4:
       case 0xd0: case 0xd4: case 0xd5: case 0xd6:
	 goto donthave;
       case 0x152: value = 0xd4; break; /* &OElig; */
       case 0x153: value = 0xd5; break; /* &oelig; */
#if 0
       case 0x0: value = 0xa4; break; /* euro */
       case 0x0: value = 0xa6; break; /* Scaron */
       case 0x0: value = 0xb0; break; /* scaron */
       case 0x0: value = 0xc4; break; /* Zcaron */
       case 0x0: value = 0xd0; break; /* zcaron */
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
#if (OS==MSDOS || OS==WIN32)
   case CHARSET_DOSCP850:
      value = xlate_dos_cp850(value);
      if (value) {
	 *p = value;
	 return 1;
      }
      break;
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
    case 199 /* Ç */:
      *p = 'C'; return 1;
    case 200 /* È */: case 201 /* É */: case 202 /* Ê */: case 203 /* Ë */:
      *p = 'E'; return 1;
    case 204 /* Ì */: case 205 /* Í */: case 206 /* Î */: case 207 /* Ï */:
      *p = 'I'; return 1;
    case 208 /* Ð */: case 222 /* Þ */:
      *p = 'T'; p[1] = 'H'; return 2;
    case 209 /* Ñ */:
      *p = 'N'; return 1;
    case 210 /* Ò */: case 211 /* Ó */: case 212 /* Ô */: case 213 /* Õ */:
      *p = 'O'; return 1;
    case 214 /* Ö */: /* &Ouml; */ case 0x152: /* &OElig; */
      *p = 'O'; p[1] = 'E'; return 2;
    case 217 /* Ù */: case 218 /* Ú */: case 219 /* Û */:
      *p = 'U'; return 1;
    case 220 /* Ü */: /* &Uuml; */
      *p = 'U'; p[1] = 'E'; return 2;
    case 221 /* Ý */:
      *p = 'Y'; return 1;
    case 223 /* ß */:
      p[1] = *p = 's'; return 2;
    case 224 /* à */: case 225 /* á */: case 226 /* â */: case 227 /* ã */:
      *p = 'a'; return 1;
    case 228 /* ä */: /* &auml; */ case 230 /* æ */:
      *p = 'a'; p[1] = 'e'; return 2;
    case 229 /* å */:
      p[1] = *p = 'a'; return 2;
    case 231 /* ç */:
      *p = 'c'; return 1;
    case 232 /* è */: case 233 /* é */: case 234 /* ê */: case 235 /* ë */:
      *p = 'e'; return 1;
    case 236 /* ì */: case 237 /* í */: case 238 /* î */: case 239 /* ï */:
      *p = 'i'; return 1;
    case 241 /* ñ */:
      *p = 'n'; return 1;
    case 240 /* ð */: case 254 /* þ */:
      *p = 't'; p[1] = 'h'; return 2;
    case 242 /* ò */: case 243 /* ó */: case 244 /* ô */: case 245 /* õ */: 
      *p = 'o'; return 1;
    case 246 /* ö */: /* &ouml; */ case 0x153: /* &oelig; */
      *p = 'o'; p[1] = 'e'; return 2;
    case 249 /* ù */: case 250 /* ú */: case 251 /* û */:
      *p = 'u'; return 1;
    case 252 /* ü */: /* &uuml; */
      *p = 'u'; p[1] = 'e'; return 2;
    case 253 /* ý */: case 255 /* ÿ */:
      *p = 'y'; return 1;
   }
#ifdef DEBUG
   fprintf(stderr, "failed to transliterate\n");
#endif
   return 0;
}

/* fall back on looking in the current directory */
static const char *pth_cfg_files = "";

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

/* This is the name of the default language.  Add -DDEFAULTLANG to CFLAGS
 * e.g. with `CFLAGS="-DDEFAULTLANG=fr" ./configure'
 */
#ifdef DEFAULTLANG
/* No point extracting these errors as they won't get used if file opens */
/* FIXME: this works on gcc but not some other compilers (e.g. norcroft),
 * and also ../lib/fr.h, etc don't go into srcN_NN.zip */
# define HDR(D) "../lib/"STRING(D)".h"
# include HDR(DEFAULTLANG)
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
   ASSERT(argv);

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
   appname_copy = p = leaf_from_fnm(argv[0]);
   while (*p) {
      *p = tolower(*p);
      p++;
   }
#endif

   /* shortcut --version so you can check the version number when the correct
    * message file can't be found... */
   if (argv[1] && strcmp(argv[1], "--version") == 0) {
      cmdline_version();
      exit(0);
   }

   /* Look for env. var. "SURVEXHOME" or the like */
   p = getenv("SURVEXHOME");
   if (p && *p) {
      pth_cfg_files = osstrdup(p);
#if (OS==UNIX) && defined(DATADIR) && defined(PACKAGE)
   } else {
      /* under Unix, we compile in the configured path */
      pth_cfg_files = DATADIR "/" PACKAGE;
#else
   } else if (argv[0]) {
      /* else try the path on argv[0] */
      pth_cfg_files = path_from_fnm(argv[0]);
#endif
   }

   msg_lang = getenv("SURVEXLANG");
#ifdef DEBUG
   fprintf(stderr, "msg_lang = %p (= \"%s\")\n", msg_lang, msg_lang?msg_lang:"(null)");
#endif

   if (!msg_lang || !*msg_lang) {
      msg_lang = getenv("LANG");
      if (!msg_lang || !*msg_lang) {
#if (OS==WIN32)
	 LCID locid;
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
	     case LANG_SPANISH:
	       msg_lang = "es";
	       break;
	    }
	 }
#elif (OS==RISCOS)
	 switch (xterritory_number()) {
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
	  case 5: /* Spain */
	  case 27: /* Mexico */
	    msg_lang = "es"; /* or possibly ca for Spain */
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
	  case 48: /* USA */
	    msg_lang = "en_US";
	    break;
	  case 28: /* LatinAm */
	    msg_lang = "pt_BR"; /* or many other possibilities */
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
	  case 23: /* Hong Kong */
	  case 24: /* Russia */
	  case 25: /* Russia2 */
	  case 26: /* Israel */
#endif	    
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
   /* NB can't use ASSERT here! */
   static char badbuf[256];
   if (en >= 1000 && en < 1000 + N_DONTEXTRACTMSGS)
      return dontextract[en - 1000];
   if (!msg_array) {
      if (en != 1)  {
	 sprintf(badbuf, "Message %d requested before msg_array initialised\n", en);
	 return badbuf;
      }
      /* this should be the only other message which can be requested before
       * the message file is opened and read... */
      if (!dontextract) return "Out of memory (couldn't find %lu bytes).";
      return dontextract[4];
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
