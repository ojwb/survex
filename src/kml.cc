/* kml.cc
 * Export from Aven as KML.
 */
/* Copyright (C) 2012 Olaf Kähler
 * Copyright (C) 2012,2013,2014,2015,2016,2017,2018,2019 Olly Betts
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

KML::KML(const char * input_datum, bool clamp_to_ground_)
    : clamp_to_ground(clamp_to_ground_)
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

KML::~KML()
{
    if (pj)
	proj_destroy(pj);
}

const int *
KML::passes() const
{
    static const int default_passes[] = {
	PASG, XSECT, WALL1, WALL2, LEGS|SURF, LABELS|ENTS|FIXES|EXPORTS, 0
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
KML::start_pass(int)
{
    if (in_linestring) {
	fputs("</coordinates></LineString></MultiGeometry></Placemark>\n", fh);
	in_linestring = false;
    }
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
	if (clamp_to_ground) {
	    fputs("<LineString><coordinates>\n", fh);
	} else {
	    fputs("<LineString><altitudeMode>absolute</altitudeMode><coordinates>\n", fh);
	}

	PJ_COORD coord{{p1->x, p1->y, p1->z, HUGE_VAL}};
	coord = proj_trans(pj, PJ_FWD, coord);
	if (coord.xyzt.x == HUGE_VAL ||
	    coord.xyzt.y == HUGE_VAL ||
	    coord.xyzt.z == HUGE_VAL) {
	    // FIXME report errors
	}
	// %.8f is at worst just over 1mm.
	fprintf(fh, "%.8f,%.8f,%.2f\n",
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
    fprintf(fh, "%.8f,%.8f,%.2f\n",
	    coord.xyzt.x,
	    coord.xyzt.y,
	    coord.xyzt.z);
}

void
KML::xsect(const img_point *p, double angle, double d1, double d2)
{
    if (clamp_to_ground) {
	fputs("<Placemark><name></name><LineString><coordinates>", fh);
    } else {
	fputs("<Placemark><name></name><LineString><altitudeMode>absolute</altitudeMode><coordinates>", fh);
    }

    double s = sin(rad(angle));
    double c = cos(rad(angle));

    {
	PJ_COORD coord{{p->x + s * d1, p->y + c * d1, p->z, HUGE_VAL}};
	coord = proj_trans(pj, PJ_FWD, coord);
	if (coord.xyzt.x == HUGE_VAL ||
	    coord.xyzt.y == HUGE_VAL ||
	    coord.xyzt.z == HUGE_VAL) {
	    // FIXME report errors
	}
	// %.8f is at worst just over 1mm.
	fprintf(fh, "%.8f,%.8f,%.2f ",
		coord.xyzt.x,
		coord.xyzt.y,
		coord.xyzt.z);
    }

    {
	PJ_COORD coord{{p->x - s * d2, p->y - c * d2, p->z, HUGE_VAL}};
	coord = proj_trans(pj, PJ_FWD, coord);
	if (coord.xyzt.x == HUGE_VAL ||
	    coord.xyzt.y == HUGE_VAL ||
	    coord.xyzt.z == HUGE_VAL) {
	    // FIXME report errors
	}
	// %.8f is at worst just over 1mm.
	fprintf(fh, "%.8f,%.8f,%.2f\n",
		coord.xyzt.x,
		coord.xyzt.y,
		coord.xyzt.z);
    }

    fputs("</coordinates></LineString></Placemark>\n", fh);
}

void
KML::wall(const img_point *p, double angle, double d)
{
    if (!in_wall) {
	if (clamp_to_ground) {
	    fputs("<Placemark><name></name><LineString><coordinates>", fh);
	} else {
	    fputs("<Placemark><name></name><LineString><altitudeMode>absolute</altitudeMode><coordinates>", fh);
	}
	in_wall = true;
    }

    double s = sin(rad(angle));
    double c = cos(rad(angle));

    PJ_COORD coord{{p->x + s * d, p->y + c * d, p->z, HUGE_VAL}};
    coord = proj_trans(pj, PJ_FWD, coord);
    if (coord.xyzt.x == HUGE_VAL ||
	coord.xyzt.y == HUGE_VAL ||
	coord.xyzt.z == HUGE_VAL) {
	// FIXME report errors
    }
    // %.8f is at worst just over 1mm.
    fprintf(fh, "%.8f,%.8f,%.2f\n",
	    coord.xyzt.x,
	    coord.xyzt.y,
	    coord.xyzt.z);
}

void
KML::passage(const img_point *p, double angle, double d1, double d2)
{
    double s = sin(rad(angle));
    double c = cos(rad(angle));

    PJ_COORD coord1{{p->x + s * d1, p->y + c * d1, p->z, HUGE_VAL}};
    coord1 = proj_trans(pj, PJ_FWD, coord1);
    if (coord1.xyzt.x == HUGE_VAL ||
	coord1.xyzt.y == HUGE_VAL ||
	coord1.xyzt.z == HUGE_VAL) {
	// FIXME report errors
    }
    double x1 = coord1.xyzt.x;
    double y1 = coord1.xyzt.y;
    double z1 = coord1.xyzt.z;

    PJ_COORD coord2{{p->x - s * d2, p->y - c * d2, p->z, HUGE_VAL}};
    coord2 = proj_trans(pj, PJ_FWD, coord2);
    if (coord2.xyzt.x == HUGE_VAL ||
	coord2.xyzt.y == HUGE_VAL ||
	coord2.xyzt.z == HUGE_VAL) {
	// FIXME report errors
    }
    double x2 = coord2.xyzt.x;
    double y2 = coord2.xyzt.y;
    double z2 = coord2.xyzt.z;

    // Define each passage as a multigeometry comprising of one quadrilateral
    // per section.  This prevents invalid geometry (such as self-intersecting
    // polygons) being created.

    if (!in_passage){
	in_passage = true;
	fputs("<Placemark><name></name><MultiGeometry>\n", fh);
    } else {
	if (clamp_to_ground) {
	    fputs("<Polygon>"
		  "<outerBoundaryIs><LinearRing><coordinates>\n", fh);
	} else {
	    fputs("<Polygon><altitudeMode>absolute</altitudeMode>"
		  "<outerBoundaryIs><LinearRing><coordinates>\n", fh);
	}

	// Draw anti-clockwise around the ring.
	fprintf(fh, "%.8f,%.8f,%.2f\n", v2.GetX(), v2.GetY(), v2.GetZ());
	fprintf(fh, "%.8f,%.8f,%.2f\n", v1.GetX(), v1.GetY(), v1.GetZ());

	fprintf(fh, "%.8f,%.8f,%.2f\n", x1, y1, z1);
	fprintf(fh, "%.8f,%.8f,%.2f\n", x2, y2, z2);

	// Close the ring.
	fprintf(fh, "%.8f,%.8f,%.2f\n", v2.GetX(), v2.GetY(), v2.GetZ());

	fputs("</coordinates></LinearRing></outerBoundaryIs>"
	      "</Polygon>\n", fh);
    }

    v2 = Vector3(x2, y2, z2);
    v1 = Vector3(x1, y1, z1);
}

void
KML::tube_end()
{
    if (in_passage){
	fputs("</MultiGeometry></Placemark>\n", fh);
	in_passage = false;
    }
    if (in_wall) {
	fputs("</coordinates></LineString></Placemark>\n", fh);
	in_wall = false;
    }
}

void
KML::label(const img_point *p, const wxString& str, bool /*fSurface*/, int type)
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
    fprintf(fh, "<Placemark><Point><coordinates>%.8f,%.8f,%.2f</coordinates></Point><name>",
	    coord.xyzt.x,
	    coord.xyzt.y,
	    coord.xyzt.z);
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
