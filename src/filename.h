/* filename.h
 * Function prototypes for filename.c
 * Copyright (C) 1998-2001,2003 Olly Betts
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

char * Far path_from_fnm(const char *fnm);
char * Far base_from_fnm(const char *fnm);
char * Far baseleaf_from_fnm(const char *fnm);
char * Far leaf_from_fnm(const char *fnm);
char * Far use_path(const char *pth, const char *lf);
char * Far add_ext(const char *fnm, const char *ext);

FILE *fopenWithPthAndExt(const char *pth, const char *fnm, const char *ext,
			 const char *mode, char **fnmUsed);

FILE *fopen_portable(const char *pth, const char *fnm, const char *ext,
		     const char *mode, char **fnmUsed);

FILE *safe_fopen(const char *fnm, const char *mode);
FILE *safe_fopen_with_ext(const char *fnm, const char *ext, const char *mode);
void safe_fclose(FILE *f);

void filename_register_output(const char *fnm);
void filename_delete_output(void);

#endif /* FILENAME_H */
