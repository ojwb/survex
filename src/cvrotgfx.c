/* cvrotgfx.c */

/* Copyright (C) Olly Betts 1997 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "message.h"

/* number of screen banks to use (only used for RISCOS currently) */
#define nBanks 3

#include "cvrotgfx.h"

int colText, colHelp;
static int fSwapScreen;

int mouse_buttons = -3; /*!HACK! special value => "not yet initialised" */
static int bank = 0;

int _cvrotgfx_textcol, _cvrotgfx_drawcol;

#if (OS==MSDOS) || (OS==UNIX)

# ifdef MSC
/* Microsoft C */

# elif defined(JLIB)
/* DJGPP + JLIB */
buffer_rec *BitMap;

# elif defined(ALLEGRO)
/* DJGPP + Allegro (or UNIX+allegro) */
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

# elif defined(__DJGPP__)
/* DJGPP + GRX */
GrContext *BitMap;

# else
/* Borland C */

# endif

# ifdef NO_MOUSE_SUPPORT
/* cvrotgfx_read_mouse() #define-d in cvrotgfx.h */
# elif defined(JLIB)

void
cvrotgfx_read_mouse(int *pdx, int *pdy, int *pbut)
{
   static int x_old, y_old;
   int x_new, y_new;

   mouse_get_status(&x_new, &y_new, pbut);
   *pdx = x_new - x_old;
   *pdy = y_new - y_old;
   x_old = x_new;
   y_old = y_new;
}

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

# elif defined(__DJGPP__)

void
cvrotgfx_read_mouse(int *pdx, int *pdy, int *pbut)
{
   GrMouseEvent event;
   static int x_old, y_old;

   GrMouseGetEventT(GR_M_EVENT, &event, 0L);
   *pdx = event.x - x_old;
   *pdy = y_old - event.y; /* mouse y goes the wrong way */
   x_old = event.x;
   y_old = event.y;
   *pbut = event.buttons;
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
   byte = (unsigned char)*(long far*)vector;
   
   /* vector empty or -> iret */
   if ((vector == 0ul) || (byte == 0xcf)) return -2; /* No mouse driver */

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

static int mode_picker = 0;
# if 0
int driver = GFX_AUTODETECT;
int drx = 800;
int dry = 600;
# endif

/* Called just after message file is read in to allow graphics library */
/* specific command line switches to be processed */
/* You should modify argv and *pargc to remove any switches you process */
int
cvrotgfx_parse_cmdline(int *pargc, char **argv)
{
#ifdef ALLEGRO
   if (*pargc > 2 && (strcmp(argv[*pargc - 1], "--mode-picker") == 0)) {
      mode_picker = 1;
      (*pargc)--;
      argv[*pargc] = NULL;
   }
#if 0 /*def ALLEGRO*/
   if (*pargc > 2 && isdigit(argv[*pargc - 1][0])) {
      char *p = argv[*pargc - 1];
      driver = atoi(p);
      p = strchr(p, ',');
      if (p) drx = atoi(++p);
      p = strchr(p, ',');
      if (p) dry = atoi(++p);
      (*pargc)--;
      argv[*pargc] = NULL;
   }
#endif
#else
   /* avoid compiler warnings */
   pargc = pargc;
   argv = argv;
#endif
   return 1;
}

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
#elif defined(JLIB)
   screen_set_video_mode();
   /* !HACK! this func is intended to remove the pointer to prevent problems */
   /* when plotting over it, and may not actually hide the pointer :(        */
   mouse_hide_pointer();

   screen_put_pal(255, 0xff, 0xff, 0xff); /*!HACK!*/
   screen_put_pal(0, 0, 0, 0); /*!HACK!*/

   /* initialise screen sized buffer */
   BitMap = buff_init(SCREEN_WIDTH, SCREEN_HEIGHT);
   /* check that initialisation was OK */
   if (BitMap == NULL)
      fatalerror(/*Error initialising graphics card*/81);
   colText = colHelp = SCREEN_NUM_COLORS - 1;
   _cvrotgfx_drawcol = 1;
   xcMac = SCREEN_WIDTH;
   ycMac = SCREEN_HEIGHT;
   y_stretch *= (float)(((float)xcMac / ycMac) * (350.0 / 640.0) * 1.3);
   fSwapScreen = 1;
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

   res = allegro_init();
   /* test for res !=0, but never the case ATM */
   if (res) {
      allegro_exit();
      printf("bad allegro_init\n");
      printf("%s\n", allegro_error);
      exit(1);
   }
   
   if (!mode_picker) {
#if (OS==UNIX)
      set_color_depth(16);
#endif
      res = set_gfx_mode(GFX_AUTODETECT, 800, 600, 0, 0);
      /* if we couldn't get that mode, give the mode picker */
      if (res) mode_picker = 1;
   }

   if (mode_picker) {
      do {
	 res = set_gfx_mode(GFX_VGA, 320, 200, 0, 0);
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
#if 0
   /*!HACK! may have "not initialised" problems -- this is a quick fix */
   BitMapDraw = screen;
#endif
#if 0
   /* test res */
   if (res) {
      allegro_exit();
      printf("bad mode select 2\n");
      printf("%s\n", allegro_error);
      exit(1);
   }
#endif
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
#elif defined(__DJGPP__)
   const GrVideoMode *mode;
/*	    GrSetMode(GR_width_height_color_graphics, x, y, c);
 * else if ((x >= 320) && (y >= 200))
 *	    GrSetMode(GR_width_height_graphics, x, y);
 *	else
 */
/* if (!GrSetMode(GR_default_graphics)) */
   if (!GrSetMode(GR_width_height_color_graphics, 800, 600, 256))
      fatalerror(/*Error initialising graphics card*/81);

   GrMouseEraseCursor();

   /* Creates a bitmap 'context' in system memory, allocating memory for us */
   BitMap = GrCreateContext(GrScreenX() + 1, GrScreenY() + 1, NULL, NULL);
   if (!BitMap) exit(1); /* !HACK! */
   mode = GrCurrentVideoMode();
   colText = colHelp = (1 << ((int)(mode->bpp))) - 1;
   _cvrotgfx_drawcol = 1;
   xcMac = mode->width;
   ycMac = mode->height;
   y_stretch *= (float)(((float)xcMac / ycMac) * (350.0 / 640.0) * 1.3);
   fSwapScreen = 1;
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
      colText = colHelp = 255; /* guess city !HACK! */
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
#elif defined(JLIB)
   mouse_buttons = -1;
   /* !HACK! can't distinguish no mouse from no driver */
   if (mouse_present() == MOUSE_PRESENT)
      mouse_buttons = MOUSE_NUM_BUTTONS; /* !HACK! this is max # of buttons */
#elif defined(ALLEGRO)
   /* mouse initialised above */
#elif defined(__DJGPP__)
   mouse_buttons = -1;
   /* !HACK! can't distinguish no mouse from no driver */
   if (GrMouseDetect()) {
      GrMouseEventMode(1);
      GrMouseInit(); /* not actually needed, but call for neatness */
      mouse_buttons = 3; /* !HACK! assume 3 buttons */
   }
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
#if defined(JLIB)
   /* ignore fSwapScreen -- JLIB can't draw directly to screen */
   buff_clear(BitMap);
#elif defined(ALLEGRO)
   if (fSwapScreen) { /* bank swappin' */
      BitMapDraw = BitMap;
   } else {
      BitMapDraw = screen;
   }
   clear(BitMapDraw);
#elif defined(__DJGPP__)
   if (fSwapScreen) { /* bank swappin' */
      GrSetContext(BitMap);
      GrClearContext(0);
   } else {
      GrClearScreen(0);
   }
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
#if defined(JLIB)
   screen_blit_fs_buffer(BitMap);
#elif defined(ALLEGRO)
   if (fSwapScreen) {
      extern int xcMac, ycMac;
      blit(BitMap, screen, 0, 0, 0, 0, xcMac, ycMac);
   }
#elif defined(__DJGPP__)
   if (fSwapScreen) {
# if 1
      GrBitBltNC(GrScreenContext(), 0, 0, BitMap /*NULL*/ /* current */, 0, 0,
		 GrMaxX(), GrMaxY(), GrWRITE);
# else
      GrBitBltNCFS(GrScreenContext(), BitMap /*NULL*/ /* current */);
# endif
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
#elif defined(__DJGPP__) && !defined(JLIB)
   GrSetContext(0); /* write to screen */
#endif
   return 1;
}

/* Called after adding to an existing image */
/* Typically this might blit an updated bitmap to the screen */
int
cvrotgfx_post_supp_draw(void)
{
#if defined(JLIB)
   screen_blit_fs_buffer(BitMap);
#endif
   return 1;
}

/* Switch back to text mode (or close window) */
int
cvrotgfx_final(void)
{
#if defined(JLIB)
   screen_restore_video_mode();
#elif defined(ALLEGRO)
#elif defined(__DJGPP__)
   /* get rid of BitMap */
   /* GrDestroyContext(BitMap); */
   /* GrSetMode(GR_default_text); */
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
   if (!keypressed())
      return -1; /* -1 => no key pressed */
   keycode = readkey();
   /* flush the keyboard buffer to stop key presses backing up */
   clear_keybuf();
#if 0
{FILE *fh = fopen("key.log", "a");
fprintf(fh, "%04x\n", keycode);
fclose(fh);
}
#endif
   /* check for cursor keys/delete/end - these give enhanced DOS key codes (+ 0x100) */
   if (keycode && (keycode & 0xff) == 0) return (keycode >> 8) | 0x100;
   /* otherwise throw away scan code */
   return keycode & 0xff;
}
#else
/* !HACK! use _bios_keybd() instead? */
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

#if 0
void cvrotgfx_beep(void); /* make a beep */
#endif

#if defined(__DJGPP__) || (OS==UNIX)
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
# ifdef JLIB
   buff_draw_line(BitMap, last_x, last_y, X, Y, _cvrotgfx_drawcol);
# elif defined(ALLEGRO)
   line(BitMapDraw, last_x, last_y, X, Y, _cvrotgfx_drawcol);
# else
   GrLine(last_x, last_y, X, Y, 15); /* 15 is colour !HACK! */
# endif
   last_x = X; last_y = Y;
}
#endif

#elif (OS==RISCOS)

static int mode_on_entry;

static int xcMacOS, ycMacOS; /* limits for OS_Plot */
static int eigX, eigY;

/* Called just after message file is read in to allow graphics library */
/* specific command line switches to be processed */
/* You should modify argv and *pargc to remove any switches you process */
int
cvrotgfx_parse_cmdline(int *pargc, char **argv)
{
   /* avoid compiler warnings */
   pargc = pargc;
   argv = argv;
   return 1;
}

int
cvrotgfx_init(void)
{
   extern void fastline_init(void); /* initialise fast lines routines */
   /* FIXME: Acorn C sez 2x "Warning implicit cast (to 'int') overflow": */
   oswordpointer_bbox_block bbox = {{}, 1, 0x8000, 0x8000, 0x7fff, 0x7fff };
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
#if 0
		      (cCols == 1) /* need 2 colours for older version of fastlinedraw */
#endif
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
   fSwapScreen = 1; /* may want to turn off if we haven't enough memory */
/*   swap_screen(1); !HACK! may need to sort this out... */
/*   xos_cls(); */ /* and clear the other bank too... */

   /* make cursor keys return values */
/*   xos_byte(osbyte_INTERPRETATION_ARROWS, 1, 0, NULL, NULL); */
   xosbyte_write(osbyte_INTERPRETATION_ARROWS, 1);
   /* make escape like normal keys */
   xosbyte_write(osbyte_VAR_ESCAPE_CHAR, 0);
/*   xos_byte(osbyte_VAR_ESCAPE_CHAR, 0, 0, NULL, NULL);*/
   /* infinite mouse bounding box */
   xoswordpointer_set_bbox(&bbox);
/*   _kernel_osword(0x15, (int*)mausblk); */
   /* turn off mouse pointer */
   xosbyte_write(osbyte_SELECT_POINTER, 0);
/*   _kernel_osbyte(106, 0, 0); */
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
/*   bbc_origin((xcMacOS >> 1) + 8, (ycMacOS >> 1) + 8);*/
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
   xosbyte_write(osbyte_OUTPUT_SCREEN_BANK, bank); /* drawn bank */
   xos_cls();
   return 1;
}

/* Called after drawing an initial image */
/* Typically this might display the bitmap just drawn */
int
cvrotgfx_post_main_draw(void)
{
   xosbyte_write(osbyte_DISPLAY_SCREEN_BANK, bank); /* shown bank */
/*   bank ^= 1; */
   bank = (bank % nBanks) + 1; /* change written-to bank */
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
