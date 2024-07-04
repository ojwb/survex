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
   if (pstr->capacity - pstr->len < s_len || s_len == 0)
       s_expand_(pstr, s_len);
   memcpy(pstr->s + pstr->len, s, s_len);
   pstr->len += s_len;
   pstr->s[pstr->len] = '\0';
}

void
s_catn(string *pstr, int n, char c)
{
   if (pstr->capacity - pstr->len < n || n == 0)
       s_expand_(pstr, n);
   memset(pstr->s + pstr->len, c, n);
   pstr->len += n;
   pstr->s[pstr->len] = '\0';
}
