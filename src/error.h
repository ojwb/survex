/* > error.h
 * Function prototypes for error.c
 * Copyright (C) 1993-1997 Olly Betts
 */

/*
1993.05.12 (W) added FNTYPE for diff DOS memory models (non-ANSI)
1993.06.07 FNTYPE -> FAR to aid comprehension
1993.06.16 make sure we only include this once
1993.06.16 syserr() -> fatal(); added osmalloc() etc
1993.07.19 "common.h" -> "osdepend.h"
1993.08.15 fettled header
1993.08.16 added pfPthUsed argument to fopenWithPthAndExt
           added AddExt; added pfExtUsed arg to fopenWithPthAndExt
1993.09.23 (IH)DOS osfree uses farfree
1993.11.03 altered warning/error/fatal to take *function for context info
1993.11.05 OSSIZE_T added
           added msg() and msgPerm()
1993.11.15 added xosmalloc() which returns NULL if malloc fails
1993.11.18 added xosrealloc() as macro
           added safe_fopen prototype
1993.11.29 (IH) void * FAR -> void FAR * ; void * FILE -> void FILE *
           ossizeof added
1993.12.01 added error_summary()
1993.12.16 farmalloc use controlled by NO_FLATDOS macro
1994.03.14 altered fopenWithPthAndExt to give filename actually used
1994.06.09 added home directory environmental variable
1994.06.20 added int argument to warning, error and fatal
1994.08.27 added msgFree()
1994.09.27 added RISCOS_LEAKFIND stuff
1994.09.28 xosmalloc is now a macro here
1994.10.01 removed xosmalloc prototype
1994.10.05 added language arg to ReadErrorFile
1994.10.08 added osnew() macro
1994.10.24 fixed osnew() macro to allocate sizeof(T) rather than sizeof(T*)
1995.03.25 added osstrdup
1996.02.19 malloc and friends split off into osalloc.h
1996.04.04 introduced NOP
1997.06.05 added const
1998.03.21 fixed up to compile cleanly on Linux
*/

#ifndef ERROR_H /* only include once */
#define ERROR_H

#include "useful.h"
#include "osdepend.h"
#include "osalloc.h"

#ifdef RISCOS_LEAKFIND
# include "Mnemosyne.mnemosyn.h"
#endif

extern const char * FAR ReadErrorFile( const char *szAppName, const char *szEnvVar,
				 const char *szLangVar, const char *argv0,
				 const char *lfErrs );

int error_summary( void ); /* report total warnings and non-fatal errors */

/* Message may be overwritten by next call */
extern const char *msg( int en );
/* Returns persistent copy of message */
extern const char *msgPerm( int en);
/* Kill persistent copy of message */
#define msgFree( SZ ) NOP

extern void FAR warning( int en, void (*fn)( const char *, int ), const char *, int );
extern void FAR   error( int en, void (*fn)( const char *, int ), const char *, int );
extern void FAR   fatal( int en, void (*fn)( const char *, int ), const char *, int );

void wr( const char *, int ); /* write string sz and \n - return void */

extern char * FAR PthFromFnm( const char *fnm);
extern char * FAR LfFromFnm( const char *fnm);
extern char * FAR UsePth( const char *pth, const char *lf );
extern char * FAR AddExt( const char *fnm, const char *ext );

extern FILE FAR *fopenWithPthAndExt( const char *pth, const char *fnm,
				      const char *szExt, const char *szMode,
                                      char **fnmUsed);

extern FILE *safe_fopen( const char *fnm, const char *mode );

#endif /* !ERROR_H */
