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

#include <stdlib.h>
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
	 if (res == 0) res = name_cmp(dot_a + 1, dot_b + 1);
	 return res;
      }
      return -1;
   }

   if (dot_b) return 1;
   
   /* skip common prefix */
   while (*a && !isdigit(*a) && *a == *b) {
      a++;
      b++;
   }
   
   /* sort numbers numerically and before non-numbers */
   if (!isdigit(a[0])) {
      if (isdigit(b[0])) return 1;
   } else {
      long n_a, n_b;
      if (!isdigit(b[0])) return -1;
      /* FIXME: check errno, etc in case out of range */
      n_a = strtoul(a, NULL, 10);
      n_b = strtoul(b, NULL, 10);
      if (n_a != n_b) {
	 if (n_a > n_b)
	    return 1;
	 else
	    return -1;
      }
      /* drop through - the numbers match, but there may be a suffix
       * and also we want to give an order to "01" vs "1"... */
   }

   /* if numbers match, sort by suffix */
   return strcmp(a, b);
}
