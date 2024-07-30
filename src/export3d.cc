/* survex3d.cc
 * Export from Aven as Survex .3d.
 */
/* Copyright (C) 2001-2024 Olly Betts
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

#include "export3d.h"

#include "export.h" // For LABELS, etc
#include "img.h"

using namespace std;

Export3D::~Export3D()
{
    if (pimg) {
	img_close(pimg);
    }
}

const int *
Export3D::passes() const
{
    static const int default_passes[] = { LEGS|SURF|LABELS|ENTS|FIXES|EXPORTS, 0 };
    return default_passes;
}

void Export3D::header(const char* title, const char *, time_t,
		      double, double, double, double, double, double)
{
    pimg = img_write_stream(fh, NULL, title,
			    cs.empty() ? NULL : (const char*)cs.utf8_str(),
			    img_FFLAG_SEPARATOR(separator));
}

void
Export3D::line(const img_point* p1, const img_point* p, unsigned flags, bool fPendingMove)
{
    int img_flags = (flags & MASK_);
    if (fPendingMove) {
	img_write_item(pimg, img_MOVE, 0, NULL, p1->x, p1->y, p1->z);
    }
    img_write_item(pimg, img_LINE, img_flags, NULL, p->x, p->y, p->z);
}

void
Export3D::label(const img_point* p, const wxString& str, int sflags, int)
{
    const char* s = str.utf8_str();
    img_write_item(pimg, img_LABEL, sflags, s, p->x, p->y, p->z);
}

void
Export3D::footer()
{
    img_close(pimg);
    pimg = nullptr;
}
