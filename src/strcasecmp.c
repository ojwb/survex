/* portable case insensitive string compare */
/* Copyright (C) Olly Betts 1994,1999
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

#include <ctype.h>

/* What about top bit set chars? */
int strcasecmp(const char *s1, const char *s2) {
   register c1, c2;
   do {
      c1 = *s1++;
      c2 = *s2++;
   } while (c1 && toupper(c1) == toupper(c2));
   /* now calculate real difference */
   return c1 - c2;
}
