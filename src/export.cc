/* export.cc
 * Export to GIS formats, CAD formats, and other formats.
 */

/* Copyright (C) 1994-2025 Olly Betts
 * Copyright (C) 2004 John Pybus (SVG Output code)
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

#include <config.h>

#include "export.h"

#include "wx.h"
#include <wx/utils.h>
#include "export3d.h"
#include "exportfilter.h"
#include "gdalexport.h"
#include "gpx.h"
#include "hpgl.h"
#include "json.h"
#include "kml.h"
#include "mainfrm.h"
#include "osalloc.h"
#include "pos.h"

#include <float.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <utility>
#include <vector>

#include "cmdline.h"
#include "debug.h"
#include "filename.h"
#include "hash.h"
#include "img_for_survex.h"
#include "message.h"
#include "useful.h"

#define SQRT_2		1.41421356237309504880168872420969

// Order here needs to match order of export_format enum in export.h.

const format_info export_format_info[] = {
    { ".3d", /*Survex 3d files*/207,
      LABELS|LEGS|SURF|SPLAYS|ENTS|FIXES|EXPORTS, /* FIXME: expand... */
      LABELS|LEGS|SURF|SPLAYS|ENTS|FIXES|EXPORTS },
    { ".csv", /*CSV files*/101,
      LABELS|ENTS|FIXES|EXPORTS,
      LABELS },
    { ".dxf", /*DXF files*/411,
      LABELS|LEGS|SURF|SPLAYS|STNS|PASG|XSECT|WALLS|MARKER_SIZE|TEXT_HEIGHT|GRID|FULL_COORDS|ORIENTABLE,
      LABELS|LEGS|STNS },
    { ".eps", /*EPS files*/412,
      LABELS|LEGS|SURF|SPLAYS|STNS|PASG|XSECT|WALLS|ORIENTABLE,
      LABELS|LEGS|STNS },
    { ".gpx", /*GPX files*/413,
      LABELS|LEGS|SURF|SPLAYS|ENTS|FIXES|EXPORTS|PROJ,
      LABELS },
    /* TRANSLATORS: Here "plotter" refers to a machine which draws a printout
     * on a (usually large) sheet of paper using a pen mounted in a motorised
     * mechanism. */
    { ".hpgl", /*HPGL for plotters*/414,
      LABELS|LEGS|SURF|SPLAYS|STNS|CENTRED|SCALE|ORIENTABLE,
      LABELS|LEGS|STNS },
    { ".json", /*JSON files*/445,
      LEGS|SURF|SPLAYS|CENTRED,
      LEGS },
    { ".kml", /*KML files*/444,
      LABELS|LEGS|SURF|SPLAYS|PASG|XSECT|WALLS|ENTS|FIXES|EXPORTS|PROJ|CLAMP_TO_GROUND,
      LABELS|LEGS },
    /* TRANSLATORS: "Compass" and "Carto" are the names of software packages,
     * so should not be translated:
     * https://www.fountainware.com/compass/ */
    { ".plt", /*Compass PLT for use with Carto*/415,
      LABELS|LEGS|SURF|SPLAYS|ORIENTABLE,
      LABELS|LEGS },
    /* TRANSLATORS: Survex is the name of the software, and "pos" refers to a
     * file extension, so neither should be translated. */
    { ".pos", /*Survex pos files*/166,
      LABELS|ENTS|FIXES|EXPORTS,
      LABELS },
    { ".svg", /*SVG files*/417,
      LABELS|LEGS|SURF|SPLAYS|STNS|PASG|XSECT|WALLS|MARKER_SIZE|TEXT_HEIGHT|SCALE|ORIENTABLE,
      LABELS|LEGS|STNS },
    { ".shp", /*Shapefiles (lines)*/523,
      LEGS|SURF|SPLAYS,
      LEGS },
    { ".shp", /*Shapefiles (points)*/524,
      LABELS|ENTS|FIXES|EXPORTS|STNS,
      LABELS|STNS },
};

static_assert(sizeof(export_format_info) == FMT_MAX_PLUS_ONE_ * sizeof(export_format_info[0]),
	      "export_format_info[] matches enum export_format");

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

// Used by SVG.
static const char *layer_name(int mask) {
    switch (mask) {
	case LEGS: case LEGS|SURF:
	    return "Legs";
	case SURF:
	    return "Surface";
	case STNS:
	    return "Stations";
	case LABELS:
	    return "Labels";
	case XSECT:
	    return "Cross-sections";
	case WALL1: case WALL2: case WALLS:
	    return "Walls";
	case PASG:
	    return "Passages";
    }
    return "";
}

static double marker_size; /* for station markers */
static double grid; /* grid spacing (or 0 for no grid) */

const int *
ExportFilter::passes() const
{
    static const int default_passes[] = { LEGS|SURF|STNS|LABELS, 0 };
    return default_passes;
}

class DXF : public ExportFilter {
    const char * to_close = nullptr;
    /* for station labels */
    double text_height;
    char pending[1024];

  public:
    explicit DXF(double text_height_)
	: text_height(text_height_) { pending[0] = '\0'; }
    const int * passes() const override;
    bool fopen(const wxString& fnm_out) override;
    void header(const char *, time_t,
		double min_x, double min_y, double min_z,
		double max_x, double max_y, double max_z) override;
    void line(const img_point *, const img_point *, unsigned, bool) override;
    void label(const img_point *, const wxString&, int, int) override;
    void cross(const img_point *, const wxString&, int) override;
    void xsect(const img_point *, double, double, double) override;
    void wall(const img_point *, double, double) override;
    void passage(const img_point *, double, double, double) override;
    void tube_end() override;
    void footer() override;
};

const int *
DXF::passes() const
{
    static const int dxf_passes[] = {
	PASG, XSECT, WALL1, WALL2, LEGS|SURF|STNS|LABELS, 0
    };
    return dxf_passes;
}

bool
DXF::fopen(const wxString& fnm_out)
{
    // DXF gets written as text rather than binary.
    fh = wxFopen(fnm_out.fn_str(), wxT("w"));
    return (fh != NULL);
}

void
DXF::header(const char *, time_t,
	    double min_x, double min_y, double min_z,
	    double max_x, double max_y, double max_z)
{
   fprintf(fh, "0\nSECTION\n"
	       "2\nHEADER\n");
   fprintf(fh, "9\n$EXTMIN\n"); /* lower left corner of drawing */
   fprintf(fh, "10\n%#-.2f\n", min_x); /* x */
   fprintf(fh, "20\n%#-.2f\n", min_y); /* y */
   fprintf(fh, "30\n%#-.2f\n", min_z); /* min z */
   fprintf(fh, "9\n$EXTMAX\n"); /* upper right corner of drawing */
   fprintf(fh, "10\n%#-.2f\n", max_x); /* x */
   fprintf(fh, "20\n%#-.2f\n", max_y); /* y */
   fprintf(fh, "30\n%#-.2f\n", max_z); /* max z */
   fprintf(fh, "9\n$PDMODE\n70\n3\n"); /* marker style as CROSS */
   fprintf(fh, "9\n$PDSIZE\n40\n%6.2f\n", marker_size); /* marker size */
   fprintf(fh, "0\nENDSEC\n");

   fprintf(fh, "0\nSECTION\n"
	       "2\nTABLES\n");
   fprintf(fh, "0\nTABLE\n" /* Define CONTINUOUS and DASHED line types. */
	       "2\nLTYPE\n"
	       "70\n10\n"
	       "0\nLTYPE\n"
	       "2\nCONTINUOUS\n"
	       "70\n64\n"
	       "3\nContinuous\n"
	       "72\n65\n"
	       "73\n0\n"
	       "40\n0.0\n"
	       "0\nLTYPE\n"
	       "2\nDASHED\n"
	       "70\n64\n"
	       "3\nDashed\n"
	       "72\n65\n"
	       "73\n2\n"
	       "40\n2.5\n"
	       "49\n1.25\n"
	       "49\n-1.25\n"
	       "0\nLTYPE\n" /* define DOT line type */
	       "2\nDOT\n"
	       "70\n64\n"
	       "3\nDotted\n"
	       "72\n65\n"
	       "73\n2\n"
	       "40\n1\n"
	       "49\n0\n"
	       "49\n1\n"
	       "0\nENDTAB\n");
   fprintf(fh, "0\nTABLE\n"
	       "2\nLAYER\n");
   fprintf(fh, "70\n10\n"); /* max # off layers in this DXF file : 10 */
   /* First Layer: CentreLine */
   fprintf(fh, "0\nLAYER\n2\nCentreLine\n");
   fprintf(fh, "70\n64\n"); /* shows layer is referenced by entities */
   fprintf(fh, "62\n5\n"); /* color: kept the same used by SpeleoGen */
   fprintf(fh, "6\nCONTINUOUS\n"); /* linetype */
   /* Next Layer: Stations */
   fprintf(fh, "0\nLAYER\n2\nStations\n");
   fprintf(fh, "70\n64\n"); /* shows layer is referenced by entities */
   fprintf(fh, "62\n7\n"); /* color: kept the same used by SpeleoGen */
   fprintf(fh, "6\nCONTINUOUS\n"); /* linetype */
   /* Next Layer: Labels */
   fprintf(fh, "0\nLAYER\n2\nLabels\n");
   fprintf(fh, "70\n64\n"); /* shows layer is referenced by entities */
   fprintf(fh, "62\n7\n"); /* color: kept the same used by SpeleoGen */
   fprintf(fh, "6\nCONTINUOUS\n"); /* linetype */
   /* Next Layer: Surface */
   fprintf(fh, "0\nLAYER\n2\nSurface\n");
   fprintf(fh, "70\n64\n"); /* shows layer is referenced by entities */
   fprintf(fh, "62\n5\n"); /* color */
   fprintf(fh, "6\nDASHED\n"); /* linetype */
   /* Next Layer: SurfaceStations */
   fprintf(fh, "0\nLAYER\n2\nSurfaceStations\n");
   fprintf(fh, "70\n64\n"); /* shows layer is referenced by entities */
   fprintf(fh, "62\n7\n"); /* color */
   fprintf(fh, "6\nCONTINUOUS\n"); /* linetype */
   /* Next Layer: SurfaceLabels */
   fprintf(fh, "0\nLAYER\n2\nSurfaceLabels\n");
   fprintf(fh, "70\n64\n"); /* shows layer is referenced by entities */
   fprintf(fh, "62\n7\n"); /* color */
   fprintf(fh, "6\nCONTINUOUS\n"); /* linetype */
   /* Next Layer: Splays */
   fprintf(fh, "0\nLAYER\n2\nSplays\n");
   fprintf(fh, "70\n64\n"); /* shows layer is referenced by entities */
   fprintf(fh, "62\n5\n"); /* color */
   fprintf(fh, "6\nDOT\n"); /* linetype;  */
   if (grid > 0) {
      /* Next Layer: Grid */
      fprintf(fh, "0\nLAYER\n2\nGrid\n");
      fprintf(fh, "70\n64\n"); /* shows layer is referenced by entities */
      fprintf(fh, "62\n7\n"); /* color: kept the same used by SpeleoGen */
      fprintf(fh, "6\nCONTINUOUS\n"); /* linetype */
   }
   fprintf(fh, "0\nENDTAB\n"
	       "0\nENDSEC\n");

   fprintf(fh, "0\nSECTION\n"
	       "2\nENTITIES\n");

   if (grid > 0) {
      double x, y;
      x = floor(min_x / grid) * grid + grid;
      y = floor(min_y / grid) * grid + grid;
      while (x < max_x) {
	 /* horizontal line */
	 fprintf(fh, "0\nLINE\n");
	 fprintf(fh, "8\nGrid\n"); /* Layer */
	 fprintf(fh, "10\n%6.2f\n", x);
	 fprintf(fh, "20\n%6.2f\n", min_y);
	 fprintf(fh, "30\n0\n");
	 fprintf(fh, "11\n%6.2f\n", x);
	 fprintf(fh, "21\n%6.2f\n", max_y);
	 fprintf(fh, "31\n0\n");
	 x += grid;
      }
      while (y < max_y) {
	 /* vertical line */
	 fprintf(fh, "0\nLINE\n");
	 fprintf(fh, "8\nGrid\n"); /* Layer */
	 fprintf(fh, "10\n%6.2f\n", min_x);
	 fprintf(fh, "20\n%6.2f\n", y);
	 fprintf(fh, "30\n0\n");
	 fprintf(fh, "11\n%6.2f\n", max_x);
	 fprintf(fh, "21\n%6.2f\n", y);
	 fprintf(fh, "31\n0\n");
	 y += grid;
      }
   }
}

void
DXF::line(const img_point *p1, const img_point *p, unsigned flags, bool fPendingMove)
{
   bool fSurface = (flags & SURF);
   bool fSplay = (flags & SPLAYS);
   (void)fPendingMove; /* unused */
   fprintf(fh, "0\nLINE\n");
   if (fSurface) { /* select layer */
      fprintf(fh, "8\nSurface\n" );
   } else if (fSplay) {
      fprintf(fh, "8\nSplays\n");
   } else {
      fprintf(fh, "8\nCentreLine\n");
   }
   fprintf(fh, "10\n%6.2f\n", p1->x);
   fprintf(fh, "20\n%6.2f\n", p1->y);
   fprintf(fh, "30\n%6.2f\n", p1->z);
   fprintf(fh, "11\n%6.2f\n", p->x);
   fprintf(fh, "21\n%6.2f\n", p->y);
   fprintf(fh, "31\n%6.2f\n", p->z);
}

void
DXF::label(const img_point *p, const wxString& str, int sflags, int)
{
   // Use !UNDERGROUND as the criterion - we want stations where a surface and
   // underground survey meet to be in the underground layer.
   bool surface = !(sflags & img_SFLAG_UNDERGROUND);
   /* write station label to dxf file */
   const char* s = str.utf8_str();
   fprintf(fh, "0\nTEXT\n");
   fprintf(fh, surface ? "8\nSurfaceLabels\n" : "8\nLabels\n"); /* Layer */
   fprintf(fh, "10\n%6.2f\n", p->x);
   fprintf(fh, "20\n%6.2f\n", p->y);
   fprintf(fh, "30\n%6.2f\n", p->z);
   fprintf(fh, "40\n%6.2f\n", text_height);
   fprintf(fh, "1\n%s\n", s);
}

void
DXF::cross(const img_point *p, const wxString&, int sflags)
{
   // Use !UNDERGROUND as the criterion - we want stations where a surface and
   // underground survey meet to be in the underground layer.
   bool surface = !(sflags & img_SFLAG_UNDERGROUND);
   /* write station marker to dxf file */
   fprintf(fh, "0\nPOINT\n");
   fprintf(fh, surface ? "8\nSurfaceStations\n" : "8\nStations\n"); /* Layer */
   fprintf(fh, "10\n%6.2f\n", p->x);
   fprintf(fh, "20\n%6.2f\n", p->y);
   fprintf(fh, "30\n%6.2f\n", p->z);
}

void
DXF::xsect(const img_point *p, double angle, double d1, double d2)
{
   double s = sin(rad(angle));
   double c = cos(rad(angle));
   fprintf(fh, "0\nLINE\n");
   fprintf(fh, "8\nCross-sections\n"); /* Layer */
   fprintf(fh, "10\n%6.2f\n", p->x + s * d1);
   fprintf(fh, "20\n%6.2f\n", p->y + c * d1);
   fprintf(fh, "30\n%6.2f\n", p->z);
   fprintf(fh, "11\n%6.2f\n", p->x - s * d2);
   fprintf(fh, "21\n%6.2f\n", p->y - c * d2);
   fprintf(fh, "31\n%6.2f\n", p->z);
}

void
DXF::wall(const img_point *p, double angle, double d)
{
   if (!to_close) {
       fprintf(fh, "0\nPOLYLINE\n");
       fprintf(fh, "8\nWalls\n"); /* Layer */
       fprintf(fh, "70\n0\n"); /* bit 0 == 0 => Open polyline */
       to_close = "0\nSEQEND\n";
   }
   double s = sin(rad(angle));
   double c = cos(rad(angle));
   fprintf(fh, "0\nVERTEX\n");
   fprintf(fh, "8\nWalls\n"); /* Layer */
   fprintf(fh, "10\n%6.2f\n", p->x + s * d);
   fprintf(fh, "20\n%6.2f\n", p->y + c * d);
   fprintf(fh, "30\n%6.2f\n", p->z);
}

void
DXF::passage(const img_point *p, double angle, double d1, double d2)
{
   fprintf(fh, "0\nSOLID\n");
   fprintf(fh, "8\nPassages\n"); /* Layer */
   double s = sin(rad(angle));
   double c = cos(rad(angle));
   double x1 = p->x + s * d1;
   double y1 = p->y + c * d1;
   double x2 = p->x - s * d2;
   double y2 = p->y - c * d2;
   if (*pending) {
       fputs(pending, fh);
       fprintf(fh, "12\n%6.2f\n22\n%6.2f\n32\n%6.2f\n"
		   "13\n%6.2f\n23\n%6.2f\n33\n%6.2f\n",
		   x1, y1, p->z,
		   x2, y2, p->z);
   }
   snprintf(pending, sizeof(pending),
	    "10\n%6.2f\n20\n%6.2f\n30\n%6.2f\n"
	    "11\n%6.2f\n21\n%6.2f\n31\n%6.2f\n",
	    x1, y1, p->z,
	    x2, y2, p->z);
}

void
DXF::tube_end()
{
   *pending = '\0';
   if (to_close) {
      fputs(to_close, fh);
      to_close = NULL;
   }
}

void
DXF::footer()
{
   fprintf(fh, "000\nENDSEC\n");
   fprintf(fh, "000\nEOF\n");
}

typedef struct point {
   img_point p;
   const char *label;
   struct point *next;
} point;

#define HTAB_SIZE 0x2000

static point **htab;

static void
set_name(const img_point *p, const char *s)
{
   point *pt;
   union {
      char data[sizeof(int) * 3];
      int x[3];
   } u;

   u.x[0] = (int)(p->x * 100);
   u.x[1] = (int)(p->y * 100);
   u.x[2] = (int)(p->z * 100);
   unsigned hash = (hash_data(u.data, sizeof(int) * 3) & (HTAB_SIZE - 1));
   for (pt = htab[hash]; pt; pt = pt->next) {
      if (pt->p.x == p->x && pt->p.y == p->y && pt->p.z == p->z) {
	 /* already got name for these coordinates */
	 /* FIXME: what about multiple names for the same station? */
	 return;
      }
   }

   pt = osnew(point);
   pt->label = osstrdup(s);
   pt->p = *p;
   pt->next = htab[hash];
   htab[hash] = pt;

   return;
}

static const char *
find_name(const img_point *p)
{
   point *pt;
   union {
      char data[sizeof(int) * 3];
      int x[3];
   } u;
   wxASSERT(p);

   u.x[0] = (int)(p->x * 100);
   u.x[1] = (int)(p->y * 100);
   u.x[2] = (int)(p->z * 100);
   unsigned hash = (hash_data(u.data, sizeof(int) * 3) & (HTAB_SIZE - 1));
   for (pt = htab[hash]; pt; pt = pt->next) {
      if (pt->p.x == p->x && pt->p.y == p->y && pt->p.z == p->z)
	 return pt->label;
   }
   return "?";
}

class SVG : public ExportFilter {
    const char * to_close = nullptr;
    bool close_g = false;
    double factor;
    /* for station labels */
    double text_height;
    char pending[1024];

  public:
    SVG(double scale, double text_height_)
	: factor(1000.0 / scale),
	  text_height(text_height_) {
	pending[0] = '\0';
    }
    const int * passes() const override;
    void header(const char *, time_t,
		double min_x, double min_y, double min_z,
		double max_x, double max_y, double max_z) override;
    void start_pass(int layer) override;
    void line(const img_point *, const img_point *, unsigned, bool) override;
    void label(const img_point *, const wxString&, int, int) override;
    void cross(const img_point *, const wxString&, int) override;
    void xsect(const img_point *, double, double, double) override;
    void wall(const img_point *, double, double) override;
    void passage(const img_point *, double, double, double) override;
    void tube_end() override;
    void footer() override;
};

const int *
SVG::passes() const
{
    static const int svg_passes[] = {
	PASG, LEGS|SURF, XSECT, WALL1, WALL2, LABELS, STNS, 0
    };
    return svg_passes;
}

void
SVG::header(const char * title, time_t,
	    double min_x, double min_y, double /*min_z*/,
	    double max_x, double max_y, double /*max_z*/)
{
   const char *unit = "mm";
   const double SVG_MARGIN = 5.0; // In units of "unit".
   htab = (point **)osmalloc(HTAB_SIZE * ossizeof(point *));
   for (size_t i = 0; i < HTAB_SIZE; ++i) htab[i] = NULL;
   fprintf(fh, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
   double width = (max_x - min_x) * factor + SVG_MARGIN * 2;
   double height = (max_y - min_y) * factor + SVG_MARGIN * 2;
   fprintf(fh, "<svg version=\"1.1\" baseProfile=\"full\"\n"
	       "xmlns=\"http://www.w3.org/2000/svg\"\n"
	       "xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
	       "xmlns:ev=\"http://www.w3.org/2001/xml-events\"\n"
	       "width=\"%.3f%s\" height=\"%.3f%s\"\n"
	       "viewBox=\"0 0 %0.3f %0.3f\">\n",
	   width, unit, height, unit, width, height);
   if (title && title[0]) {
       fputs("<title>", fh);
       html_escape(fh, title);
       fputs("</title>\n", fh);
   }
   fprintf(fh, "<g transform=\"translate(%.3f %.3f)\">\n",
	   SVG_MARGIN - min_x * factor, SVG_MARGIN + max_y * factor);
   to_close = NULL;
   close_g = false;
}

void
SVG::start_pass(int layer)
{
   if (to_close) {
      fputs(to_close, fh);
      to_close = NULL;
   }
   if (close_g) {
      fprintf(fh, "</g>\n");
   }
   fprintf(fh, "<g id=\"%s\"", layer_name(layer));
   if (layer & LEGS)
      fprintf(fh, " stroke=\"black\" fill=\"none\" stroke-width=\"0.4px\"");
   else if (layer & STNS)
      fprintf(fh, " stroke=\"black\" fill=\"none\" stroke-width=\"0.05px\"");
   else if (layer & LABELS)
      fprintf(fh, " font-size=\"%.3fem\"", text_height);
   else if (layer & XSECT)
      fprintf(fh, " stroke=\"grey\" fill=\"none\" stroke-width=\"0.1px\"");
   else if (layer & WALLS)
      fprintf(fh, " stroke=\"black\" fill=\"none\" stroke-width=\"0.1px\"");
   else if (layer & PASG)
      fprintf(fh, " stroke=\"none\" fill=\"peru\"");
   fprintf(fh, ">\n");

   close_g = true;
}

void
SVG::line(const img_point *p1, const img_point *p, unsigned flags, bool fPendingMove)
{
   bool splay = (flags & SPLAYS);
   if (fPendingMove) {
       if (to_close) {
	   fputs(to_close, fh);
       }
       fprintf(fh, "<path ");
       if (splay) fprintf(fh, "stroke=\"grey\" stroke-width=\"0.1px\" ");
       fprintf(fh, "d=\"M%.3f %.3f", p1->x * factor, p1->y * -factor);
   }
   fprintf(fh, "L%.3f %.3f", p->x * factor, p->y * -factor);
   to_close = "\"/>\n";
}

void
SVG::label(const img_point *p, const wxString& str, int sflags, int)
{
   const char* s = str.utf8_str();
   (void)sflags; /* unused */
   fprintf(fh, "<text transform=\"translate(%.3f %.3f)\">",
	   p->x * factor, p->y * -factor);
   html_escape(fh, s);
   fputs("</text>\n", fh);
   set_name(p, s);
}

void
SVG::cross(const img_point *p, const wxString& str, int sflags)
{
   const char* s = str.utf8_str();
   (void)sflags; /* unused */
   fprintf(fh, "<circle id=\"%s\" cx=\"%.3f\" cy=\"%.3f\" r=\"%.3f\"/>\n",
	   s, p->x * factor, p->y * -factor, marker_size * SQRT_2);
   fprintf(fh, "<path d=\"M%.3f %.3fL%.3f %.3fM%.3f %.3fL%.3f %.3f\"/>\n",
	   p->x * factor - marker_size, p->y * -factor - marker_size,
	   p->x * factor + marker_size, p->y * -factor + marker_size,
	   p->x * factor + marker_size, p->y * -factor - marker_size,
	   p->x * factor - marker_size, p->y * -factor + marker_size);
}

void
SVG::xsect(const img_point *p, double angle, double d1, double d2)
{
   double s = sin(rad(angle));
   double c = cos(rad(angle));
   fprintf(fh, "<path d=\"M%.3f %.3fL%.3f %.3f\"/>\n",
	   (p->x + s * d1) * factor, (p->y + c * d1) * -factor,
	   (p->x - s * d2) * factor, (p->y - c * d2) * -factor);
}

void
SVG::wall(const img_point *p, double angle, double d)
{
   if (!to_close) {
       fprintf(fh, "<path d=\"M");
       to_close = "\"/>\n";
   } else {
       fprintf(fh, "L");
   }
   double s = sin(rad(angle));
   double c = cos(rad(angle));
   fprintf(fh, "%.3f %.3f", (p->x + s * d) * factor, (p->y + c * d) * -factor);
}

void
SVG::passage(const img_point *p, double angle, double d1, double d2)
{
   double s = sin(rad(angle));
   double c = cos(rad(angle));
   double x1 = (p->x + s * d1) * factor;
   double y1 = (p->y + c * d1) * -factor;
   double x2 = (p->x - s * d2) * factor;
   double y2 = (p->y - c * d2) * -factor;
   if (*pending) {
       fputs(pending, fh);
       fprintf(fh, "L%.3f %.3fL%.3f %.3fZ\"/>\n", x2, y2, x1, y1);
   }
   snprintf(pending, sizeof(pending),
	    "<path d=\"M%.3f %.3fL%.3f %.3f", x1, y1, x2, y2);
}

void
SVG::tube_end()
{
   *pending = '\0';
   if (to_close) {
      fputs(to_close, fh);
      to_close = NULL;
   }
}

void
SVG::footer()
{
   if (to_close) {
      fputs(to_close, fh);
      to_close = NULL;
   }
   if (close_g) {
      fprintf(fh, "</g>\n");
      close_g = false;
   }
   fprintf(fh, "</g>\n</svg>\n");
}

class PLT : public ExportFilter {
    string escaped;

    const char * find_name_plt(const img_point *p);

    double min_N, max_N, min_E, max_E, min_A, max_A;

    unsigned anon_counter = 0;

  public:
    PLT() { }
    const int * passes() const override;
    void header(const char *, time_t,
		double min_x, double min_y, double min_z,
		double max_x, double max_y, double max_z) override;
    void line(const img_point *, const img_point *, unsigned, bool) override;
    void label(const img_point *, const wxString&, int, int) override;
    void footer() override;
};

const int *
PLT::passes() const
{
    static const int plt_passes[] = { LABELS, LEGS|SURF, 0 };
    return plt_passes;
}

void
PLT::header(const char *title, time_t,
	    double min_x, double min_y, double min_z,
	    double max_x, double max_y, double max_z)
{
   // FIXME: allow survey to be set from aven somehow!
   const char *survey = NULL;
   htab = (point **)osmalloc(HTAB_SIZE * ossizeof(point *));
   for (size_t i = 0; i < HTAB_SIZE; ++i) htab[i] = NULL;
   /* Survex is E, N, Alt - PLT file is N, E, Alt */
   min_N = min_y / METRES_PER_FOOT;
   max_N = max_y / METRES_PER_FOOT;
   min_E = min_x / METRES_PER_FOOT;
   max_E = max_x / METRES_PER_FOOT;
   min_A = min_z / METRES_PER_FOOT;
   max_A = max_z / METRES_PER_FOOT;
   fprintf(fh, "Z %.3f %.3f %.3f %.3f %.3f %.3f\r\n",
	   min_N, max_N, min_E, max_E, min_A, max_A);
   fprintf(fh, "N%s D 1 1 1 C%s\r\n", survey ? survey : "X",
	   (title && title[0]) ? title : "X");
}

void
PLT::line(const img_point *p1, const img_point *p, unsigned flags, bool fPendingMove)
{
   if (fPendingMove) {
       /* Survex is E, N, Alt - PLT file is N, E, Alt */
       fprintf(fh, "M %.3f %.3f %.3f ",
	       p1->y / METRES_PER_FOOT, p1->x / METRES_PER_FOOT, p1->z / METRES_PER_FOOT);
       /* dummy passage dimensions are required to avoid compass bug */
       fprintf(fh, "S%s P -9 -9 -9 -9\r\n", find_name_plt(p1));
   }
   /* Survex is E, N, Alt - PLT file is N, E, Alt */
   fprintf(fh, "D %.3f %.3f %.3f ",
	   p->y / METRES_PER_FOOT, p->x / METRES_PER_FOOT, p->z / METRES_PER_FOOT);
   /* dummy passage dimensions are required to avoid compass bug */
   fprintf(fh, "S%s P -9 -9 -9 -9", find_name_plt(p));
   if (flags & (MASK_)) {
       fprintf(fh, " F");
       if (flags & img_FLAG_DUPLICATE) PUTC('L', fh);
       if (flags & img_FLAG_SURFACE) PUTC('P', fh);
       if (flags & img_FLAG_SPLAY) PUTC('S', fh);
   }
   fprintf(fh, "\r\n");
}

const char *
PLT::find_name_plt(const img_point *p)
{
    const char * s = find_name(p);
    escaped.resize(0);
    if (*s == '\0') {
	// Anonymous station - number sequentially using a counter.  We start
	// the name with "%:" since we escape any % in a real station name
	// below, but only insert % followed by two hex digits.
	char buf[32];
	snprintf(buf, sizeof(buf), "%%:%u", ++anon_counter);
	escaped = buf;
	return escaped.c_str();
    }

    // PLT format can't handle spaces or control characters, so escape them
    // like in URLs (an arbitrary choice of escaping, but at least a familiar
    // one and % isn't likely to occur in station names).
    const char * q;
    for (q = s; *q; ++q) {
	unsigned char ch = *q;
	if (ch <= ' ' || ch == '%') {
	    escaped.append(s, q - s);
	    escaped += '%';
	    escaped += "0123456789abcdef"[ch >> 4];
	    escaped += "0123456789abcdef"[ch & 0x0f];
	    s = q + 1;
	}
    }
    if (!escaped.empty()) {
	escaped.append(s, q - s);
	return escaped.c_str();
    }
    return s;
}

void
PLT::label(const img_point *p, const wxString& str, int sflags, int)
{
   const char* s = str.utf8_str();
   (void)sflags; /* unused */
   set_name(p, s);
}

void
PLT::footer(void)
{
   /* Survex is E, N, Alt - PLT file is N, E, Alt */
   fprintf(fh, "X %.3f %.3f %.3f %.3f %.3f %.3f\r\n",
	   min_N, max_N, min_E, max_E, min_A, max_A);
   /* Yucky DOS "end of textfile" marker */
   PUTC('\x1a', fh);
}

class EPS : public ExportFilter {
    double factor;
    bool first;
    vector<pair<double, double>> psg;
  public:
    explicit EPS(double scale)
	: factor(POINTS_PER_MM * 1000.0 / scale) { }
    const int * passes() const override;
    void header(const char *, time_t,
		double min_x, double min_y, double min_z,
		double max_x, double max_y, double max_z) override;
    void start_pass(int layer) override;
    void line(const img_point *, const img_point *, unsigned, bool) override;
    void label(const img_point *, const wxString&, int, int) override;
    void cross(const img_point *, const wxString&, int) override;
    void xsect(const img_point *, double, double, double) override;
    void wall(const img_point *, double, double) override;
    void passage(const img_point *, double, double, double) override;
    void tube_end() override;
    void footer() override;
};

const int *
EPS::passes() const
{
    static const int eps_passes[] = {
	PASG, XSECT, WALL1, WALL2, LEGS|SURF|STNS|LABELS, 0
    };
    return eps_passes;
}

void
EPS::header(const char *title, time_t,
	    double min_x, double min_y, double /*min_z*/,
	    double max_x, double max_y, double /*max_z*/)
{
   const char * fontname_labels = "helvetica"; // FIXME
   int fontsize_labels = 10; // FIXME
   fputs("%!PS-Adobe-2.0 EPSF-1.2\n", fh);
   fputs("%%Creator: Survex " VERSION " EPS Export Filter\n", fh);

   if (title && title[0])
       fprintf(fh, "%%%%Title: %s\n", title);

   char buf[64];
   time_t now = time(NULL);
   if (strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z\n", localtime(&now))) {
      fputs("%%CreationDate: ", fh);
      fputs(buf, fh);
   }

   string name;
   if (name.empty()) {
       name = ::wxGetUserName().mb_str();
       if (name.empty()) {
	   name = ::wxGetUserId().mb_str();
       }
   }
   if (!name.empty()) {
       fprintf(fh, "%%%%For: %s\n", name.c_str());
   }

   fprintf(fh, "%%%%BoundingBox: %d %d %d %d\n",
	   int(floor(min_x * factor)), int(floor(min_y * factor)),
	   int(ceil(max_x * factor)), int(ceil(max_y * factor)));
   fprintf(fh, "%%%%HiResBoundingBox: %.4f %.4f %.4f %.4f\n",
	   min_x * factor, min_y * factor, max_x * factor, max_y * factor);
   fputs("%%LanguageLevel: 1\n"
	 "%%PageOrder: Ascend\n"
	 "%%Pages: 1\n"
	 "%%Orientation: Portrait\n", fh);

   fprintf(fh, "%%%%DocumentFonts: %s\n", fontname_labels);

   fputs("%%EndComments\n"
	 "%%Page 1 1\n"
	 "save countdictstack mark\n", fh);

   /* this code adapted from a2ps */
   fputs("%%BeginResource: encoding ISO88591Encoding\n"
	 "/ISO88591Encoding [\n", fh);
   fputs("/.notdef /.notdef /.notdef /.notdef\n", fh);
   fputs("/.notdef /.notdef /.notdef /.notdef\n", fh);
   fputs("/.notdef /.notdef /.notdef /.notdef\n", fh);
   fputs("/.notdef /.notdef /.notdef /.notdef\n", fh);
   fputs("/.notdef /.notdef /.notdef /.notdef\n", fh);
   fputs("/.notdef /.notdef /.notdef /.notdef\n", fh);
   fputs("/.notdef /.notdef /.notdef /.notdef\n", fh);
   fputs("/.notdef /.notdef /.notdef /.notdef\n", fh);
   fputs(
"/space /exclam /quotedbl /numbersign\n"
"/dollar /percent /ampersand /quoteright\n"
"/parenleft /parenright /asterisk /plus\n"
"/comma /minus /period /slash\n"
"/zero /one /two /three\n"
"/four /five /six /seven\n"
"/eight /nine /colon /semicolon\n"
"/less /equal /greater /question\n"
"/at /A /B /C /D /E /F /G\n"
"/H /I /J /K /L /M /N /O\n"
"/P /Q /R /S /T /U /V /W\n"
"/X /Y /Z /bracketleft\n"
"/backslash /bracketright /asciicircum /underscore\n"
"/quoteleft /a /b /c /d /e /f /g\n"
"/h /i /j /k /l /m /n /o\n"
"/p /q /r /s /t /u /v /w\n"
"/x /y /z /braceleft\n"
"/bar /braceright /asciitilde /.notdef\n", fh);
   fputs("/.notdef /.notdef /.notdef /.notdef\n", fh);
   fputs("/.notdef /.notdef /.notdef /.notdef\n", fh);
   fputs("/.notdef /.notdef /.notdef /.notdef\n", fh);
   fputs("/.notdef /.notdef /.notdef /.notdef\n", fh);
   fputs("/.notdef /.notdef /.notdef /.notdef\n", fh);
   fputs("/.notdef /.notdef /.notdef /.notdef\n", fh);
   fputs("/.notdef /.notdef /.notdef /.notdef\n", fh);
   fputs("/.notdef /.notdef /.notdef /.notdef\n", fh);
   fputs(
"/space /exclamdown /cent /sterling\n"
"/currency /yen /brokenbar /section\n"
"/dieresis /copyright /ordfeminine /guillemotleft\n"
"/logicalnot /hyphen /registered /macron\n"
"/degree /plusminus /twosuperior /threesuperior\n"
"/acute /mu /paragraph /bullet\n"
"/cedilla /onesuperior /ordmasculine /guillemotright\n"
"/onequarter /onehalf /threequarters /questiondown\n"
"/Agrave /Aacute /Acircumflex /Atilde\n"
"/Adieresis /Aring /AE /Ccedilla\n"
"/Egrave /Eacute /Ecircumflex /Edieresis\n"
"/Igrave /Iacute /Icircumflex /Idieresis\n"
"/Eth /Ntilde /Ograve /Oacute\n"
"/Ocircumflex /Otilde /Odieresis /multiply\n"
"/Oslash /Ugrave /Uacute /Ucircumflex\n"
"/Udieresis /Yacute /Thorn /germandbls\n"
"/agrave /aacute /acircumflex /atilde\n"
"/adieresis /aring /ae /ccedilla\n"
"/egrave /eacute /ecircumflex /edieresis\n"
"/igrave /iacute /icircumflex /idieresis\n"
"/eth /ntilde /ograve /oacute\n"
"/ocircumflex /otilde /odieresis /divide\n"
"/oslash /ugrave /uacute /ucircumflex\n"
"/udieresis /yacute /thorn /ydieresis\n"
"] def\n"
"%%EndResource\n", fh);

   /* this code adapted from a2ps */
   fputs(
"/reencode {\n" /* def */
"dup length 5 add dict begin\n"
"{\n" /* forall */
"1 index /FID ne\n"
"{ def }{ pop pop } ifelse\n"
"} forall\n"
"/Encoding exch def\n"

/* Use the font's bounding box to determine the ascent, descent,
 * and overall height; don't forget that these values have to be
 * transformed using the font's matrix.
 * We use `load' because sometimes BBox is executable, sometimes not.
 * Since we need 4 numbers and not an array avoid BBox from being executed
 */
"/FontBBox load aload pop\n"
"FontMatrix transform /Ascent exch def pop\n"
"FontMatrix transform /Descent exch def pop\n"
"/FontHeight Ascent Descent sub def\n"

/* Define these in case they're not in the FontInfo (also, here
 * they're easier to get to.
 */
"/UnderlinePosition 1 def\n"
"/UnderlineThickness 1 def\n"

/* Get the underline position and thickness if they're defined. */
"currentdict /FontInfo known {\n"
"FontInfo\n"

"dup /UnderlinePosition known {\n"
"dup /UnderlinePosition get\n"
"0 exch FontMatrix transform exch pop\n"
"/UnderlinePosition exch def\n"
"} if\n"

"dup /UnderlineThickness known {\n"
"/UnderlineThickness get\n"
"0 exch FontMatrix transform exch pop\n"
"/UnderlineThickness exch def\n"
"} if\n"

"} if\n"
"currentdict\n"
"end\n"
"} bind def\n", fh);

   fprintf(fh, "/lab ISO88591Encoding /%s findfont reencode definefont pop\n",
	   fontname_labels);

   fprintf(fh, "/lab findfont %d scalefont setfont\n", int(fontsize_labels));

#if 0
   /* C<digit> changes colour */
   /* FIXME: read from ini */
   {
      size_t i;
      for (i = 0; i < sizeof(colour) / sizeof(colour[0]); ++i) {
	 fprintf(fh, "/C%u {stroke %.3f %.3f %.3f setrgbcolor} def\n", i,
		 (double)(colour[i] & 0xff0000) / 0xff0000,
		 (double)(colour[i] & 0xff00) / 0xff00,
		 (double)(colour[i] & 0xff) / 0xff);
      }
   }
   fputs("C0\n", fh);
#endif

   /* Postscript definition for drawing a cross */
   fprintf(fh, "/X {stroke moveto %.2f %.2f rmoveto %.2f %.2f rlineto "
	   "%.2f 0 rmoveto %.2f %.2f rlineto %.2f %.2f rmoveto} def\n",
	   -marker_size, -marker_size,  marker_size * 2, marker_size * 2,
	   -marker_size * 2,  marker_size * 2, -marker_size * 2,
	   -marker_size, marker_size );

   /* define some functions to keep file short */
   fputs("/M {stroke moveto} def\n"
	 "/P {stroke newpath moveto} def\n"
	 "/F {closepath gsave 0.8 setgray fill grestore} def\n"
	 "/L {lineto} def\n"
	 "/R {rlineto} def\n"
	 "/S {show} def\n", fh);

   fprintf(fh, "gsave %.8f dup scale\n", factor);
#if 0
   if (grid > 0) {
      double x, y;
      x = floor(min_x / grid) * grid + grid;
      y = floor(min_y / grid) * grid + grid;
      while (x < max_x) {
	 /* horizontal line */
	 fprintf(fh, "0\nLINE\n");
	 fprintf(fh, "8\nGrid\n"); /* Layer */
	 fprintf(fh, "10\n%6.2f\n", x);
	 fprintf(fh, "20\n%6.2f\n", min_y);
	 fprintf(fh, "30\n0\n");
	 fprintf(fh, "11\n%6.2f\n", x);
	 fprintf(fh, "21\n%6.2f\n", max_y);
	 fprintf(fh, "31\n0\n");
	 x += grid;
      }
      while (y < max_y) {
	 /* vertical line */
	 fprintf(fh, "0\nLINE\n");
	 fprintf(fh, "8\nGrid\n"); /* Layer */
	 fprintf(fh, "10\n%6.2f\n", min_x);
	 fprintf(fh, "20\n%6.2f\n", y);
	 fprintf(fh, "30\n0\n");
	 fprintf(fh, "11\n%6.2f\n", max_x);
	 fprintf(fh, "21\n%6.2f\n", y);
	 fprintf(fh, "31\n0\n");
	 y += grid;
      }
   }
#endif
}

void
EPS::start_pass(int layer)
{
    first = true;
    switch (layer) {
	case LEGS|SURF|STNS|LABELS:
	    fprintf(fh, "0.1 setlinewidth\n");
	    break;
	case PASG: case XSECT: case WALL1: case WALL2:
	    fprintf(fh, "0.01 setlinewidth\n");
	    break;
    }
}

void
EPS::line(const img_point *p1, const img_point *p, unsigned flags, bool fPendingMove)
{
   (void)flags; /* unused */
   if (fPendingMove) {
       fprintf(fh, "%.2f %.2f M\n", p1->x, p1->y);
   }
   fprintf(fh, "%.2f %.2f L\n", p->x, p->y);
}

void
EPS::label(const img_point *p, const wxString& str, int /*sflags*/, int)
{
   const char* s = str.utf8_str();
   fprintf(fh, "%.2f %.2f M\n", p->x, p->y);
   PUTC('(', fh);
   while (*s) {
       unsigned char ch = *s++;
       switch (ch) {
	   case '(': case ')': case '\\': /* need to escape these characters */
	       PUTC('\\', fh);
	       PUTC(ch, fh);
	       break;
	   default:
	       PUTC(ch, fh);
	       break;
       }
   }
   fputs(") S\n", fh);
}

void
EPS::cross(const img_point *p, const wxString&, int sflags)
{
   (void)sflags; /* unused */
   fprintf(fh, "%.2f %.2f X\n", p->x, p->y);
}

void
EPS::xsect(const img_point *p, double angle, double d1, double d2)
{
    double s = sin(rad(angle));
    double c = cos(rad(angle));
    fprintf(fh, "%.2f %.2f M %.2f %.2f R\n",
	    p->x - s * d2, p->y - c * d2,
	    s * (d1 + d2), c * (d1 + d2));
}

void
EPS::wall(const img_point *p, double angle, double d)
{
    double s = sin(rad(angle));
    double c = cos(rad(angle));
    fprintf(fh, "%.2f %.2f %c\n", p->x + s * d, p->y + c * d, first ? 'M' : 'L');
    first = false;
}

void
EPS::passage(const img_point *p, double angle, double d1, double d2)
{
    double s = sin(rad(angle));
    double c = cos(rad(angle));
    double x1 = p->x + s * d1;
    double y1 = p->y + c * d1;
    double x2 = p->x - s * d2;
    double y2 = p->y - c * d2;
    fprintf(fh, "%.2f %.2f %c\n", x1, y1, first ? 'P' : 'L');
    first = false;
    psg.push_back(make_pair(x2, y2));
}

void
EPS::tube_end()
{
    if (!psg.empty()) {
	vector<pair<double, double>>::const_reverse_iterator i;
	for (i = psg.rbegin(); i != psg.rend(); ++i) {
	    fprintf(fh, "%.2f %.2f L\n", i->first, i->second);
	}
	fputs("F\n", fh);
	psg.clear();
    }
}

void
EPS::footer(void)
{
   fputs("stroke showpage grestore\n"
	 "%%Trailer\n"
	 "cleartomark countdictstack exch sub { end } repeat restore\n"
	 "%%EOF\n", fh);
}

class UseNumericCLocale {
    char * current_locale;

  public:
    UseNumericCLocale() {
	current_locale = osstrdup(setlocale(LC_NUMERIC, NULL));
	setlocale(LC_NUMERIC, "C");
    }

    ~UseNumericCLocale() {
	setlocale(LC_NUMERIC, current_locale);
	osfree(current_locale);
    }
};

static void
transform_point(const Point& pos, const Vector3* pre_offset,
		double COS, double SIN, double COST, double SINT,
		img_point* p)
{
    double x = pos.GetX();
    double y = pos.GetY();
    double z = pos.GetZ();
    if (pre_offset) {
	x += pre_offset->GetX();
	y += pre_offset->GetY();
	z += pre_offset->GetZ();
    }
    p->x = x * COS - y * SIN;
    double tmp = x * SIN + y * COS;
    p->y = z * COST - tmp * SINT;
    p->z = -(z * SINT + tmp * COST);
}

bool
Export(const wxString &fnm_out, const wxString &title,
       const Model& model,
       const SurveyFilter* filter,
       double pan, double tilt, int show_mask, export_format format,
       double grid_, double text_height, double marker_size_,
       double scale)
{
   UseNumericCLocale dummy;
   bool fPendingMove = false;
   img_point p, p1;
   const int *pass;
   double SIN = sin(rad(pan));
   double COS = cos(rad(pan));
   double SINT = sin(rad(tilt));
   double COST = cos(rad(tilt));

   grid = (show_mask & GRID) ? grid_ : 0.0;
   marker_size = marker_size_;

   // Do we need to calculate min and max for each dimension?
   bool need_bounds = true;
   ExportFilter * filt;
   switch (format) {
       case FMT_3D:
	   filt = new Export3D(model.GetCSProj(), model.GetSeparator());
	   show_mask |= FULL_COORDS;
	   need_bounds = false;
	   break;
       case FMT_CSV:
	   filt = new POS(model.GetSeparator(), true);
	   show_mask |= FULL_COORDS;
	   need_bounds = false;
	   break;
       case FMT_DXF:
	   filt = new DXF(text_height);
	   break;
       case FMT_EPS:
	   filt = new EPS(scale);
	   break;
       case FMT_GPX:
	   filt = new GPX(model.GetCSProj().c_str());
	   show_mask |= FULL_COORDS;
	   need_bounds = false;
	   break;
       case FMT_HPGL:
	   filt = new HPGL(scale);
	   // HPGL doesn't use the bounds itself, but they are needed to set
	   // the origin to the centre of lower left.
	   break;
       case FMT_JSON:
	   filt = new JSON;
	   break;
       case FMT_KML: {
	   bool clamp_to_ground = (show_mask & CLAMP_TO_GROUND);
	   filt = new KML(model.GetCSProj().c_str(), clamp_to_ground);
	   show_mask |= FULL_COORDS;
	   need_bounds = false;
	   break;
       }
       case FMT_PLT:
	   filt = new PLT;
	   show_mask |= FULL_COORDS;
	   break;
       case FMT_POS:
	   filt = new POS(model.GetSeparator(), false);
	   show_mask |= FULL_COORDS;
	   need_bounds = false;
	   break;
       case FMT_SVG:
	   filt = new SVG(scale, text_height);
	   break;
       case FMT_SHP_LINES:
	   filt = new ShapefileLines(fnm_out.utf8_str(),
				     model.GetCSProj().c_str());
	   show_mask |= FULL_COORDS;
	   need_bounds = false;
	   break;
       case FMT_SHP_POINTS:
	   filt = new ShapefilePoints(fnm_out.utf8_str(),
				      model.GetCSProj().c_str());
	   show_mask |= FULL_COORDS;
	   need_bounds = false;
	   break;
       default:
	   return false;
   }

   if (!filt->fopen(fnm_out)) {
       delete filt;
       return false;
   }

   const Vector3* pre_offset = NULL;
   if (show_mask & FULL_COORDS) {
	pre_offset = &(model.GetOffset());
   }

   /* Get bounding box */
   double min_x, min_y, min_z, max_x, max_y, max_z;
   min_x = min_y = min_z = HUGE_VAL;
   max_x = max_y = max_z = -HUGE_VAL;
   if (need_bounds) {
	for (int f = 0; f != 8; ++f) {
	    if ((show_mask & (f & img_FLAG_SURFACE) ? SURF : LEGS) == 0) {
		// Not showing traverse because of surface/underground status.
		continue;
	    }
	    if ((f & show_mask & SPLAYS) == 0) {
		// Not showing because it's a splay.
		continue;
	    }
	    list<traverse>::const_iterator trav = model.traverses_begin(f, filter);
	    list<traverse>::const_iterator tend = model.traverses_end(f);
	    for ( ; trav != tend; trav = model.traverses_next(f, filter, trav)) {
		vector<PointInfo>::const_iterator pos = trav->begin();
		vector<PointInfo>::const_iterator end = trav->end();
		for ( ; pos != end; ++pos) {
		    transform_point(*pos, pre_offset, COS, SIN, COST, SINT, &p);

		    if (p.x < min_x) min_x = p.x;
		    if (p.x > max_x) max_x = p.x;
		    if (p.y < min_y) min_y = p.y;
		    if (p.y > max_y) max_y = p.y;
		    if (p.z < min_z) min_z = p.z;
		    if (p.z > max_z) max_z = p.z;
		}
	    }
	}
	list<LabelInfo*>::const_iterator pos = model.GetLabels();
	list<LabelInfo*>::const_iterator end = model.GetLabelsEnd();
	for ( ; pos != end; ++pos) {
	    if (filter && !filter->CheckVisible((*pos)->GetText()))
		continue;

	    transform_point(**pos, pre_offset, COS, SIN, COST, SINT, &p);

	    if (p.x < min_x) min_x = p.x;
	    if (p.x > max_x) max_x = p.x;
	    if (p.y < min_y) min_y = p.y;
	    if (p.y > max_y) max_y = p.y;
	    if (p.z < min_z) min_z = p.z;
	    if (p.z > max_z) max_z = p.z;
	}

	if (grid > 0) {
	    min_x -= grid / 2;
	    max_x += grid / 2;
	    min_y -= grid / 2;
	    max_y += grid / 2;
	}
   }

   /* Handle empty file and gracefully, and also zero for the !need_bounds
    * case. */
   if (min_x > max_x) {
      min_x = min_y = min_z = 0;
      max_x = max_y = max_z = 0;
   }

   double x_offset, y_offset, z_offset;
   if (show_mask & FULL_COORDS) {
       // Full coordinates - offset is applied before rotations.
       x_offset = y_offset = z_offset = 0.0;
   } else if (show_mask & CENTRED) {
       // Centred.
       x_offset = (min_x + max_x) * -0.5;
       y_offset = (min_y + max_y) * -0.5;
       z_offset = (min_z + max_z) * -0.5;
   } else {
       // Origin at lowest SW corner.
       x_offset = -min_x;
       y_offset = -min_y;
       z_offset = -min_z;
   }
   if (need_bounds) {
	min_x += x_offset;
	max_x += x_offset;
	min_y += y_offset;
	max_y += y_offset;
	min_z += z_offset;
	max_z += z_offset;
   }

   /* Header */
   filt->header(title.utf8_str(), model.GetDateStamp(),
		min_x, min_y, min_z, max_x, max_y, max_z);

   p1.x = p1.y = p1.z = 0; /* avoid compiler warning */

   for (pass = filt->passes(); *pass; ++pass) {
      int pass_mask = show_mask & *pass;
      if (!pass_mask)
	  continue;
      filt->start_pass(*pass);
      int leg_mask = pass_mask & (LEGS|SURF);
      if (leg_mask) {
	  for (int f = 0; f != 8; ++f) {
	      unsigned flags = f;
	      if (!(flags & img_FLAG_SURFACE)) flags |= LEGS;
	      if ((leg_mask & flags) == 0) {
		  // Not showing traverse because of surface/underground status.
		  continue;
	      }
	      if ((flags & SPLAYS) && (show_mask & SPLAYS) == 0) {
		  // Not showing because it's a splay.
		  continue;
	      }
	      list<traverse>::const_iterator trav = model.traverses_begin(f, filter);
	      list<traverse>::const_iterator tend = model.traverses_end(f);
	      for ( ; trav != tend; trav = model.traverses_next(f, filter, trav)) {
		  assert(trav->size() > 1);
		  vector<PointInfo>::const_iterator pos = trav->begin();
		  vector<PointInfo>::const_iterator end = trav->end();
		  for ( ; pos != end; ++pos) {
		      transform_point(*pos, pre_offset, COS, SIN, COST, SINT, &p);
		      p.x += x_offset;
		      p.y += y_offset;
		      p.z += z_offset;

		      if (pos == trav->begin()) {
			  // First point is move...
			  fPendingMove = true;
		      } else {
			  filt->line(&p1, &p, flags, fPendingMove);
			  fPendingMove = false;
		      }
		      p1 = p;
		  }
	      }
	  }
      }
      if (pass_mask & (STNS|LABELS|ENTS|FIXES|EXPORTS)) {
	  list<LabelInfo*>::const_iterator pos = model.GetLabels();
	  list<LabelInfo*>::const_iterator end = model.GetLabelsEnd();
	  for ( ; pos != end; ++pos) {
	      if (filter && !filter->CheckVisible((*pos)->GetText()))
		  continue;

	      transform_point(**pos, pre_offset, COS, SIN, COST, SINT, &p);
	      p.x += x_offset;
	      p.y += y_offset;
	      p.z += z_offset;

	      int type = 0;
	      if ((pass_mask & ENTS) && (*pos)->IsEntrance()) {
		  type = ENTS;
	      } else if ((pass_mask & FIXES) && (*pos)->IsFixedPt()) {
		  type = FIXES;
	      } else if ((pass_mask & EXPORTS) && (*pos)->IsExportedPt())  {
		  type = EXPORTS;
	      } else if (pass_mask & LABELS) {
		  type = LABELS;
	      }
	      int sflags = (*pos)->get_flags();
	      if (type) {
		  filt->label(&p, (*pos)->GetText(), sflags, type);
	      }
	      if (pass_mask & STNS) {
		  filt->cross(&p, (*pos)->GetText(), sflags);
	      }
	  }
      }
      if (pass_mask & (XSECT|WALLS|PASG)) {
	  bool elevation = (tilt == 0.0);
	  list<vector<XSect>>::const_iterator tube = model.tubes_begin();
	  list<vector<XSect>>::const_iterator tube_end = model.tubes_end();
	  for ( ; tube != tube_end; ++tube) {
	      vector<XSect>::const_iterator pos = tube->begin();
	      vector<XSect>::const_iterator end = tube->end();
	      size_t active_tube_len = 0;
	      for ( ; pos != end; ++pos) {
		  const XSect & xs = *pos;
		  // FIXME: This filtering can create tubes containing a single
		  // cross-section, which otherwise don't exist in aven (the
		  // Model class currently filters them out).  Perhaps we
		  // should just always include these - a single set of LRUD
		  // measurements is useful even if a single cross-section
		  // 3D tube perhaps isn't.
		  if (filter && !filter->CheckVisible(xs.GetLabel())) {
		      // Close any active tube.
		      if (active_tube_len > 0) {
			  active_tube_len = 0;
			  filt->tube_end();
		      }
		      continue;
		  }

		  ++active_tube_len;
		  transform_point(xs.GetPoint(), pre_offset, COS, SIN, COST, SINT, &p);
		  p.x += x_offset;
		  p.y += y_offset;
		  p.z += z_offset;

		  if (elevation) {
		      if (pass_mask & XSECT)
			  filt->xsect(&p, 90, xs.GetU(), xs.GetD());
		      if (pass_mask & WALL1)
			  filt->wall(&p, 90, xs.GetU());
		      if (pass_mask & WALL2)
			  filt->wall(&p, 270, xs.GetD());
		      if (pass_mask & PASG)
			  filt->passage(&p, 90, xs.GetU(), xs.GetD());
		  } else {
		      // Should only be enabled in plan or elevation mode.
		      double angle = xs.get_right_bearing() - pan;
		      if (pass_mask & XSECT)
			  filt->xsect(&p, angle + 180, xs.GetL(), xs.GetR());
		      if (pass_mask & WALL1)
			  filt->wall(&p, angle + 180, xs.GetL());
		      if (pass_mask & WALL2)
			  filt->wall(&p, angle, xs.GetR());
		      if (pass_mask & PASG)
			  filt->passage(&p, angle + 180, xs.GetL(), xs.GetR());
		  }
	      }
	      if (active_tube_len > 0) {
		  filt->tube_end();
	      }
	  }
      }
   }
   filt->footer();
   delete filt;
   osfree(htab);
   htab = NULL;
   return true;
}
