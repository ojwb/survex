/* > dosrot.h
 * header for caverot plot & translate routines for MS-DOS
 * Copyright (C) 1993-1997 Olly Betts
 * Also for the Atari ST (unfinished)
 */

/*
1993.06.12 first attempt at nastier but probably faster version
1993.06.13 got new, faster version to compile under DOS
1993.06.16 added NO_SETJMP to compile old version if there's no setjmp.h
           moved MOVE, DRAW and STOP #defines here
1993.08.02 split off "dosrot.c"
1993.08.11 moved comment
           added stop() prototype and #include <graphics.h>
           made stop() far
1993.08.13 added NO_FUNC_PTRS
1993.08.14 tidied up after debugging sesh with MF
1994.01.17 hacking for msc
1994.01.18 ditto
1994.02.06 tidy up msc hacks for release
1994.03.19 added crosses
1994.03.29 removed draw_cross() and moved plot*() to caverot.h
1994.03.30 moved DOS specific stuff from caverot.h to here
           drawcross reference removed
1994.04.06 added comment about UK keyboards
1994.04.10 added TOS
1994.04.13 moved cleardevice and outtextxy macros here
1994.04.14 removed CROSS
1994.04.16 use BLK()
1995.03.17 GRX-ed
1995.03.24 GRX text added
1995.04.20 grx.h -> grx20.h (Csaba renamed it)
1995.10.10 __GO32__ -> __DJGPP__ ; removed path from grx20.h include
1996.01.20 added JLIB support
1997.01.30 kludged in colour for JLIB
1998.03.22 autoconf-ed
*/

#if (OS==MSDOS)
# ifdef MSC
#  include <graph.h>
#  define cleardevice() _clearscreen(_GCLEARSCREEN)
#  define outtextxy(X,Y,SZ) BLK(_moveto(X,Y);_outgtext(SZ);)
#  define set_tcolour(X) _setcolor(X)
#  define set_gcolour(X) _setcolor(X)
#  define text_xy(X,Y,S) outtextxy((X)*12,(Y)*12,S)
# elif defined(JLIB)
#  include "jlib.h"
#  define cleardevice() buff_clear(BitMap)
#  define outtextxy(X,Y,SZ) buff_draw_string(BitMap,(SZ),(X),(Y),15); /*15 is colour !HACK!*/
#  define set_tcolour(X) /* !!HACK!! */
#  define set_gcolour(X) BLK( extern int colDraw; colDraw = (X); )
#  define text_xy(X,Y,S) outtextxy(12+(X)*12,12+(Y)*12,(S))
#  define far
# elif defined(__DJGPP__)
#  include "grx20.h"
#  define cleardevice() GrClearContext(0)
#  define outtextxy(X,Y,SZ) GrTextXY((X),(Y),(SZ),15,0) /* !HACK! 14 and 0 are colours */
#  define set_tcolour(X) /* !!HACK!! */
#  define set_gcolour(X) /* !!HACK!! */
#  define text_xy(X,Y,S) outtextxy(12+(X)*12,12+(Y)*12,S)
# else
#  include <graphics.h>
#  define set_tcolour(X) setcolor(X)
#  define set_gcolour(X) setcolor(X)
#  define text_xy(X,Y,S) outtextxy((X)*12,(Y)*12,S)
# endif
# undef  Y_UP /* PC has increasing Y down screen */
#elif (OS==TOS)
# include <vdi.h>
extern struct param_block vdi_ptr;
# define Y_UP
#endif

typedef long coord;  /* data type used after data is read in */

#define COORD_MAX   LONG_MAX
#define COORD_MIN   LONG_MIN

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
#define SHIFT_QUOTE      '@'  /* UK keyboards only I think !HACK! */
#define QUOTE           (39)

#ifdef NO_FUNC_PTRS

/* really plebby version (needed if fn pointers won't fit in a coord) */
# define MOVE  (coord)1
# define DRAW  (coord)2
# define STOP  (coord)0

#elif defined(HAVE_SETJMP)

/* for speed, store function ptrs in table */

# ifdef MSC
#  define MOVE  (coord)(_moveto)
#  define DRAW  (coord)(_lineto)
# else
#  define MOVE  (coord)(moveto)
#  define DRAW  (coord)(lineto)
# endif
# define STOP  (coord)(stop)

/* prototype so STOP macro will work */
extern void stop( int X, int Y );

#else

/* for speed, store function ptrs in table */

# ifdef MSC
#  define MOVE  (coord)(_moveto)
#  define DRAW  (coord)(_lineto)
# else
#  define MOVE  (coord)(moveto)
#  define DRAW  (coord)(lineto)
# endif
# define STOP  (coord)NULL

#endif

#ifdef __DJGPP__
#ifdef JLIB
extern buffer_rec *BitMap;
#endif
extern void moveto( int X, int Y );
extern void lineto( int X, int Y );
#endif
