/* > filename.h
 * Function prototypes for filename.c
 * Copyright (C) 1998 Olly Betts
 */

/*
1998.03.22 split from error.h
*/

#ifndef FILENAME_H /* only include once */
#define FILENAME_H

#include "useful.h"
#include "osdepend.h"

extern char * FAR PthFromFnm( const char *fnm);
extern char * FAR LfFromFnm( const char *fnm);
extern char * FAR UsePth( const char *pth, const char *lf );
extern char * FAR AddExt( const char *fnm, const char *ext );

extern FILE FAR *fopenWithPthAndExt( const char *pth, const char *fnm,
				      const char *szExt, const char *szMode,
                                      char **fnmUsed);

extern FILE *safe_fopen( const char *fnm, const char *mode );

#endif /* FILENAME_H */
