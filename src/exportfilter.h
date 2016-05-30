/* exportfilter.h
 * Export to CAD-like formats (DXF, Skencil, SVG, EPS, HPGL) and also Compass
 * PLT.
 */

/* Copyright (C) 2005,2012,2013,2014,2015,2016 Olly Betts
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
    virtual void header(const char *, const char *, time_t,
			double, double, double,
			double, double, double) { }
    virtual void start_pass(int) { }
    virtual void line(const img_point *, const img_point *, bool, bool) { }
    virtual void label(const img_point *, const char *, bool, int) = 0;
    virtual void cross(const img_point *, bool) { }
    virtual void xsect(const img_point *, double, double, double) { }
    virtual void wall(const img_point *, double, double) { }
    virtual void passage(const img_point *, double, double, double) { }
    virtual void tube_end() { }
    virtual void footer() { }
};

#endif
