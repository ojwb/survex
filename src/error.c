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
1997.06.05 added const
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

#ifndef NO_SIGNAL
# include <signal.h>
#endif

#include "whichos.h"
#include "error.h"
#include "osdepend.h"
#include "filelist.h"
#include "debug.h"
#include "version.h"

#ifndef NO_TEX
# include "tex.h" /* Will define NO_TEX if no special character set */
#endif

/* This is the name of the default language -- set like this so folks can
 * add (for eg) -DDEFAULTLANG="fren" to UFLG in the makefile
 */
#ifndef DEFAULTLANG
# define DEFAULTLANG "engi"
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
static char *  ergBootstrap[]={
   "",
   "Out of memory (couldn't find %ul bytes).\n",
   "\nFatal error from %s: ",
   "\nError from %s: ",
   "\nWarning from %s: ",
   "Error message file has incorrect format\n",
   "Negative error numbers are not allowed\n",
   NULL /* NULL marks end of list */
};

static char *  *erg=ergBootstrap;
static int enMac=32; /* Initially, grows automatically */
static char *  szBadEn="???";

static int cWarnings=0; /* to keep track of how many warnings we've given */
static int cErrors=0;   /* and how many (non-fatal) errors */

extern int error_summary( void ) {
   fprintf( STDERR, msg(16), cWarnings, cErrors );
   fputnl(STDERR);
   return ( cErrors ? EXIT_FAILURE : EXIT_SUCCESS );
}

/* in case osmalloc() fails before szAppNameCopy is set up */
static const char *szAppNameCopy="anonymous program";

extern FILE *safe_fopen( const char *fnm, const char *mode ) {
   return ( fDirectory(fnm) ? (FILE *)NULL : fopen( fnm, mode ) );
}

/* error code for failed osmalloc and osrealloc calls */
static void outofmem( OSSIZE_T size ) {
   fprintf( STDERR, erg[2], szAppNameCopy );
   fprintf( STDERR, erg[1], (unsigned long)size );
   exit(EXIT_FAILURE);
}

/* malloc with error catching if it fails. Also allows us to write special
 * versions easily eg for DOS EMS or MS Windows.
 */
extern void FAR * osmalloc( OSSIZE_T size ) {
   void FAR *p;
   p = xosmalloc( size );
   if (p == NULL)
      outofmem(size);
   return p;
}

/* realloc with error catching if it fails. */
extern void FAR * osrealloc( void *p, OSSIZE_T size ) {
   p = xosrealloc( p, size );
   if (p == NULL)
      outofmem(size);
   return p;
}

extern void FAR * osstrdup( const char *str ) {
   char *p;
   int len;
   len = strlen(str) + 1;
   p = osmalloc(len);
   memcpy( p, str, len );
   return p;
}

/* osfree is currently a macro in error.h */

#ifndef NO_SIGNAL
# ifdef NO_SETJMP
#  error Can not handle signals if NO_SETJMP defined - define NO_SIGNAL too
# else
#  include <setjmp.h>
static jmp_buf jmpbufSignal;
# endif

static int sigReceived;

static CDECL void FAR report_sig( int sig ) {
   sigReceived = sig;
   longjmp( jmpbufSignal, 1 );
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
   if (errno) {
# ifndef NO_STRERROR
      fputsnl(strerror(errno),STDERR);
# else
#  ifndef NO_PERROR
      perror(NULL); /* always goes to stderr */
      /* if (arg!=NULL && *arg!='\0') fputs("<arg>: <err>\n",stderr); */
      /* else fputs("<err>\n",stderr); */
#  endif
# endif
   }
   if (sigReceived!=SIGINT && sigReceived!=SIGTERM)
      fatal(11,NULL,NULL,0); /* shouldn't get any others => bug */
   exit(EXIT_FAILURE);
}
#endif /* !defined(NO_SIGNAL) */

/* write string and nl to STDERR */
extern void wr( const char *str, int n ) {
   n = n; /* suppress warning */
   fputsnl( str, STDERR );
}

extern const char * FAR ReadErrorFile( const char *szAppName, const char *szEnvVar,
				 const char *szLangVar, const char *argv0,
				 const char *lfErrs ) {
  FILE *fh;
  char estr[256], line[256], prefix[5];
  int  en;
  int  c;
  bool fQuoted;
  const char *pthMe;
  char *szTmp;
#ifndef NO_SIGNAL
  init_signals();
#endif
  /* This code *should* be completely bomb-proof :) even if strcpy()
   * generates a signal
   */
  szAppNameCopy = szAppName; /* ... in case the osstrdup() fails */
  szAppNameCopy = osstrdup(szAppName);

  /* Look for env. var. "SURVEXHOME" or the like */
  if (szEnvVar && *szEnvVar && (szTmp=getenv(szEnvVar))!=0 && *szTmp) {
    pthMe=osstrdup(szTmp);
  } else if (argv0) /* else try the path on argv[0] */
    pthMe=PthFromFnm(argv0);
  else /* otherwise, forget it - go for the current directory */
    pthMe="";

 /* Look for env. var. "SURVEXLANG" or the like */
  if ((szTmp=getenv(szLangVar))==0 || !*szTmp)
    szTmp=DEFAULTLANG;
  for( c=0 ; c<4 && szTmp[c] ; c++ )
    prefix[c]=tolower(szTmp[c]);
  prefix[c++]=':';
  prefix[c]='\0';

  fh=fopenWithPthAndExt( pthMe, lfErrs, "", "rb", NULL );
  if (!fh) {
    /* no point extracting this error, as it won't get used if file opens */
    fprintf(STDERR, erg[3], szAppName );
    fprintf(STDERR, "Can't open error message file '%s' using path '%s'\n",
            lfErrs,pthMe);
    exit(EXIT_FAILURE);
  }

  { /* copy bootstrap erg[] which'll get overwritten by file entries */
    char * *ergMalloc;
    ergMalloc=osmalloc( enMac*ossizeof(char*) );
    for ( en=0 ; erg[en] ; en++ )
      ergMalloc[en]=erg[en]; /* NULL marks end of list */
    for ( ; en<enMac ; en++ )
      ergMalloc[en]=szBadEn;
    erg=ergMalloc;
  }

  while (!feof( fh )) {
    char *q;
    getline(line,ossizeof(line),fh);
    if (strncmp(line,prefix,strlen(prefix))==0) {
      long val=strtol(line+strlen(prefix),&q,0);
      if (val<0 || val>(unsigned long)INT_MAX) {
        fprintf(STDERR, erg[3], szAppName);
        fprintf(STDERR,erg[ (errno==ERANGE)?5:6 ]);
        exit(EXIT_FAILURE);
      }
      en=(int)val;
      while (isspace(*q))
        q++;
      fQuoted=(*q=='\"');
      if (fQuoted)
        q++;
      c=0;
      while (*q && !(fQuoted && *q=='\"') ) {
        if (*q=='\'' || *q=='`') /* convert `` and '' to left/right " */ {
          if (*q==*(q+1)) {
#ifndef NO_TEX
            estr[c++] = ( *q=='`' ? chOpenQuotes : chCloseQuotes );
#else
            estr[c++] = '\"';
#endif /* !NO_TEX */
            q+=2;
            continue;
          }
        }
        if (*q!='\\')
          estr[c++]=*q;
        else {
          q++;
          switch (*q) {
            case 'n': /* '\n' */
              estr[c++]='\n';
              break;
            case '\0': /* \ at end of line, so concatenate lines */
              getline(line,ossizeof(line),fh);
              if (strncmp(line,prefix,strlen(prefix))!=0) { /* bad format */
                fprintf(STDERR, erg[3], szAppName);
                fprintf(STDERR,erg[5]);
                exit(EXIT_FAILURE);
              }
              q=line+strlen(prefix);
              continue;
#ifndef NO_TEX
            default: {
              char *p;
              int i, accent;
              p=strchr(szSingles,*q);
              if (p && szSingTab) {
                estr[c++]=szSingTab[p-szSingles];
                break;
              }
              p=strchr(szAccents,*q);
              if (!p) {
                if (*q!='\\')
                  estr[c++]='\\';
                break; /* \\ becomes \ and \<unknown> is not translated */
              }
              accent=p-szAccents;
              q++;
              p=strchr(szLetters,*q);
              if (!p) {
                estr[c++]='\\';
                estr[c++]=szAccents[accent];
                break;
              }
              i=p-szLetters;
              if (pszTable[accent] && i<strlen(pszTable[accent])) {
                char ch;
                ch=pszTable[accent][i];
                if (ch!=32) {
                  estr[c++]=ch;
                  break;
                }
              }
              estr[c++]='\\';
              estr[c++]=szAccents[accent];
              estr[c++]=szLetters[i];
            }
#endif /* !NO_TEX */
          }
        }
        q++;
      }

      if (!fQuoted) /* if string wasn't quoted, trim off spaces... */
        while (c>0 && estr[c-1]==' ')
          c--;
      estr[c]='\0';
      if (en>=enMac) {
        int enTmp;
        enTmp=enMac;
        enMac=enMac<<1;
        erg=osrealloc(erg, enMac * ossizeof(char*) );
        for ( ; enTmp<enMac ; enTmp++ )
          erg[enTmp]=szBadEn;
      }
      erg[en]=osmalloc(c+1);
      strcpy(erg[en],estr); /* !HACK! osstrdup? */
      /* printf("Error number %d: %s\n",en,erg[en]); */
    }
  }
  fclose( fh );
  if (erg[0]==szBadEn) { /* no point extracting this error */
    fprintf(STDERR, erg[3], szAppName );
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

static void FAR errdisp( int en, void (*fn)( const char*, int ), const char *szArg, int n,
                         int type ) {
  fputnl( STDERR );
  fprintf( STDERR, erg[type], szAppNameCopy );
  fprintf( STDERR, "%s\n", msg(en) );
  if (fn)
    (fn)( szArg, n );
}

extern void FAR warning( int en, void (*fn)( const char *, int ), const char *szArg, int n ) {
  cWarnings++; /* And another notch in the bedpost... */
  errdisp( en, fn, szArg, n, 4 );
}

extern void FAR error( int en, void (*fn)( const char *, int ), const char *szArg, int n ) {
  cErrors++; /* non-fatal errors now return... */
  errdisp( en, fn, szArg, n, 3 );
}

extern void FAR fatal( int en, void (*fn)( const char *, int ), const char *szArg, int n ) {
  errdisp( en, fn, szArg, n, 2 );
  exit(EXIT_FAILURE);
}

extern const char * FAR PthFromFnm( const char *fnm) {
   char *pth;
   const char *lf;
   int lenpth;

   lf = strrchr( fnm, FNM_SEP_LEV );
#ifdef FNM_SEP_LEV2
   if (!lf)
      lf = strrchr( fnm, FNM_SEP_LEV2 );
#endif
#ifdef FNM_SEP_DRV
   if (!lf)
      lf=strrchr( fnm, FNM_SEP_DRV );
#endif
   if (!lf)
      lf=fnm-1;

   lenpth = (int)(lf - fnm + 1);
   pth = osmalloc( lenpth + 1 );
   strncpy( pth, fnm, lenpth );
   pth[lenpth] = '\0';

   return pth;
}

extern const char * FAR LfFromFnm( const char * fnm ) {
  char * lf;
  if ((lf=strrchr( fnm, FNM_SEP_LEV ))!=NULL)
    return lf+1;
#ifdef FNM_SEP_LEV2
  if ((lf=strrchr( fnm, FNM_SEP_LEV2))!=NULL)
    return lf+1;
#endif
#ifdef FNM_SEP_DRV
  if ((lf=strrchr( fnm, FNM_SEP_DRV ))!=NULL)
    return lf+1;
#endif
  return fnm;
}

/* Make fnm from pth and lf, inserting an FNM_SEP_LEV if appropriate */
extern const char * FAR UsePth( const char * pth, const char * lf ) {
  char * fnm;
  int len = strlen(pth), lenTotal;
  bool fAddSep = fFalse;

/*  printf("'%s' '%s'\n",pth?pth:"(null)",lf?lf:"(null)");*/

  lenTotal=len+strlen(lf)+1;
  /* if len==0, just use the name */
  if (len!=0 && pth[len-1]!=FNM_SEP_LEV) {
#ifdef FNM_SEP_LEV2
    if (pth[len-1]!=FNM_SEP_LEV2) {
#endif
#ifdef FNM_SEP_DRV
      if (pth[len-1]!=FNM_SEP_DRV) {
#endif
        fAddSep=fTrue;
        lenTotal++;
#ifdef FNM_SEP_LEV2
      }
#endif
#ifdef FNM_SEP_DRV
    }
#endif
  }
  fnm=osmalloc(lenTotal);
  strcpy( fnm, pth );
  if (fAddSep)
    fnm[len++]=FNM_SEP_LEV;
  strcpy( fnm+len, lf );
/*  printf("'%s'\n",fnm);*/
  return fnm;
}

/* Add ext to fnm, inserting an FNM_SEP_EXT if appropriate */
extern const char * FAR AddExt( const char * fnm, const char * ext ) {
  char * fnmNew;
  int len=strlen(fnm), lenTotal;
#ifdef FNM_SEP_EXT
  bool fAddSep=fFalse;
#endif
  lenTotal=len+strlen(ext)+1;
#ifdef FNM_SEP_EXT
  if (ext[0]!=FNM_SEP_EXT) {
    fAddSep=fTrue;
    lenTotal++;
  }
#endif
  fnmNew=osmalloc(lenTotal);
  strcpy( fnmNew, fnm );
#ifdef FNM_SEP_EXT
  if (fAddSep)
    fnmNew[len++]=FNM_SEP_EXT;
#endif
  strcpy( fnmNew+len, ext );
  return fnmNew;
}

/* fopen file, found using pth and fnm
 * pfnmUsed used to return filename used to open file (ignored if NULL)
 * or NULL if file didn't open
 */
extern FILE FAR * fopenWithPthAndExt( const char * pth, const char * fnm, const char * szExt, const char * szMode,
                                      const char **pfnmUsed ) {
  const char *fnmFull = NULL;
  FILE *fh = NULL;
  bool fAbs;
  /* if no pth treat fnm as absolute */
  fAbs = (pth == NULL || *pth == '\0' || fAbsoluteFnm(fnm));

  /* if appropriate, try it without pth */
  if (fAbs || fAmbiguousFnm(fnm)) {
    fh=safe_fopen( fnm, szMode );
    if (fh) {
      if (pfnmUsed)
        fnmFull=osstrdup(fnm);
    } else {
      if (szExt && *szExt) { /* szExt!="" => got an extension to try */
        fnmFull=AddExt( fnm, szExt );
        fh=safe_fopen( fnmFull, szMode );
      }
    }
  }

  /* if filename could be relative, and file not opened yet */
  if (!fAbs && fh==NULL) {
    osfree(fnmFull);
    /* try using path given - first of all without the extension */
    fnmFull=UsePth( pth, fnm );
    fh=safe_fopen( fnmFull, szMode );
    if (!fh) {
      if (szExt && *szExt) {
        /* we've been given an extension so try using it */
        const char *fnmTmp;
        fnmTmp = fnmFull;
        fnmFull = AddExt( fnmFull, szExt );
        osfree(fnmTmp);
        fh = safe_fopen( fnmFull, szMode );
      }
    }
  }

  /* either it opened or didn't. If not, fh==NULL from safe_fopen() */
  if (fh==NULL || pfnmUsed==NULL)
    osfree(fnmFull); /* free name if it didn't open or name isn't wanted */
  if (pfnmUsed)
    *pfnmUsed = (fh ? fnmFull : NULL);
  return fh;
}
