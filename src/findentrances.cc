/** Simple converter from survex .3d files to a list of entrances in GPX format
 *
 * This program parses the survex file, creates a list of entrances and does
 * the coordinate transformations from almost arbitrary formats into WGS84
 * using the PROJ.4 library. The output is then written as a GPX file ready
 * for use in your favourite GPS software. Everything else is kept as simple
 * and minimalistic as possible.
 *
 * Compile with:
 * g++ findentrances.cc useful.o img.o cmdline.o message.o filename.o osdepend.o z_getopt.o getopt1.o -lproj -o findentrances
 *
 * Usage:
 *   findentrances -s <input.3d> -d <+proj +datum +string>
 *
 * Example for data given in BMN M31 (Totes Gebirge, Austria):
 *   findentrances -s cucc_austria.3d -d '+proj=tmerc +lat_0=0 +lon_0=13d20 +k=1 +x_0=0 +y_0=-5200000 +ellps=bessel +towgs84=577.326,90.129,463.919,5.137,1.474,5.297,2.4232' > ent.gpx
 *
 * Example for data given in british grid SD (Yorkshire):
 *   findentrances -s yorkshire/all.3d -d '+proj=tmerc +lat_0=49d +lon_0=-2d +k=0.999601 +x_0=100000 +y_0=-500000 +ellps=airy +towgs84=375,-111,431,0,0,0,0' > ent.gpx
 *
 * Example for data given as proper british grid reference:
 *   findentrances -s all.3d -d '+proj=tmerc +lat_0=49d +lon_0=-2d +k=0.999601 +x_0=400000 +y_0=-100000 +ellps=airy +towgs84=375,-111,431,0,0,0,0' > ent.gpx
 *
 */

#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <math.h>

#include <proj_api.h>

#include "message.h"
#include "cmdline.h"
#include "img.h"

using namespace std;

struct Point {
    string label;
    double x, y, z;
};

bool readSurvey(const char *filename, vector<Point> & points)
{
    img *survey = img_open_survey(filename, NULL);
    if (!survey) {
	fprintf(stderr, "failed to open survey '%s'\n", filename);
	return false;
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
	    fprintf(stderr, "error while reading file\n");
	    return false;
	}
    } while (result != img_STOP);

    img_close(survey);

    return true;
}

bool convertCoordinates(vector<Point> & points, const char *inputDatum, const char *outputDatum)
{
    projPJ pj_input, pj_output;
    if (!(pj_input = pj_init_plus(inputDatum))) {
	fprintf(stderr, "failed to initialise input coordinate system '%s'\n", inputDatum);
	return false;
    }
    if (!(pj_output = pj_init_plus(outputDatum))) {
	fprintf(stderr, "failed to initialise output coordinate system '%s'\n", outputDatum);
	return false;
    }

    for (size_t i=0; i<points.size(); ++i) {
	pj_transform(pj_input, pj_output, 1, 1, &(points[i].x), &(points[i].y), &(points[i].z));
    }

    return true;
}

struct SortPointsByLabel {
    bool operator()(const Point & a, const Point & b)
    { return a.label < b.label; }
} SortPointsByLabel;

bool sortPoints(vector<Point> & points)
{
    sort(points.begin(), points.end(), SortPointsByLabel);
    return true;
}

bool writeGPX(const vector<Point> & points, FILE *file)
{
    fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<gpx version=\"1.0\" creator=\"survex - findentrances\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.topografix.com/GPX/1/0\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd\">\r\n");
    for (size_t i=0; i<points.size(); ++i) {
	const Point & pt = points[i];
	fprintf(file, "<wpt lon=\"%.8f\" lat=\"%.8f\"><ele>%.2f</ele><name>%s</name></wpt>\r\n", pt.x*180.0/M_PI, pt.y*180.0/M_PI, pt.z, pt.label.c_str());
    }

    fprintf(file, "</gpx>\r\n");
    return true;
}


static const struct option long_opts[] = {
    {"datum", required_argument, 0, 'd'},
    {"help", no_argument, 0, HLP_HELP},
    {"version", no_argument, 0, HLP_VERSION},
    {0, 0, 0, 0}
};

static const char *short_opts = "d:";

int main(int argc, char **argv)
{
    msg_init(argv);

    const char *datum_string = NULL;
    cmdline_init(argc, argv, short_opts, long_opts, NULL, NULL, 1, 1);
    while (1) {
	int opt = cmdline_getopt();
	if (opt == EOF) break;
	else if (opt == 'd') datum_string = optarg;
    }

    if (!datum_string) {
	cerr << argv[0] << ": -d DATUM_STRING is required" << endl;
	cmdline_syntax();
	exit(1);
    }

    const char *survey_filename = argv[optind];

    vector<Point> points;
    if (!readSurvey(survey_filename, points)) return -1;
    if (!convertCoordinates(points, datum_string, "+proj=longlat +ellps=WGS84 +datum=WGS84")) return -1;
    if (!sortPoints(points)) return -1;
    if (!writeGPX(points, stdout)) return -1;

    return 0;
}

