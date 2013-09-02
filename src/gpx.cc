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

/*
 * This program parses the survex file, creates a list of entrances and does
 * the coordinate transformations from almost arbitrary formats into WGS84
 * using the PROJ.4 library. The output is then written as a GPX file ready
 * for use in your favourite GPS software. Everything else is kept as simple
 * and minimalistic as possible.
 *
 * Usage:
 *   findentrances -d <+proj +datum +string> <input.3d>
 *
 * Example for data given in BMN M31 (Totes Gebirge, Austria):
 *   findentrances -d '+proj=tmerc +lat_0=0 +lon_0=13d20 +k=1 +x_0=0 +y_0=-5200000 +ellps=bessel +towgs84=577.326,90.129,463.919,5.137,1.474,5.297,2.4232' cucc_austria.3d > ent.gpx
 *
 * Example for data given in british grid SD (Yorkshire):
 *   findentrances -d '+proj=tmerc +lat_0=49d +lon_0=-2d +k=0.999601 +x_0=100000 +y_0=-500000 +ellps=airy +towgs84=375,-111,431,0,0,0,0' yorkshire/all.3d > ent.gpx
 *
 * Example for data given as proper british grid reference:
 *   findentrances -d '+proj=tmerc +lat_0=49d +lon_0=-2d +k=0.999601 +x_0=400000 +y_0=-100000 +ellps=airy +towgs84=375,-111,431,0,0,0,0' all.3d > ent.gpx
 *
 */

#include "gpx.h"

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

GPX::GPX()
    : pj_input(NULL), pj_output(NULL)
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

/* Check if this line intersects the current page */
/* Initialise GPX routines. */
void GPX::header(const char *)
{
    fprintf(fh,
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gpx version=\"1.0\" creator=\"survex - aven\""
" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
" xmlns=\"http://www.topografix.com/GPX/1/0\""
" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0"
" http://www.topografix.com/GPX/1/0/gpx.xsd\">\n");
}

void
GPX::line(const img_point *, const img_point *, bool, bool)
{
}

void
GPX::label(const img_point *p, const char *s, bool /*fSurface*/)
{
    double X = p->x, Y = p->y, Z = p->z;
    pj_transform(pj_input, pj_output, 1, 1, &X, &Y, &Z);
    X = deg(X);
    Y = deg(Y);
    // %.8f is at worst just over 1mm.
    fprintf(fh, "<wpt lon=\"%.8f\" lat=\"%.8f\"><ele>%.2f</ele><name>", X, Y, Z);
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
    fputs("</name></wpt>\n", fh);
}

void
GPX::footer()
{
    fprintf(fh, "</gpx>\n");
}
