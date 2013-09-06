/* export.h
 * Export to CAD-like formats (DXF, Skencil, SVG, EPS, HPGL) and also Compass
 * PLT.
 */

/* Copyright (C) 2004,2005,2012 Olly Betts
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

#ifndef SURVEX_INCLUDED_EXPORT_H
#define SURVEX_INCLUDED_EXPORT_H

#include "wx.h"

class MainFrm;

typedef enum {
    FMT_DXF,
    FMT_EPS,
    FMT_GPX,
    FMT_HPGL,
    FMT_PLT,
    FMT_SK,
    FMT_SVG
} export_format;

#define LEGS 1
#define SURF 2
#define STNS 4
#define LABELS 8
#define XSECT 16
#define WALL1 32
#define WALL2 64
#define WALLS (WALL1|WALL2)
#define PASG 128
#define EXPORT_3D 256
#define EXPORT_CENTRED 512

bool Export(const wxString &fnm_out, const wxString &title,
	    const MainFrm * mainfrm, double pan, double tilt, int show_mask,
	    export_format format);

#endif
