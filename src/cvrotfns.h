/* > cvrotfns.h
 * Machine dependent functions (and function macros) for cave rotator
 * Copyright (C) 1993-1997 Olly Betts
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

/*
1993.08.02 split from caverot.h
1993.08.13 added #include "whichos.h"
           standardised header
1993.08.14 tidied after debugging sesh with MF
1993.08.15 DOS -> MSDOS
1993.08.16 added y_stretch to allow aspect ratio correction
1993.08.20 put in code to set y_stretch for all DOS modes (needs testing)
1993.09.23 (W) added missing extern float y_stretch
           (W) added required cast to atexit in DOS init_graphics
1993.10.26 fettled back in "lost" changes to caverot.h:
>>1993.06.13 towards animation by palette switching (MS DOS)
>>           added USE_VGAHI
1993.11.04 changed error routines
1993.11.05 changed error numbers so we can use same error list as survex
1993.11.30 changed error(81,...) to fatal(81,...)
1993.12.07 X_size -> xcMac, Y_size -> ycMac
1994.01.17 hacking for msc
1994.01.19 ditto
1994.02.06 tidy up msc hacks for release
1994.03.19 fast line code for Arc needs 256 colour mode for now
1994.03.22 fettling for fast line draw ARM code
1994.03.24 fettled for labels
1994.03.28 killed Norcroft warning
1994.04.07 added do_translate for RISC OS
1994.04.10 started to add Atari ST support
1994.04.13 moved cleardevice and outtextxy macros to dosrot.h
1994.04.14 RISC OS outtextxy will now cope with '%' in string
1994.06.20 made more variables static
1994.06.21 added extra arg to fatal calls
1994.08.11 added prototype for outtextxy under RISC OS
1994.09.11 RISC OS version now does 3 bank animation
           fast line draw now for 1bpp modes
1995.03.17 DJGPP/GRX stuff started
1995.03.20 [xy]cMacOS no longer static
1995.03.21 added text_xy
1995.03.23 [xy]cMacOS static again
           removed RISC OS outtextxy
1995.03.24 added missing const for GRX stuff
1995.04.20 grx.h -> grx20.h (Csaba renamed it)
1995.10.10 __GO32__ -> __DJGPP__ ; removed path from grx20.h include
1995.11.22 started to add GRX mouse support
1996.01.20 Added JLIB support
1996.01.21 added exit_graphics
1996.01.28 GRX now calls GrBitBltNCFS
1996.02.13 fixed RISC OS close_graphics to exit_graphics
1996.04.04 added dummy beep() for MSDOS ; introduced NOP
1996.04.09 fixed some dos_gcc warnings
1997.01.30 JLIB hacks
           added mouse support for GRX and JLIB
           downcased header names
1997.02.01 Mostly converted RISC OS code to use OSLib
1997.02.02 RISC OS fixes
*/

/* number of screen banks to use for RISC OS */
#define nBanks 3

/* *** Recode to shut down msc graphics library nicely using atexit() *** */

/* -DNO_MOUSE_SUPPORT if no mouse function library available */
/* (only actually affects DOS currently, since RISC OS has mouse support */
/* built in) */

#undef USE_VGAHI /* VGAHI only has one bank, but is higher res */

/* prototypes */
void init_graphics(void); /* init graphics hardware/software */
void exit_graphics(void); /* close down graphics hardware/software */
void swap_screen( bool ); /* swap displayed/changing screen */
int init_mouse(void);     /* test for mouse (returns # of buttons) */
void read_mouse(int *pdx, int *pdy, int *pbut);
                          /* read change in mouse co-ords/button status */
int get_key(void);        /* get key pressed, return it or -1 if no key */

static bool fSwapScreen;         /* flag controlling screen bank swapping */
/*!HACK! removed for JLIB1.7 static*/ int colText, colDraw;     /* as supplied to set_colour() */
#include "whichos.h"

/* sets of constants for different machines */
#if (OS==RISCOS)
/* # include "bbc.h" */
/* # include "kernel.h" */
# include "oslib/os.h"
# include "oslib/osbyte.h"
# include "oslib/osword.h"

void outtextxy(int x, int y, sz sz);
void text_xy(int x, int y, sz sz);

static int mode_on_entry;

static int xcMacOS, ycMacOS; /* limits for OS_Plot */
static int eigX,eigY;

void init_graphics(void) {
   extern void fastline_init(void); /* initialise fast lines routines */
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

   os_byte( osbyte_SCREEN_CHAR, 0, 0, NULL, &mode_on_entry );
   os_read_vdu_variables( (os_vdu_var_list*)&var_list, val_list );
   mode = val_list[0];
   scrmem = val_list[1]; /* screen memory size */
   modeBest = -1;
   cPixelsBest = 0;
   cColsBest = 0;
   for( ; mode>=0 ; mode-- ) {
      if (xos_check_mode_valid( (os_mode)mode, &modeAlt, NULL, NULL )!=NULL)
         continue;
      /* modeAlt is -1 for no such mode, -2 for not feasible */
      if (modeAlt >= 0) {
         int mode_flags;
	 /* Check we can draw lines (ie graphics, non-teletext, non-gap) */
         os_read_mode_variable( (os_mode)mode, os_MODEVAR_MODE_FLAGS, &mode_flags );
	 if ((mode_flags & 7) == 0) {
	    int screen_size;
            os_read_mode_variable( (os_mode)mode, os_MODEVAR_SCREEN_SIZE, &screen_size );
	    /* check we have enough memory for nBanks banks */
	    if (screen_size * nBanks <= scrmem) {
	       int x,y;
               os_read_mode_variable( (os_mode)mode, os_MODEVAR_XWIND_LIMIT, &x );
               os_read_mode_variable( (os_mode)mode, os_MODEVAR_YWIND_LIMIT, &y );
	       cPixels = x*y;
	       if (cPixels>=cPixelsBest) {
		  os_read_mode_variable( (os_mode)mode, os_MODEVAR_NCOLOUR, &cCols );
		  if (cCols==63) { /* need 256 colours for fastlinedraw */
#if 0
		  if (cCols==1) { /* need 2 colours for fastlinedraw */
#endif
		     if (cPixels>cPixelsBest || cCols>cColsBest) {
		        modeBest=mode;
			cPixelsBest=cPixels;
			cColsBest=cCols;
		     }
		  }
	       }
	    }
	 }
      }
   }
   xos_set_mode(), xos_writec(modeBest);
   /* defaults: */
   colText=cColsBest;
   /* colDraw not used currently */
   colDraw=2;
   switch (cColsBest) {
    case 1: /* fall through */
    case 3:
      colDraw=1; break;
    case 15:
      colText=7; break;
   }
   fSwapScreen=fTrue; /* may want to turn off if we haven't enough memory */
   swap_screen(fTrue);
   xos_cls(); /* and clear the other bank too... */

   /* make cursor keys return values */
/*   xos_byte( osbyte_INTERPRETATION_ARROWS, 1, 0, NULL, NULL ); */
   xosbyte_write( osbyte_INTERPRETATION_ARROWS, 1 );
   /* make escape like normal keys */
   xosbyte_write( osbyte_VAR_ESCAPE_CHAR, 0 );
/*   xos_byte( osbyte_VAR_ESCAPE_CHAR, 0, 0, NULL, NULL );*/
   /* infinite mouse bounding box */
   xoswordpointer_set_bbox( &bbox );
/*   _kernel_osword( 0x15, (int*)mausblk ); */
   /* turn off mouse pointer */
   xosbyte_write( osbyte_SELECT_POINTER, 0 );
/*   _kernel_osbyte( 106, 0, 0 ); */
   /* limits for fastline drawing routines */
   os_read_vdu_variables( (os_vdu_var_list*)&var_list2, val_list );
   xcMac = val_list[0] + 1;
   ycMac = val_list[1] + 1;
   eigX = val_list[2];
   eigY = val_list[3];
   /* limits for OS routines (used for text) */
   xcMacOS = xcMac<<eigX;
   ycMacOS = ycMac<<eigY;
   y_stretch *= (float)(int)(1<<eigX) / (float)(int)(1<<eigY);
   /* set origin to centre of screen +/- adjustment for labels */
/*   bbc_origin( (xcMacOS>>1)+8, (ycMacOS>>1)+8 );*/
   {
      int v;
      xos_set_graphics_origin();
      v = (xcMacOS>>1)+8;
      xos_writec( v );
      xos_writec( v>>8 );
      v = (ycMacOS>>1)+8;
      xos_writec( v );
      xos_writec( v>>8 );
   }
   /* adjust eig factors for positioning text correctly */
   eigX += 3;
   eigY += 3;

   /* write text at the graphics cursor */
   xos_writec( os_VDU_GRAPH_TEXT_ON );
   fastline_init();
}

/* restore screen & keybd on exit */
void exit_graphics( void ) { /* called when program terminates */
   /* make cursor keys move cursor */
   xosbyte_write( osbyte_INTERPRETATION_ARROWS, 0 );
   /* restore screen access to normal */
   xosbyte_write( osbyte_OUTPUT_SCREEN_BANK, 0 );
   xosbyte_write( osbyte_DISPLAY_SCREEN_BANK, 0 );
   /* make escape generate escape condition */
   xosbyte_write( osbyte_VAR_ESCAPE_CHAR, 27 );
   /* write text at text cursor */
   xos_writec( os_VDU_GRAPH_TEXT_OFF );
   /* and restore screen mode on entry */
   xos_set_mode(), xos_writec(mode_on_entry);
}

# define set_gcolour(X) (ol_setcol((X)))
# define set_tcolour(X) ( xos_set_gcol(), xos_writec(0), xos_writec((X)) )
# define cleardevice() (xos_cls())
/* # define shift_pressed() (bbc_inkey(-1)) */
# define shift_pressed() (osbyte_read( osbyte_VAR_KEYBOARD_STATE ) & 0x08)

int get_key(void) {
   int keycode, r2;
   /* bbc_inkey(0); */
   os_byte( osbyte_IN_KEY, 0, 0, &keycode, &r2 );
   osbyte_write( osbyte_FLUSH_BUFFER, 0 ); /* flush kbd buffer */
   return ( r2==0xff ? -1 : keycode );
}

# define beep() xos_bell()

/* x and y are in character widths/heights */
/* do something sane for proportional fonts */
/* could be a little neater here... :) */
void text_xy( int x, int y, sz sz ) {
   xos_plot( os_MOVE_TO, (x<<eigX)-(xcMacOS>>1)+16, (ycMacOS>>1)-(y<<eigY)-16 );
   xos_write0( sz );
}

void swap_screen( bool fDo ) { /* if fDo then switch banks */
   static int bankDraw=1, bankShow=2;
   if (fSwapScreen) {
      if (fDo) {
         bankShow = bankDraw;
         bankDraw = (bankDraw % nBanks) + 1; /* change written-to bank */
      } else {
         bankDraw = bankShow;
      }
#if 1
      xosbyte_write( osbyte_OUTPUT_SCREEN_BANK, bankDraw ); /* drawn bank */
      xosbyte_write( osbyte_DISPLAY_SCREEN_BANK, bankShow ); /* shown bank */
#endif
   }
}

void bop_screen( void ) {
   /* do nothing */
}

# define init_mouse() (3) /* assume Archie mouse is there & has 3 buttons */

void read_mouse(int *pdx, int *pdy, int *pbut) {
   static int xOld=0,yOld=0;
   int x,y;
   os_mouse( &x, &y, (bits*)pbut, NULL );
   *pdx=(short)x-(short)xOld;
   *pdy=(short)y-(short)yOld;
   xOld=x;
   yOld=y;
}

# define LBUT 4 /* mask for left mouse button */
# define RBUT 1 /* mask for right mouse button (if present) */
# define MBUT 2 /* mask for middle mouse button (if present) */

void do_translate ( lid *plid, coord dX, coord dY, coord dZ ) {
   /* do_trans is an ARM code routine in armrot.s */
   extern do_trans( point*, coord, coord, coord );
   for ( ; plid ; plid=plid->next )
      do_trans( plid->pData, dX, dY, dZ );
}

#elif (OS==MSDOS)

/* for DOS-specific I/O functions */
# include <conio.h>
# include <bios.h>
# include <dos.h>

# ifdef MSC
#  include <graph.h>
# else
#  if defined(JLIB)
#   include "jlib.h"
buffer_rec *BitMap;
#  elif defined(__DJGPP__)
#   include "grx20.h"
GrContext *BitMap;
#  else
#   include <graphics.h>
#  endif
# endif

void init_graphics(void) {
 extern int xcMac, ycMac;
 extern float y_stretch;
# ifdef MSC
 /* detect graphics hardware available */
 if (_setvideomode(_MAXRESMODE) == 0)
  fatal(81,NULL,NULL,0);
# elif defined(JLIB)
 screen_set_video_mode();

/*!HACK!*/ screen_put_pal(255,0xff,0xff,0xff);
/*!HACK!*/ screen_put_pal(0,0,0,0);
   
 /* initialise screen sized buffer */
 BitMap = buff_init(SCREEN_WIDTH,SCREEN_HEIGHT);
 /* check that initialisation was OK */
 if (BitMap == NULL)
    fatal(81,NULL,NULL,0);

# elif defined(__DJGPP__)
/*	    GrSetMode(GR_width_height_color_graphics,x,y,c);
 * else if((x >= 320) && (y >= 200))
 *	    GrSetMode(GR_width_height_graphics,x,y);
 *	else
 */
/* if (!GrSetMode(GR_default_graphics)) */
 if (!GrSetMode(GR_width_height_color_graphics,800,600,256))
  fatal(81,NULL,NULL,0);

 /* Creates a bitmap 'context' in system memory, allocating memory for us */
 BitMap = GrCreateContext( GrScreenX()+1, GrScreenY()+1, NULL, NULL );
 if (!BitMap) exit(1);
 /* get rid of it */
 /*GrDestroyContext( BitMap );*/
 /*	GrSetMode(GR_default_text); */
# else
 extern char *pthMe; /* SURVEXHOME if set, else directory prog. was run from */
 int gdriver, gmode, errorcode;
 /* detect graphics hardware available */
 detectgraph(&gdriver, &gmode);
 /* read result of detectgraph call */
 errorcode = graphresult();
 if (errorcode != grOk)
  fatal(81,wr,grapherrormsg(errorcode),0); /* something went wrong */
 /* defaults to make code shorter */
 colText=1;
 colDraw=2;
 fSwapScreen=fFalse;
/* gdriver=EGA64;*/ /******************************************************/
 switch (gdriver) {
  case (CGA):
   if (gmode<CGAHI) {
    gmode=CGAC1; /* choose palette with white */
    colText=3;
   }
   break;
  case (MCGA): case (ATT400): /* modes are identical */
   if (gmode<MCGAMED) {
    gmode=MCGAC1; /* choose palette with white */
    colText=3;
   }
   break;
  case (EGA):
   colText=15;
   fSwapScreen=fTrue;
   break;
  case (EGA64):
   if (gmode==EGA64LO)
    colText=15;
   else
    colText=3;
   break;
  case (EGAMONO): case (HERCMONO):
   fSwapScreen=fTrue; /* won't work on 64k Mono EGA cards (only 256k ones) */
   break;
  case (VGA):
   if (gmode==VGAHI)
#ifdef USE_VGAHI
    fSwapScreen=fFalse;
#else /* !USE_VGAHI */
    gmode=VGAMED; /* because VGAHI has only one bank */
#endif /* ?USE_VGAHI */
   colText=15;
   fSwapScreen=fTrue;
   break;
  case (PC3270):
   break;
  case (IBM8514):
   colText=255; /* guess city !HACK! */
   break;
  default:
   fatal(81,wr,grapherrormsg(errorcode),0); /* unknown graphics card */
  }
# endif

# ifdef MSC
  {
    struct _videoconfig foo;
    _getvideoconfig( &foo );
    xcMac=foo.numxpixels;
    ycMac=foo.numypixels;
    /* defaults to make code shorter */
    colText=1;
    colDraw= (foo.numcolors>2) ? 2 : 1 ;
    fSwapScreen=(foo.numvideopages>1);
  }
  y_stretch *= (float)(((float)xcMac/ycMac)*(350.0/640.0)*1.3);
# elif defined(JLIB)
  {
    colText=SCREEN_NUM_COLORS-1;
    colDraw=1;
    xcMac=SCREEN_WIDTH;
    ycMac=SCREEN_HEIGHT;
    y_stretch *= (float)(((float)xcMac/ycMac)*(350.0/640.0)*1.3);
    fSwapScreen=fTrue; /* False;*/ /* !HACK! for now */
  }
# elif defined(__DJGPP__)
  {
    const GrVideoMode * mode=GrCurrentVideoMode();
    colText=(1<<((int)(mode->bpp))) - 1;
    colDraw=1;
    xcMac=mode->width;
    ycMac=mode->height;
    y_stretch *= (float)(((float)xcMac/ycMac)*(350.0/640.0)*1.3);
    fSwapScreen=fTrue; /* False;*/ /* !HACK! for now */
  }
# else
  /* initialize graphics and global variables */
  initgraph(&gdriver, &gmode, pthMe );

  /* read result of initgraph call */
  errorcode = graphresult();
  if (errorcode != grOk) /* something wrong: no .bgi file? */
    fatal(81,wr,grapherrormsg(errorcode),0);

  xcMac=getmaxx();
  ycMac=getmaxy();
  y_stretch *= (float)(((float)xcMac/ycMac)*(350.0/640.0)*0.751677852);
# endif
}

void exit_graphics( void ) { /* called when program terminates */
#if defined(JLIB)
   screen_restore_video_mode();
#elif defined(__DJGPP__)
#elif defined(MSC)
#else
   closegraph(); /* restore screen on exit */
#endif
}

/* returns a keycode - if enhanced keycode then 0x100 added; -1 if none */
int get_key(void) {
   int keycode;
   if (!kbhit())
      return -1; /* -1 => no key pressed */
   keycode = getch();
   if (keycode==0) /* if enhanced key_code add 0x100 */
      keycode = getch() + 0x100;
   while (kbhit())
      getch(); /* flush the keyboard buffer to stop key presses backing up */
   return (keycode);
}

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

extern void beep(void); /* make a beep */
/* hmm, I thought beep() used to work, but not for Borland C !HACK! */
# define beep() NOP

void swap_screen( bool fDo ) { /* if fDo then switch banks */
   if (fSwapScreen) { /* bank swappin' */
# if defined(JLIB)
      if (fDo) {
	 screen_blit_fs_buffer(BitMap);
#if 0
	 buff_clear(BitMap); /* CLS */
#endif
      } else {
	 /* Hmmm, can't draw directly on screen with JLIB */
      }
# elif defined(__DJGPP__)
      if (fDo) {
	 /* use GrBitBltNC for speed ... */
	 GrSetContext( BitMap ); /* pass NULL for screen */
#if 1
	 GrBitBltNC( GrScreenContext(), 0, 0, BitMap /*NULL*/ /* current */, 0, 0, GrMaxX(), GrMaxY(), GrWRITE );
#else
	 GrBitBltNCFS( GrScreenContext(), BitMap /*NULL*/ /* current */ );
#endif
#if 0
	 GrClearContext( 0 ); /* CLS */
#endif
      } else {
	 GrSetContext( NULL ); /* pass NULL for screen */
      }
# else
      static int bankDraw=1,bankShow=0;
      if (fDo) {
	 bankShow = bankDraw;
	 bankDraw ^= 1; /* toggle written to bank */
      } else {
	 bankDraw = bankShow;
      }
#  ifdef MSC
      _setvisualpage( bankShow );
      _setactivepage( bankDraw );
#  elif !defined(__DJGPP__)
      setvisualpage( bankShow );
      setactivepage( bankDraw );
#  else
/*	  GrCurrentVideoMode()->extinfo->setrwbanks( bankShow+1, bankDraw+1 );*/
#  endif
# endif
   }
#if 0
     { /* palette switchin' */
	setwritemode(XOR_PUT);
	if (fDo) {
	   bankShow = bankDraw;
	   bankDraw ^= 1; /* toggle written to "bank" */
	} else {
	   bankDraw = bankShow;
	}
	setpalette(bankShow+1,1);
	setpalette(bankDraw+1,0);
     }
#endif
}

void bop_screen( void ) {
# ifdef JLIB
   screen_blit_fs_buffer(BitMap);
# else
   /* do nothing */
# endif
}

# ifndef NO_MOUSE_SUPPORT
#  ifdef __DJGPP__
int init_mouse(void) { /* for GRX mouse support */
   if (!GrMouseDetect())
      return -1; /* !HACK! could be no mouse or no driver... */
   GrMouseEventMode(1);
   GrMouseInit(); /* not actually needed, but call for neatness */
   return 3; /* !HACK! assume 3 buttons */
}

void read_mouse(int *pdx, int *pdy, int *pbut) {
   GrMouseEvent event;
   static int x_old, y_old;

   GrMouseGetEventT( GR_M_EVENT, &event, 0L );
   *pdx = event.x - x_old;
   *pdy = event.y - y_old;
   x_old = event.x;
   y_old = event.y;
   *pbut = event.buttons;
}

#define LBUT GR_M_LEFT
#define MBUT GR_M_MIDDLE
#define RBUT GR_M_RIGHT
#  elif defined(JLIB)
int init_mouse(void) { /* for GRX mouse support */
   if (mouse_present() != MOUSE_PRESENT)
      return -1; /* !HACK! could be no mouse or no driver... */
   return MOUSE_NUM_BUTTONS; /* this is is the max number of buttons... !HACK! */
}

void read_mouse(int *pdx, int *pdy, int *pbut) {
   static int x_old, y_old;
   int x_new, y_new;

   mouse_get_status( &x_new, &y_new, pbut );
   *pdx = x_new - x_old;
   *pdy = y_new - y_old;
   x_old = x_new;
   y_old = y_new;
}

#define LBUT MOUSE_B_LEFT
#define MBUT MOUSE_B_MIDDLE
#define RBUT MOUSE_B_RIGHT
#  else
extern void cmousel(int*,int*,int*,int*); /* prototype to kill warnings */

int init_mouse(void) { /* for Microsoft mouse driver */
   unsigned long vector;
   unsigned char byte;
   union REGS inregs, outregs;
   struct SREGS segregs;        /* nasty grubby structs to hold reg values */

   int cmd,cBut,dummy; /* parameters to pass to cmousel() */

   inregs.x.ax=0x3533;          /* get interrupt vector for int 33 */
   intdosx( &inregs, &outregs, &segregs );
   vector= (((long)segregs.es)<<16) + (long)outregs.x.bx;
   byte=(unsigned char)*(long far*)vector;
   if ((vector==0l)||(byte==0xcf)) /* vector empty or -> iret */
      return -2; /* No mouse driver */

   cmd=0; /* check mouse & mouse driver working */
   cmousel( &cmd, &cBut, &dummy, &dummy );
   return (cmd==-1) ? cBut : -1; /* return -1 if mouse dead */
}

void read_mouse(int *pdx, int *pdy, int *pbut) {
   int cmd,dummy;
   cmd=3;  /* read mouse buttons */
   cmousel(&cmd,pbut,pdx,pdy);
   cmd=11; /* read 'mickeys' ie change in mouse X and Y posn */
   cmousel(&cmd,&dummy,pdx,pdy);
   *pdy=-*pdy; /* mouse coords in Y up X right please */
}
#  endif

# else /* NO_MOUSE_SUPPORT so offer a dead mouse */

#  define init_mouse() (-2) /* return 'No mouse driver' */
#  define read_mouse(PDX,PDY,PBUT) NOP

# endif /* ?NO_MOUSE_SUPPORT */

# ifndef __DJGPP__
#  define LBUT 1 /* mask for left mouse button */
#  define RBUT 2 /* mask for right mouse button (if present) */
#  define MBUT 4 /* mask for middle mouse button (if present) */
# endif

#elif (OS==TOS)

/* graphics library written my Mark M's mate */
# include <vdi.h>

/* global graphics context pointer */
struct param_block *vdi_ptr=NULL;

# define cleardevice() v_clrwk(vdi_ptr)
# define outtextxy(X,Y,SZ) v_gtext(vdi_ptr, (X), (Y), (SZ) )

void init_graphics(void) {
   extern int xcMac, ycMac;
   extern float y_stretch;

   /* Initialise vdi library */
   vdi_ptr=v_opnvwk();

   colText=1;
   colDraw=2;
   fSwapScreen=fFalse;

   xcMac=640;
   ycMac=400;
   y_stretch *= (float)(((float)xcMac/ycMac)*(350.0/640.0)*0.751677852);
}

void exit_graphics( void ) {
   v_clsvwk(vdi_ptr);
}

/* black and white only */
# define set_gcolour(X) NOP
# define set_tcolour(X) NOP

/* returns a keycode - if enhanced keycode then 0x100 added; -1 if none */
int get_key(void) {
   int keycode;
   if (!kbhit())
      return -1; /* -1 => no key pressed */
   keycode = getch();
   if (keycode==0) /* if enhanced key_code add 0x100 */
      keycode = getch() + 0x100;
   while (kbhit())
      getch(); /* flush the keyboard buffer to stop key presses backing up */
   return (keycode);
}

# define shift_pressed() (_bios_keybrd(_KEYBRD_SHIFTSTATUS) & 0x03 )

extern void beep(void); /* make a beep */

/* nope */
# define swap_screen( fDo ) NOP

# define init_mouse() (1) /* return no. of buttons! */

void read_mouse(int *pdx, int *pdy, int *pbut) {
   static int xOld=0,yOld=0;
   int x,y;
   vq_mouse( vdi_ptr, pbut, &x, &y );
   *pdx=x; *pdy=y; /* !HACK! should be the difference */
/*
   *pdx=(short)x-(short)xOld;
   *pdy=(short)y-(short)yOld;
   xOld=x;
   yOld=y;
*/
   /* NB mouse coords in Y up X right please */
}

# define LBUT 1 /* mask for left mouse button */
# define RBUT 2 /* mask for right mouse button (if present) */
# define MBUT 4 /* mask for middle mouse button (if present) */

#else
# error Operating System not known
#endif
