/* cvrotimg.h
 * Header file for
 * Reads a .3d image file into two linked lists of blocks, suitable for use
 * by caverot.c
 * Copyright (C) 1994-2001 Olly Betts
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

#ifndef SVX_CVROTIMG_H_INCLUDED
#define SVX_CVROTIMG_H_INCLUDED

/* data type used after data is read in */
typedef INT32_T coord;

/* Data structures */
typedef struct {
   union {
      coord action; /* plot option for this point - draw/move/endofdata */
      char *str; /* string for a label */
   } _;
   coord X, Y, Z;  /* coordinates */
} point;

void set_codes(coord move_, coord draw_, coord stop_);

bool load_data(const char *fnmData, const char *survey,
	       point Huge **ppLegs, point Huge **ppSLegs, point Huge **ppStns);

void reset_limits(point *pmin, point *pmax);

void update_limits(point *pmin, point *pmax,
		   point Huge *pLegs, point Huge *pStns);

double scale_to_screen(const point *pmin, const point *pmax,
		       int xcMac, int ycMac, double y_stretch);

extern coord Xorg, Yorg, Zorg; /* position of centre of survey */
extern coord Xrad, Yrad, Zrad; /* "radii" */

#endif
