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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "gpx.h"

#include "export.h" // For LABELS, etc

#include <stdio.h>
#include <string>
#include <time.h>
#include <math.h>

#include "useful.h"
#include <proj_api.h>

#include "aven.h"
#include "message.h"

using namespace std;

#define WGS84_DATUM_STRING "+proj=longlat +ellps=WGS84 +datum=WGS84"

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

GPX::GPX(const char * input_datum)
    : pj_input(NULL), pj_output(NULL), in_trkseg(false), trk_name(NULL)
{
    if (!(pj_input = pj_init_plus(input_datum))) {
	wxString m = wmsg(/*Failed to initialise input coordinate system “%s”*/287);
	m = wxString::Format(m.c_str(), input_datum);
	throw m;
    }
    if (!(pj_output = pj_init_plus(WGS84_DATUM_STRING))) {
	wxString m = wmsg(/*Failed to initialise output coordinate system “%s”*/288);
	m = wxString::Format(m.c_str(), WGS84_DATUM_STRING);
	throw m;
    }
}

GPX::~GPX()
{
    if (pj_input)
	pj_free(pj_input);
    if (pj_output)
	pj_free(pj_output);
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
	double X = p1->x, Y = p1->y, Z = p1->z;
	pj_transform(pj_input, pj_output, 1, 1, &X, &Y, &Z);
	X = deg(X);
	Y = deg(Y);
	// %.8f is at worst just over 1mm.
	fprintf(fh, "<trkpt lon=\"%.8f\" lat=\"%.8f\"><ele>%.2f</ele></trkpt>\n", X, Y, Z);
    }
    double X = p->x, Y = p->y, Z = p->z;
    pj_transform(pj_input, pj_output, 1, 1, &X, &Y, &Z);
    X = deg(X);
    Y = deg(Y);
    // %.8f is at worst just over 1mm.
    fprintf(fh, "<trkpt lon=\"%.8f\" lat=\"%.8f\"><ele>%.2f</ele></trkpt>\n", X, Y, Z);
}

void
GPX::label(const img_point *p, const char *s, bool /*fSurface*/, int type)
{
    double X = p->x, Y = p->y, Z = p->z;
    pj_transform(pj_input, pj_output, 1, 1, &X, &Y, &Z);
    X = deg(X);
    Y = deg(Y);
    // %.8f is at worst just over 1mm.
    fprintf(fh, "<wpt lon=\"%.8f\" lat=\"%.8f\"><ele>%.2f</ele><name>", X, Y, Z);
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
