/* append a string */
/* Copyright (c) Olly Betts 1999, 2001, 2012, 2014
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

#include "osalloc.h"

void s_cat(char **pstr, int *plen, const char *s);

void s_catlen(char **pstr, int *plen, const char *s, int s_len);

/* append a character */
void s_catchar(char **pstr, int *plen, char /*ch*/);

/* append n copies of a character */
void s_catn(char **pstr, int *plen, int n, char /*ch*/);

/* truncate string to zero length */
#define s_zero(P) do { \
	char **s_zero__P = (P); \
	if (*s_zero__P) **s_zero__P = '\0'; \
    } while (0)

#define s_free(P) do { \
	char **s_free__P = (P); \
	if (*s_free__P) { \
	    osfree(*s_free__P); \
	    *s_free__P = NULL; \
	} \
    } while (0)
