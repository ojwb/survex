/* > armrot.h
 * Caverot plot & translate headers for RISCOS - armrot.s contains ARM code
 * Copyright (C) 1993,1994,1995,1997 Olly Betts
 */

#define Y_UP 1 /* Arc has Y increasing up screen */

typedef int coord;  /* type of data used once data is read in */

#define COORD_MAX   INT_MAX
#define COORD_MIN   INT_MIN

#define FIXED_PT_POS   16

#define RED_3D          9   /* colours for 3d view effect */
#define GREEN_3D        8
#define BLUE_3D         6

#define RETURN_KEY     13   /* key definitions */
#define ESCAPE_KEY     27
#define DELETE_KEY    127
#define COPYEND_KEY   135
#define CURSOR_LEFT   136
#define CURSOR_RIGHT  137
#define CURSOR_DOWN   138
#define CURSOR_UP     139

#define QUOTE          39
#define SHIFT_QUOTE   '\"'
#define SHIFT_3       0x9C

#define MOVE 4 /* OS_Plot,4,x,y moves to (x,y) */
#define DRAW 5 /* OS_Plot,5,x,y draws to (x,y) */
#define STOP 0
