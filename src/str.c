/* append a string */
/* Copyright (c) Olly Betts 1999, 2014
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

#include <string.h>

#include "osalloc.h"
#include "str.h"

/* append a string */
void
s_cat(char **pstr, int *plen, const char *s)
{
   s_catlen(pstr, plen, s, strlen(s));
}

/* append a string with length */
void
s_catlen(char **pstr, int *plen, const char *s, int s_len)
{
   int new_len = s_len + 1; /* extra 1 for nul */
   int len = 0;

   if (*pstr) {
      len = strlen(*pstr);
      new_len += len;
   }

   if (!*pstr || new_len > *plen) {
      *plen = (new_len + 32) & ~3;
      *pstr = osrealloc(*pstr, *plen);
   }

   memcpy(*pstr + len, s, s_len);
   (*pstr + len)[s_len] = '\0';
}

/* append a character */
void
s_catchar(char **pstr, int *plen, char ch)
{
   int new_len = 2; /* extra 1 for nul */
   int len = 0;

   if (*pstr) {
      len = strlen(*pstr);
      new_len += len;
   }

   if (!*pstr || new_len > *plen) {
      *plen = (new_len + 32) & ~3;
      *pstr = osrealloc(*pstr, *plen);
   }

   (*pstr)[len] = ch;
   (*pstr)[len + 1] = '\0';
}

void
s_catn(char **pstr, int *plen, int n, char ch)
{
   int new_len = n + 1; /* extra 1 for nul */
   int len = 0;

   if (*pstr) {
      len = strlen(*pstr);
      new_len += len;
   }

   if (!*pstr || new_len > *plen) {
      *plen = (new_len + 32) & ~3;
      *pstr = osrealloc(*pstr, *plen);
   }

   memset(*pstr + len, ch, n);
   (*pstr + len)[n] = '\0';
}
