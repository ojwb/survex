/* namecmp.c */
/* Ordering function for station names */
/* Copyright (C) 1991-2002,2004 Olly Betts
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

#include <string.h>
#include <ctype.h>

#include "namecmp.h"

int
name_cmp(const char *a, const char *b, int separator)
{
   while (1) {
      int cha = (unsigned char)*a, chb = (unsigned char)*b;

      /* done if end of either first string */
      if (!cha || !chb) return cha - chb;

      /* check for end of non-numeric prefix */
      if (isdigit(cha)) {
	 /* sort numbers numerically and before non-numbers */
	 const char *sa, *sb, *ea, *eb;
	 int res;

	 if (!isdigit(chb)) return chb == separator ? 1 : -1;

	 sa = a;
	 while (*sa == '0') sa++;
	 ea = sa;
	 while (isdigit((unsigned char)*ea)) ea++;

	 sb = b;
	 while (*sb == '0') sb++;
	 eb = sb;
	 while (isdigit((unsigned char)*eb)) eb++;

	 /* shorter sorts first */
	 res = (ea - sa) - (eb - sb);
	 /* same length, all digits, so memcmp() sorts numerically */
	 if (!res) res = memcmp(sa, sb, ea - sa);
	 /* more leading zeros sorts first */
	 if (!res) res = (sb - b) - (sa - a);
	 if (res) return res;

	 /* if numbers match, sort by suffix */
	 a = ea;
	 b = eb;
	 continue;
      }

      if (cha != chb) {
	 if (cha == separator) return -1;
	 if (isdigit(chb) || chb == separator) return 1;
	 return cha - chb;
      }

      a++;
      b++;
   }
}
