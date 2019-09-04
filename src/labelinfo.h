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
#include "img_hosted.h"
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

#define LFLAG_NOT_ANON		0x01
#define LFLAG_NOT_WALL		0x02
#define LFLAG_SURFACE		0x04
#define LFLAG_UNDERGROUND	0x08
#define LFLAG_EXPORTED		0x10
#define LFLAG_FIXED		0x20
#define LFLAG_ENTRANCE		0x40
#define LFLAG_HIGHLIGHTED	0x80

class LabelInfo : public Point {
    wxString text;
    unsigned width;
    int flags;

public:
    wxTreeItemId tree_id;

    LabelInfo() : Point(), text(), flags(0) { }
    LabelInfo(const img_point &pt, const wxString &text_, int flags_)
	: Point(pt), text(text_), flags(flags_) {
	if (text.empty())
	    flags &= ~LFLAG_NOT_ANON;
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
    bool IsAnon() const { return (flags & LFLAG_NOT_ANON) == 0; }
    bool IsWall() const { return (flags & LFLAG_NOT_WALL) == 0; }
    // This should really also return true for non-anonymous splay ends, and not
    // return true for anonymous stations in other situations, but the .3d
    // format doesn't tell us this information currently, and it's not trivial
    // to rediscover.
    bool IsSplayEnd() const { return IsAnon(); }
};

#endif
