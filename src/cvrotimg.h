/* > cvrotimg.h
 * Header file for
 * Reads a .3d image file into two linked lists of blocks, suitable for use
 * by caverot.c
 * Copyright (C) 1994,1997,1999 Olly Betts
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

extern bool load_data(const char *fnmData, lid Huge **ppLegs, lid Huge **ppStns);

extern float scale_to_screen(lid Huge **pplid, lid Huge **pplid2);

extern coord Xorg, Yorg, Zorg; /* position of centre of survey */
extern coord Xrad, Yrad, Zrad; /* "radii" */

