/* > armrot.h
 * Caverot plot & translate headers for RISCOS - armrot.s contains ARM code
 * Copyright (C) 1993-2001 Olly Betts
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

#define Y_UP 1 /* Arc has Y increasing up screen */

#define FIXED_PT_POS   16

#define RED_3D          9   /* colours for 3d view effect */
#define GREEN_3D        8
#define BLUE_3D         6

#define RETURN_KEY     13   /* key definitions */
#define ESCAPE_KEY     27
#define DELETE_KEY    127
#define END_KEY       135   /* marked COPY on older Acorn machines */
#define CURSOR_LEFT   136
#define CURSOR_RIGHT  137
#define CURSOR_DOWN   138
#define CURSOR_UP     139

/* #define SHIFT_3       0x9C */

#define MOVE 4 /* OS_Plot,4,x,y moves to (x,y) */
#define DRAW 5 /* OS_Plot,5,x,y draws to (x,y) */
#define STOP 0
