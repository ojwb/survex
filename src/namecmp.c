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
   while (1) {
      int cha = *a, chb = *b;

      /* done if end of either first string */
      if (!cha || !chb) return cha - chb;

      /* check for end of non-numeric prefix */
      if (isdigit(cha)) {
	 /* sort numbers numerically and before non-numbers */
	 const char *sa, *sb, *ea, *eb;
	 int res;

	 if (!isdigit(chb)) return chb == '.' ? 1 : -1;

	 sa = a;
	 while (*sa == '0') sa++;
	 ea = sa;
	 while (isdigit(*ea)) ea++;

	 sb = b;
	 while (*sb == '0') sb++;
	 eb = sb;
	 while (isdigit(*eb)) eb++;

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
	 if (cha == '.') return -1;
	 if (isdigit(chb) || chb == '.') return 1;
	 return cha - chb;
      }

      a++;
      b++;
   }
}
