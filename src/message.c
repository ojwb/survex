/* > message.c
 * Fairly general purpose message and error routines
 * Copyright (C) 1993-1998 Olly Betts
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

#ifdef TOMBSTONES
#define TOMBSTONE_SIZE 16
static char tombstone[TOMBSTONE_SIZE] = "012345\xfftombstone";
#endif

/* malloc with error catching if it fails. Also allows us to write special
 * versions easily eg for DOS EMS or MS Windows.
 */
extern void FAR *
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
printf("osmalloc truep=%p truesize=%d\n",p,size);
   memcpy(p, tombstone, TOMBSTONE_SIZE);
   memcpy(p + size - TOMBSTONE_SIZE, tombstone, TOMBSTONE_SIZE);
   *(size_t *)p = size;
   p += TOMBSTONE_SIZE;
#endif
   return p;
}

/* realloc with error catching if it fails. */
extern void FAR *
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
printf("osrealloc (in truep=%p truesize=%d)\n",p,true_size);
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
printf("osrealloc truep=%p truesize=%d\n",p,size);
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

/* osfree is usually just a macro in osalloc.h */
#ifdef TOMBSTONES
extern void
osfree(void *p)
{
   int true_size;
   if (!p) return;
   p -= TOMBSTONE_SIZE;
   true_size = *(size_t *)p;
printf("osfree truep=%p truesize=%d\n",p,true_size);
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

static CDECL RETSIGTYPE FAR report_sig( int sig ) {
   sigReceived = sig;
   longjmp(jmpbufSignal, 1);
}

static void
init_signals(void)
{
   int en;
   if (!setjmp(jmpbufSignal)) {
#if 0 /* FIXME: disable for now so we get a core dump */
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

/* fall back on looking in the current directory */
static const char *pth_cfg_files = "";

static int num_msgs = 0;
static char **msg_array = NULL;

static void
parse_msg_file(int charset_code)
{
   FILE *fh;
   unsigned char header[20];
   /*const*/ char *lang;
   int i;
   unsigned len;
   unsigned char *p;
   
#ifdef DEBUG
   fprintf(stderr, "parse_msg_file(%d)\n", charset_code);
#endif

   lang = getenv("SURVEXLANG");
#ifdef DEBUG
   fprintf(stderr, "lang = %p (= \"%s\")\n", lang, lang?lang:"(null)");
#endif
   
   if (!lang || !*lang) {
      lang = getenv("LANG");
      if (!lang || !*lang) lang = DEFAULTLANG;
   }
#ifdef DEBUG
   fprintf(stderr, "lang = %p (= \"%s\")\n", lang, lang?lang:"(null)");
#endif

#if 1
   /* backward compatibility - FIXME deprecate? */
   if (strcasecmp(lang, "engi") == 0) {
      lang = "en";
   } else if (strcasecmp(lang, "engu") == 0) {
      lang = "en-us";
   } else if (strcasecmp(lang, "fren") == 0) {
      lang = "fr";
   } else if (strcasecmp(lang, "germ") == 0) {
      lang = "de";
   } else if (strcasecmp(lang, "ital") == 0) {
      lang = "it";
   } else if (strcasecmp(lang, "span") == 0) {
      lang = "es";
   } else if (strcasecmp(lang, "cata") == 0) {
      lang = "ca";
   } else if (strcasecmp(lang, "port") == 0) {
      lang = "pt";
   }
#endif
#ifdef DEBUG
   fprintf(stderr, "lang = %p (= \"%s\")\n", lang, lang?lang:"(null)");
#endif

   lang = strdup(lang);
   /* On my RedHat 6.1 Linux box, LANG defaults to en_US - be nice and
    * handle this... */
   if (strchr(lang, '_')) {
      char *under = strchr(lang, '_');
      *under = '-';
   }

   fh = fopenWithPthAndExt(pth_cfg_files, lang, EXT_SVX_MSG, "rb", NULL);

   if (!fh) {
      /* e.g. if 'en-COCKNEY' is unknown, see if we know 'en' */
      if (strlen(lang) > 3 && lang[2] == '-') {
	 char lang_generic[3];
         lang_generic[0] = lang[0];
         lang_generic[1] = lang[1];
	 lang_generic[2] = '\0';
	 fh = fopenWithPthAndExt(pth_cfg_files, lang_generic, EXT_SVX_MSG,
				 "rb", NULL);
      }
   }

   if (!fh) {
      /* no point extracting this error, as it won't get used if file opens */
      fprintf(STDERR, "Can't open message file '%s' using path '%s'\n",
	      lang, pth_cfg_files);
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

   p = osmalloc(len);
   if (fread(p, 1, len, fh) < len) {
      /* no point extracting this error - it won't get used once file's read */
      fprintf(STDERR, "Message file truncated?\n");
      exit(EXIT_FAILURE);
   }
   fclose(fh);

#ifdef DEBUG
   fprintf(stderr, "lang = '%s', num_msgs = %d, len = %d\n", lang, num_msgs, len);
#endif

   msg_array = osmalloc(sizeof(char *) * num_msgs);

   for (i = 0; i < num_msgs; i++) {
      unsigned char *to = p;
      int ch;
      msg_array[i] = (char *)p;
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
   /* This code *should* be completely bomb-proof even if strcpy
    * generates a signal
    */
   szAppNameCopy = argv0; /* FIXME... */
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

   select_charset(default_charset());
}

/* message may be overwritten by next call (but not in current implementation) */
extern const char *
msg(int en)
{
   static const char *szBadEn = "???";

   if (!msg_array) {
      if (en != 1) return szBadEn;
      /* this should be the only message which can be requested before
       * the message file is opened and read... */
      return "Out of memory (couldn't find %ul bytes).\n";
   }

   if (en < 0 || en >= num_msgs) return szBadEn;

   return msg_array[en];
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
   fprintf(stderr, "select_charset(%d), old charset = %d\n", charset_code, charset);
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
