/* findentrances.cc
 * Simple converter from survex .3d files to a list of entrances in GPX format
 *
 * Copyright (C) 2012 Olaf Kähler
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

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

#include <stdio.h>
#include <string>
#include <vector>
#include <algorithm>
#include <math.h>

#include <proj_api.h>

#include "message.h"
#include "cmdline.h"
#include "img_hosted.h"

using namespace std;

#define WGS84_DATUM_STRING "+proj=longlat +ellps=WGS84 +datum=WGS84"

struct Point {
    string label;
    double x, y, z;
};

static void
read_survey(const char *filename, vector<Point> & points)
{
    img *survey = img_open_survey(filename, NULL);
    if (!survey) {
	fatalerror(img_error2msg(img_error()), filename);
    }

    int result;
    do {
	img_point pt;
	result = img_read_item(survey, &pt);

	if (result == img_LABEL) {
	    if (survey->flags & img_SFLAG_ENTRANCE) {
		Point newPt;
		newPt.label.assign(survey->label);
		newPt.x = pt.x;
		newPt.y = pt.y;
		newPt.z = pt.z;
		points.push_back(newPt);
	    }
	} else if (result == img_BAD) {
	    img_close(survey);
	    fatalerror(img_error2msg(img_error()), filename);
	}
    } while (result != img_STOP);

    img_close(survey);
}

static void
convert_coordinates(vector<Point> & points, const char *inputDatum, const char *outputDatum)
{
    projPJ pj_input, pj_output;
    if (!(pj_input = pj_init_plus(inputDatum))) {
	fatalerror(/*Failed to initialise input coordinate system “%s”*/287, inputDatum);
    }
    if (!(pj_output = pj_init_plus(outputDatum))) {
	fatalerror(/*Failed to initialise output coordinate system “%s”*/288, outputDatum);
    }

    for (size_t i=0; i<points.size(); ++i) {
	pj_transform(pj_input, pj_output, 1, 1, &(points[i].x), &(points[i].y), &(points[i].z));
    }
}

struct SortPointsByLabel {
    bool operator()(const Point & a, const Point & b)
    { return a.label < b.label; }
} SortPointsByLabel;

static void
sort_points(vector<Point> & points)
{
    sort(points.begin(), points.end(), SortPointsByLabel);
}

static void
write_gpx(const vector<Point> & points, FILE *file)
{
    fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<gpx version=\"1.0\" creator=\"survex - findentrances\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.topografix.com/GPX/1/0\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd\">\n");
    for (size_t i=0; i<points.size(); ++i) {
	const Point & pt = points[i];
	// %.8f is at worst just over 1mm.
	fprintf(file, "<wpt lon=\"%.8f\" lat=\"%.8f\"><ele>%.2f</ele><name>%s</name></wpt>\n", pt.x*180.0/M_PI, pt.y*180.0/M_PI, pt.z, pt.label.c_str());
    }

    fprintf(file, "</gpx>\n");
}

static const struct option long_opts[] = {
    {"datum", required_argument, 0, 'd'},
    {"help", no_argument, 0, HLP_HELP},
    {"version", no_argument, 0, HLP_VERSION},
    {0, 0, 0, 0}
};

static const char *short_opts = "d:";

static struct help_msg help[] = {
/*				<-- */
   /* TRANSLATORS: The PROJ library is used to do coordinate transformations
    * (https://trac.osgeo.org/proj/) - the user passes a string to tell PROJ
    * what the input datum is. */
   {HLP_ENCODELONG(0),        /*input datum as string to pass to PROJ*/389, 0},
   {0, 0, 0}
};

int main(int argc, char **argv)
{
    msg_init(argv);

    const char *datum_string = NULL;
    cmdline_set_syntax_message(/*-d PROJ_DATUM 3D_FILE*/388, 0, NULL);
    cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 1, 1);
    while (1) {
	int opt = cmdline_getopt();
	if (opt == EOF) break;
	else if (opt == 'd') datum_string = optarg;
    }

    if (!datum_string) {
	cmdline_syntax();
	exit(1);
    }

    const char *survey_filename = argv[optind];

    vector<Point> points;
    read_survey(survey_filename, points);
    convert_coordinates(points, datum_string, WGS84_DATUM_STRING);
    sort_points(points);
    write_gpx(points, stdout);
}
