/* namecmp.c */
/* Ordering function for station names */
/* Copyright (C) 1991-2001 Olly Betts
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <ctype.h>

#include "namecmp.h"

int
name_cmp(const char *a, const char *b)
{
   const char *dot_a = strchr(a, '.');
   const char *dot_b = strchr(b, '.');

   if (dot_a) {
      if (dot_b) {
	 size_t len_a = dot_a - a;
	 size_t len_b = dot_b - b;
	 int res = memcmp(a, b, len_a < len_b ? len_a : len_b);
	 if (res == 0) res = len_a - len_b;
	 if (res) return res;
	 return name_cmp(dot_a + 1, dot_b + 1);
      }
      return -1;
   }

   if (dot_b) return 1;
   
   repeat:

   /* skip common prefix */
   while (*a && !isdigit(*a) && *a == *b) {
      a++;
      b++;
   }

   if (!*a) {
      if (*b) return -1;
      return 0;
   }

   if (!*b) return 1;

   if (isdigit(a[0])) {
      /* sort numbers numerically and before non-numbers */
      const char *sa, *sb, *ea, *eb;
      int res;

      if (!isdigit(b[0])) return -1;

      sa = a;
      while (*sa == '0') sa++;
      ea = sa;
      while (isdigit(*ea)) ea++;

      sb = b;
      while (*sb == '0') sb++;
      eb = sb;
      while (isdigit(*eb)) eb++;

      res = (ea - sa) - (eb - sb); /* shorter sorts first */
      if (!res) res = memcmp(sa, sb, ea - sa); /* same length, all digits, so memcmp() sorts numerically */
      if (!res) res = (sb - b) - (sa - a); /* more leading zeros sorts first */
      if (res) return res;

      /* if numbers match, sort by suffix */
      a = ea;
      b = eb;
      goto repeat;
   }

   if (isdigit(b[0])) return 1;
   return strcmp(a, b);
}
