/* > filename.h
 * Function prototypes for filename.c
 * Copyright (C) 1998 Olly Betts
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
