/* > prbitmap.h
 * Header files for
 * Bitmap routines for Survex Dot-matrix and Inkjet printer drivers
 * Copyright (C) 1994,2001 Olly Betts
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

extern void (*PlotDot)(long X, long Y);

void DrawLineDots(long x, long y, long x2, long y2);
void MoveTo(long x, long y);
void DrawTo(long x, long y);
void DrawCross(long x, long y);
void read_font(const char *pth, const char *leaf, int dpiX, int dpiY);
void WriteString(const char *s);
