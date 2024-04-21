/* namecompare.cc */
/* Ordering function for station names */
/* Copyright (C) 1991-2002,2004,2012,2016 Olly Betts
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

#include "namecompare.h"

inline bool u_digit(unsigned ch) {
    return (ch - unsigned('0')) <= unsigned('9' - '0');
}

int name_cmp(const wxString &a, const wxString &b, int separator) {
   size_t i = 0;
   size_t shorter = std::min(a.size(), b.size());
   while (i != shorter) {
      int cha = a[i];
      int chb = b[i];
      /* check for end of non-numeric prefix */
      if (u_digit(cha)) {
	 /* sort numbers numerically and before non-numbers */
	 size_t sa, sb, ea, eb;
	 int res;

	 if (!u_digit(chb)) return chb == separator ? 1 : -1;

	 sa = i;
	 while (sa != a.size() && a[sa] == '0') sa++;
	 ea = sa;
	 while (ea != a.size() && u_digit(a[ea])) ea++;

	 sb = i;
	 while (sb != b.size() && b[sb] == '0') sb++;
	 eb = sb;
	 while (eb != b.size() && u_digit(b[eb])) eb++;

	 /* shorter sorts first */
	 res = int(ea - sa) - int(eb - sb);
	 /* same length, all digits, so character value compare sorts
	  * numerically */
	 for (size_t j = sa; !res && j != ea; ++j) {
	    res = int(a[j]) - int(b[j - sa + sb]);
	 }
	 /* more leading zeros sorts first */
	 if (!res) res = int(sb) - int(sa);
	 if (res) return res;

	 /* if numbers match, sort by suffix */
	 i = ea;
	 continue;
      }

      if (cha != chb) {
	 if (cha == separator) return -1;
	 if (u_digit(chb) || chb == separator) return 1;
	 return cha - chb;
      }

      i++;
   }
   return int(a.size()) - int(b.size());
}
