/* > message.c
 * Fairly general purpose message and error routines
 * Copyright (C) 1993-1998 Olly Betts
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

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

/* This is the name of the default language -- set like this so folks can
 * add (for eg) -DDEFAULTLANG="fr" to UFLG in the makefile
 * FIXME - update wrt automake/autoconf
 */
#ifndef DEFAULTLANG
# define DEFAULTLANG "en"
#endif

/* For funcs which want to be immune from messing around with different
 * calling conventions */
#ifndef CDECL
# define CDECL
#endif

static int cWarnings = 0; /* keep track of how many warnings we've given */
static int cErrors = 0;   /* and how many (non-fatal) errors */

extern int error_summary(void) {
   fprintf(STDERR, msg(/*There were %d warning(s) and %d non-fatal error(s).*/16),
	   cWarnings, cErrors);
   fputnl(STDERR);
   return (cErrors ? EXIT_FAILURE : EXIT_SUCCESS);
}

/* in case osmalloc() fails before szAppNameCopy is set up */
const char *szAppNameCopy = "anonymous program";

/* error code for failed osmalloc and osrealloc calls */
static void
outofmem(OSSIZE_T size)
{
   fatalerror(1/*Out of memory (couldn't find %lu bytes).*/, (unsigned long)size);
}

/* malloc with error catching if it fails. Also allows us to write special
 * versions easily eg for DOS EMS or MS Windows.
 */
extern void FAR *
osmalloc(OSSIZE_T size)
{
   void FAR *p;
   p = xosmalloc(size);
   if (p == NULL) outofmem(size);
   return p;
}

/* realloc with error catching if it fails. */
extern void FAR *
osrealloc(void *p, OSSIZE_T size)
{
   p = xosrealloc(p, size);
   if (p == NULL) outofmem(size);
   return p;
}

extern void FAR *
osstrdup(const char *str)
{
   char *p;
   OSSIZE_T len;
   len = strlen(str) + 1;
   p = osmalloc(len);
   memcpy(p, str, len);
   return p;
}

/* osfree is currently a macro in error.h */

#ifdef HAVE_SIGNAL

static int sigReceived;

/* for systems not using autoconf, assume the signal handler returns void
 * unless specified elsewhere */
#ifndef RETSIGTYPE
# define RETSIGTYPE void
#endif

static CDECL RETSIGTYPE FAR report_sig( int sig ) {
   sigReceived = sig;
   longjmp(jmpbufSignal, 1);
}

static void
init_signals(void)
{
   int en;
   if (!setjmp(jmpbufSignal)) {
#if 0 /* FIXME disable for now so we get a core dump */
      signal(SIGABRT, report_sig); /* abnormal termination eg abort() */
      signal(SIGFPE,  report_sig); /* arithmetic error eg /0 or overflow */
      signal(SIGILL,  report_sig); /* illegal function image eg illegal instruction */
      signal(SIGSEGV, report_sig); /* illegal storage access eg access outside memory limits */
#endif
      signal(SIGINT,  report_sig); /* interactive attention eg interrupt */
      signal(SIGTERM, report_sig); /* termination request sent to program */
# ifdef SIGSTAK /* only on RISC OS AFAIK */
      signal(SIGSTAK, report_sig); /* stack overflow */
# endif
      return;
   }

   switch (sigReceived) {
   case SIGABRT: en=90; break;
   case SIGFPE:  en=91; break;
   case SIGILL:  en=92; break;
   case SIGINT:  en=93; break;
   case SIGSEGV: en=94; break;
   case SIGTERM: en=95; break;
# ifdef SIGSTAK
   case SIGSTAK: en=96; break;
# endif
   default:      en=97; break;
   }
   fputsnl(msg(en), STDERR);
   if (errno >= 0) {
# ifdef HAVE_STRERROR
      fputsnl(strerror(errno), STDERR);
# elif defined(HAVE_SYS_ERRLIST)
      if (errno < sys_nerr) fputsnl(STDERR, sys_errlist[errno]);
# elif defined(HAVE_PERROR)
      perror(NULL); /* always goes to stderr */
      /* if (arg!=NULL && *arg!='\0') fputs("<arg>: <err>\n",stderr); */
      /* else fputs("<err>\n",stderr); */
# else
      fprintf(STDERR, "error code %d\n", errno);
# endif
   }
   /* Any signals apart from SIGINT and SIGTERM suggest a bug */
   if (sigReceived != SIGINT && sigReceived != SIGTERM)
      fatalerror(/*Bug in program detected! Please report this to the authors*/11);

   exit(EXIT_FAILURE);
}
#endif

#define CHARSET_BAD       -1
#define CHARSET_USASCII    0
#define CHARSET_ISO_8859_1 1
#define CHARSET_DOSCP850   2
#define CHARSET_RISCOS31   3
static int default_charset( void ) {
#ifdef ISO8859_1
   return CHARSET_ISO_8859_1;
#elif (OS==RISCOS)
/* RISCOS 3.1 and above CHARSET_RISCOS31 (ISO_8859_1 + extras in 128-159)
 * RISCOS < 3.1 is ISO_8859_1 !HACK! */
   return CHARSET_RISCOS31;
#elif (OS==MSDOS)
   return CHARSET_DOSCP850;
#else
   return CHARSET_ISO_8859_1; /* Look at env var CHARSET ? !HACK! */
#endif
}

static const char *pthMe = NULL;

#if (OS==MSDOS)
static int
xlate_dos_cp850(int unicode)
{
   switch (unicode) {
#include "unicode-to-dos-cp-default.tab"
   }
   return 0;
}
#endif

static int
add_unicode(int charset, char *p, int value)
{
   if (value == 0) return 0;
   switch (charset) {
   case CHARSET_USASCII:
      if (value < 128) {
	 *p = value;
	 return 1;
      }
      break;
   case CHARSET_ISO_8859_1:
#if (OS==RISCOS)
   case CHARSET_RISCOS31: /* RISC OS 3.1 has a few extras in 128-159 */
#endif
      if (value < 256) {
	 *p = value;
	 return 1;
      }
#if (OS==RISCOS)
      /* FIXME: if OS version >= 3.1 handle extras here */
      /* RISC OS 3.1 (and later) extensions to ISO-8859-1:
       * \^y = \x86
       * \^Y = \x85
       * \^w = \x82
       * \^W = \x81
       * \oe = \x9b
       * \OE = \x9a
       */
#endif
      break;
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

static unsigned char *msg_blk;
static int num_msgs = 0;

static void
parse_msg_file(int charset_code)
{
   FILE *fh;
   unsigned char header[20];
   const char *lang;
   int i;
   unsigned len;
   char *p = msg_blk;

   lang = getenv("SURVEXLANG");
   if (!lang || !*lang) lang = DEFAULTLANG;

#if 1
   /* backward compatibility - FIXME deprecate? */
   if (strcmp(lang, "engi") == 0) {
      lang = "en";
   } else if (strcmp(lang, "engu") == 0) {
      lang = "en-us";
   } else if (strcmp(lang, "fren") == 0) {
      lang = "fr";
   } else if (strcmp(lang, "germ") == 0) {
      lang = "de";
   } else if (strcmp(lang, "ital") == 0) {
      lang = "it";
   } else if (strcmp(lang, "span") == 0) {
      lang = "es";
   } else if (strcmp(lang, "cata") == 0) {
      lang = "ca";
   } else if (strcmp(lang, "port") == 0) {
      lang = "pt";
   }
#endif

   fh = fopenWithPthAndExt(pthMe, lang, EXT_SVX_MSG, "rb", NULL);

   if (!fh) {
      /* e.g. if 'en-COCKNEY' is unknown, see if we know 'en' */
      if (strlen(lang) > 3 && lang[2] == '-') {
	 char lang_generic[3];
         lang_generic[0] = lang[0];
         lang_generic[1] = lang[1];
	 lang_generic[2] = '\0';
	 fh = fopenWithPthAndExt(pthMe, lang_generic, EXT_SVX_MSG, "rb", NULL);
      }
   }

   if (!fh) {
      /* no point extracting this error, as it won't get used if file opens */
      fprintf(STDERR, "Can't open message file '%s' using path '%s'\n",
	      lang, pthMe);
      exit(EXIT_FAILURE);
   }

   if (fread(header, 1, 20, fh) < 20 ||
       memcmp(header, "Svx\nMsg\r\n\xfe\xff", 12) != 0) {
      /* no point extracting this error, as it won't get used if file opens */
      fprintf(STDERR, "Problem with message file '%s'\n", lang);
      exit(EXIT_FAILURE);
   }

   if (header[12] != 0) {
      /* no point extracting this error, as it won't get used if file opens */
      fprintf(STDERR, "I don't understand this message file version\n");
      exit(EXIT_FAILURE);
   }

   num_msgs = (header[14] << 8) | header[15];

   len = 0;
   for (i = 16; i < 20; i++) len = (len << 8) | header[i];

   msg_blk = osmalloc(len);
   if (fread(msg_blk, 1, len, fh) < len) {
      /* no point extracting this error - it won't get used once file's read */
      fprintf(STDERR, "Message file truncated?\n");
      exit(EXIT_FAILURE);
   }

   p = msg_blk;
   for (i = 0; i < num_msgs; i++) {
      char *to = p;
      int ch;
      /* FIXME note message i is p? */
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
            /* FIXME this rather assumes a 2 byte UTF-8 code never
             * transliterates to more than 2 characters */
	    to += add_unicode(charset_code, to, ch);
	 }
      }
      *to++ = '\0';
   }

   fclose(fh);
}

extern const char * FAR
ReadErrorFile(const char *argv0)
{
   char *p;

#ifdef HAVE_SIGNAL
   init_signals();
#endif
   /* This code *should* be completely bomb-proof even if strcpy
    * generates a signal
    */
   szAppNameCopy = argv0; /* FIXME... */
   szAppNameCopy = osstrdup(argv0);

   /* Look for env. var. "SURVEXHOME" or the like */
   p = getenv("SURVEXHOME");
   if (p && *p) {
      pthMe = osstrdup(p);
   } else if (argv0) {
      /* else try the path on argv[0] */
      pthMe = PthFromFnm(argv0);
   } else {
      /* otherwise, forget it - go for the current directory */
      pthMe = "";
   }

   select_charset(default_charset());

   return pthMe;
}

/* These are English versions of messages which might be needed before the
 * alternative language version has been read from the message file.
 */
/* message may be overwritten by next call (but not in current implementation) */
extern const char *
msg(int en)
{
   static const char *erg[] = {
      "",
      "Out of memory (couldn't find %ul bytes).\n",
      "\nFatal error from %s: ",
      "\nError from %s: ",
      "\nWarning from %s: ",
   };

   static const char *szBadEn = "???";

   const char *p;

   if (!msg_blk) {
      if (1 <= en && en <= 4) return erg[en];
      return szBadEn;
   }

   if (en < 0 || en >= num_msgs) return szBadEn;

   p = msg_blk;
   /* skip to en-th message */
   while (en--) p += strlen(p) + 1;

   return p;
}

/* returns persistent copy of message */
extern const char *
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
   
   /* FIXME allow "warnings are errors" and/or "errors are fatal" */
   switch (severity) {
    case 0:
      cWarnings++;
      break;
    case 1:
      cErrors++;
      if (cErrors == 50)
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
   unsigned char *msg_blk;
} charset_li;

static charset_li *charset_head = NULL;

static int charset = CHARSET_BAD;

int
select_charset(int charset_code)
{
   int old_charset = charset;
   charset_li *p;

/*   printf( "select_charset(%d), old charset = %d\n", charset_code, charset ); */

   charset = charset_code;

   /* check if we've already parsed messages for new charset */
   for (p = charset_head; p; p = p->next) {
/*      printf("%p: code %d msg_blk %p\n",p,p->code,p->msg_blk); */
      if (p->code == charset) {
         msg_blk = p->msg_blk;
         return old_charset;
      }
   }

   /* nope, got to reparse message file */
   parse_msg_file(charset_code);

   /* add to list */
   p = osnew(charset_li);
   p->code = charset;
   p->msg_blk = msg_blk;
   p->next = charset_head;
   charset_head = p;

   return old_charset;
}

/***************************************************************************/

#if 0
     case CHARSET_DOSCP850: {
        /* MS DOS - Code page 850 */
	static char *my_pszTable[]={
 	 "\x85\x8A\x95∑\x8D\x97‘ „ÎÏÌ",
	 "†\x82¢µ°£\x90÷‡È",
	 "\x83\x88\x93∂\x8C\x96“◊‚Í",
	 "\x84\x89\x94\x8E\x8B\x81”ÿ\x99\x9A\x98",
	 "∆ ‰«    Â   §•",
	 NULL,
	 NULL,
	 NULL,
	 NULL,
	 NULL,
	 NULL,
	 "              \x87",
	 "               \x80",
	 NULL,
	 "¶ ß",
	 "\x86\x91",
	 "   \x8F  \x92",
	 "                  ·",
	 NULL,
	 NULL,
	 "  \x9B     \x9D"
 	};
	chOpenQuotes='\"';
	chCloseQuotes='\"';
	szSingles="lLij";
	szSingTab="  ’";
	szAccents="`'^\"~=.uvHtcCdbaAsOo/";
	szLetters="aeoAiuEIOUyYnNcCwWs";
	pszTable = my_pszTable;
 	break;
     }
#if 0
/* MS DOS - PC-8 (code page 437?) */
static char chOpenQuotes='\"', chCloseQuotes='\"';
static char *szSingles="";
static char *szSingTab=NULL;
static char *szAccents="`'^\"~=.uvHtcCdbaAsOo/";
static char *szLetters="aeoAiuEIOUyYnNcCwWs";
static char *pszTable[]={
 "\x85\x8A\x95 \x8D\x97",
 "†\x82¢ °£\x90",
 "\x83\x88\x93 \x8C\x96",
 "\x84\x89\x94\x8E\x8B\x81  \x99\x9A\x98",
 "            §•",
 NULL,
 NULL,
 NULL,
 NULL,
 NULL,
 NULL,
 "              \x87",
 "               \x80",
 NULL,
 "¶ ß",
 "\x86\x91",
 "   \x8F  \x92",
 "                  ·",
 NULL,
 NULL,
 NULL
};

#elif 0
/* MS DOS - PC-8 Denmark/Norway */
static char chOpenQuotes='\"', chCloseQuotes='\"';
static char *szSingles="";
static char *szSingTab=NULL;
static char *szAccents="`'^\"~=.uvHtcCdbaAsOo/";
static char *szLetters="aeoAiuEIOUyYnNcCwWs";
static char *pszTable[]={
 "\x85\x8A\x95 \x8D\x97",
 "†\x82¢ °£\x90     ¨",
 "\x83\x88\x93 \x8C\x96",
 "\x84\x89\x94\x8E\x8B\x81  \x99\x9A\x98",
 "© ¶™    ß   §•",
 NULL,
 NULL,
 NULL,
 NULL,
 NULL,
 NULL,
 "              \x87",
 "               \x80",
 NULL,
 NULL,
 "\x86\x91",
 "   \x8F  \x92",
 "                  ·",
 NULL,
 NULL,
 NULL
};
#elif 0
/* No special chars... */
# define NO_TEX
#endif
default: /*!HACK! do something -- no_tex variable version of NO_TEX ? */
printf("oops, bad charset...\n");
(void)0;
  }
#endif

/* loosely based on Survex's error.c, but uses SGML entities for accented
 * characters, and is rather more generic */

/* maps: */

/* (lookup table or switch) &#nnn; code to best rendition in each charset */


/* filename.c split off with filename manipulation stuff */

/* Beware: This file contains top-bit-set characters (160-255), so be     */
/* careful of mailers, ascii ftp, etc                                     */

/* Tables for TeX style accented chars, etc for use with Survex           */
/* Copyright (C) Olly Betts 1993-1996                                     */

/* NB if (as in TeX) \o and \O mean slashed-o and slashed-O, we can't
 * have \oe and \OE for linked-oe and linked-OE without cleverer code.
 * Therefore, I've changed slashed-o and slashed-O to \/o and \/O.
 */
