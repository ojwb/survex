/* exportfilter.h
 * Export to CAD-like formats (DXF, Skencil, SVG, EPS, HPGL) and also Compass
 * PLT.
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

#ifndef SURVEX_EXPORTFILTER_H
#define SURVEX_EXPORTFILTER_H

#include <stdio.h>
#include "wx.h"

#include "img_hosted.h"

class ExportFilter {
  protected:
    FILE * fh;
  public:
    ExportFilter() : fh(NULL) { }
    // FIXME: deal with errors closing file... (safe_fclose?)
    virtual ~ExportFilter() { if (fh) fclose(fh); }
    virtual const int * passes() const;
    virtual bool fopen(const wxString& fnm_out) {
	fh = wxFopen(fnm_out.fn_str(), wxT("wb"));
	return (fh != NULL);
    }
    virtual void header(const char* title,
			const char* datestamp_string,
			time_t datestamp,
			double min_x, double min_y, double min_z,
			double max_x, double max_y, double max_z);
    virtual void start_pass(int);
    virtual void line(const img_point *, const img_point *, unsigned, bool);
    virtual void label(const img_point* p, const wxString& s,
		       int sflags, int type) = 0;
    virtual void cross(const img_point *, const wxString&, int sflags);
    virtual void xsect(const img_point *, double, double, double);
    virtual void wall(const img_point *, double, double);
    virtual void passage(const img_point *, double, double, double);
    virtual void tube_end();
    virtual void footer();
};

inline void
ExportFilter::header(const char*,
		     const char*,
		     time_t,
		     double, double, double,
		     double, double, double) { }

inline void
ExportFilter::start_pass(int) { }

inline void
ExportFilter::line(const img_point *, const img_point *, unsigned, bool) { }

inline void
ExportFilter::cross(const img_point *, const wxString&, int) { }

inline void
ExportFilter::xsect(const img_point *, double, double, double) { }

inline void
ExportFilter::wall(const img_point *, double, double) { }

inline void
ExportFilter::passage(const img_point *, double, double, double) { }

inline void
ExportFilter::tube_end() { }

inline void
ExportFilter::footer() { }

#endif
