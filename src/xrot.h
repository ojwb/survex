/* > xrot.h
 * header for caverot plot & translate routines for Unix + X
 * Copyright (C) 1997-2000 Olly Betts
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

#if 0
# include "grx20.h"
# define cleardevice() GrClearContext(0)
/* FIXME: 15 and 0 in next line are colours */
# define outtextxy(X, Y, S) GrTextXY((X), (Y), (S), 15, 0)
# define set_tcolour(X) NOP /* FIXME */
# define set_gcolour(X) NOP /* FIXME */
# define text_xy(X, Y, S) outtextxy(12 + (X) * 12, 12 + (Y) * 12, S)
#endif

#define Y_UP

#define FIXED_PT_POS     16 /* position of fixed decimal point in word */

#define RED_3D           (9)  /* colours for 3d view effect */
#define GREEN_3D         (8)
#define BLUE_3D          (6)

#define RETURN_KEY      (13)  /* key definitions */
#define ESCAPE_KEY      (27)
/*#if 0*/
#define DELETE_KEY   (0x153)  /* enhanced DOS key codes (+ 0x100) */
#define COPYEND_KEY  (0x14f)  /* to distinguish them from normal key codes */
#define CURSOR_LEFT  (0x14b)
#define CURSOR_RIGHT (0x14d)
#define CURSOR_DOWN  (0x150)
#define CURSOR_UP    (0x148)
/*#endif*/
