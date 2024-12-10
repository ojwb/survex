/* gdalexport.h
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

#include "exportfilter.h"

#ifdef HAVE_GDAL
# include <ogrsf_frmts.h>
#endif

#include "vector3.h"

#include <vector>

class ExportWithGDAL : public ExportFilter {
#ifdef HAVE_GDAL
    GDALDataset* gdal_dataset = nullptr;
    OGRLayer* gdal_layer = nullptr;
    OGRSpatialReference* srs = nullptr;
    OGRLineString line_string;

    void finish_line_string();
#endif

  public:
    ExportWithGDAL(const char* filename,
		   const char* input_datum,
		   const char* gdal_driver_name);
    ~ExportWithGDAL();
#ifdef HAVE_GDAL
    void header(const char *, const char *, time_t,
		double, double, double,
		double, double, double) override;
    void start_pass(int pass) override;
    void line(const img_point *, const img_point *, unsigned, bool) override;
    void label(const img_point *, const wxString&, int, int) override;
    void footer() override;
#endif
};

class ShapefilePoints : public ExportWithGDAL {
  public:
    ShapefilePoints(const char* filename,
		    const char* input_datum)
	: ExportWithGDAL(filename, input_datum, "ESRI Shapefile") {}

    const int * passes() const override;
};

class ShapefileLines : public ExportWithGDAL {
  public:
    ShapefileLines(const char* filename,
		   const char* input_datum)
	: ExportWithGDAL(filename, input_datum, "ESRI Shapefile") {}

    const int * passes() const override;
};
