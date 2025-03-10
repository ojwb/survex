/* export.h
 * Export to GIS formats, CAD formats, and other formats.
 */

/* Copyright (C) 2004-2024 Olly Betts
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

#include "img_for_survex.h"
#include "wx.h"

class Model;
class SurveyFilter;

// Order here needs to match order of extension array in export.cc.
typedef enum {
    FMT_3D,
    FMT_CSV,
    FMT_DXF,
    FMT_EPS,
    FMT_GPX,
    FMT_HPGL,
    FMT_JSON,
    FMT_KML,
    FMT_PLT,
    FMT_POS,
    FMT_SVG,
    FMT_SHP_LINES,
    FMT_SHP_POINTS,
    FMT_MAX_PLUS_ONE_
} export_format;

struct format_info {
    const char* extension;
    int msg_filetype;
    unsigned mask;
    // Defaults for survexport (aven defaults to what is currently shown).
    unsigned defaults;
};

extern const format_info export_format_info[];

// Align flag values with img_FLAG_*.
#define SURF		img_FLAG_SURFACE
// img_FLAG_DUPLICATE
#define SPLAYS		img_FLAG_SPLAY
#define MASK_ (img_FLAG_SURFACE|img_FLAG_DUPLICATE|img_FLAG_SPLAY)
static_assert(MASK_ < 0x00000008, "MASK_ only contains expected bits");
#define LEGS		0x00000008
#define STNS		0x00000010
#define LABELS		0x00000020
#define XSECT		0x00000040
#define WALL1		0x00000080
#define WALL2		0x00000100
#define WALLS (WALL1|WALL2)
#define PASG		0x00000200
#define ORIENTABLE	0x00000400
#define CENTRED		0x00000800
#define ENTS		0x00001000
#define FIXES		0x00002000
#define EXPORTS		0x00004000
#define PROJ		0x00008000
#define GRID		0x00010000
#define TEXT_HEIGHT	0x00020000
#define MARKER_SIZE	0x00040000
#define SCALE		0x00080000
#define FULL_COORDS	0x00100000
#define CLAMP_TO_GROUND	0x00200000

#define DEFAULT_GRID_SPACING 100 // metres
#define DEFAULT_TEXT_HEIGHT 0.6
#define DEFAULT_MARKER_SIZE 0.8

bool Export(const wxString &fnm_out, const wxString &title,
	    const Model& model,
	    const SurveyFilter* filter,
	    double pan, double tilt, int show_mask, export_format format,
	    double grid_, double text_height_, double marker_size_,
	    double scale);

#endif
