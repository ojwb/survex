/* cvrotgfx.h */

/* Copyright (C) Olly Betts 1997 */

/*
1997.02.24 written
1997.02.26 Allegro work
1997.02.27 moved code here from dos.h; Allegro text added
1997.03.01 more allegro work
1997.05.06 RISCOS code added
1997.05.08 fettled to work on RISCOS again
1997.05.10 protected against multiple inclusion
1997.05.11 better solution to OSLib problems
1998.10.31 xallegro support
*/

#define ALLEGRO 1

#ifndef SVX_CVROTGFX_H
#define SVX_CVROTGFX_H 1

#include "whichos.h"

extern int mouse_buttons;

#if (OS==UNIX)

/* UNIX + Allegro */
# include "allegro.h"
# define ALLEGRO 1
extern BITMAP *BitMap, *BitMapDraw;

# define CVROTGFX_LBUT 1 /* mask for left mouse button */
# define CVROTGFX_RBUT 2 /* mask for right mouse button (if present) */
# define CVROTGFX_MBUT 4 /* mask for middle mouse button (if present) */

extern int cvrotgfx_parse_cmdline( int *pargc, char **argv );
extern int cvrotgfx_init( void );
extern int cvrotgfx_pre_main_draw( void );
extern int cvrotgfx_post_main_draw( void );
extern int cvrotgfx_pre_supp_draw( void );
extern int cvrotgfx_post_supp_draw( void );
extern int cvrotgfx_final( void );
extern int cvrotgfx_get_key( void );
extern void cvrotgfx_read_mouse( int *pdx, int *pdy, int *pbut );
extern void (cvrotgfx_beep)( void ); /* make a beep */
extern void cvrotgfx_moveto( int X, int Y );
extern void cvrotgfx_lineto( int X, int Y );

# define cvrotgfx_beep() NOP /* !HACK! */

# ifdef NO_MOUSE_SUPPORT
/* emulate a dead mouse */
#  define cvrotgfx_read_mouse(PDX,PDY,PBUT) NOP
# endif

/*# define cleardevice() GrClearContext(0)*/
# ifdef NO_TEXT
#  define outtextxy(X,Y,SZ) NOP
# else
#  define outtextxy(X,Y,SZ) BLK(\
 extern int colText;\
 textout( BitMapDraw, font, (SZ), (X), (Y), colText );\
 )
# endif
# define set_tcolour(X) BLK( extern int colText; colText = (X); )
# define set_gcolour(X) BLK( extern int colDraw; colDraw = (X); )
# define text_xy(X,Y,S) outtextxy(12+(X)*12,12+(Y)*12,S)

# define shift_pressed() 0 /* !HACK! */

#elif (OS==MSDOS)

/* DOS-specific I/O functions */
# include <bios.h>
# include <conio.h>
# include <dos.h>

# ifdef MSC
/* Microsoft C */
#  include <graph.h>

# elif defined(JLIB)
/* DJGPP + JLIB */
#  include "jlib.h"
extern buffer_rec *BitMap;

# elif defined(ALLEGRO)
/* DJGPP + Allegro */
#  include "allegro.h"
extern BITMAP *BitMap, *BitMapDraw;

# elif defined(__DJGPP__)
/* DJGPP + GRX */
#  include "grx20.h"
extern GrContext *BitMap;

# else
/* Borland C */
#  include <graphics.h>

# endif

extern int cvrotgfx_parse_cmdline( int *pargc, char **argv );
extern int cvrotgfx_init( void );
extern int cvrotgfx_pre_main_draw( void );
extern int cvrotgfx_post_main_draw( void );
extern int cvrotgfx_pre_supp_draw( void );
extern int cvrotgfx_post_supp_draw( void );
extern int cvrotgfx_final( void );
extern int cvrotgfx_get_key( void );
extern void cvrotgfx_read_mouse( int *pdx, int *pdy, int *pbut );
extern void (cvrotgfx_beep)( void ); /* make a beep */
extern void cvrotgfx_moveto( int X, int Y );
extern void cvrotgfx_lineto( int X, int Y );

# ifdef NO_MOUSE_SUPPORT
/* emulate a dead mouse */
#  define cvrotgfx_read_mouse(PDX,PDY,PBUT) NOP
# endif

# ifdef JLIB
#  define CVROTGFX_LBUT MOUSE_B_LEFT
#  define CVROTGFX_MBUT MOUSE_B_MIDDLE
#  define CVROTGFX_RBUT MOUSE_B_RIGHT
# elif !defined(ALLEGRO) && defined(__DJGPP__)
#  define CVROTGFX_LBUT GR_M_LEFT
#  define CVROTGFX_MBUT GR_M_MIDDLE
#  define CVROTGFX_RBUT GR_M_RIGHT
# else
#  define CVROTGFX_LBUT 1 /* mask for left mouse button */
#  define CVROTGFX_RBUT 2 /* mask for right mouse button (if present) */
#  define CVROTGFX_MBUT 4 /* mask for middle mouse button (if present) */
# endif


# define cvrotgfx_beep() sound(256) /* 256 is frequency */

/* !HACK! fix this stuff */
# ifdef MSC
#  define shift_pressed() (_bios_keybrd(_KEYBRD_SHIFTSTATUS) & 0x03 )
# else
#  define R_SHIFT  0x01
#  define L_SHIFT  0x02
#  ifndef _KEYBRD_SHIFTSTATUS
#   define _KEYBRD_SHIFTSTATUS 2 /* for DJGPP */
#  endif
/* use function 2 to determine if shift keys are depressed */
#  define shift_pressed() (bioskey(_KEYBRD_SHIFTSTATUS) & (R_SHIFT|L_SHIFT) )
# endif

# ifdef MSC
#  define cvrotgfx_moveto(X,Y) _moveto((X),(Y))
#  define cvrotgfx_lineto(X,Y) _lineto((X),(Y))
# elif !defined(__DJGPP__)
#  define cvrotgfx_moveto(X,Y) moveto((X),(Y))
#  define cvrotgfx_lineto(X,Y) lineto((X),(Y))
# else
extern void cvrotgfx_moveto( int X, int Y );
extern void cvrotgfx_lineto( int X, int Y );
# endif

# ifdef MSC
/*# define cleardevice() _clearscreen(_GCLEARSCREEN)*/
#  define outtextxy(X,Y,SZ) BLK(cvrotgfx_moveto(X,Y);_outgtext(SZ);)
#  define set_tcolour(X) _setcolor(X)
#  define set_gcolour(X) _setcolor(X)
#  define text_xy(X,Y,S) outtextxy((X)*12,(Y)*12,S)
# elif defined(JLIB)
/*#  define cleardevice() buff_clear(BitMap)*/
#  define outtextxy(X,Y,SZ) buff_draw_string(BitMap,(SZ),(X),(Y),15); /*15 is colour !HACK!*/
#  define set_tcolour(X) /* !!HACK!! */
#  define set_gcolour(X) BLK( extern int colDraw; colDraw = (X); )
#  define text_xy(X,Y,S) outtextxy(12+(X)*12,12+(Y)*12,(S))
#  define far
# elif defined(ALLEGRO)
/*# define cleardevice() GrClearContext(0)*/
#  ifdef NO_TEXT
#   define outtextxy(X,Y,SZ) NOP
#  else
#   define outtextxy(X,Y,SZ) BLK(\
 extern int colText;\
 textout( BitMapDraw, font, (SZ), (X), (Y), colText );\
 )
#  endif
#  define set_tcolour(X) BLK( extern int colText; colText = (X); )
#  define set_gcolour(X) BLK( extern int colDraw; colDraw = (X); )
#  define text_xy(X,Y,S) outtextxy(12+(X)*12,12+(Y)*12,S)
# elif defined(__DJGPP__)
/*#  define cleardevice() GrClearContext(0)*/
#  define outtextxy(X,Y,SZ) GrTextXY((X),(Y),(SZ),15,0) /* !HACK! 14 and 0 are colours */
#  define set_tcolour(X) /* !!HACK!! */
#  define set_gcolour(X) /* !!HACK!! */
#  define text_xy(X,Y,S) outtextxy(12+(X)*12,12+(Y)*12,S)
# else
#  define set_tcolour(X) setcolor(X)
#  define set_gcolour(X) setcolor(X)
#  define text_xy(X,Y,S) outtextxy((X)*12,(Y)*12,S)
# endif
# undef  Y_UP /* PC has increasing Y down screen */

#elif (OS==RISCOS)
/* # include "bbc.h" */
/* # include "kernel.h" */

# undef bool /* bloody namespace poluting libraries */
# include "oslib/os.h"
# include "oslib/osbyte.h"
# include "oslib/osword.h"
# define bool BOOL

extern void outtextxy( int x, int y, char *str );
extern void text_xy( int x, int y, char *str );

# define set_gcolour(X) (ol_setcol((X)))
# define set_tcolour(X) ( xos_set_gcol(), xos_writec(0), xos_writec((X)) )
/* # define cleardevice() (xos_cls()) */
/* # define shift_pressed() (bbc_inkey(-1)) */
# define shift_pressed() (osbyte_read( osbyte_VAR_KEYBOARD_STATE ) & 0x08)

# define cvrotgfx_beep() xos_bell()

# define CVROTGFX_LBUT 4
# define CVROTGFX_MBUT 1
# define CVROTGFX_RBUT 2

/* !HACK! very crap -- these get set to themselves... */
extern int colText, colDraw;

#else
# error Operating System not known
#endif

#endif
