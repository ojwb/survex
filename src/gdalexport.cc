/* gdalexport.cc
 * Export using GDAL
 */
/* Copyright (C) 2024 Olly Betts
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

#include "gdalexport.h"

#include "export.h" // For LABELS, etc.

#ifdef HAVE_GDAL
# include <ogrsf_frmts.h>
#endif

#include "aven.h"
#include "useful.h"
#include "message.h"

using namespace std;

ExportWithGDAL::ExportWithGDAL(const char* filename,
			       const char* input_datum,
			       const char* gdal_driver_name)
{
#ifdef HAVE_GDAL
    GDALAllRegister();

    auto manager = GetGDALDriverManager();
    auto driver = manager->GetDriverByName(gdal_driver_name);
    if (!driver) {
	throw wxString::Format(wmsg(/*Failed to initialise GDAL “%s” driver*/527),
			       gdal_driver_name);
    }

    gdal_dataset = driver->Create(filename, 0, 0, 0, GDT_Unknown, nullptr);
    if (!gdal_dataset) {
	throw wxString::Format(wmsg(/*Failed to initialise GDAL “%s” driver*/527),
			       gdal_driver_name);
    }

    if (input_datum) {
	srs = new OGRSpatialReference();
	srs->SetFromUserInput(input_datum);
	// This apparently only works for raster data:
	// gdal_dataset->SetSpatialRef(srs);
    }
#else
    wxMessageBox(wxT("GDAL support not enabled in this build"),
		 wxT("Aven GDAL support"),
		 wxOK | wxICON_INFORMATION);
#endif
}

ExportWithGDAL::~ExportWithGDAL()
{
#ifdef HAVE_GDAL
    if (srs) srs->Release();
#endif
}

#ifdef HAVE_GDAL
/* Initialise ExportWithGDAL routines. */
void ExportWithGDAL::header(const char * title, const char *, time_t,
		 double, double, double, double, double, double)
{
    (void)title;
}

void
ExportWithGDAL::start_pass(int layer)
{
    if (!line_string.IsEmpty()) {
	finish_line_string();
    }

    const char* name = nullptr;
    OGRwkbGeometryType type = wkbUnknown;

    switch (layer) {
	case PASG:
	    name = "passages";
	    type = wkbPolygon;
	    break;
	case XSECT:
	    name = "passages";
	    type = wkbPolygon;
	    break;
	case WALL1|WALL2:
	    name = "walls";
	    type = wkbLineString;
	    break;
	case LEGS:
	    name = "legs";
	    type = wkbLineString;
	    break;
	case SPLAYS:
	    name = "splays";
	    type = wkbLineString;
	    break;
	case SURF:
	    name = "surface legs";
	    type = wkbLineString;
	    break;
	case LABELS:
	    name = "stations";
	    type = wkbPoint;
	    break;
	case ENTS:
	    name = "entrances";
	    type = wkbPoint;
	    break;
	case FIXES:
	    name = "fixed points";
	    type = wkbPoint;
	    break;
	case EXPORTS:
	    name = "exported points";
	    type = wkbPoint;
	    break;
    }
    gdal_layer = gdal_dataset->CreateLayer(name, srs, type, nullptr);
    if (!gdal_layer) {
	throw wmsg(/*Failed to create GDAL layer*/528);
    }

    switch (layer) {
	case LABELS:
	case ENTS:
	case FIXES:
	case EXPORTS: {
	    OGRFieldDefn field("Name", OFTString);
	    if (gdal_layer->CreateField(&field) != OGRERR_NONE) {
		throw wmsg(/*Failed to create GDAL field*/529);
	    }
	    break;
	}
    }
}

void
ExportWithGDAL::line(const img_point *p1, const img_point *p, unsigned /*flags*/, bool fPendingMove)
{
    if (fPendingMove) {
	if (!line_string.IsEmpty()) {
	    finish_line_string();
	}

	line_string.addPoint(p1->x, p1->y, p1->z);
    }

    line_string.addPoint(p->x, p->y, p->z);
}

void
ExportWithGDAL::label(const img_point *p, const wxString& str, int, int)
{
    OGRFeature* feature = OGRFeature::CreateFeature(gdal_layer->GetLayerDefn());
    feature->SetField("Name", str.utf8_str());

    OGRPoint pt;
    pt.setX(p->x);
    pt.setY(p->y);
    pt.setZ(p->z);
    feature->SetGeometry(&pt);

    if (gdal_layer->CreateFeature(feature) != OGRERR_NONE) {
	OGRFeature::DestroyFeature(feature);
	throw wmsg(/*Failed to create GDAL feature*/530);
    }
    OGRFeature::DestroyFeature(feature);
}

void
ExportWithGDAL::finish_line_string()
{
    OGRFeature* feature =
	OGRFeature::CreateFeature(gdal_layer->GetLayerDefn());
    feature->SetGeometry(&line_string);
    if (gdal_layer->CreateFeature(feature) != OGRERR_NONE) {
	OGRFeature::DestroyFeature(feature);
	throw wmsg(/*Failed to create GDAL feature*/530);
    }
    OGRFeature::DestroyFeature(feature);
    line_string.empty();
}

void
ExportWithGDAL::footer()
{
    if (!line_string.IsEmpty()) {
	finish_line_string();
    }
    GDALClose(gdal_dataset);
}
#endif

const int*
ShapefilePoints::passes() const
{
    static const int default_passes[] = {
	ENTS, FIXES, EXPORTS, LABELS, 0
    };
    return default_passes;
}

const int*
ShapefileLines::passes() const
{
    static const int default_passes[] = {
	LEGS, SPLAYS, SURF, 0
    };
    return default_passes;
}
