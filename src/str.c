/* append a string */
/* Copyright (c) Olly Betts 1999, 2014, 2024
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

#include <config.h>

#include "str.h"

#include <string.h>

#include "osalloc.h"

void s_expand_(string *pstr, int addition) {
    int new_size = (pstr->len + addition + 33) & ~7;
    pstr->s = osrealloc(pstr->s, new_size);
    pstr->capacity = new_size - 1;
}

void
s_catlen(string* pstr, const char *s, int s_len)
{
   if (pstr->capacity - pstr->len < s_len)
       s_expand_(pstr, s_len);
   memcpy(pstr->s + pstr->len, s, s_len);
   pstr->len += s_len;
   pstr->s[pstr->len] = '\0';
}

void
s_catn(string *pstr, int n, char c)
{
   if (pstr->capacity - pstr->len < n)
       s_expand_(pstr, n);
   memset(pstr->s + pstr->len, c, n);
   pstr->len += n;
   pstr->s[pstr->len] = '\0';
}

extern inline void s_cat(string *pstr, const char *s);

extern inline void s_catchar(string *pstr, char c);

extern inline void s_clear(string *pstr);

extern inline void s_truncate(string *pstr, int new_len);

extern inline void s_free(string *pstr);

extern inline char *s_steal(string *pstr);

extern inline void s_donate(string *pstr, char *s);

extern inline int s_len(const string *pstr);

extern inline bool s_empty(const string *pstr);

extern inline const char *s_str(string *pstr);

extern inline bool s_eqlen(const string *pstr, const char *s, int s_len);

extern inline bool s_eq(const string *pstr, const char *s);
