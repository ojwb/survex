/* kml.cc
 * Export from Aven as KML.
 */
/* Copyright (C) 2012 Olaf Kähler
 * Copyright (C) 2012,2013,2014,2015 Olly Betts
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

#include "kml.h"

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

KML::KML(const char * input_datum)
    : pj_input(NULL), pj_output(NULL), in_trkseg(false)
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

KML::~KML()
{
    if (pj_input)
	pj_free(pj_input);
    if (pj_output)
	pj_free(pj_output);
}

const int *
KML::passes() const
{
    // static const int default_passes[] = { LABELS|ENTS|FIXES|EXPORTS, LEGS|SURF, 0 };
    static const int default_passes[] = { LABELS|ENTS|FIXES|EXPORTS, 0 };
    return default_passes;
}

/* Initialise KML routines. */
void KML::header(const char * title, const char *, time_t,
		 double, double, double, double, double, double)
{
    fputs(
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n", fh);
    fputs("<Document><name>", fh);
    html_escape(fh, title);
    fputs("</name>\n", fh);
    // Set up styles for the icons to reduce the file size.
    fputs("<Style id=\"fix\"><IconStyle>"
	  "<Icon><href>http://maps.google.com/mapfiles/kml/pushpin/red-pushpin.png</href></Icon>"
	  "</IconStyle></Style>\n", fh);
    fputs("<Style id=\"exp\"><IconStyle>"
	  "<Icon><href>http://maps.google.com/mapfiles/kml/pushpin/blue-pushpin.png</href></Icon>"
	  "</IconStyle></Style>\n", fh);
    fputs("<Style id=\"ent\"><IconStyle>"
	  "<Icon><href>http://maps.google.com/mapfiles/kml/pushpin/grn-pushpin.png</href></Icon>"
	  "</IconStyle></Style>\n", fh);
    // FIXME: does KML allow bounds?
    // NB Lat+long bounds are not necessarily the same as the bounds in survex
    // coords translated to WGS84 lat+long...
}

void
KML::line(const img_point *p1, const img_point *p, bool /*fSurface*/, bool fPendingMove)
{
    (void)p1;
    (void)p;
    (void)fPendingMove;
#if 0
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
#endif
}

void
KML::label(const img_point *p, const char *s, bool /*fSurface*/, int type)
{
    (void)type;
    double X = p->x, Y = p->y, Z = p->z;
    pj_transform(pj_input, pj_output, 1, 1, &X, &Y, &Z);
    X = deg(X);
    Y = deg(Y);
    // %.8f is at worst just over 1mm.
    fprintf(fh, "<Placemark><Point><coordinates>%.8f,%.8f,%.2f</coordinates></Point><name>", X, Y, Z);
    html_escape(fh, s);
    fputs("</name>", fh);
    // Add a "pin" symbol with colour matching what aven shows.
    switch (type) {
	case FIXES:
	    fputs("<styleUrl>#fix</styleUrl>", fh);
	    break;
	case EXPORTS:
	    fputs("<styleUrl>#exp</styleUrl>", fh);
	    break;
	case ENTS:
	    fputs("<styleUrl>#ent</styleUrl>", fh);
	    break;
    }
    fputs("</Placemark>\n", fh);
}

void
KML::footer()
{
#if 0
    if (in_trkseg)
	fputs("</trkseg></trk>\n", fh);
#endif
    fputs("</Document></kml>\n", fh);
}
