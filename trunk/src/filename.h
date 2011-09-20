/* filename.h
 * Function prototypes for filename.c
 * Copyright (C) 1998-2003,2010 Olly Betts
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef FILENAME_H /* only include once */
#define FILENAME_H

#ifdef __cplusplus
extern "C" {
#endif

#include "useful.h"
#include "osdepend.h"

char * path_from_fnm(const char *fnm);
char * base_from_fnm(const char *fnm);
char * baseleaf_from_fnm(const char *fnm);
char * leaf_from_fnm(const char *fnm);
char * use_path(const char *pth, const char *lf);
char * add_ext(const char *fnm, const char *ext);

FILE *fopenWithPthAndExt(const char *pth, const char *fnm, const char *ext,
			 const char *mode, char **fnmUsed);

FILE *fopen_portable(const char *pth, const char *fnm, const char *ext,
		     const char *mode, char **fnmUsed);

FILE *safe_fopen(const char *fnm, const char *mode);
FILE *safe_fopen_with_ext(const char *fnm, const char *ext, const char *mode);
void safe_fclose(FILE *f);

void filename_register_output(const char *fnm);
void filename_delete_output(void);

#ifdef __cplusplus
}
#endif

#endif /* FILENAME_H */
