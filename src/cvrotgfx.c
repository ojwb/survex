/* cvrotgfx.c */

/* Copyright (C) Olly Betts 1997-2001
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "debug.h"
#include "message.h"

#include "cvrotgfx.h"

int colText, colHelp;
static int fSwapScreen;

volatile bool fRedraw = fTrue;

int mouse_buttons = -3; /* special value => "not yet initialised" */
static int bank = 0;

/* number of screen banks to use (only used for RISCOS currently) */
static int nBanks = 3;

int _cvrotgfx_textcol, _cvrotgfx_drawcol;

#if (OS==MSDOS) || (OS==UNIX) || (OS == WIN32)

# ifdef MSC
/* Microsoft C */

# elif defined(ALLEGRO)

/* Turn off all the sound, midi, and joystick drivers, since we don't
 * use them */

BEGIN_DIGI_DRIVER_LIST
END_DIGI_DRIVER_LIST

BEGIN_MIDI_DRIVER_LIST
END_MIDI_DRIVER_LIST

BEGIN_JOYSTICK_DRIVER_LIST
END_JOYSTICK_DRIVER_LIST

BEGIN_COLOR_DEPTH_LIST
#if (OS==UNIX)
COLOR_DEPTH_16
#else
COLOR_DEPTH_8
#endif
END_COLOR_DEPTH_LIST

/* Allegro (with DJGPP, Unix, or mingw) */
BITMAP *BitMap, *BitMapDraw;

static
void set_gui_colors(void)
{
   static RGB black = { 0,  0,  0  };
   static RGB grey  = { 48, 48, 48 };
   static RGB white = { 63, 63, 63 };

   set_color(0, &black);
   set_color(16, &black);
   set_color(1, &grey);
   set_color(255, &white);

   gui_fg_color = 0;
   gui_bg_color = 1;
}

# else
/* Borland C */

# endif

# ifdef NO_MOUSE_SUPPORT
/* cvrotgfx_read_mouse() #define-d in cvrotgfx.h */
# elif defined(ALLEGRO)

void
cvrotgfx_read_mouse(int *pdx, int *pdy, int *pbut) {
#if 1 /*(OS==MSDOS)*/
   extern int xcMac, ycMac;
   *pbut = mouse_b;
   *pdx = mouse_x - xcMac / 2;
   *pdy = ycMac / 2 - mouse_y; /* mouse y goes the wrong way */
   position_mouse(xcMac / 2, ycMac / 2);
# if OS==MSDOS
   *pdx = *pdx * 3 / 2;
   *pdy = *pdy * 3 / 2;
# endif
#else
   static int x_old, y_old;
   *pbut = mouse_b;
   *pdx = mouse_x - x_old;
   *pdy = y_old - mouse_y; /* mouse y goes the wrong way */
   x_old = mouse_x;
   y_old = mouse_y;
#endif
}

# else

/* prototype of function from mouse.lib */
extern void cmousel(int*, int*, int*, int*);

/* for Microsoft mouse driver */
static int
init_ms_mouse(void)
{
   unsigned long vector;
   unsigned char byte;
   union REGS inregs, outregs;
   struct SREGS segregs;        /* nasty grubby structs to hold reg values */

   int cmd, cBut, dummy; /* parameters to pass to cmousel() */

   inregs.x.ax = 0x3533; /* get interrupt vector for int 33 */
   intdosx(&inregs, &outregs, &segregs);
   vector = (((unsigned long)segregs.es) << 16) + (unsigned long)outregs.x.bx;

   /* vector empty */
   if (vector == 0ul) return -2; /* No mouse driver */

   byte = (unsigned char)*(long far*)vector;

   /* vector -> iret */
   if (byte == 0xcf) return -2; /* No mouse driver */

   cmd = 0; /* check mouse & mouse driver working */
   cmousel(&cmd, &cBut, &dummy, &dummy);
   return (cmd == -1) ? cBut : -1; /* return -1 if mouse dead */
}

void
cvrotgfx_read_mouse(int *pdx, int *pdy, int *pbut)
{
   int cmd, dummy;
   cmd = 3;  /* read mouse buttons */
   cmousel(&cmd, pbut, pdx, pdy);
   cmd = 11; /* read 'mickeys' ie change in mouse X and Y posn */
   cmousel(&cmd, &dummy, pdx, pdy);
   *pdy = -*pdy; /* mouse coords in Y up X right please */
}

# endif

/*****************************************************************************/

int cvrotgfx_mode_picker = 0;
#if (OS==WIN32)
int cvrotgfx_window = 0;
#endif
# if 0
int driver = GFX_AUTODETECT;
int drx = 800;
int dry = 600;
# endif

/* Set up graphics screen/window and initialise any state needed */
int
cvrotgfx_init(void)
{
   extern int xcMac, ycMac;
   extern float y_stretch;
#ifdef MSC
   struct _videoconfig vidcfg;
   /* detect graphics hardware available */
   if (_setvideomode(_MAXRESMODE) == 0)
      fatalerror(/*Error initialising graphics card*/81);
   _getvideoconfig( &vidcfg );
   xcMac = vidcfg.numxpixels;
   ycMac = vidcfg.numypixels;
   /* defaults to make code shorter */
   colText = colHelp = 1;
   _cvrotgfx_drawcol = (vidcfg.numcolors > 2) ? 2 : 1;
   fSwapScreen = (vidcfg.numvideopages > 1);
   y_stretch *= (float)(((float)xcMac / ycMac) * (350.0 / 640.0) * 1.3);
# elif defined(ALLEGRO)
   int res;
   int c, w, h;

   void setup_mouse(void) {
      mouse_buttons = install_mouse(); /* returns # of buttons or -1 */
      if (mouse_buttons >= 0) {
	 int dummy;
	 /* skip the glitch we cause in the first reading */
	 cvrotgfx_read_mouse(&dummy, &dummy, &dummy);
      }
   }

   void force_redraw(void) {
      fRedraw = fTrue;
   }

   char *p, *q;

   /* set language for Allegro messages */
   p = osmalloc(10 + strlen(msg_lang));
   sprintf(p, "language=%s", msg_lang);
   q = strchr(p, '-');
   if (q) *q = 0;
   set_config_data(p, strlen(p));
   osfree(p);

   /* On platforms where keyboard_needs_poll() returns TRUE we need to use:
    *    #define shift_pressed() (poll_keyboard(), key_shifts & KB_SHIFT_FLAG)
    * Ensure that this isn't one of those platforms...
    */
   ASSERT(!keyboard_needs_poll());

   res = allegro_init();
   /* test for res != 0, but never the case ATM */
   if (res) {
      allegro_exit();
      printf("bad allegro_init\n");
      printf("%s\n", allegro_error);
      exit(1);
   }

#if (OS!=MSDOS)
   set_window_title("caverot");
#endif

   if (!cvrotgfx_mode_picker) {
#if (OS==WIN32)
      if (cvrotgfx_window) {
	  /* this doesn't work (tested on NT4) */
	  res = set_gfx_mode(GFX_DIRECTX_OVL, 640, 480, 0, 0);
	  if (res) res = set_gfx_mode(GFX_DIRECTX_WIN, 640, 480, 0, 0);
	  if (res) res = set_gfx_mode(GFX_GDI, 640, 480, 0, 0);
      } else {
	  res = set_gfx_mode(GFX_DIRECTX, 800, 600, 0, 0);
	  if (res) res = set_gfx_mode(GFX_DIRECTX_SOFT, 800, 600, 0, 0);
      }
#else
#if (OS==UNIX)
      set_color_depth(16);
#endif
      /* Could do this: set_color_depth(GFX_SAFE_DEPTH); but we'd need to
       * cope with different colour depths, and 256 colours seems to work
       * in most cases. */
      if (os_type == OSTYPE_WINNT) {
	  /* In DOS under Windows NT we can't do better than this */
	  res = set_gfx_mode(GFX_VGA, 320, 200, 0, 0);
      } else {
	  res = set_gfx_mode(GFX_AUTODETECT, 800, 600, 0, 0);
      }
#endif
      /* if we couldn't get that mode, give the mode picker */
      if (res) cvrotgfx_mode_picker = 1;
   }

   if (cvrotgfx_mode_picker) {
      do {
	 res = set_gfx_mode(GFX_SAFE, GFX_SAFE_W, GFX_SAFE_H, 0, 0);
	 if (res) {
	    allegro_exit();
	    printf("bad mode select 0\n");
	    printf("%s\n", allegro_error);
	    exit(1);
	 }
	 setup_mouse();
	 install_keyboard();
	 clear(screen);
	 install_timer();
	 set_gui_colors();

	 if (!gfx_mode_select(&c, &w, &h)) {
	    allegro_exit();
	    printf("bad mode select\n");
	    printf("%s\n", allegro_error);
	    exit(1);
	 }
	 remove_timer();
	 remove_mouse();
	 remove_keyboard();
	 /*   set_palette(desktop_palette); */
	 /* try and select the requested resolution - retry if not valid */
	 res = set_gfx_mode(c, w, h, 0, 0);
      } while (res);
   }
   text_mode(-1); /* don't paint in text background */
   BitMap = create_bitmap(SCREEN_W, SCREEN_H);
   /* check that initialisation was OK */
   if (BitMap == NULL) {
      allegro_exit();
      fatalerror(/*Error initialising graphics card*/81);
   }

   /* re-setup mouse to make sure it picks up the correct screen size */
   setup_mouse();
   install_keyboard();

   colText = 255;
   colHelp = 222;
   _cvrotgfx_drawcol = 1;
   xcMac = SCREEN_W;
   ycMac = SCREEN_H;
   y_stretch *= (float)(((float)xcMac / ycMac) * (350.0 / 640.0) * 1.3);
   fSwapScreen = 1;
   {
      RGB temp;
      int col;
      for (col = 255; col >= 0; col--) {
	 temp.r = (col & 7) << 3; /* range 0-63 */
	 temp.g = (col & (7 << 3)); /* range 0-63 */
	 temp.b = (col & (3 << 6)) >> 3; /* range 0-63 */
	 set_color(col, &temp);
      }
   }
/*   set_mouse_range(INT_MIN, INT_MIN, INT_MAX, INT_MAX); */
   select_charset(CHARSET_UTF8);

   if (get_display_switch_mode() == SWITCH_AMNESIA)
      set_display_switch_callback(SWITCH_IN, force_redraw);

# else
   int gdriver, gmode, errorcode;
   /* detect graphics hardware available */
   detectgraph(&gdriver, &gmode);
   /* read result of detectgraph call */
   errorcode = graphresult();
   if (errorcode != grOk) {
      /* something went wrong */
      puts(grapherrormsg(errorcode));
      fatalerror(/*Error initialising graphics card*/81);
   }
   /* defaults to make code shorter */
   colText = colHelp = 1;
   _cvrotgfx_drawcol = 2;
   fSwapScreen = fFalse;
   /* gdriver=EGA64;*/ /******************************************************/
   switch (gdriver) {
    case CGA:
      if (gmode < CGAHI) {
	 gmode = CGAC1; /* choose palette with white */
	 colText = colHelp = CGA_WHITE;
      }
      break;
    case MCGA: case ATT400: /* modes are identical */
      if (gmode < MCGAMED) {
	 gmode = MCGAC1; /* choose palette with white */
	 colText = colHelp = CGA_WHITE;
      }
      break;
    case EGA:
      colText = colHelp = EGA_WHITE; /* was 15, but EGA_WHITE is 63 */
      fSwapScreen = 1;
      break;
    case EGA64:
      if (gmode == EGA64LO)
	 colText = colHelp = EGA_WHITE; /* was 15, but EGA_WHITE is 63 */
      else
	 colText = colHelp = 3;
      break;
    case EGAMONO: case HERCMONO:
      fSwapScreen = 1; /* won't work on 64k Mono EGA cards (only 256k ones) */
      break;
    case VGA:
      if (gmode == VGAHI)
# ifdef USE_VGAHI
	fSwapScreen = fFalse;
# else /* !USE_VGAHI */
        gmode = VGAMED; /* because VGAHI has only one bank */
# endif /* ?USE_VGAHI */
      colText = colHelp = 15;
      fSwapScreen = 1;
      break;
    case PC3270:
      break;
    case IBM8514:
      /* total guess - don't have access to an IBM8514, but then I suspect
       * nobody else does either. */
      colText = colHelp = 255;
      break;
    default:
      /* unknown graphics card */
      puts(grapherrormsg(errorcode));
      fatalerror(/*Error initialising graphics card*/81);
   }
   /* initialize graphics and global variables */
   initgraph(&gdriver, &gmode, msg_cfgpth());

   /* read result of initgraph call */
   errorcode = graphresult();
   if (errorcode != grOk) {
      /* something wrong: no .bgi file? */
      puts(grapherrormsg(errorcode));
      fatalerror(/*Error initialising graphics card*/81);
   }

   xcMac = getmaxx();
   ycMac = getmaxy();
   y_stretch *= (float)(((float)xcMac / ycMac) * (350.0 / 640.0)
			* 0.751677852);
#endif
#ifdef NO_MOUSE_SUPPORT
   mouse_buttons = -2;
#elif defined(ALLEGRO)
   /* mouse initialised above */
#else
   mouse_buttons = init_ms_mouse();
#endif
   return 1;
}

/* Called before starting to draw a fresh image */
/* Typically this might set the bitmap we draw to and clear it */
int
cvrotgfx_pre_main_draw(void)
{
#if defined(ALLEGRO)
   if (fSwapScreen) { /* bank swappin' */
      BitMapDraw = BitMap;
   } else {
      BitMapDraw = screen;
   }
   clear(BitMapDraw);
#elif defined(MSC)
   _setactivepage(bank);
   _clearscreen(_GCLEARSCREEN);
#else
   setactivepage(bank);
   cleardevice();
#endif
#if 0
   /* palette switchin' would look something like this */
   int bank_old = bank;
   bank ^= 1;
   setwritemode(XOR_PUT);
   setpalette(bank_old + 1, 1);
   setpalette(bank + 1, 0);
#endif
   return 1;
}

/* Called after drawing an initial image */
/* Typically this might display the bitmap just drawn */
int
cvrotgfx_post_main_draw(void)
{
#if defined(ALLEGRO)
   if (fSwapScreen) {
      extern int xcMac, ycMac;
      blit(BitMap, screen, 0, 0, 0, 0, xcMac, ycMac);
   }
#elif defined(MSC)
   _setvisualpage(bank);
   bank ^= 1;
#else
   setvisualpage(bank);
   bank ^= 1;
#endif
   return 1;
}

/* Called before starting to add to an existing image */
/* Typically this might set drawing to the screen */
int
cvrotgfx_pre_supp_draw(void)
{
#ifdef ALLEGRO
   BitMapDraw = screen;
#endif
   return 1;
}

/* Called after adding to an existing image */
/* Typically this might blit an updated bitmap to the screen */
int
cvrotgfx_post_supp_draw(void)
{
   return 1;
}

/* Switch back to text mode (or close window) */
int
cvrotgfx_final(void)
{
#if defined(ALLEGRO)
#elif defined(MSC)
#else
   closegraph(); /* restore screen on exit */
#endif
   return 1;
}

#ifdef ALLEGRO
/* returns a keycode - if enhanced keycode then 0x100 added; -1 if none */
int
cvrotgfx_get_key(void)
{
   int keycode;
   if (!keypressed()) return -1; /* -1 => no key pressed */
   keycode = readkey();
   /* flush the keyboard buffer to stop key presses backing up */
   clear_keybuf();
#if 0
   {FILE*f=fopen("key.log","a");if(f){fprintf(f,"%04x\n",keycode);fclose(f);}}
#endif
   /* check for cursor keys/delete/end - these give enhanced DOS key codes (+ 0x100) */
   if (keycode && (keycode & 0xff) == 0) return (keycode >> 8) | 0x100;
   /* otherwise throw away scan code */
   return keycode & 0xff;
}
#else
#if 1
/* returns a keycode - if enhanced keycode then 0x100 added; -1 if none */
int
cvrotgfx_get_key(void)
{
   int keycode;
   if (!_bios_keybrd(_KEYBRD_READY))
      return -1; /* -1 => no key pressed */
   keycode = _bios_keybrd(_KEYBRD_READ);
   if (keycode & 0xff == 0) /* if enhanced key_code add 0x100 */
      keycode = keycode >> 8 | 0x100;
   /* flush the keyboard buffer to stop key presses backing up */
   while (_bios_keybrd(_KEYBRD_READY)) _bios_keybrd(_KEYBRD_READ);
   return keycode;
}
#else
/* returns a keycode - if enhanced keycode then 0x100 added; -1 if none */
int
cvrotgfx_get_key(void)
{
   int keycode;
   if (!kbhit())
      return -1; /* -1 => no key pressed */
   keycode = getch();
   if (keycode == 0) /* if enhanced key_code add 0x100 */
      keycode = getch() + 0x100;
   /* flush the keyboard buffer to stop key presses backing up */
   /* NB: make sure we swallow both bytes of an extended keypress */
   while (kbhit())
      if (getch() == 0) getch();
   return keycode;
}
#endif
#endif

#ifdef ALLEGRO
static int last_x = 0, last_y = 0;
extern void
cvrotgfx_moveto(int X, int Y)
{
   last_x = X;
   last_y = Y;
}

extern void
cvrotgfx_lineto(int X, int Y)
{
   line(BitMapDraw, last_x, last_y, X, Y, _cvrotgfx_drawcol);
   last_x = X; last_y = Y;
}
#endif

#elif (OS==RISCOS)

static int mode_on_entry;

static int xcMacOS, ycMacOS; /* limits for OS_Plot */
static int eigX, eigY;

int
cvrotgfx_init(void)
{
   extern void fastline_init(void); /* initialise fast lines routines */
   oswordpointer_bbox_block bbox = {{}, 1, -32768, -32768, 0x7fff, 0x7fff };
   extern float y_stretch;
   extern int xcMac, ycMac;
   int mode, modeAlt, modeBest;
   int scrmem;
   int cPixelsBest, cPixels, cColsBest, cCols;
   static const os_VDU_VAR_LIST(3) var_list = {
      os_VDUVAR_MAX_MODE,
      os_VDUVAR_TOTAL_SCREEN_SIZE,
      -1
   };
   static const os_VDU_VAR_LIST(5) var_list2 = {
      os_MODEVAR_XWIND_LIMIT,
      os_MODEVAR_YWIND_LIMIT,
      os_MODEVAR_XEIG_FACTOR,
      os_MODEVAR_YEIG_FACTOR,
      -1
   };
   int val_list[4]; /* return block used by both of the above */

   mouse_buttons = 3; /* all RISCOS mice should have 3 buttons */
   bank = 1; /* RISCOS numbers screen banks 1, 2, 3, ... */

   os_byte(osbyte_SCREEN_CHAR, 0, 0, NULL, &mode_on_entry);
   os_read_vdu_variables((os_vdu_var_list*)&var_list, val_list);
   scrmem = val_list[1]; /* screen memory size */
   modeBest = -1;
   cPixelsBest = 0;
   cColsBest = 0;
   for (nBanks = 3; modeBest == -1 && nBanks > 1; nBanks--) {
      for (mode = val_list[0]; mode >= 0; mode--) {
	 if (xos_check_mode_valid((os_mode)mode, &modeAlt, NULL, NULL) != NULL)
	   continue;
	 /* modeAlt is -1 for no such mode, -2 for not feasible */
	 if (modeAlt >= 0) {
	    int mode_flags;
	    /* Check we can draw lines (ie graphics, non-teletext, non-gap) */
	    os_read_mode_variable((os_mode)mode, os_MODEVAR_MODE_FLAGS, &mode_flags);
	    if ((mode_flags & 7) == 0) {
	       int screen_size;
	       os_read_mode_variable((os_mode)mode, os_MODEVAR_SCREEN_SIZE, &screen_size);
	       /* check we have enough memory for nBanks banks */
	       if (screen_size * nBanks <= scrmem) {
		  int x, y;
		  os_read_mode_variable((os_mode)mode, os_MODEVAR_XWIND_LIMIT, &x);
		  os_read_mode_variable((os_mode)mode, os_MODEVAR_YWIND_LIMIT, &y);
		  cPixels = x * y;
		  if (cPixels >= cPixelsBest) {
		     os_read_mode_variable((os_mode)mode, os_MODEVAR_NCOLOUR, &cCols);
		     if (
			 (cCols == 63) /* need 256 colours for fastlinedraw */
# if 0
			 (cCols == 1) /* need 2 colours for older version of fastlinedraw */
# endif
			 && (cPixels > cPixelsBest || cCols > cColsBest)) {
			modeBest = mode;
			cPixelsBest = cPixels;
			cColsBest = cCols;
		     }
		  }
	       }
	    }
	 }
      }
   }
   if (modeBest == -1) {
      /* we probably don't have enough video memory assigned... */
      fatalerror(/*Error initialising graphics card*/81);
   }
   xos_set_mode(), xos_writec(modeBest);
   /* defaults: */
   colText = colHelp = cColsBest;
   /* _cvrotgfx_drawcol not used currently */
   _cvrotgfx_drawcol = 2;
   switch (cColsBest) {
    case 1: case 3:
      _cvrotgfx_drawcol = 1; break;
    case 15:
      colText = colHelp = 7; break;
   }
   fSwapScreen = (nBanks > 1);

   /* make cursor keys return values */
   xosbyte_write(osbyte_INTERPRETATION_ARROWS, 1);
   /* make escape like normal keys */
   xosbyte_write(osbyte_VAR_ESCAPE_CHAR, 0);
   /* infinite mouse bounding box */
   xoswordpointer_set_bbox(&bbox);
   /* turn off mouse pointer */
   xosbyte_write(osbyte_SELECT_POINTER, 0);
   /* limits for fastline drawing routines */
   os_read_vdu_variables((os_vdu_var_list*)&var_list2, val_list);
   xcMac = val_list[0] + 1;
   ycMac = val_list[1] + 1;
   eigX = val_list[2];
   eigY = val_list[3];
   /* limits for OS routines (used for text) */
   xcMacOS = xcMac << eigX;
   ycMacOS = ycMac << eigY;
   y_stretch *= (float)(int)(1 << eigX) / (float)(int)(1 << eigY);
   /* set origin to centre of screen +/- adjustment for labels */
   {
      int v;
      xos_set_graphics_origin();
      v = (xcMacOS >> 1) + 8;
      xos_writec(v);
      xos_writec(v >> 8);
      v = (ycMacOS >> 1) + 8;
      xos_writec(v);
      xos_writec(v >> 8);
   }
   /* adjust eig factors for positioning text correctly */
   eigX += 3;
   eigY += 3;

   /* write text at the graphics cursor */
   xos_writec(os_VDU_GRAPH_TEXT_ON);
   fastline_init();
   return 1;
}

/* Called before starting to draw a fresh image */
/* Typically this might set the bitmap we draw to and clear it */
int
cvrotgfx_pre_main_draw(void)
{
   if (fSwapScreen)
      xosbyte_write(osbyte_OUTPUT_SCREEN_BANK, bank); /* drawn bank */
   xos_cls();
   return 1;
}

/* Called after drawing an initial image */
/* Typically this might display the bitmap just drawn */
int
cvrotgfx_post_main_draw(void)
{
   if (fSwapScreen) {
      xosbyte_write(osbyte_DISPLAY_SCREEN_BANK, bank); /* shown bank */
      bank = (bank % nBanks) + 1; /* change written-to bank */
   }
   return 1;
}

/* Called before starting to add to an existing image */
/* Typically this might set drawing to the screen */
int
cvrotgfx_pre_supp_draw(void)
{
   return 1;
}

/* Called after adding to an existing image */
/* Typically this might blit an updated bitmap to the screen */
int
cvrotgfx_post_supp_draw(void)
{
   return 1;
}

/* restore screen & keybd on exit */
int
cvrotgfx_final(void)
{
   /* make cursor keys move cursor */
   xosbyte_write(osbyte_INTERPRETATION_ARROWS, 0);
   /* restore screen access to normal */
   xosbyte_write(osbyte_OUTPUT_SCREEN_BANK, 0);
   xosbyte_write(osbyte_DISPLAY_SCREEN_BANK, 0);
   /* make escape generate escape condition */
   xosbyte_write(osbyte_VAR_ESCAPE_CHAR, 27);
   /* write text at text cursor */
   xos_writec(os_VDU_GRAPH_TEXT_OFF);
   /* and restore screen mode on entry */
   xos_set_mode(), xos_writec(mode_on_entry);
   return 1;
}

/* returns a keycode - if enhanced keycode then 0x100 added; -1 if none */
int
cvrotgfx_get_key(void)
{
   int keycode, r2;
   /* bbc_inkey(0); */
   os_byte(osbyte_IN_KEY, 0, 0, &keycode, &r2);
   osbyte_write(osbyte_FLUSH_BUFFER, 0); /* flush kbd buffer */
   return (r2 == 0xff ? -1 : keycode);
}

/* x and y are in character widths/heights */
/* do something sane for proportional fonts */
/* could be a little neater here... :) */
void
text_xy(int x, int y, const char *str) {
   xos_plot(os_MOVE_TO, (x << eigX) - (xcMacOS >> 1) + 16,
	    (ycMacOS >> 1) - (y << eigY) - 16);
   xos_write0((char *)str);
}

void
cvrotgfx_read_mouse(int *pdx, int *pdy, int *pbut)
{
   static int xOld = 0, yOld = 0;
   int x, y;
   os_mouse(&x, &y, (bits*)pbut, NULL);
   *pdx = (short)x - (short)xOld;
   *pdy = (short)y - (short)yOld;
   xOld = x;
   yOld = y;
}

#else
# error Operating System not known
#endif
