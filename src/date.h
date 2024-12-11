/* date.h
 * Routines for date handling
 * Copyright (C) 2010,2015,2018 Olly Betts
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifdef __cplusplus
extern "C" {
#endif

extern int is_leap_year(int year);

/* Return last day for given month of given year. */
int last_day(int year, int month);

int days_since_1900(int y, int m, int d);

void ymd_from_days_since_1900(int days, int * py, int * pm, int * pd);

double julian_date_from_days_since_1900(int days);

#ifdef __cplusplus
}
#endif
