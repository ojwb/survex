/* osalloc.h
 * Function prototypes for OS dep. malloc etc - funcs in error.c
 * Copyright (C) 1996,1997,2001,2003,2004,2010,2025 Olly Betts
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

#ifndef OSALLOC_H /* only include once */
#define OSALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

/* Allocate like C++ new -- call osnew(<type>) eg. osnew(point) */
#define osnew(T) (T*)osmalloc(sizeof(T))

void *osmalloc(size_t);
void *osrealloc(void *, size_t);
char *osstrdup(const char *str);

#ifdef __cplusplus
}
#endif

#endif
