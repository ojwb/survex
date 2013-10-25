/* gpx.cc
 * Export from Aven as GPX.
 */
/* Copyright (C) 2012 Olaf Kähler
 * Copyright (C) 2012,2013 Olly Betts
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

/* We us the PROJ.4 library to transform coordinates from almost arbitrary
 * grids into WGS84.  The output is then written as a GPX file ready for use in
 * your favourite GPS software.
 *
 * Example for data given in BMN M31 (Totes Gebirge, Austria):
 *   +proj=tmerc +lat_0=0 +lon_0=13d20 +k=1 +x_0=0 +y_0=-5200000 +ellps=bessel +towgs84=577.326,90.129,463.919,5.137,1.474,5.297,2.4232
 *
 * Example for data given in british grid SD (Yorkshire):
 *   +proj=tmerc +lat_0=49d +lon_0=-2d +k=0.999601 +x_0=100000 +y_0=-500000 +ellps=airy +towgs84=375,-111,431,0,0,0,0
 *
 * Example for data given as proper british grid reference:
 *   +proj=tmerc +lat_0=49d +lon_0=-2d +k=0.999601 +x_0=400000 +y_0=-100000 +ellps=airy +towgs84=375,-111,431,0,0,0,0
 */

#include "gpx.h"

#include "export.h" // For LABELS, etc

#include <stdio.h>
#include <string>
#include <math.h>

#include "useful.h"
#include <proj_api.h>

#include "aven.h"
#include "message.h"

using namespace std;

#define WGS84_DATUM_STRING "+proj=longlat +ellps=WGS84 +datum=WGS84"

//   {HLP_ENCODELONG(0),        /*input datum as string to pass to PROJ*/389, 0},

// cmdline_set_syntax_message(/*-d PROJ_DATUM 3D_FILE*/388, 0, NULL);
static const char * input_datum = "+proj=tmerc +lat_0=0 +lon_0=13d20 +k=1 +x_0=0 +y_0=-5200000 +ellps=bessel +towgs84=577.326,90.129,463.919,5.137,1.474,5.297,2.4232";

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

GPX::GPX()
    : pj_input(NULL), pj_output(NULL), in_trkseg(false)
{
    if (!(pj_input = pj_init_plus(input_datum))) {
	wxString m = wmsg(/*Failed to initialise input coordinate system “%s”*/287);
	m = wxString::Format(m.c_str(), input_datum);
	wxGetApp().ReportError(m);
	return;
    }
    if (!(pj_output = pj_init_plus(WGS84_DATUM_STRING))) {
	wxString m = wmsg(/*Failed to initialise output coordinate system “%s”*/288);
	m = wxString::Format(m.c_str(), WGS84_DATUM_STRING);
	wxGetApp().ReportError(m);
	return;
    }
}

GPX::~GPX()
{
    if (pj_input)
	pj_free(pj_input);
    if (pj_output)
	pj_free(pj_output);
}

const int *
GPX::passes() const
{
    static const int default_passes[] = { LABELS|ENTS|FIXES|EXPORTS, LEGS|SURF, 0 };
    return default_passes;
}

/* Initialise GPX routines. */
void GPX::header(const char * title)
{
    fputs(
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gpx version=\"1.0\" creator=\""PACKAGE_STRING" (aven) - http://survex.com/\""
" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
" xmlns=\"http://www.topografix.com/GPX/1/0\""
" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0"
" http://www.topografix.com/GPX/1/0/gpx.xsd\">\n", fh);
    fputs("<name>", fh);
    html_escape(fh, title);
    fputs("</name>\n", fh);
    // FIXME: optional in GPX, but perhaps useful:
    // <bounds minlat="..." minlon="..." maxlat="..." maxlon="..." />
    // NB Not necessarily the same as the bounds in survex coords translated
    // to WGS85 lat+long...
}

void
GPX::line(const img_point *p1, const img_point *p, bool /*fSurface*/, bool fPendingMove)
{
    if (fPendingMove) {
	if (in_trkseg) {
	    fputs("</trkseg><trkseg>\n", fh);
	} else {
	    fputs("<trk><trkseg>\n", fh);
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
