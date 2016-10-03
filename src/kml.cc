/* kml.cc
 * Export from Aven as KML.
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
    : pj_input(NULL), pj_output(NULL), in_linestring(false)
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
    static const int default_passes[] = {
	PASG, LEGS|SURF, LABELS|ENTS|FIXES|EXPORTS, 0
    };
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
	  "<Icon><href>http://maps.google.com/mapfiles/kml/paddle/red-blank.png</href></Icon>"
	  "</IconStyle></Style>\n", fh);
    fputs("<Style id=\"exp\"><IconStyle>"
	  "<Icon><href>http://maps.google.com/mapfiles/kml/paddle/blu-blank.png</href></Icon>"
	  "</IconStyle></Style>\n", fh);
    fputs("<Style id=\"ent\"><IconStyle>"
	  "<Icon><href>http://maps.google.com/mapfiles/kml/paddle/grn-blank.png</href></Icon>"
	  "</IconStyle></Style>\n", fh);
    // FIXME: does KML allow bounds?
    // NB Lat+long bounds are not necessarily the same as the bounds in survex
    // coords translated to WGS84 lat+long...
}

void
KML::line(const img_point *p1, const img_point *p, unsigned /*flags*/, bool fPendingMove)
{
    if (fPendingMove) {
	if (!in_linestring) {
	    in_linestring = true;
	    fputs("<Placemark><MultiGeometry>\n", fh);
	} else {
	    fputs("</coordinates></LineString>\n", fh);
	}
	fputs("<LineString><altitudeMode>absolute</altitudeMode><coordinates>\n", fh);
	double X = p1->x, Y = p1->y, Z = p1->z;
	pj_transform(pj_input, pj_output, 1, 1, &X, &Y, &Z);
	X = deg(X);
	Y = deg(Y);
	// %.8f is at worst just over 1mm.
	fprintf(fh, "%.8f,%.8f,%.2f\n", X, Y, Z);
    }
    double X = p->x, Y = p->y, Z = p->z;
    pj_transform(pj_input, pj_output, 1, 1, &X, &Y, &Z);
    X = deg(X);
    Y = deg(Y);
    // %.8f is at worst just over 1mm.
    fprintf(fh, "%.8f,%.8f,%.2f\n", X, Y, Z);
}

void
KML::passage(const img_point *p, double angle, double d1, double d2)
{
    double s = sin(rad(angle));
    double c = cos(rad(angle));

    // Draw along one side and push the other onto a stack, then at the end pop
    // the stack and write out those points to give one polygon, fewer points,
    // and a smaller file.
    double x1 = p->x + c * d1;
    double y1 = p->y + s * d1;
    double z1 = p->z;
    pj_transform(pj_input, pj_output, 1, 1, &x1, &y1, &z1);
    x1 = deg(x1);
    y1 = deg(y1);

    double x2 = p->x - c * d2;
    double y2 = p->y - s * d2;
    double z2 = p->z;
    pj_transform(pj_input, pj_output, 1, 1, &x2, &y2, &z2);
    x2 = deg(x2);
    y2 = deg(y2);

    if (psg.empty()) {
	fprintf(fh, "<Placemark><name></name><Polygon><altitudeMode>absolute</altitudeMode>"
		    "<outerBoundaryIs><LinearRing><coordinates>\n");
    }
    // NB - order of vertices should be anti-clockwise in a KML file, so go
    // along the right wall now, and put the left wall points on a stack to
    // come back along at the end.
    fprintf(fh, "%.8f,%.8f,%.8f\n", x2, y2, z2);
    psg.push_back(Vector3(x1, y1, z1));
}

void
KML::tube_end()
{
    if (psg.empty()) return;

    vector<Vector3>::const_reverse_iterator i;
    for (i = psg.rbegin(); i != psg.rend(); ++i) {
	fprintf(fh, "%.8f,%.8f,%.8f\n", i->GetX(), i->GetY(), i->GetZ());
    }
    psg.clear();
    fprintf(fh, "</coordinates></LinearRing></outerBoundaryIs>"
		"</Polygon></Placemark>\n");

}

void
KML::label(const img_point *p, const char *s, bool /*fSurface*/, int type)
{
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
    if (in_linestring)
	fputs("</coordinates></LineString></MultiGeometry></Placemark>\n", fh);
    fputs("</Document></kml>\n", fh);
}
