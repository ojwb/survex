/* pos.h
 * Export from Aven as Survex .pos or .csv.
 */

/* Copyright (C) 2005,2013,2014,2015,2018 Olly Betts
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

#include <vector>

class POS : public ExportFilter {
  public:
    struct pos_label {
	double x, y, z;
	char name[1];
    };

  private:
    std::vector<pos_label *> todo;

    char separator;

    bool csv;

  public:
    POS(char separator_, bool csv_) : separator(separator_), csv(csv_) { }
    ~POS();
    const int * passes() const;
    void header(const char *, const char *, time_t,
		double, double, double,
		double, double, double);
    void label(const img_point *, const wxString&, bool, int);
    void footer();
};
