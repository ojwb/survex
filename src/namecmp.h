/* namecmp.h */
/* Ordering function for station names */
/* Copyright (C) 2001,2002,2008 Olly Betts
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

#ifdef __cplusplus
#include "wx.h"

extern "C" {
#endif

extern int name_cmp(const char *a, const char *b, int separator);

#ifdef __cplusplus
};

inline int name_cmp(const wxString &a, const wxString &b, int separator) {
    const char * p_a = a.mb_str();
    const char * p_b = b.mb_str();
    return name_cmp(p_a, p_b, separator);
}
#endif
