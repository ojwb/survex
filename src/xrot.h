/* xrot.h
 * header for caverot plot & translate routines for Unix + X
 * Copyright (C) 1997-2001 Olly Betts
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

#define Y_UP

#define FIXED_PT_POS     16 /* position of fixed decimal point in word */

#define RED_3D           9  /* colours for 3d view effect */
#define GREEN_3D         8
#define BLUE_3D          6

#define RETURN_KEY      13  /* key definitions */
#define ESCAPE_KEY      27

#define DELETE_KEY   (0x100 | KEY_DEL)
#define END_KEY      (0x100 | KEY_END)
#define CURSOR_LEFT  (0x100 | KEY_LEFT)
#define CURSOR_RIGHT (0x100 | KEY_RIGHT)
#define CURSOR_DOWN  (0x100 | KEY_DOWN)
#define CURSOR_UP    (0x100 | KEY_UP)
