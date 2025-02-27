/* kml.h
 * Export from Aven as KML.
 */
/* Copyright (C) 2005-2024 Olly Betts
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

#include <proj.h>

#include "vector3.h"

#include <vector>

class KML : public ExportFilter {
    PJ* pj = NULL;
    unsigned linestring_flags = 0;
    bool in_wall = false;
    bool in_passage = false;
    bool clamp_to_ground;
    Vector3 v1, v2;
  public:
    KML(const char * input_datum, bool clamp_to_ground_);
    ~KML();
    const int * passes() const override;
    void header(const char *, time_t,
		double, double, double,
		double, double, double) override;
    void start_pass(int pass) override;
    void line(const img_point *, const img_point *, unsigned, bool) override;
    void label(const img_point *, const wxString&, int, int) override;
    void xsect(const img_point *, double, double, double) override;
    void wall(const img_point *, double, double) override;
    void passage(const img_point *, double, double, double) override;
    void tube_end() override;
    void footer() override;
};
