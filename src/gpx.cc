/* gpx.cc
 * Export from Aven as GPX.
 */
/* Copyright (C) 2012 Olaf Kähler
 * Copyright (C) 2012,2013,2014,2015,2016 Olly Betts
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

#include "gpx.h"

#include "export.h" // For LABELS, etc

#include <stdio.h>
#include <string>
#include <time.h>
#include <math.h>

#include "useful.h"
#include <proj.h>

#include "aven.h"
#include "message.h"

using namespace std;

#define WGS84_DATUM_STRING "EPSG:4326"

static void
html_escape(FILE *fh, const char *s)
{
    while (*s) {
	switch (*s) {
	    case '<':
		fputs("&lt;", fh);
		break;
	    case '>':
		fputs("&gt;", fh);
		break;
	    case '&':
		fputs("&amp;", fh);
		break;
	    default:
		PUTC(*s, fh);
	}
	++s;
    }
}

static void discarding_proj_logger(void *, int, const char *) { }

GPX::GPX(const char * input_datum)
{
    /* Prevent stderr spew from PROJ. */
    proj_log_func(PJ_DEFAULT_CTX, nullptr, discarding_proj_logger);

    pj = proj_create_crs_to_crs(PJ_DEFAULT_CTX,
				input_datum, WGS84_DATUM_STRING,
				NULL);

    if (pj) {
	// Normalise the output order so x is longitude and y latitude - by
	// default new PROJ has them switched for EPSG:4326 which just seems
	// confusing.
	PJ* pj_norm = proj_normalize_for_visualization(PJ_DEFAULT_CTX, pj);
	proj_destroy(pj);
	pj = pj_norm;
    }

    if (!pj) {
	wxString m = wmsg(/*Failed to initialise input coordinate system “%s”*/287);
	m = wxString::Format(m.c_str(), input_datum);
	throw m;
    }
}

GPX::~GPX()
{
    if (pj)
	proj_destroy(pj);
    free((void*)trk_name);
}

const int *
GPX::passes() const
{
    static const int default_passes[] = { LABELS|ENTS|FIXES|EXPORTS, LEGS|SURF, 0 };
    return default_passes;
}

/* Initialise GPX routines. */
void GPX::header(const char * title, const char *, time_t datestamp_numeric,
		 double, double, double, double, double, double)
{
    fputs(
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gpx version=\"1.0\" creator=\"" PACKAGE_STRING " (aven) - https://survex.com/\""
" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
" xmlns=\"http://www.topografix.com/GPX/1/0\""
" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0"
" http://www.topografix.com/GPX/1/0/gpx.xsd\">\n", fh);
    if (title) {
	fputs("<name>", fh);
	html_escape(fh, title);
	fputs("</name>\n", fh);
	trk_name = strdup(title);
    }
    if (datestamp_numeric != time_t(-1)) {
	struct tm * tm = gmtime(&datestamp_numeric);
	if (tm) {
	    char buf[32];
	    if (strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", tm)) {
		fputs("<time>", fh);
		fputs(buf, fh);
		fputs("</time>\n", fh);
	    }
	}
    }
    // FIXME: optional in GPX, but perhaps useful:
    // <bounds minlat="..." minlon="..." maxlat="..." maxlon="..." />
    // NB Not necessarily the same as the bounds in survex coords translated
    // to WGS84 lat+long...
}

void
GPX::line(const img_point *p1, const img_point *p, unsigned /*flags*/, bool fPendingMove)
{
    if (fPendingMove) {
	if (in_trkseg) {
	    fputs("</trkseg><trkseg>\n", fh);
	} else {
	    fputs("<trk>", fh);
	    if (trk_name) {
		fputs("<name>", fh);
		html_escape(fh, trk_name);
		fputs("</name>", fh);
	    }
	    fputs("<trkseg>\n", fh);
	    in_trkseg = true;
	}
	PJ_COORD coord{{p1->x, p1->y, p1->z, HUGE_VAL}};
	coord = proj_trans(pj, PJ_FWD, coord);
	if (coord.xyzt.x == HUGE_VAL ||
	    coord.xyzt.y == HUGE_VAL ||
	    coord.xyzt.z == HUGE_VAL) {
	    // FIXME report errors
	}
	// %.8f is at worst just over 1mm.
	fprintf(fh, "<trkpt lon=\"%.8f\" lat=\"%.8f\"><ele>%.2f</ele></trkpt>\n",
		coord.xyzt.x,
		coord.xyzt.y,
		coord.xyzt.z);
    }

    PJ_COORD coord{{p->x, p->y, p->z, HUGE_VAL}};
    coord = proj_trans(pj, PJ_FWD, coord);
    if (coord.xyzt.x == HUGE_VAL ||
	coord.xyzt.y == HUGE_VAL ||
	coord.xyzt.z == HUGE_VAL) {
	// FIXME report errors
    }
    // %.8f is at worst just over 1mm.
    fprintf(fh, "<trkpt lon=\"%.8f\" lat=\"%.8f\"><ele>%.2f</ele></trkpt>\n",
	    coord.xyzt.x,
	    coord.xyzt.y,
	    coord.xyzt.z);
}

void
GPX::label(const img_point *p, const wxString& str, bool /*fSurface*/, int type)
{
    const char* s = str.utf8_str();
    PJ_COORD coord{{p->x, p->y, p->z, HUGE_VAL}};
    coord = proj_trans(pj, PJ_FWD, coord);
    if (coord.xyzt.x == HUGE_VAL ||
	coord.xyzt.y == HUGE_VAL ||
	coord.xyzt.z == HUGE_VAL) {
	// FIXME report errors
    }
    // %.8f is at worst just over 1mm.
    fprintf(fh, "<wpt lon=\"%.8f\" lat=\"%.8f\"><ele>%.2f</ele><name>",
	    coord.xyzt.x,
	    coord.xyzt.y,
	    coord.xyzt.z);
    html_escape(fh, s);
    fputs("</name>", fh);
    // Add a "pin" symbol with colour matching what aven shows.
    switch (type) {
	case FIXES:
	    fputs("<sym>Pin, Red</sym>", fh);
	    break;
	case EXPORTS:
	    fputs("<sym>Pin, Blue</sym>", fh);
	    break;
	case ENTS:
	    fputs("<sym>Pin, Green</sym>", fh);
	    break;
    }
    fputs("</wpt>\n", fh);
}

void
GPX::footer()
{
    if (in_trkseg)
	fputs("</trkseg></trk>\n", fh);
    fputs("</gpx>\n", fh);
}
