/* > message.h
 * Function prototypes for message.c
 * Copyright (C) 1998 Olly Betts
 */

/*
1998.03.22 split from error.h
*/

#ifndef MESSAGE_H /* only include once */
#define MESSAGE_H

#include "useful.h"
#include "osdepend.h"
#include "osalloc.h"

#ifdef RISCOS_LEAKFIND
# include "Mnemosyne.mnemosyn.h"
#endif

/* name of current application */
extern const char *szAppNameCopy;

extern const char * FAR ReadErrorFile( const char *szAppName, const char *szEnvVar,
				 const char *szLangVar, const char *argv0,
				 const char *lfErrs );

extern int error_summary( void ); /* report total warnings and non-fatal errors */

/* Message may be overwritten by next call */
extern const char *msg( int en );
/* Returns persistent copy of message */
extern const char *msgPerm( int en);
/* Kill persistent copy of message */
#define msgFree( SZ ) NOP

extern void FAR warning( int en, void (*fn)( const char *, int ), const char *, int );
extern void FAR   error( int en, void (*fn)( const char *, int ), const char *, int );
extern void FAR   fatal( int en, void (*fn)( const char *, int ), const char *, int );

extern void wr( const char *, int ); /* write string sz and \n - return void */

#endif /* MESSAGE_H */
