/* > caverot.c
 * Reads in SURVEX .3d image files & allows quick rotation and examination
 * Copyright (C) 1990,1993-1997 Olly Betts
 * Portions Copyright (C) 1993 Wookey
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctype.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "filename.h"
#include "message.h"
#include "filelist.h"
#include "useful.h"

#include "caverot.h"
#include "cvrotimg.h"
#include "cvrotgfx.h"
#include "rotplot.h"
#include "labels.h"

/*#define ANIMATE*/
#define ANIMATE_FPS 1

static unsigned char acolDraw[] = {
   255, 23, 119, 139, 67, 210, 207, 226, 31, 39, 55, 83, 186, 195, 75
};

/***************************************************************************/

/* prototypes */

void setup(void);              /* initialize as required */
static void parse_command(int argc, char **argv);
static void show_help(void);   /* displays help screen */
void set_defaults(void);       /* reset default values */
bool process_key(void);        /* read & process keyboard and mouse input */
void swap_screen(bool);        /* swap displayed screen and drawing screen */

/* translate all points */
void translate_data(coord Xchange, coord Ychange, coord Zchange);

/***************************************************************************/

/* global variables */

/* lots of these could be in structures - eg a view struct
   then it could be passed to input_values, process_key etc */

/*
Hungarian style types:
 deg  - angle in degrees
 sc   - scaling factor
 f    - flag
 c    - count
 pth  - filing system path
 sz   - pointer to zero-terminated string
*/

static float degView;          /* current direction of view (degrees) */
static float degViewStep;      /* current size of change in view direction */
static float degStereoSep;     /* half the stereo separation of 3d view */
float scDefault;        /* scale to show whole survey on screen */
static float sc;               /* current scale */
static float ZoomFactor;       /* zoom in/out factor */
static bool fRotating;        /* flag indicating auto-rotation */
static bool fNames = fFalse;  /* Draw station names? */
bool fAllNames = fFalse;  /* Draw all station names? */
static bool fLegs = fTrue;   /* Draw legs? */
static bool fStns = fFalse;  /* Draw crosses at stns? */
static bool fAll = fFalse;  /* Draw all during movement? */
static bool fRevSense = fFalse; /* Movements relate to cave/observer */
static bool fSlowMachine = fFalse; /* Controls slick tilt, etc */
static float elev;             /* angle of elevation of viewpoint */
static float nStepsize;        /* stepsize for movements */
static bool f3D;              /* flag indicating red/green 3d view mode */
int xcMac, ycMac;            /* screen size in plot units (==pixels usually) */
#if 1
#ifdef Y_UP
float y_stretch = 1.0f;   /* factor to multiply y by to correct aspect ratio */
#else /* !defined(Y_UP) */
float y_stretch = -1.0f;  /* factor to multiply y by to correct aspect ratio */
#endif
#else
float y_stretch = -0.3f;  /* test that aspect ratio works */
#endif

#if 0
static int Stored_views;     /* number of defined stores */
static view *pView_store;     /* pointer to array of view stores */
#endif

static int locked; /* no data along this axis (1=>x, 2=>y, 3=>z, 0=>none) */

#ifdef ANIMATE
int cAnimate = 0;
int incAnimate = 1;
#endif

static lid Huge **ppLegs = NULL;
static lid Huge **ppStns = NULL;

static void
draw_legs(void)
{
#ifndef ANIMATE
   int c;
   for (c = 0; ppLegs[c]; c++) {
      set_gcolour(acolDraw[c]);
      draw_view_legs(ppLegs[c]);
   }
#else
   set_gcolour(acolDraw[0]);
   draw_view_legs(ppLegs[cAnimate]);
#endif
}

static void
draw_stns(void)
{
#ifndef ANIMATE
   int c;
   for (c = 0; ppStns[c]; c++) {
      set_gcolour(acolDraw[c]);
      draw_view_stns(ppStns[c]);
   }
#else
   set_gcolour(acolDraw[c]);
   draw_view_stns(ppStns[cAnimate]);
#endif
}

static void
draw_names(void)
{
#ifndef ANIMATE
   int c;
   set_tcolour(colText);
   for (c = 0; ppStns[c]; c++) {
      draw_view_labs(ppStns[c]);
   }
#else
   set_tcolour(colText);
   draw_view_labs(ppStns[cAnimate]);
#endif
}

/*************************************************************************/
/*************************************************************************/

int
main(int argc, char **argv)
{
   enum { LEGS, STNS, LABS, DONE } item = LEGS;

   msg_init(argv[0]);

   puts("Survex cave rotator v"VERSION"\n  "COPYRIGHT_MSG);

/*#include "expire.h"*/

   /* !HACK! check return value */ cvrotgfx_parse_cmdline( &argc, argv );
   parse_command(argc, argv);

   /* these aren't in set_defaults() 'cos we don't want DELETE to reset them */
   degView = 0;
   elev = 90.0f;       /* display plan by default */
   fRotating = fFalse; /* stationary to start with */
   f3D = fFalse;       /* red/green 3d view off by default */
   degStereoSep = 2.5f;

   /* setup colour values for 3dview (RED_3D,GREEN_3D,BLUE_3D) or ??? */

#if 0
   /* setup view store */
   Stored_views = 9;
   pView_store = (viewstore*) osmalloc(Stored_views * sizeof(view));
#endif

   /* initialise graphics h/ware & s/ware */
   cvrotgfx_init();
   init_map();

   /* display help screen to give user something to read while we do scaling */
/*   cleardevice();*/
/*   swap_screen(fTrue);*/
   show_help();

   /* can't do this until after we've initialised the graphics */
   scDefault = scale_to_screen(ppLegs, ppStns);

   /* Check if we've got a flat plot aligned perpedicular to an axis */
   locked = 0;
   if (Xrad == 0) locked = 1;
   if (Yrad == 0) locked = 2;
   if (Zrad == 0) locked = 3;
   switch (locked) {
    case 1:
      degView = 90;
      elev = 0.0f; /* elevation looking along X axis */
      break;
    case 2:
      degView = 0;
      elev = 0.0f; /* elevation looking along Y axis */
      break;
    case 3:
      degView = 0;
      elev = 90.0f; /* plan */
      break;
    default: break; /* avoid compiler warning */
   }

   /* set base step size according to screen size */
   nStepsize = min(xcMac, ycMac) / 25.0f;
   set_defaults();

     {
	bool fRedraw = fTrue;
	const char *szPlan;
	const char *szElev;

	szPlan = msgPerm(/*Plan, %05.1f up screen*/101);
	szElev = msgPerm(/*View towards %05.1f*/102);

	while (fTrue) {
	   char sz[80];

	   if (fRedraw) {
	      while (degView >= 360) degView -= 360;
	      while (degView <0) degView += 360;
	      clear_map();
	      cvrotgfx_pre_main_draw();
	      if (f3D) {
#if 0
		 int c;
		 set_gcolour(col3dL);
		 set_view(sc, degView - degStereoSep, elev);
		 for (c = 0; ppLegs[c]; c++) draw_view_legs(ppLegs[c]);
		 set_gcolour(col3dR);
		 set_view(sc, degView + degStereoSep, elev);
		 for (c = 0; ppLegs[c]; c++) draw_view_legs(ppLegs[c]);
#endif
	      } else {
		 set_view(sc, degView, elev);
		 if (fAll) {
		    if (fLegs) draw_legs();
		    if (fStns) draw_stns();
		    if (fNames) draw_names();
		    item = DONE;
		 } else if (fLegs) {
		    draw_legs();
		    item = STNS;
		 } else if (fStns) {
		    draw_stns();
		    item = LABS;
		 } else {
		    if (fNames) draw_names();
		    item = DONE;
		 }
	      }
	      set_tcolour(colText);
	      sprintf(sz, (elev == 90.0f) ? szPlan : szElev, degView);
	      text_xy(0, 0, sz); /* outtextxy(8, 0, sz); */
	      draw_scale_bar();
	      /* fprintf(fhDbug, "Drawing %s view\n", (elev == 90.0f) ? "plan" : "side"); */
	      cvrotgfx_post_main_draw();
	   } else {
	      switch (item) {
	       case STNS:
		 if (fStns) {
		    cvrotgfx_pre_supp_draw();
		    draw_stns();
		    cvrotgfx_post_supp_draw();
		 }
		 item++;
		 break;
	       case LABS:
		 if (fNames) {
		    cvrotgfx_pre_supp_draw();
		    draw_names();
		    cvrotgfx_post_supp_draw();
		 }
		 item++;
		 break;
	       default:
		 break; /* suppress warning */
	      }
	   }

	   /* if (fDemo)
	    * fRedraw = demo_step();
	    * else */
	   fRedraw = process_key();
	}
     }
}

/***************************************************************************/

/* things in here get reset by DELETE key */
void
set_defaults(void)
{
   degViewStep = 3;
   sc = scDefault;
   translate_data(-Xorg, -Yorg, -Zorg); /* rotate about 'centre' of cave */
}

/***************************************************************************/

/*
bool
demo_step(void)
{
   static n = 0;
   struct {
   };
} */
static int dt;

bool
process_key(void) /* and mouse! */
{
   float nStep, Accel;
   int iKeycode;
   static bool fRedraw = fFalse;
   double s = SIND(degView), c = COSD(degView);
   static int autotilt = 0;
   static float autotilttarget;
   static clock_t new_time = 0;
   clock_t old_time;
   static float tsc = 1.0f; /* sane starting value */
#ifdef ANIMATE
   static clock_t last_animate = 0;
#endif

   old_time = new_time;
   new_time = clock();
   if (fRedraw) {
      dt = (int)(new_time - old_time);
      /* better to be paranoid */
      if (dt < 1) dt = 1;
      tsc = (float)dt * (10.0f / CLOCKS_PER_SEC);
      if (tsc > 1000.0f) {
	 cvrotgfx_beep(/*!HACK!*/);
	 tsc = 1000.0f;
      }
      fRedraw = fFalse;
   }

#ifdef ANIMATE
   if (new_time - last_animate > CLOCKS_PER_SEC / ANIMATE_FPS) {
      cAnimate += incAnimate;
      if (cAnimate == 0 || ppLegs[cAnimate + 1] == NULL)
         incAnimate = -incAnimate;

      if (last_animate == 0)
         last_animate = new_time;
      else
         last_animate += CLOCKS_PER_SEC / ANIMATE_FPS;

      fRedraw = fTrue;
   }
#endif

   iKeycode = cvrotgfx_get_key();
   if (shift_pressed()) {
      nStep = 5.0f * nStepsize / sc;
      ZoomFactor = BIG_MAGNIFY_FACTOR;
      Accel = 5.0f;
   } else {
      nStep = nStepsize / sc;
      ZoomFactor = LITTLE_MAGNIFY_FACTOR;
      Accel = 1.0f;
   }
   nStep *= tsc;
   ZoomFactor = (float)pow(ZoomFactor, tsc);
   Accel *= tsc;

   if (fRevSense) nStep = -nStep;

   if (iKeycode > -1) {
      autotilt = 0;
      if (iKeycode >= 'a' && iKeycode <= 'z')
	 iKeycode -= 32; /* force lower case letters to upper case */
      if (locked == 0 || locked == 3) {
	 /* no restriction, or plan only */
	 switch (iKeycode) {
/*        case '3': case SHIFT_3:    f3D = !f3D; break; */
	  case 'Z': if (degViewStep < 45) degViewStep *= 1.2f; break;
	  case 'X': degViewStep /= 1.2f; break;
	  case 'C': degView -= degViewStep * Accel; fRedraw = fTrue; break;
	  case 'V': degView += degViewStep * Accel; fRedraw = fTrue; break;
	  case 'R': degViewStep = -degViewStep; break;
	  case ' ':            fRotating = fFalse; break;
	  case RETURN_KEY:     fRotating = fTrue; break;
	 }
      }
      if (locked == 0) {
	 switch (iKeycode) {
	  case '\'': case '@': case '"':
	    /* shift-' varies with keyboard layout
	     * so check both '@' (UK) and '"' (US) */
	    if (fRevSense) goto tiltup;
	    tiltdown:
	    if (elev > -90.0f) {
	       elev -= degViewStep*Accel;
	       if (elev < -90.0f) elev = -90.0f;
	       fRedraw = fTrue;
	    }
	    break;
	  case '/': case '?':
	    if (fRevSense) goto tiltdown;
	    tiltup:
	    if (elev < 90.0f) {
	       elev += degViewStep * Accel;
	       if (elev > 90.0f) elev = 90.0f;
	       fRedraw = fTrue;
	    }
	    break;
	  case 'P':
	    if (elev != 90.0f) {
	       if (fSlowMachine)
		  elev = 90.0f;
	       else {
/*fprintf(stderr, "P:tsc = %f\n", tsc);fflush(stderr);*/
		  autotilt = (int)ceil(90.0 / ceil(9.0 / tsc));
/*fprintf(stderr,"P:autotilt = %d\n",autotilt);fflush(stderr);*/
		  if (autotilt > 16) {
		     autotilt = 0;
		     elev = 90.0f;
		  }
	       }
	       autotilttarget = 90.0f;
	       fRedraw = fTrue;
	    }
	    break;
	  case 'L':
	    if (elev!=0.0f) {
	       if (fSlowMachine)
		  elev = 0.0f;
	       else {
/*fprintf(stderr,"L:tsc = %f\n",tsc);fflush(stderr);*/
		  autotilt = (int)ceil(90.0 / ceil(9.0 / tsc));
/*fprintf(stderr,"L:autotilt = %d\n",autotilt);fflush(stderr);*/
		  if (autotilt > 16) {
		     elev = 0.0f;
		     autotilt = 0;
		  } else if (elev > 0.0f)
		     autotilt = -autotilt;
	       }
	       autotilttarget = 0.0f;
	       fRedraw = fTrue;
	    }
	    break;
	 }
      }
      switch (iKeycode) {
       case ']': case '}':
         sc = fRevSense ? sc / ZoomFactor : sc * ZoomFactor;
         fRedraw = fTrue; break;
       case '[': case '{':
         sc = fRevSense ? sc * ZoomFactor : sc / ZoomFactor;
         fRedraw = fTrue; break;
       case 'U': translate_data(0, 0, (coord)nStep); fRedraw = fTrue; break;
       case 'D': translate_data(0, 0, (coord)-nStep); fRedraw = fTrue; break;
       case 'N': translate_data(0, (coord)nStep, 0); fRedraw = fTrue; break;
       case 'S': translate_data(0, (coord)-nStep, 0); fRedraw = fTrue; break;
       case 'E': translate_data((coord)nStep, 0, 0); fRedraw = fTrue; break;
       case 'W': translate_data((coord)-nStep, 0, 0); fRedraw = fTrue; break;
       case DELETE_KEY:  set_defaults(); fRedraw = fTrue; break;
       case 'O':   fAllNames = !fAllNames; fRedraw = fNames; break;
/*       case 'I':   get_view_details(); fRedraw=fTrue; break; */
       case CURSOR_UP:
         if (elev == 90.0f)
            translate_data((coord)(nStep * s), (coord)(nStep * c), 0);
         else
            translate_data(0, 0, (coord)nStep);
         fRedraw = fTrue; break;
       case CURSOR_DOWN:
         if (elev == 90.0f)
            translate_data(-(coord)(nStep * s), -(coord)(nStep * c), 0);
         else
            translate_data(0, 0, (coord)-nStep);
         fRedraw = fTrue; break;
       case CURSOR_LEFT:
         translate_data(-(coord)(nStep * c), (coord)(nStep * s), 0);
         fRedraw = fTrue; break;
       case CURSOR_RIGHT:
         translate_data((coord)(nStep * c), -(coord)(nStep * s), 0);
         fRedraw = fTrue; break;
       case 'H': {
	  clock_t start_time;
	  start_time = clock();
	  fRedraw = fTrue;
	  show_help();
	  new_time += (clock() - start_time); /* ignore time user spends viewing help */
	  /* cruder: new_time = clock(); */
	  break;
       }
       case ('A' - '@'):
         fAll = !fAll; break; /* no need to redraw */
       case ('N' - '@'):
         fRedraw = fTrue; fNames = !fNames; break;
       case ('X' - '@'):
         fRedraw = fTrue; fStns = !fStns; break;
       case ('L' - '@'):
         fRedraw = fTrue; fLegs = !fLegs; break;
       case ('R' - '@'):
         fRevSense = !fRevSense; break;
       case ESCAPE_KEY:
	 cvrotgfx_final();
         exit(EXIT_SUCCESS); break; /* That's all folks */
      }
   }

   if (mouse_buttons >= 0) {
      static fOldMBut = fFalse;
      int buttons = 0; /* zero in case read_mouse() doesn't set */
      int dx = 0, dy = 0;
      cvrotgfx_read_mouse(&dx, &dy, &buttons);
      if (buttons & (CVROTGFX_LBUT | CVROTGFX_RBUT | CVROTGFX_MBUT)) autotilt = 0;
      if (buttons & CVROTGFX_LBUT) {
         if (fRevSense) {
            sc *= (float)pow(LITTLE_MAGNIFY_FACTOR, 0.08 * dy);
            if (locked == 0 || locked == 3) degView += dx * 0.16f;
         } else {
            sc *= (float)pow(LITTLE_MAGNIFY_FACTOR, -0.08 * dy);
            if (locked == 0 || locked == 3) degView -= dx * 0.16f;
         }
         fRedraw = fTrue;
      }
      if (buttons & CVROTGFX_RBUT) {
         nStep *= 0.025f;
         if (elev == 90.0f)
            translate_data((coord)(nStep * (dx * c + dy * s)),
                           -(coord)(nStep * (dx * s - dy * c)), 0);
         else
            translate_data((coord)(nStep * (double)dx * c),
                           -(coord)(nStep * (double)dx * s),
			   (coord)(nStep * dy));
         fRedraw = fTrue;
      }
      if (locked == 0 && (buttons & CVROTGFX_MBUT) && !fOldMBut) {
         elev = (elev == 90.0f) ? 0.0f : 90.0f;
         fRedraw = fTrue;
      }
      fOldMBut = ((buttons & CVROTGFX_MBUT) > 0);
   }

   if (autotilt) {
      elev += (float)autotilt;
      fRedraw = fTrue;
      if (autotilt > 0) {
         if (elev >= autotilttarget) {
            elev = autotilttarget;
            autotilt = 0;
         }
      } else {
         if (elev <= autotilttarget) {
            elev = autotilttarget;
            autotilt = 0;
         }
      }
   }

   /* rotate by current stepsize */
   if (fRotating) {
      degView += degViewStep*tsc;
      fRedraw = fTrue; /* and force a redraw */
   }

   return (fRedraw);
}

/***************************************************************************/

#define FLAG_ALWAYS 1
#define FLAG_MOUSE 2
#define FLAG_MOUSE2 4|FLAG_MOUSE
#define FLAG_MOUSE3 8|FLAG_MOUSE
#define FLAG_NOMOUSE 16

typedef struct {
   const char *msg;
   int flags;
} flagged_msg;

static void
show_help(void)
{
   /* help text stored as static array of strings, and printed as graphics
    *  to the screen */
   /* FIXME: TRANSLATE */
   static flagged_msg help_msgs[] = {
	{"                  Z,X : Faster/Slower rotation", FLAG_ALWAYS},
	{"                    R : [R]everse direction of rotation", FLAG_ALWAYS},
	{"          Enter,Space : Start/Stop auto-rotation", FLAG_ALWAYS},
	{"                  C,V : Rotate cave one step clockwise/anti", FLAG_ALWAYS},
	{"                  ',/ : Higher/Lower viewpoint", FLAG_ALWAYS},
	{"                  ],[ : Zoom In/Out", FLAG_ALWAYS},
	{"                  U,D : Cave [U]p/[D]own", FLAG_ALWAYS},
	{"              N,S,E,W : Cave [N]orth/[S]outh/[E]ast/[W]est", FLAG_ALWAYS},
	{"               Delete : Default scale, rotation rate, etc", FLAG_ALWAYS},
	{"                  P,L : [P]lan/e[L]evation", FLAG_ALWAYS},
      /*  "                    I : [I]nput values", */
      /*  "                  3,# : Toggle [3]D goggles view", */
	{"    Cursor Left,Right : Cave [Left]/[Right] (on screen)", FLAG_ALWAYS},
	{"       Cursor Up,Down : Cave [Up]/[Down] (on screen)", FLAG_ALWAYS},
      /*  "    Sft Copy + Letter : Store view in store <Letter>", */
      /*  "        Copy + Letter : Recall view in store <Letter>", */
	{"               Ctrl-N : Toggle display of station [N]ames", FLAG_ALWAYS},
	{"               Ctrl-X : Toggle display of crosses ([X]s) at stations", FLAG_ALWAYS},
	{"               Ctrl-L : Toggle display of survey [L]egs", FLAG_ALWAYS},
	{"               Ctrl-A : Toggle display of [A]ll/skeleton when moving", FLAG_ALWAYS},
	{"                    O : Toggle display of non-overlapping/all names", FLAG_ALWAYS},
	{"               Ctrl-R : Reverse sense of controls", FLAG_ALWAYS},
	{"               ESCAPE : Quit program", FLAG_ALWAYS},
	{"             Shift accelerates all movement keys", FLAG_ALWAYS},
	{"", FLAG_ALWAYS},
	{"                    H : [H]elp screen (this!)", FLAG_ALWAYS},
	{"", FLAG_ALWAYS},
	{"Mouse controls:  LEFT : left/right rotates, up/down zooms", FLAG_MOUSE},
	{"               MIDDLE : toggles plan/elevation", FLAG_MOUSE3},
	{"                RIGHT : mouse moves cave", FLAG_MOUSE2},
	{"No mouse detected", FLAG_NOMOUSE},
#if 0
      puts(msg(/*No mouse detected.*/100));      
#endif
	{"", FLAG_ALWAYS},
	{"              PRESS  ANY  KEY  TO  CONTINUE", FLAG_ALWAYS},
	{NULL, 0}
   };
   int i, buttons, flags, y;
   
   buttons = 0;

   flags = FLAG_ALWAYS;
   if (mouse_buttons < 0) flags |= FLAG_NOMOUSE;
   if (mouse_buttons >= 1) flags |= FLAG_MOUSE;
   if (mouse_buttons >= 2) flags |= FLAG_MOUSE2;
   if (mouse_buttons >= 3) flags |= FLAG_MOUSE3;
   
   /* display help text over currently displayed image */
   /* swap_screen(fFalse);*/ /* write to the displayed image */
   cvrotgfx_pre_supp_draw();
   set_tcolour(colHelp);

   y = 0;
   for (i = 0; help_msgs[i].msg; i++) {
      if (help_msgs[i].flags & flags) {
	 text_xy(4, 2 + y, help_msgs[i].msg);
	 y++;
      }
   }

   cvrotgfx_post_supp_draw();
   /* clear keyboard buffer */
   cvrotgfx_get_key();
   
   /* wait until key pressed or mouse clicked */
   do {
      if (mouse_buttons > 0) cvrotgfx_read_mouse(&i, &i, &buttons);
   } while (cvrotgfx_get_key() < 0 && !buttons);
   /* swap_screen(fTrue); */
}

/***************************************************************************/

void
translate_data(coord Xchange, coord Ychange, coord Zchange)
{
   int c;

   Xorg += Xchange;
   Yorg += Ychange;
   Zorg += Zchange;

   for (c = 0; ppLegs[c]; c++)
      do_translate(ppLegs[c], Xchange, Ychange, Zchange);
   for (c = 0; ppStns[c]; c++)
      do_translate_stns(ppStns[c], Xchange, Ychange, Zchange);
}

/**************************************************************************/

static char *cmdline_load_files(int argc, char **argv);

static void
parse_command(int argc, char **argv)
{
   char *p;

   /* skip argv[0] */
   argv++;
   argc--;
   if (argv[0] && argv[0][0] == '-' && argv[0][1] == 'c') {
      char *p;
      int i;
      p = argv[0] + 2;
      i = 0;
      while (*p && i < (sizeof(acolDraw) / sizeof(acolDraw[0]))) {
	 acolDraw[i++] = (unsigned char)strtol(p, &p, 0);
	 if (*p != ',') break;
	 p++;
      }
      argv++;
      argc--;
   }

   if (argc == 0) {
      puts(msgPerm(/*Syntax: caverot <image file>*/103));
      fatalerror(/*Bad command line parameters*/82);
   }

   putnl();
   puts(msg(/*Reading in data - please wait...*/105));
   
   p = cmdline_load_files(argc, argv);
   if (p) {
      printf(/*Bad 3d image file '%s'*/msg(106), p);
      putnl();
      exit(1);
   }
}

/* process the tail end of the command line */
/* (i.e. the bit which just lists one or more files to load */
static char *
cmdline_load_files(int argc, char **argv)
{
   int c;
   ppLegs = osmalloc((argc + 1) * sizeof(lid Huge *));
   ppStns = osmalloc((argc + 1) * sizeof(lid Huge *));
   /* load data into memory */
   for (c = 0; c < argc; c++) {
      if (!load_data(argv[c], ppLegs + c, ppStns + c)) return argv[c];
   }
   ppLegs[argc] = NULL;
   ppStns[argc] = NULL;
   return NULL;
}

/**************************************************************************/
/**************************************************************************/

#if (OS==RISCOS)
/* !HACK! this should really be in "armrot.c" */
void
do_translate(lid *plid, coord dX, coord dY, coord dZ)
{
   /* do_trans is an ARM code routine in armrot.s */
   extern do_trans(point*, coord, coord, coord);
   for ( ; plid; plid = plid->next) do_trans(plid->pData, dX, dY, dZ);
}
#endif
