/* pos.cc
 * Export from Aven as Survex .pos or .csv.
 */
/* Copyright (C) 2001-2024 Olly Betts
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

#include <config.h>

#include "pos.h"

#include "export.h" // For LABELS, etc

#include <algorithm>
#include <stdio.h>
#include <string.h>

#include "message.h"
#include "namecompare.h"
#include "useful.h"

using namespace std;

static void
csv_quote(const char* s, FILE* fh)
{
    size_t i = 0;
    while (true) {
	switch (s[i]) {
	    case '\0':
		fputs(s, fh);
		return;
	    case ',':
	    case '"':
	    case '\r':
	    case '\n':
		break;
	}
	++i;
    }
    PUTC('"', fh);
    FWRITE_(s, i, 1, fh);
    while (s[i]) {
	// Double up any " in the string to escape them.
	if (s[i] == '"')
	    PUTC(s[i], fh);
	PUTC(s[i], fh);
	++i;
    }
    PUTC('"', fh);
}

POS::~POS()
{
    vector<pos_label*>::const_iterator i;
    for (i = todo.begin(); i != todo.end(); ++i) {
	free(*i);
    }
    todo.clear();
}

const int *
POS::passes() const
{
    static const int default_passes[] = { LABELS|ENTS|FIXES|EXPORTS, 0 };
    return default_passes;
}

void POS::header(const char *, time_t,
		 double, double, double, double, double, double)
{
    if (csv) {
	bool comma = false;
	for (int msgno : { /*Easting*/378,
			   /*Northing*/379,
			   /*Altitude*/335,
			   /*Station Name*/100 }) {
	    if (comma) PUTC(',', fh);
	    csv_quote(msg(msgno), fh);
	    comma = true;
	}
    } else {
	/* TRANSLATORS: Heading line for .pos file.  Please try to ensure the
	 * “,”s (or at least the columns) are in the same place */
	fputs(msg(/*( Easting, Northing, Altitude )*/195), fh);
    }
    PUTC('\n', fh);
}

void
POS::label(const img_point *p, const wxString& str, int /*sflags*/, int /*type*/)
{
    const char* s = str.utf8_str();
    size_t len = strlen(s);
    pos_label * l = (pos_label*)malloc(offsetof(pos_label, name) + len + 1);
    if (l == NULL)
	throw std::bad_alloc();
    l->x = p->x;
    l->y = p->y;
    l->z = p->z;
    memcpy(l->name, s, len + 1);
    todo.push_back(l);
}

class pos_label_ptr_cmp {
    char separator;

  public:
    explicit pos_label_ptr_cmp(char separator_) : separator(separator_) { }

    bool operator()(const POS::pos_label* a, const POS::pos_label* b) {
	return name_cmp(a->name, b->name, separator) < 0;
    }
};

void
POS::footer()
{
    sort(todo.begin(), todo.end(), pos_label_ptr_cmp(separator));
    vector<pos_label*>::const_iterator i;
    for (i = todo.begin(); i != todo.end(); ++i) {
	if (csv) {
	    fprintf(fh, "%.2f,%.2f,%.2f,", (*i)->x, (*i)->y, (*i)->z);
	    csv_quote((*i)->name, fh);
	    PUTC('\n', fh);
	} else {
	    fprintf(fh, "(%8.2f, %8.2f, %8.2f ) %s\n",
		    (*i)->x, (*i)->y, (*i)->z, (*i)->name);
	}
    }
}
