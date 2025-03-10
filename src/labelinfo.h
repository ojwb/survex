//
//  labelinfo.h
//
//  Main frame handling for Aven.
//
//  Copyright (C) 2000-2003,2005 Mark R. Shinwell
//  Copyright (C) 2001-2003,2004,2005,2006,2010,2011,2012,2013,2014 Olly Betts
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#ifndef labelinfo_h
#define labelinfo_h

#include "aven.h"
#include "img_for_survex.h"
#include "message.h"
#include "vector3.h"
#include "wx.h"

// macOS headers pollute the global namespace with generic names like
// "class Point", which clashes with our "class Point".  So for __WXMAC__
// put our class in a namespace and define Point as a macro.
#ifdef __WXMAC__
namespace svx {
#endif

class Point : public Vector3 {
  public:
    Point() {}
    explicit Point(const Vector3 & v) : Vector3(v) { }
    explicit Point(const img_point & pt) : Vector3(pt.x, pt.y, pt.z) { }
    double GetX() const { return x; }
    double GetY() const { return y; }
    double GetZ() const { return z; }
};

#ifdef __WXMAC__
}
#define Point svx::Point
#endif

// Align LFLAG_* constants with img_SFLAG_* constants where they overlap so
// that we can just copy those bits when loading a file.

constexpr int LFLAG_SURFACE = img_SFLAG_SURFACE;
constexpr int LFLAG_UNDERGROUND = img_SFLAG_UNDERGROUND;
constexpr int LFLAG_ENTRANCE = img_SFLAG_ENTRANCE;
constexpr int LFLAG_EXPORTED = img_SFLAG_EXPORTED;
constexpr int LFLAG_FIXED = img_SFLAG_FIXED;
constexpr int LFLAG_ANON = img_SFLAG_ANON;
constexpr int LFLAG_WALL = img_SFLAG_WALL;

constexpr int LFLAG_IMG_MASK =
	LFLAG_SURFACE |
	LFLAG_UNDERGROUND |
	LFLAG_ENTRANCE |
	LFLAG_EXPORTED |
	LFLAG_FIXED |
	LFLAG_ANON |
	LFLAG_WALL;

static_assert(LFLAG_IMG_MASK < 0x80, "LFLAG_IMG_MASK only contains expected bits");

// Bits 0x0100 to 0x2000 are set lazily to allow us to sort labels into plot
// order with the first comparison being an integer subtraction.

// Set for matching stations when a search is done (and cleared for others).
constexpr int LFLAG_HIGHLIGHTED	= 0x4000;

class LabelInfo : public Point {
    wxString text;
    unsigned width;
    int flags = 0;

public:
    wxTreeItemId tree_id;

    LabelInfo() { }
    LabelInfo(const img_point &pt, const wxString &text_, int flags_)
	: Point(pt), text(text_), flags(flags_) {
	if (text.empty())
	    flags |= LFLAG_ANON;
    }
    const wxString & GetText() const { return text; }
    wxString name_or_anon() const {
	if (!text.empty()) return text;
	/* TRANSLATORS: Used in place of the station name when talking about an
	 * anonymous station. */
	return wmsg(/*anonymous station*/56);
    }
    int get_flags() const { return flags; }
    void set_flags(int mask) { flags |= mask; }
    void clear_flags(int mask) { flags &= ~mask; }
    unsigned get_width() const { return width; }
    void set_width(unsigned width_) { width = width_; }

    bool IsEntrance() const { return (flags & LFLAG_ENTRANCE) != 0; }
    bool IsFixedPt() const { return (flags & LFLAG_FIXED) != 0; }
    bool IsExportedPt() const { return (flags & LFLAG_EXPORTED) != 0; }
    bool IsUnderground() const { return (flags & LFLAG_UNDERGROUND) != 0; }
    bool IsSurface() const { return (flags & LFLAG_SURFACE) != 0; }
    bool IsHighLighted() const { return (flags & LFLAG_HIGHLIGHTED) != 0; }
    bool IsAnon() const { return (flags & LFLAG_ANON) != 0; }
    bool IsWall() const { return (flags & LFLAG_WALL) != 0; }
    // This should really also return true for non-anonymous splay ends, and not
    // return true for anonymous stations in other situations, but the .3d
    // format doesn't tell us this information currently, and it's not trivial
    // to rediscover.
    bool IsSplayEnd() const { return IsAnon(); }
};

#endif
