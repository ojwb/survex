/* > filename.h
 * Function prototypes for filename.c
 * Copyright (C) 1998 Olly Betts
 */

#ifndef FILENAME_H /* only include once */
#define FILENAME_H

#include "useful.h"
#include "osdepend.h"

#if 0
#define PthFromFnm path_from_fnm
#define LfFromFnm leaf_from_fnm
#define UsePth use_path
#define AddExt add_ext
#endif

extern char * FAR path_from_fnm(const char *fnm);
extern char * FAR base_from_fnm(const char *fnm);
extern char * FAR baseleaf_from_fnm(const char *fnm);
extern char * FAR leaf_from_fnm(const char *fnm);
extern char * FAR use_path(const char *pth, const char *lf);
extern char * FAR add_ext(const char *fnm, const char *ext);

extern FILE FAR *fopenWithPthAndExt(const char *pth, const char *fnm,
				    const char *szExt, const char *szMode,
				    char **fnmUsed);

extern FILE *safe_fopen(const char *fnm, const char *mode);
extern FILE *safe_fopen_with_ext(const char *fnm, const char *ext, const char *mode);

#endif /* FILENAME_H */
