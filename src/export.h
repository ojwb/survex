/* export.h
 * Export to CAD-like formats (DXF, Sketch, SVG, EPS, HPGL) and also Compass
 * PLT.
 */

/* Copyright (C) 2004,2005 Olly Betts
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

bool Export(const wxString &fnm_out, const wxString &title,
	const MainFrm * mainfrm, double pan, double tilt,
	bool labels, bool crosses, bool legs, bool surface);
