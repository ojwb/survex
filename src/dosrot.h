/* > dosrot.h
 * header for caverot plot & translate routines for MS-DOS
 * Copyright (C) 1993-2000 Olly Betts
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

#define FIXED_PT_POS     16 /* position of fixed decimal point in word */

#define RED_3D           (9)  /* colours for 3d view effect */
#define GREEN_3D         (8)
#define BLUE_3D          (6)

#define RETURN_KEY      (13)  /* key definitions */
#define ESCAPE_KEY      (27)
#define DELETE_KEY   (0x153)  /* enhanced DOS key codes (+ 0x100) */
#define COPYEND_KEY  (0x14f)  /* to distinguish them from normal key codes */
#define CURSOR_LEFT  (0x14b)
#define CURSOR_RIGHT (0x14d)
#define CURSOR_DOWN  (0x150)
#define CURSOR_UP    (0x148)

#ifdef NO_FUNC_PTRS

/* really plebby version (needed if fn pointers won't fit in a coord) */
# define MOVE (coord)1
# define DRAW (coord)2
# define STOP (coord)0

#elif defined(HAVE_SETJMP)

/* for speed, store function ptrs in table */

# define MOVE (coord)(cvrotgfx_moveto)
# define DRAW (coord)(cvrotgfx_lineto)
# define STOP (coord)(stop)

/* prototype so STOP macro will work */
extern void stop(int X, int Y);

#else

/* for speed, store function ptrs in table */

# define MOVE (coord)(cvrotgfx_moveto)
# define DRAW (coord)(cvrotgfx_lineto)
# define STOP (coord)NULL

#endif
    
