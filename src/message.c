/* message.c */

/* ERRC show diffs to newer version of error.c - most that are left in here aren't relevant */

/* loosely based on Survex's error.c, but uses SGML entities for accented
 * characters, and is rather more generic */

/* maps: */

/* (perfect hash) entity name to &#nnn; code */

/* (lookup table or switch) &#nnn; code to best rendition in each charset */


/* filename.c split off with filename manipulation stuff */


/* > error.c
 * Fairly general purpose error routines and path handling code
 * Copyright (C) 1993-1997 Olly Betts
 */

/*
1993.01.25 PC Compatibility OKed
1993.01.27 print newline before error messages
1993.02.17 changed #ifdef RISCOS to #if OS==...
1993.02.19 now look for ErrList in dir main exec was in
           for PC ERRLIST -> ERRLIST.TXT
1993.02.23 now do exit(EXIT_FAILURE) or exit(EXIT_SUCCESS)
           added guessed UNIX stuff
1993.03.12 Major recode to make it less crap. Especially:
           Now copes with *any* undefined message number
           Error file now has spaces, not underscores
1993.03.16 Merged 'PathCode.c' with this file. Updates for 'PathCode.c' were:
>> 1993.02.19 written
>> 1993.02.23 added guess at UNIX code
>> 1993.02.24 (W) add (int) cast to suppress warning calculating lenpth
>> 1993.02.25 Don't look for FNM_SEP_DRV if it's not defined
1993.03.18 Added LfFromFnm()
1993.03.19 Corrected bug which caused infinite loop for PC text error file
1993.04.06 slight fettles and copied fn headers to error.c
1993.04.07 Added UsePth()
           #error Don't ... -> #error Do not ... for GCC's happiness
1993.04.22 added fopenWithPthAndExt()
           added application name code
1993.04.25 added AMIGA version
1993.05.05 Slightly more elegant hack than Wook's to solve BC++ stderr bug
1993.05.12 (W) added FNTYPE for different DOS memory model proofing (non-ANSI)
1993.05.22 (W) removed stderr hack as it is not that simple???
               improved 'error file not found' reporting
1993.05.27 removed commented out #define STDERR ...
           actually improved 'error file not found' et al - personally I do
            *not* regard caverot telling me 'SURVEX: <error message>' as an
            improvement. Mind you, I'm fussy.
1993.05.28 added signal catching code
1993.05.29 errno printed when signal received
1993.06.04 moved #define FNM_SEP_XXX to filelist.h
           added #ifdef SIGSTAK as unix GCC doesn't seem to have it
1993.06.05 removed 'superfluous' &'s in signal fns
           signal catcher now uses strerror()
1993.06.07 Unix libraries don't have strerror() - so don't use it!
           FNTYPE -> FAR to aid comprehension
1993.06.10 in report_sig() only print out errno if non-zero
1993.06.12 fixed bug in path&ext code: only added .ext when prepending path
1993.06.16 syserr() -> fatal(); osmalloc() added; char* -> sz
           osrealloc() added; osfree() comment added
1993.06.28 fixed bug which lost first character of each error message on
            big-endian machines
           fixed fopenWithPthAndExt() to osfree() fnmFull
1993.07.19 "common.h" -> "osdepend.h"
1993.07.27 changed "error number %d" to "#%d" in output
1993.08.10 added '\n' before errors reported in report_sig
1993.08.12 added more bomb-proofing
1993.08.13 fettled header
1993.08.16 added fAbsoluteFnm & fAmbiguousFnm; recoded fopenWithPthAndExt
           RISC OS version of fopenWithPthAndExt copes with "<My$Dir>.File"
           added AddExt; added pfExtUsed arg to fopenWithPthAndExt
1993.08.19 ./ in DOS is ambiguous, not absolute
1993.09.21 (W)fixed relative paths without preceding .\ for .svc files
           (W)changed DOS fAbsolute() so 'c:here' is returned as ambiguous
1993.09.22 (W)changed error functions for more general info line
           (W)added function list
1993.09.23 (IH)DOS uses farmalloc,farrealloc,farfree; osmalloc() takes a long
1993.10.15 (BP)changed erroneous ifdef MSDOS's to if (OS==MSDOS)
1993.10.18 (W)fWarningGiven added
1993.10.23 corrected fFALSE to fFalse
1993.11.03 changed error routines
           cWarnings rather than fWarningGiven
           fettled a bit
1993.11.05 OSSIZE_T added
           fixed problem with pth=="" in fopenWithPthAndExt
           added msg() and msgPerm()
1993.11.07 merged out of memory code from osmalloc & osrealloc
1993.11.08 added code to deal with quoted messages
1993.11.14 fettled
1993.11.15 added xosmalloc (returns NULL if malloc fails)
1993.11.18 xosrealloc added as macro in error.h
           added calls to fDirectory
1993.11.19 added TeX style escape sequences for accents; also ``,'' for "
1993.11.20 minor change to '' and `` code
           \/O & \/o now used for slashed Os
           extracted tables to tex.h
1993.11.26 moved chOpenQuotes & chCloseQuotes to tex.h
1993.11.28 added NO_TEX to turn it off for now
1993.11.29 (IH) void * FAR -> void FAR * ; void * FILE -> void FILE *
           use perror in UNIX version
           extracted messages from here too! (except for signal ones)
1993.11.30 corrected Wook's ungroks in function list
           error now returns (it was wrong, but limp on); use fatal to abort
           added cErrors
1993.12.01 added error_summary()
           most signals tell the user to report them as a bug (autograss?)
           check version of messages.txt file
1993.12.08 split off osdepend.c
1993.12.09 now makes less explicit reference to OS
1993.12.16 farmalloc use controlled by NO_FLATDOS macro
           fixed error bootstrap code problem-ette
1993.12.17 wr changed to send to stderr as it writes error info
1994.01.05 added missing FAR to fix >128 eqns bug
           and another
           bug-fixed in outofmem
1994.03.13 enabled TeX style characters
           will deal with `` and '' even if NO_TEX is defined
1994.03.14 altered fopenWithPthAndExt to give filename actually used
1994.03.19 signals now reported as `fatal error' since they are
           all error output to do with signals is now sent to stderr
1994.03.20 added a putnl() at end of error_summary()
1994.03.24 error summary to stderr too
1994.04.27 cWarnings and cErrors now static
1994.06.03 fixed so SunOS version should cope with DOS error message file
1994.06.09 added home directory environmental variable
1994.06.18 fixed Norcroft warning
1994.06.20 added int argument to warning, error and fatal
1994.08.31 added fputnl()
1994.09.13 added fix for caverot signals being invisible under RISC OS
1994.09.13 removed 'cos it doesn't work
1994.09.20 fixed signal handler to longjmp back so it's now truely ANSI
           miscellaneous fettling
1994.09.21 rearranged signal handler code to minimize stack use
1994.09.22 should now be able to read multi-language message file
1994.09.28 xosmalloc is now a macro in error.h
1994.10.04 no longer pass NULL for szExt to fopenWithPathAndExt
1994.10.05 DEFAULTLANG and szLangVar added
1994.10.08 sizeof -> ossizeof
1994.11.16 errno.h wanted even if we're not signal handling
1994.11.23 UsePth and AddExt now insert a separator if appropriate
           fopenWithPthAndExt now uses UsePth and AddExt
1994.12.03 added FNM_SEP_LEV2
1994.12.06 stderr -> STDERR; STDERR #defined to stdout
1994.12.10 fopenWithPthAndExt() copes with NULL for pth or ext
1995.03.25 added osstrdup
1995.06.26 fixed bug with UsePth("",leafname)
1995.10.06 commented out some debug code
1995.10.11 fixed getline to take a buffer length
1996.02.10 pszTable entries can now be NULL, as can szSingTab
1996.02.19 fixed 2 sizeof() to ossizeof()
1996.03.22 fettled layout
1996.05.05 added CDECL
1997.01.19 started code to support multiple charsets at once
1997.01.22 finished off up charset code (merged in tex.h):
>1993.11.20 created
>1993.11.26 moved chOpenQuotes and chCloseQuotes to here too
>1994.03.13 characters 128-159 translated to \xXX codes to placate compilers
>1994.03.23 added caveat comment about top-bit-set characters
>1994.12.03 added -DISO8859_1 to makefile to force iso-8859-1
>1995.02.14 changed "char foo[]=" to "char *foo="
>1996.02.10 pszTable is now an array of char *, which can be NULL
>	   szSingTab can also be NULL
1997.06.05 added const
1998.03.04 more const
1998.03.21 fixed up to compile cleanly on Linux
*/

/* Beware: This file contains top-bit-set characters (160-255), so be     */
/* careful of mailers, ascii ftp, etc                                     */

/* Tables for TeX style accented chars, etc for use with Survex           */
/* Copyright (C) Olly Betts 1993-1996                                     */

/* NB if (as in TeX) \o and \O mean slashed-o and slashed-O, we can't
 * have \oe and \OE for linked-oe and linked-OE without cleverer code.
 * Therefore, I've changed slashed-o and slashed-O to \/o and \/O.
 */

/*
Function List
xosmalloc:       malloc, but indirected so we can eg do DOS XMS malloc
osmalloc:        ditto, but traps failure and gives fatal error
xosrealloc:      realloc, but indirected so we can eg do DOS XMS malloc
osrealloc:       ditto, but traps failure and gives fatal error
osfree:          free, but indirected so we can eg do DOS XMS malloc
report_sig:      catches signals and prints explanatory message
ReadErrorFile:   initialisation function - should be called first (ish)
warning:         report warning
error:           report error
fatal:           report fatal error and exit
fAbsoluteFnm:    is fnm definitely absolute?
fAmbiguousFnm:   could fnm be interpreted as both absolute and relative?
PthFromFnm:      extract path from fnm
LfFromFnm:       extract leafname from fnm
UsePth:          concatenate path & leafname
UseExt:          bung an extension on
fopenWithPthAndExt:
                 open file, passing back filename actually used
safe_fopen:      like fopen, but returns NULL for directories under all OS
*/

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
#include "version.h"

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
 */
#ifndef DEFAULTLANG
# define DEFAULTLANG "en"
#endif

#define STDERR stdout

/* For funcs which want to be immune from messing around with different
 * calling conventions */
#ifndef CDECL
#define CDECL
#endif

/* These are English versions of messages which might be needed before the
 * alternative language version has been read from the message file.
 */
static const char * ergBootstrap[]={
   "",
   "Out of memory (couldn't find %ul bytes).\n",
   "\nFatal error from %s: ",
   "\nError from %s: ",
   "\nWarning from %s: ",
   "Message file has incorrect format\n", /* was "Error message file ..." */
   "Negative error numbers are not allowed\n",
   NULL /* NULL marks end of list */
};

static const char **erg = ergBootstrap;
static int enMac = 32; /* Initially, grows automatically */
static const char *szBadEn = "???";

static int cWarnings = 0; /* keep track of how many warnings we've given */
static int cErrors = 0;   /* and how many (non-fatal) errors */

extern int error_summary(void) {
   fprintf(STDERR,msg(16),cWarnings,cErrors);
   fputnl(STDERR);
   return ( cErrors ? EXIT_FAILURE : EXIT_SUCCESS );
}

/* in case osmalloc() fails before szAppNameCopy is set up */
static const char *szAppNameCopy="anonymous program";

/* error code for failed osmalloc and osrealloc calls */
static void outofmem(OSSIZE_T size) {
   fprintf( STDERR, erg[2], szAppNameCopy );
   fprintf( STDERR, erg[1], (unsigned long)size );
   exit(EXIT_FAILURE);
}

/* malloc with error catching if it fails. Also allows us to write special
 * versions easily eg for DOS EMS or MS Windows.
 */
extern void FAR * osmalloc( OSSIZE_T size ) {
   void FAR *p;
   p=xosmalloc( size );
   if (p==NULL)
      outofmem(size);
   return p;
}

/* realloc with error catching if it fails. */
extern void FAR * osrealloc( void *p, OSSIZE_T size ) {
   p=xosrealloc(p,size);
   if (p==NULL)
      outofmem(size);
   return p;
}

extern void FAR * osstrdup( const char *sz ) {
   char *p;
   int len;
   len=strlen(sz)+1;
   p=osmalloc(len);
   memmove(p,sz,len);
   return p;
}

/* osfree is currently a macro in error.h */

#ifdef HAVE_SIGNAL

static int sigReceived;

/* for systems not using autoconf, assume the signal handler returns void
 * unless specified elsewhere */
#ifndef RETSIGTYPE
#define RETSIGTYPE void
#endif

static CDECL RETSIGTYPE FAR report_sig( int sig ) {
   sigReceived=sig;
   longjmp(jmpbufSignal,1);
}

static void init_signals( void ) {
   int en;
   if (!setjmp(jmpbufSignal)) {
      signal(SIGABRT,report_sig); /* abnormal termination eg abort() */
      signal(SIGFPE ,report_sig); /* arithmetic error eg /0 or overflow */
      signal(SIGILL ,report_sig); /* illegal function image eg illegal instruction */
      signal(SIGINT ,report_sig); /* interactive attention eg interrupt */
      signal(SIGSEGV,report_sig); /* illegal storage access eg access outside memory limits */
      signal(SIGTERM,report_sig); /* termination request sent to program */
# ifdef SIGSTAK /* only on RISC OS AFAIK */
      signal(SIGSTAK,report_sig); /* stack overflow */
# endif
      return;
   }
   fprintf(STDERR,msg(2),szAppNameCopy);
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
   fputsnl(msg(en),STDERR);
   if (errno >= 0) {
# ifdef HAVE_STRERROR
      fputsnl(strerror(errno),STDERR);
# elif defined(HAVE_SYS_ERRLIST)
      if (errno < sys_nerr)
	 fputsnl( STDERR, sys_errlist[errno] );
# elif defined(HAVE_PERROR)
      perror(NULL); /* always goes to stderr */
      /* if (arg!=NULL && *arg!='\0') fputs("<arg>: <err>\n",stderr); */
      /* else fputs("<err>\n",stderr); */
# else
      fprintf( STDERR, "error code %d\n", errno );
# endif
   }
   if (sigReceived!=SIGINT && sigReceived!=SIGTERM)
      fatal(11,NULL,NULL,0); /* shouldn't get any others => bug */
   exit(EXIT_FAILURE);
}
#endif

/* write string and nl to STDERR */
extern void wr( const char *sz, int n ) {
   n=n; /* suppress warning */
   fputsnl(sz,STDERR);
}

#define CHARSET_BAD       -1
#define CHARSET_USASCII    0
#define CHARSET_ISO_8859_1 1
#define CHARSET_DOSCP850   2
#define CHARSET_RISCOS31   3
static int default_charset( void ) {
#ifdef ISO8859_1
   return CHARSET_ISO_8859_1;
#elif (OS==RISCOS)
/*
RISCOS 3.1 and above CHARSET_RISCOS31 !HACK!
*/
   return CHARSET_CHARSET_RISCOS31;
#elif (OS==MSDOS)
   return CHARSET_DOSCP850;
#else
   return CHARSET_ISO_8859_1; /*!HACK!*/
#endif
}

static const char *pthMe = NULL, *lfErrs = NULL;
static char prefix[32];
static int prefix_len;

static char prefix_root[32];
static int prefix_root_len;

#if (OS==MSDOS)
static int xlate_dos_cp850(int unicode) {
   switch (unicode) {
/*# include "unicode-to-dos-cp-default.tab"*/
case 160: return 255;
case 161: return 173;
case 162: return 189;
case 163: return 156;
case 164: return 207;
case 165: return 190;
case 167: return 245;
case 168: return 249;
case 169: return 184;
case 170: return 166;
case 171: return 174;
case 172: return 170;
case 173: return 240;
case 174: return 169;
#if 0
case 175: return 223;
case 175: return 238;
#endif
case 176: return 248;
case 177: return 241;
case 178: return 253;
case 179: return 252;
case 180: return 239;
case 181: return 230;
case 182: return 244;
case 183: return 250;
case 184: return 247;
case 185: return 251;
case 186: return 167;
case 187: return 175;
case 188: return 172;
case 189: return 171;
case 190: return 243;
case 191: return 168;
case 192: return 183;
case 193: return 181;
case 194: return 182;
case 195: return 199;
case 196: return 142;
case 197: return 143;
case 198: return 146;
case 199: return 128;
case 200: return 212;
case 201: return 144;
case 202: return 210;
case 203: return 211;
case 204: return 222;
case 205: return 214;
case 206: return 215;
case 207: return 216;
case 208: return 209;
case 209: return 165;
case 210: return 227;
case 211: return 224;
case 212: return 226;
case 213: return 229;
case 214: return 153;
case 215: return 158;
case 216: return 157;
case 217: return 235;
case 218: return 233;
case 219: return 234;
case 220: return 154;
case 221: return 237;
case 222: return 232;
case 223: return 225;
case 224: return 133;
case 225: return 160;
case 226: return 131;
case 227: return 198;
case 228: return 132;
case 229: return 134;
case 230: return 145;
case 231: return 135;
case 232: return 138;
case 233: return 130;
case 234: return 136;
case 235: return 137;
case 236: return 141;
case 237: return 161;
case 238: return 140;
case 239: return 139;
case 240: return 208;
case 241: return 164;
case 242: return 149;
case 243: return 162;
case 244: return 147;
case 245: return 228;
case 246: return 148;
case 247: return 246;
case 248: return 155;
case 249: return 151;
case 250: return 163;
case 251: return 150;
case 252: return 129;
case 253: return 236;
case 254: return 231;
case 255: return 152;
   }
   return 0;
}
#endif

static int add_unicode(int charset, char *p, int value) {
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
      /* !HACK! handle extras here */
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

static int decode_entity(const char *entity, size_t len) {
   unsigned long value;   
   int i;
   
   if (len > 6) return 0;
   value = entity[0] - '0';
   if (value >= 'a' - '0') {
      value += 36 - ('a' - '0');
   } else if (value >= 'A' - '0') {
      value += 10 - ('A' - '0');
   }
   for (i = 1; i < 6 && i < len; i++) {
      int c;
      c = toupper(entity[i]) - '0';
      if (c >= 'A' - '0') c += 10 - ('A' - '0');
      value = value * 36 + c;
   }
   switch (value) {
    case 17477224u: return 198; /* AElig */
    case 622057730u: return 193; /* Aacute */
    case 17380344u: return 194; /* Acirc */
    case 632809418u: return 192; /* Agrave */
    case 18080044u: return 197; /* Aring */
    case 654238130u: return 195; /* Atilde */
    case 506253u: return 196; /* Auml */
    case 746420205u: return 199; /* Ccedil */
    case 19205u: return 208; /* ETH */
    case 863922434u: return 201; /* Eacute */
    case 24098808u: return 202; /* Ecirc */
    case 874674122u: return 200; /* Egrave */
    case 692877u: return 203; /* Euml */
    case 1105787138u: return 205; /* Iacute */
    case 30817272u: return 206; /* Icirc */
    case 1116538826u: return 204; /* Igrave */
    case 879501u: return 207; /* Iuml */
    case 1440298418u: return 209; /* Ntilde */
    case 1468584194u: return 211; /* Oacute */
    case 40894968u: return 212; /* Ocirc */
    case 1479335882u: return 210; /* Ograve */
    case 1499211233u: return 216; /* Oslash */
    case 1500764594u: return 213; /* Otilde */
    case 1159437u: return 214; /* Ouml */
    case 49534115u: return 222; /* THORN */
    case 1831381250u: return 218; /* Uacute */
    case 50972664u: return 219; /* Ucirc */
    case 1842132938u: return 217; /* Ugrave */
    case 1439373u: return 220; /* Uuml */
    case 2073245954u: return 221; /* Yacute */
    case 2194178306u: return 225; /* aacute */
    case 61050360u: return 226; /* acirc */
    case 61065986u: return 180; /* acute */
    case 61147240u: return 230; /* aelig */
    case 2204929994u: return 224; /* agrave */
    case 61750060u: return 229; /* aring */
    case 2226358706u: return 227; /* atilde */
    case 1719309u: return 228; /* auml */
    case 2284059123u: return 166; /* brvbar */
    case 2318540781u: return 231; /* ccedil */
    case 64496109u: return 184; /* cedil */
    case 1791929u: return 162; /* cent */
    case 1804966u: return 169; /* copy */
    case 2349398399u: return 164; /* curren */
    case 51064u: return 176; /* deg */
    case 2389884098u: return 247; /* divide */
    case 2436043010u: return 233; /* eacute */
    case 67768824u: return 234; /* ecirc */
    case 2446794698u: return 232; /* egrave */
    case 52901u: return 240; /* eth */
    case 1905933u: return 235; /* euml */
    case 2524944998u: return 189; /* frac12 */
    case 2524945000u: return 188; /* frac14 */
    case 2524945072u: return 190; /* frac34 */
    case 2677907714u: return 237; /* iacute */
    case 74487288u: return 238; /* icirc */
    case 74599509u: return 161; /* iexcl */
    case 2688659402u: return 236; /* igrave */
    case 2705600621u: return 191; /* iquest */
    case 2092557u: return 239; /* iuml */
    case 79443312u: return 171; /* laquo */
    case 2252907u: return 175; /* macr */
    case 81477924u: return 181; /* micro */
    case 2933233805u: return 183; /* middot */
    case 2301433u: return 160; /* nbsp */
    case 64397u: return 172; /* not */
    case 3012418994u: return 241; /* ntilde */
    case 3040704770u: return 243; /* oacute */
    case 84564984u: return 244; /* ocirc */
    case 3051456458u: return 242; /* ograve */
    case 2368275u: return 170; /* ordf */
    case 2368282u: return 186; /* ordm */
    case 3071331809u: return 248; /* oslash */
    case 3072885170u: return 245; /* otilde */
    case 2372493u: return 246; /* ouml */
    case 2393398u: return 182; /* para */
    case 3120483695u: return 177; /* plusmn */
    case 86819881u: return 163; /* pound */
    case 89521008u: return 187; /* raquo */
    case 69208u: return 174; /* reg */
    case 2538029u: return 167; /* sect */
    case 70630u: return 173; /* shy */
    case 2559205u: return 185; /* sup1 */
    case 2559206u: return 178; /* sup2 */
    case 2559207u: return 179; /* sup3 */
    case 92360104u: return 223; /* szlig */
    case 93204131u: return 254; /* thorn */
    case 93247732u: return 215; /* times */
    case 3403501826u: return 250; /* uacute */
    case 94642680u: return 251; /* ucirc */
    case 3414253514u: return 249; /* ugrave */
    case 73389u: return 168; /* uml */
    case 2652429u: return 252; /* uuml */
    case 3645366530u: return 253; /* yacute */
    case 78287u: return 165; /* yen */
    case 2839053u: return 255; /* yuml */
   }
   return 0;
}

static void parse_msg_file( int charset_code ) {
  FILE *fh;
  char estr[512], line[512];
  int en;
  int c;
  bool fQuoted;
#ifndef NO_ACCENTS
  char chOpenQuotes, chCloseQuotes;
  char *szSingles;
  char *szSingTab;
  char *szAccents;
  char *szLetters;
  char **pszTable;

  switch (charset_code) {
     case CHARSET_USASCII: {
        /* US ASCII */
	chOpenQuotes = '\"';
	chCloseQuotes = '\"';
	szSingles = "";
	szSingTab = NULL;
        szAccents = "";
	szLetters = "";
	pszTable = NULL;
	break;
     }
     case CHARSET_ISO_8859_1: {
        /* ISO 8859/1 (Latin 1) */
        static char *my_pszTable[]={
	 "‡ËÚ¿Ï˘»Ã“Ÿ",
	 "·ÈÛ¡Ì˙…Õ”⁄˝›",
	 "‚ÍÙ¬Ó˚ Œ‘€",
	 "‰ÎˆƒÔ¸Àœ÷‹ˇ",
	 "„ ı√    ’   Ò—",
	 NULL,
	 NULL,
	 NULL,
	 NULL,
	 NULL,
	 NULL,
	 "              Á",
	 "               «",
	 NULL,
	 "™ ∫",
	 "ÂÊ",
	 "   ≈  ∆",
	 "                  ﬂ",
	 NULL,
	 NULL,
	 "  ¯     ÿ"
	};
	chOpenQuotes = '\"';
	chCloseQuotes = '\"';
	szSingles = "";
	szSingTab = NULL;
        szAccents = "`'^\"~=.uvHtcCdbaAsOo/";
	szLetters = "aeoAiuEIOUyYnNcCwWs";
	pszTable = my_pszTable;
	break;
     }
     case CHARSET_RISCOS31: {
        /* Archimedes RISC OS 3.1 and above
         * ISO 8859/1 (Latin 1) + extensions in 128-159 */
	static char *my_pszTable[]={
	 "‡ËÚ¿Ï˘»Ã“Ÿ",
	 "·ÈÛ¡Ì˙…Õ”⁄˝›",
	 "‚ÍÙ¬Ó˚ Œ‘€\x86\x85    \x82\x81",
	 "‰ÎˆƒÔ¸Àœ÷‹ˇ",
	 "„ ı√    ’   Ò—",
	 NULL,
	 NULL,
	 NULL,
	 NULL,
	 NULL,
	 NULL,
	 "              Á",
	 "               «",
	 NULL,
	 "™ ∫",
	 "ÂÊ",
	 "   ≈  ∆",
	 "                  ﬂ",
	 "      \x9a",
	 " \x9b",
	 "  ¯     ÿ"
	};
	chOpenQuotes='\x94';
	chCloseQuotes='\x95';
	szSingles="";
        szSingTab=NULL;
        szAccents="`'^\"~=.uvHtcCdbaAsOo/";
        szLetters="aeoAiuEIOUyYnNcCwWs";
	pszTable = my_pszTable;
	break;
     }
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

#if 0
printf("opening error file\n");
printf("(%s %s)\n",pthMe,lfErrs);
#endif
  fh=fopenWithPthAndExt( pthMe, lfErrs, "", "rb", NULL );
#if 0
printf("opened error file\n");
#endif
  if (!fh) {
    /* no point extracting this error, as it won't get used if file opens */
    fprintf(STDERR, erg[3], szAppNameCopy );
    fprintf(STDERR, "Can't open message file '%s' using path '%s'\n",
            lfErrs, pthMe);
    exit(EXIT_FAILURE);
  }

  { /* copy bootstrap erg[] which'll get overwritten by file entries */
     const char **ergMalloc;
     erg = ergBootstrap;
     ergMalloc = osmalloc( enMac * ossizeof(char*) );
     /* NULL marks end of list */
     for ( en = 0 ; erg[en] ; en++ ) ergMalloc[en]=erg[en];
     for ( ; en < enMac ; en++ ) ergMalloc[en] = szBadEn;
     erg = ergMalloc;
  }

  while (!feof( fh )) {
    char *p;
    char *q;
    int exact;
    getline( line, ossizeof(line), fh );
    if ((exact = (strncmp(line, prefix, prefix_len) == 0)) ||
	(prefix_root_len && strncmp(line, prefix_root, prefix_root_len) == 0)) {
      long val;
      val = strtol( line + (exact?prefix_len:prefix_root_len), &q, 0);
      if (val < 0 || val > (unsigned long)INT_MAX) {
        fprintf( STDERR, erg[3], szAppNameCopy );
        fprintf( STDERR, erg[ (errno==ERANGE) ? 5 : 6 ] );
        exit(EXIT_FAILURE);
      }
      en = (int)val;
      while (isspace(*q)) q++;

      p = q + strlen(q);
      while (p > q && isspace(p[-1])) p--;

      fQuoted = (p > q + 1 && *q == '\"' && *(p-1) == '\"');
      if (fQuoted) {
         q++;
         p--;
      }
      *p = '\0';

      c = 0;
      while (*q) {
         if (*q == '&') {
            if (*(q+1) == '#') {
	       if (isdigit(q[2])) {
                  unsigned long value = strtoul( q+2, &q, 10);
              	  if (*q == ';') q++;
		  if (value < 127) {
		     estr[c++] = (char)value;
		  } else {
		     c += add_unicode(charset_code, estr+c, value);
		  }
              	  continue;
               }
            } else if (isalnum(q[1])) { /* or isalpha? !HACK! */
               /*const*/ char *entity;
               int entity_len;
	       int len;
               entity = q+1;
               q += 2;
               while (isalnum(*q)) q++;
               entity_len = q - entity;
	       if (*q == ';') q++;
	       len = add_unicode(charset_code, estr+c, decode_entity(entity, entity_len));
	       if (len) {
		  c += len;
		  continue;
	       }
	       q = entity - 1;
            }
         }
         if (*q < 32 || *q >= 127) {
            fprintf(STDERR, "Warning: literal character '%c' (value %d) "
                    "in message %d\n", *q, (int)*q, en);
         }
	 estr[c++] = *q++;
      }
      estr[c] = '\0';

      if (en >= enMac) {
         int enTmp;
         enTmp = enMac;
         enMac = enMac<<1;
         erg = osrealloc( erg, enMac * ossizeof(char*) );
         while (enTmp < enMac) erg[enTmp++] = szBadEn;
      }
      erg[en] = osstrdup(estr);
/*printf("Error number %d: %s\n",en,erg[en]);*/
    }
  }
  fclose(fh);
}

extern const char * FAR ReadErrorFile( const char *szAppName, const char *szEnvVar,
				       const char *szLangVar, const char *argv0,
				       const char *lfErrFile ) {
   int  c;
   char *szTmp;

   lfErrs = osstrdup(lfErrFile);
#ifdef HAVE_SIGNAL
   init_signals();
#endif
   /* This code *should* be completely bomb-proof even if strcpy
    * generates a signal
    */
   szAppNameCopy = szAppName; /* ... in case the osstrdup() fails */
   szAppNameCopy = osstrdup(szAppName);

   /* Look for env. var. "SURVEXHOME" or the like */
   if (szEnvVar && *szEnvVar && (szTmp=getenv(szEnvVar))!=NULL && *szTmp) {
      pthMe = osstrdup(szTmp);
   } else if (argv0) {
      /* else try the path on argv[0] */
      pthMe = PthFromFnm(argv0);
   } else {
      /* otherwise, forget it - go for the current directory */
      pthMe = "";
   }

   /* Look for env. var. "SURVEXLANG" or the like */
   if ((szTmp=getenv(szLangVar))==0 || !*szTmp) {
      szTmp = DEFAULTLANG;
   }
   for (c = 0 ; c < 4 && szTmp[c] ; c++) prefix[c] = tolower(szTmp[c]);
   prefix[c] = '\0';
   if (c == 4) {
      if (strcmp(prefix, "engi") == 0) {
	 strcpy(prefix, "en");
      } else if (strcmp(prefix, "engu") == 0) {
	 strcpy(prefix, "en-us");
      } else if (strcmp(prefix, "fren") == 0) {
	 strcpy(prefix, "fr");
      } else if (strcmp(prefix, "germ") == 0) {
	 strcpy(prefix, "de");
      } else if (strcmp(prefix, "ital") == 0) {
	 strcpy(prefix, "it");
      } else if (strcmp(prefix, "span") == 0) {
	 strcpy(prefix, "es");
      } else if (strcmp(prefix, "cata") == 0) {
	 strcpy(prefix, "ca");
      } else if (strcmp(prefix, "port") == 0) {
	 strcpy(prefix, "pt");
      } else {
	 while (szTmp[c] && c < sizeof(prefix)) {
	    prefix[c] = tolower(szTmp[c]);
	    c++;
	 }
	 prefix[c] = '\0';
      }
   }
   strcat(prefix, ":");
   prefix_len = strlen(prefix);

   /* If the language is something like "en-us", fallback to "en" if we don't
    * have an entry for en-us */
   szTmp = strchr(prefix, '-');
   if (szTmp) {
      c = szTmp - prefix;
      memcpy(prefix_root, prefix, c);
      prefix_root[c++] = ':';
      prefix_root[c] = '\0';
      prefix_root_len = strlen(prefix_root);
   } else {
      prefix_root_len = 0;
   }

  select_charset(default_charset());

  if (erg[0]==szBadEn) {
    fprintf(STDERR, erg[3], szAppName );
    /* no point extracting this message */
    fprintf(STDERR, "No messages in language '%s'\n",prefix);
    exit(EXIT_FAILURE);
  }
  if (strcmp(MESSAGE_VERSION_MIN,erg[0])>0 || strcmp(VERSION,erg[0])<0)
    /* a little tacky, but'll work */
    fatal(191,wr,MESSAGE_VERSION_MIN" - "VERSION,0);

  return pthMe;
}

extern const char *msg( int en ) /* message may be overwritten by next call */ {
  return ( (en<0||en>=enMac) ? szBadEn : erg[en] );
}

extern const char *msgPerm( int en ) /* returns persistent copy of message */ {
  return ( (en<0||en>=enMac) ? szBadEn : erg[en] );
}

static void FAR errdisp( int en, void (*fn)( const char *, int ), const char *arg, int n,
                         int type ) {
   fputnl( STDERR );
   fputnl( STDERR );
   fprintf( STDERR, erg[type], szAppNameCopy );
   fputs( msg(en), STDERR);
   fputnl( STDERR );
   if (fn) (fn)(arg, n);
/*   if (fn) (fn)( (arg ? arg : "(null)"), n);*/
}

extern void FAR warning( int en, void (*fn)( const char *, int ), const char *szArg, int n ) {
   cWarnings++;
   errdisp( en, fn, szArg, n, 4 );
}

extern void FAR error( int en, void (*fn)( const char *, int ), const char *szArg, int n ) {
   cErrors++;
   errdisp( en, fn, szArg, n, 3 );
   /* non-fatal errors now return... */
}

extern void FAR fatal( int en, void (*fn)( const char *, int ), const char *szArg, int n ) {
   errdisp( en, fn, szArg, n, 2 );
   exit(EXIT_FAILURE);
}

#if 1
/* Code to support switching character set at runtime (e.g. for a printer
 * driver to support different character sets on screen and on the printer)
 */
typedef struct charset_li {
   struct charset_li *next;
   int code;
   const char **erg;
} charset_li;

static charset_li *charset_head = NULL;

static int charset = CHARSET_BAD;

int select_charset( int charset_code ) {
   int old_charset = charset;
   charset_li *p;

/*   printf( "select_charset(%d), old charset = %d\n", charset_code, charset ); */

   charset = charset_code;

   /* check if we've already parsed messages for new charset */
   for( p = charset_head ; p ; p = p->next ) {
/*      printf("%p: code %d erg %p\n",p,p->code,p->erg); */
      if (p->code == charset) {
         erg = p->erg;
         goto found;
      }
   }

   /* nope, got to reparse message file */
   parse_msg_file( charset_code );

   /* add to list */
   p = osnew(charset_li);
/*   p = osmalloc(256); */
   p->code = charset;
   p->erg = erg;
   p->next = charset_head;
   charset_head = p;

   found:
   return old_charset;
}
#endif
