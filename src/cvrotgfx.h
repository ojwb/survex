/* cvrotgfx.h */

/* Copyright (C) Olly Betts 1997,2001
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

#ifndef SVX_CVROTGFX_H
#define SVX_CVROTGFX_H 1

#include "whichos.h"
#include "ostypes.h"

extern int mouse_buttons;

extern int colText, colHelp;

extern int _cvrotgfx_textcol, _cvrotgfx_drawcol;

extern volatile bool fRedraw;

#ifdef __DJGPP__
# define ALLEGRO 1
#endif

int cvrotgfx_init(void);
int cvrotgfx_pre_main_draw(void);
int cvrotgfx_post_main_draw(void);
int cvrotgfx_pre_supp_draw(void);
int cvrotgfx_post_supp_draw(void);
int cvrotgfx_final(void);
int cvrotgfx_get_key(void);
void cvrotgfx_read_mouse(int *pdx, int *pdy, int *pbut);
void (cvrotgfx_beep)(void); /* make a beep */
void cvrotgfx_moveto(int X, int Y);
void cvrotgfx_lineto(int X, int Y);

#if (OS==UNIX)

/* UNIX + Allegro */
# include "allegro.h"
# define ALLEGRO 1
extern BITMAP *BitMap, *BitMapDraw;

# define CVROTGFX_LBUT 1 /* mask for left mouse button */
# define CVROTGFX_RBUT 2 /* mask for right mouse button (if present) */
# define CVROTGFX_MBUT 4 /* mask for middle mouse button (if present) */

# define cvrotgfx_beep() NOP /* no easy way I know to beep from an X app */

# ifdef NO_MOUSE_SUPPORT
/* emulate a dead mouse */
#  define cvrotgfx_read_mouse(PDX, PDY, PBUT) NOP
# endif

# ifdef NO_TEXT
#  define outtextxy(X, Y, S) NOP
# else
#  define outtextxy(X, Y, S) \
 textout(BitMapDraw, font, (char *)(S), (X), (Y), _cvrotgfx_textcol)
# endif
# define set_tcolour(X) _cvrotgfx_textcol = (X)
# define set_gcolour(X) _cvrotgfx_drawcol = (X)
# define text_xy(X, Y, S) outtextxy(12 + (X) * 12, 12 + (Y) * 12, (char *)(S))
/* # define shift_pressed() (key[KEY_LSHIFT] || key[KEY_RSHIFT]) alternative... */
# define shift_pressed() (key_shifts & KB_SHIFT_FLAG)
# define ctrl_pressed() (key_shifts & KB_CTRL_FLAG)

#elif (OS==MSDOS)

/* DOS-specific I/O functions */
# include <bios.h>
# include <conio.h>
# include <dos.h>

# ifdef MSC
/* Microsoft C */
#  include <graph.h>

# elif defined(ALLEGRO)
/* DJGPP + Allegro */
#  include "allegro.h"
extern BITMAP *BitMap, *BitMapDraw;

# else
/* Borland C */
#  include <graphics.h>

# endif

# ifdef NO_MOUSE_SUPPORT
/* emulate a dead mouse */
#  define cvrotgfx_read_mouse(PDX, PDY, PBUT) NOP
# endif

# define CVROTGFX_LBUT 1 /* mask for left mouse button */
# define CVROTGFX_RBUT 2 /* mask for right mouse button (if present) */
# define CVROTGFX_MBUT 4 /* mask for middle mouse button (if present) */

# define cvrotgfx_beep() sound(256) /* 256 is frequency */

# ifdef MSC
#  define shift_pressed() (_bios_keybrd(_KEYBRD_SHIFTSTATUS) & 0x03)
/* Guess: #  define ctrl_pressed() (_bios_keybrd(_KEYBRD_SHIFTSTATUS) & 0x0c) */
# elif defined(ALLEGRO)
#  define shift_pressed() (key_shifts & KB_SHIFT_FLAG)
#  define ctrl_pressed() (key_shifts & KB_CTRL_FLAG)
# else
#  define R_SHIFT 0x01
#  define L_SHIFT 0x02
/* Guesses:
#  define R_CTRL  0x04
#  define L_CTRL  0x08
*/
#  ifndef _KEYBRD_SHIFTSTATUS
#   define _KEYBRD_SHIFTSTATUS 2 /* for DJGPP */
#  endif
/* use function 2 to determine if shift keys are depressed */
#  define shift_pressed() (bioskey(_KEYBRD_SHIFTSTATUS) & (R_SHIFT | L_SHIFT))
#  define ctrl_pressed() (bioskey(_KEYBRD_SHIFTSTATUS) & (R_CTRL | L_CTRL))
# endif

# ifdef MSC
#  define cvrotgfx_moveto _moveto
#  define cvrotgfx_lineto _lineto
# elif !defined(__DJGPP__)
#  define cvrotgfx_moveto moveto
#  define cvrotgfx_lineto lineto
# else
void cvrotgfx_moveto(int X, int Y);
void cvrotgfx_lineto(int X, int Y);
# endif

# ifdef MSC
#  define outtextxy(X, Y, S) BLK(cvrotgfx_moveto(X, Y); _outgtext(S);)
#  define set_tcolour(X) _setcolor(X)
#  define set_gcolour(X) _setcolor(X)
#  define text_xy(X, Y, S) outtextxy((X) * 12, (Y) * 12, S)
# elif defined(ALLEGRO)
#  ifdef NO_TEXT
#   define outtextxy(X, Y, S) NOP
#  else
#  define outtextxy(X, Y, S) \
 textout(BitMapDraw, font, (char *)(S), (X), (Y), _cvrotgfx_textcol)
#  endif
#  define set_tcolour(X) _cvrotgfx_textcol = (X)
#  define set_gcolour(X) _cvrotgfx_drawcol = (X)
#  define text_xy(X, Y, S) outtextxy(12 + (X) * 12, 12 + (Y) * 12, S)
# else
#  define set_tcolour(X) setcolor(X)
#  define set_gcolour(X) setcolor(X)
#  define text_xy(X, Y, S) outtextxy((X) * 12, (Y) * 12, S)
# endif
# undef  Y_UP /* PC has increasing Y down screen */

#elif (OS==RISCOS)

# undef bool /* bloody namespace polluting libraries */
# include "oslib/os.h"
# include "oslib/osbyte.h"
# include "oslib/osword.h"
# define bool SVXBOOL

void outtextxy(int x, int y, const char *str);
void text_xy(int x, int y, const char *str);

# define set_gcolour(X) (ol_setcol((X)))
# define set_tcolour(X) (xos_set_gcol(), xos_writec(0), xos_writec((X)))
# define shift_pressed() (osbyte_read(osbyte_VAR_KEYBOARD_STATE) & 0x08)
# define ctrl_pressed() (osbyte_read(osbyte_VAR_KEYBOARD_STATE) & 0x40)

# define cvrotgfx_beep() xos_bell()

# define CVROTGFX_LBUT 4
# define CVROTGFX_MBUT 2
# define CVROTGFX_RBUT 1

#elif (OS==WIN32)

/* mingw + Allegro */
# define ALLEGRO 1

# include "allegro.h"
/* int __stdcall AllocConsole(void); */
extern BITMAP *BitMap, *BitMapDraw;

# define cvrotgfx_beep() NOP /*sound(256)*/ /* 256 is frequency */

# define shift_pressed() (key_shifts & KB_SHIFT_FLAG)
# define ctrl_pressed() (key_shifts & KB_CTRL_FLAG)

void cvrotgfx_moveto(int X, int Y);
void cvrotgfx_lineto(int X, int Y);

# ifdef NO_TEXT
#  define outtextxy(X, Y, S) NOP
# else
#  define outtextxy(X, Y, S) \
 textout(BitMapDraw, font, (char *)(S), (X), (Y), _cvrotgfx_textcol)
# endif
# define set_tcolour(X) _cvrotgfx_textcol = (X)
# define set_gcolour(X) _cvrotgfx_drawcol = (X)
# define text_xy(X, Y, S) outtextxy(12 + (X) * 12, 12 + (Y) * 12, S)

# undef  Y_UP /* PC has increasing Y down screen */

#  define CVROTGFX_LBUT 1 /* mask for left mouse button */
#  define CVROTGFX_RBUT 2 /* mask for right mouse button (if present) */
#  define CVROTGFX_MBUT 4 /* mask for middle mouse button (if present) */

#else
# error Operating System not known
#endif

#ifdef ALLEGRO
extern int cvrotgfx_mode_picker;
extern int cvrotgfx_window;

#if (OS!=WIN32)
#define CVROTGFX_LONGOPTS \
   {"mode-picker", no_argument, &cvrotgfx_mode_picker, 1},
#else
#define CVROTGFX_LONGOPTS \
   {"window", no_argument, &cvrotgfx_window, 1},\
   {"mode-picker", no_argument, &cvrotgfx_mode_picker, 1},
#endif

/*				<-- */
#if (OS!=WIN32)
#define CVROTGFX_HELP(N) \
   {HLP_ENCODELONG((N)),	"bring up screen mode chooser dialog"},
#else
#define CVROTGFX_HELP(N) \
   {HLP_ENCODELONG((N)),	"run caverot in a window (default: full-screen)"},\
   {HLP_ENCODELONG((N)+1),	"bring up screen mode chooser dialog"},
#endif
#endif

#ifndef CVROTGFX_LONGOPTS
# define CVROTGFX_LONGOPTS
#endif

#ifndef CVROTGFX_SHORTOPTS
# define CVROTGFX_SHORTOPTS
#endif

#ifndef CVROTGFX_HELP
# define CVROTGFX_HELP(X)
#endif

#endif
