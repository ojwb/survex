/* append a string */
/* Copyright (c) Olly Betts 1999
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
s_cat(char **pstr, int *plen, char *s)
{
   int new_len = strlen(s) + 1; /* extra 1 for nul */
   int len = 0;

   if (*pstr) {
      len = strlen(*pstr);
      new_len += len;
   }

   if (!*pstr || new_len > *plen) {
      *plen = (new_len + 32) & ~3;
      *pstr = osrealloc(*pstr, *plen);
   }

   strcpy(*pstr + len, s);
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

/* truncate string to zero length */
void
s_zero(char **pstr)
{
   if (*pstr) **pstr = '\0';
}

void
s_free(char **pstr)
{
   if (*pstr) {
      osfree(*pstr);
      *pstr = NULL;
   }
}
