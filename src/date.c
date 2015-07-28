/* date.c
 * Routines for date handling
 * Copyright (C) 2010,2015 Olly Betts
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "date.h"

#include "debug.h"

int
is_leap_year(int year)
{
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

static int lastday[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

int
last_day(int year, int month)
{
    SVX_ASSERT(month >= 1 && month <= 12);
    return (month == 2 && is_leap_year(year)) ? 29 : lastday[month - 1];
}

int
days_since_1900(int y, int m, int d)
{
    static const int m_to_d[12] = {
	0 - (1900 * 365) - 461 + 365,
	31 - (1900 * 365) - 461 + 365,
	59 - (1900 * 365) - 461,
	90 - (1900 * 365) - 461,
	120 - (1900 * 365) - 461,
	151 - (1900 * 365) - 461,
	181 - (1900 * 365) - 461,
	212 - (1900 * 365) - 461,
	243 - (1900 * 365) - 461,
	273 - (1900 * 365) - 461,
	304 - (1900 * 365) - 461,
	334 - (1900 * 365) - 461
    };
    if (m < 3)
	--y;
    return d + (y * 365) + m_to_d[m - 1] + (y / 4) - (y / 100) + (y / 400);
}

void
ymd_from_days_since_1900(int days, int * py, int * pm, int * pd)
{
    int g, dg, c, dc, b, db, a, da, y, m;
    days += 693901;
    g = days / 146097;
    dg = days % 146097;
    c = (dg / 36524 + 1) * 3 / 4;
    dc = dg - c * 36524;
    b = dc / 1461;
    db = dc % 1461;
    a = (db / 365 + 1) * 3 / 4;
    da = db - a * 365;
    y = g * 400 + c * 100 + b * 4 + a;
    m = (da * 5 + 308) / 153;
    *py = y + m / 12;
    *pm = m % 12 + 1;
    *pd = da - (m + 2) * 153 / 5 + 123;
}

double
julian_date_from_days_since_1900(int days)
{
    int g, dg, c, dc, b, db, a, da, y, m;
    int days_in = days;
    int dys;
    double scale;
    days += 693901;
    g = days / 146097;
    dg = days % 146097;
    c = (dg / 36524 + 1) * 3 / 4;
    dc = dg - c * 36524;
    b = dc / 1461;
    db = dc % 1461;
    a = (db / 365 + 1) * 3 / 4;
    da = db - a * 365;
    y = g * 400 + c * 100 + b * 4 + a;
    m = (da * 5 + 308) / 153;
    y = y + m / 12;
    /* dys is days since 1900 for the start of the year y. */
    dys = (y - 1900) * 365 - 460;
    dys += ((y - 1) / 4) - ((y - 1) / 100) + ((y - 1) / 400);
    scale = (is_leap_year(y) ? 1 / 366.0 : 1 / 365.0);
    return y + scale * (days_in - dys);
}
