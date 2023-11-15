/* json.cc
 * Export from Aven as JSON.
 */
/* Copyright (C) 2015,2016,2022 Olly Betts
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

#include "json.h"

#include "export.h"

#include <stdio.h>

using namespace std;

const int *
JSON::passes() const
{
    static const int default_passes[] = { LEGS|SURF, 0 };
    return default_passes;
}

void
JSON::header(const char * title, const char *, time_t datestamp_numeric,
	     double min_x, double min_y, double min_z,
	     double max_x, double max_y, double max_z)
{
    (void)title;
    (void)datestamp_numeric;
#if 0
    if (title) {
	fputs("title: \"", fh);
	json_escape(fh, title);
	fputs("\",\n", fh);
    }
    if (datestamp_numeric != time_t(-1)) {
	fprintf("date: %ld,\n", (long)datestamp_numeric);
    }
#endif
    fprintf(fh, "{\"bounds\":[%.2f,%.2f,%.2f,%.2f,%.2f,%.2f],\n\"traverses\":[\n",
	    min_x, min_z, min_y, max_x, max_z, max_y);
}

void
JSON::line(const img_point *p1, const img_point *p, unsigned /*flags*/, bool fPendingMove)
{
    if (fPendingMove) {
	if (in_segment) {
	    fputs("],\n[", fh);
	} else {
	    fputs("[", fh);
	    in_segment = true;
	}
	fprintf(fh, "[%.2f,%.2f,%.2f]", p1->x, p1->z, p1->y);
    }
    fprintf(fh, ",[%.2f,%.2f,%.2f]", p->x, p->z, p->y);
}

void
JSON::label(const img_point *p, const char *s, bool /*fSurface*/, int type)
{
    (void)p;
    (void)s;
    (void)type;
}

void
JSON::footer()
{
    if (in_segment)
	fputs("]\n", fh);
    fputs("]}\n", fh);
}
