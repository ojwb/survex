/* > caverot.c
 * Reads in SURVEX .3d image files & allows quick rotation and examination
 * Copyright (C) 1990,1993-1997 Olly Betts
 * Portions Copyright (C) 1993 Wookey
 */

/*
1993.02.08 (W) Conversion from Ol's BASIC/Arm code
1993.04.06 Fettled some of the most awful bits of Wook's code ;)
            intern -> coord, Xcentr -> Xorg
           tried to prise out OS dependant code
1993.04.07 Debugged - now runs under DOS
           Suppressed 6 yes-I-know warnings
           Sorted out filename pasing
           Altered View angle to be 'viewed towards'
1993.04.08 (with Wook) added DOS mouse code
           help screen -> graphics mode, cancelled by mouse click)
           changed to use switch in draw routines
           checked and corrected view angle
1993.04.22 modified to use "useful.h"
1993.04.24 hacked about a lot to make porting possible
1993.04.25 suppressed all warnings on Arc
           radical tidy up
1993.04.28 Fettled so new version works on DOS
1993.04.30 Unfettled a bit so it works on RISCOS
1993.05.02 renamed some things
           switch to graphics as late as possible now
           tidied up interface to ARM code
1993.05.03 Fixed YA sodding mouse bug (plan U/D by mouse wrong) - OKed RISCOS
1993.05.04 Moved getline() to useful.c
1993.05.05 Removed spaces at ends of lines
1993.05.11 Replaced X_size & Y_size, removed in a bout of overenthusiasm
1993.05.13 Respliced in the code to read binary files - very nasty hack, but
            I have exams much too soon, so it'll do for now
1993.05.19 (W) added filelist.h for extension use
1993.05.21 (W) added default filename if none given
               16 bit int/long fix so load_data works with files >32K
               added expiry check
1993.05.27 fettled expiry code to keep Norcroft C happy
1993.05.28 cPoints and c long in case we have >32767 points in a cave
            (unfortunately malloc() takes an int, but a different memory
            allocation routines might cope. (NB 20x KH to get this problem))
1993.05.30 cured dos up/down on plan bug (hopefully)
           corrected use in draw_view() of nHeight to z (passed view height)
           plan was reflected in 90<-->270 vertical plane
1993.06.04 debounced middle mouse button
           added () to #if
           FALSE -> fFalse
           reworded default 'image.3d' message
           scale acording to actual screen size rather than 'hack factor'
1993.06.05 changed type of cPoints and c to size_t
1993.06.06 moved #directives to start in column 1 for crap C compilers
1993.06.07 minor fettles
           corrected help screen disappearing if mouse not connected
1993.06.08 only redraw if view has changed to increase responsiveness and
            improve CGA which only has modes with one video page
1993.06.09 corrected help screen position and bank swapping around help
1993.06.10 tidied up a couple of messages
           try spong.3d if spong not found
1993.06.12 nasty, but faster dosrot.h coded and some mods here for it
1993.06.13 fiddled to get new DOS code to compile (faster by 5s from 35s)
1993.06.14 moved all OS specific to caverot.h
1993.06.16 malloc -> osmalloc
1993.06.19 moved one OS specific #include back :(
1993.06.30 LF only at ends of lines for consistency
           added SIN() COS() SIND() COSD() to neaten and to solve SUN et al
            double cos();  problem
           minor fettle
1993.07.14 tweaked code so it copes with null-padding of binary image.3d
            files
1993.07.16 changed code so that cave isn't always drawn in colour 2
1993.07.17 changed uses of fgetpos/fsetpos to ftell/fseek
1993.07.18 No references to OS anymore
1993.07.20 replaced image file reading code by calls to readimg.c functions
1993.07.27 and now changed them to img.c calls
1993.08.02 fettled to get it to compile
           split load_data off into "cvrot_img.c"
1993.08.08 tidied and reduced lines to 78 or less chars
           cvrot_img.c -> cvrotimg.c (MSDOS needs <= 8 char filenames)
1993.08.10 fettled Copyright message displayed on start-up
           now print version number (from version.h)
1993.08.11 minor tidy
1993.08.12 pRaw_data -> pData
           rearranged code so that parse_command calls load_data
1993.08.13 fettled header
           removed #include "img.h"
           tidied up parse_command a bit
1993.08.14 sorted out realloc bug in cvrotimg.c with help from MF
1993.08.15 minor fettle
           parse_command now returns pointer to the data read in
1993.08.16 added y_stretch to allow aspect ratio correction
           use Y_UP and y_stretch to deal with DOS Y increasing down screen
1993.08.19 fettled parse_command to kill bc warnings
1993.11.03 changed error routines
1993.11.05 changed error numbers so we can use same error list as survex
           extracted all messages except help screen and (C) msgs
           removed use of image.3d if no arguments given
1993.11.14 removed 'sz lfImagefile=IMAGE_FILE;'
1993.11.21 ROTERRS_FILE -> MESSAGE_FILE
1993.11.30 sorted out fatal and error
1993.12.07 X_size -> xcMac, Y_size -> ycMac
           array indices from int -> OSSIZE_T
1993.12.14 (W) DOS point pointers made Huge to stop data wrapping at 64K
1993.12.15 minor fettles
1993.12.16 fiddled with to fix problem which was actually in makefile
1994.01.05 defaults to plan view on start-up
1994.03.15 error 10->38
1994.03.22 scale_to_screen now copes with ystretch being not very near 1
1994.03.26 data is now blocked with legs and stations separate
1994.03.27 changed my mind a bit
1994.03.28 pData is now passed to plot functions rather than being global
           split off rotplot.c
           add draw_view_labs()
           now draw crosses and labels on idle
           nuked commented out calloc
1994.04.06 added Huge-s for BC
           do_translate now passed correct params
           added abs() in case y_stretch is negative
1994.04.07 added (float) to suppress warning
           tidied up translate_data()
1994.04.10 added ^L,^S,^N to control display of legs, stns and names
1994.04.13 DOS doesn't like ^S, so use ^P
1994.04.14 suppressed `LEGS' not used warning
1994.04.19 ^P no good for DOS - trying ^X (what a fun game)
           C,V now relative to cave; Shift accel's C,V and :,/
           fiddled with help page a bit
           added ^A to draw toggle [A]ll redrawn
1994.04.27 lfMessages gone; item is initialised to kill warnings
1994.05.05 fixed init of item
1994.05.11 fixed tilt bug (+ should be *)
1994.05.13 swapped LEGS and STNS in ordering of items
1994.06.09 added SURVEXHOME; added static in front of lots of variables
1994.06.19 removed static from xcMac and ycMac as dosrot.c wants to see them
1994.06.20 created cvrotimg.h
	   added int argument to warning, error and fatal
1994.06.21 created rotplot.h
1994.08.31 tweaked help screen display code to end on NULL rather than ""
1994.09.10 removed prototypes duplicated from rotplot.h
           added call to draw_scale_bar
1994.09.22 fixed bug if no stations
1994.09.23 sliding point implemented; reversed tilt controls for consistency
1994.10.05 added language arg to ReadErrorFile
1994.11.13 added code to test aspect ratio works; fettled indenting
           fixed sliding point problems
1995.03.18 added clever labels
1995.03.21 added text_xy
1995.03.23 minor fettles
1995.03.24 pthMe not static
1995.03.25 added fAllNames (O toggles)
1995.03.28 added "O" to help screen
1995.04.17 moved rotate code into process_key(); fettled process_key()
1995.09.27 hacked in coloured plots of multi-surveys for BCRA conference
1995.10.12 fettled
1995.10.19 changed set_view to use phi instead of z
1995.10.23 removed fPlan
1995.10.26 added stuff
1995.11.20 timing code adjusted
1995.11.22 chose some better colours
1995.12.11 smooth tilt only used if it would take 6 or more steps
1996.01.21 added call to exit_graphics()
1996.02.15 crosses off by default
1996.02.19 dt set to 1 if 0 or negative
1996.03.19 fixed timing code to take CLOCKS_PER_SEC into account
1996.03.27 CLOCKS_PER_SEC fix was off by factor of 10000
           now defaults to plan again
1996.04.04 fixed 2 warnings
1996.04.09 fixed 3 djgpp warnings ; scale_to_screen reworked
1996.05.04 new scale_to_screen() broken -- hacked old version to work better
1996.05.05 added ANIMATE stuff for Dave Gibson
           added locked to hnadle "flat" plots aligned with axes
1997.01.10 worked in code for specifying colours on the command line
1997.02.01 timing code now ignores time spent displaying help screen
1997.02.15 added some debug code
*/

/*#define ANIMATE*/
#define ANIMATE_FPS 1

static unsigned char acolDraw[]=
   {255,23,119,139,67,210,207,226,31,39,55,83,186,195,75};

/* avoid problems with eg cos(10) where cos prototype is double cos(); */
#define SIN(X) (sin((double)(X)))
#define COS(X) (cos((double)(X)))
#define SQRT(X) (sqrt((double)(X)))

/* SIND(A)/COSD(A) give sine/cosine of angle A degrees */
#define SIND(X) (sin(rad((double)(X))))
#define COSD(X) (cos(rad((double)(X))))

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <float.h>
#include <time.h>

#include "useful.h"
#include "error.h"
#include "version.h"
#include "filelist.h"

#include "caverot.h"
#include "cvrotimg.h"
#include "cvrotfns.h"
#include "rotplot.h"
#include "labels.h"

/***************************************************************************/

/* prototypes */

void setup(void);                              /* initialize as required */
static void parse_command(int argc, char *argv[]);
static void show_help(void);                          /* displays help screen */
void set_defaults(void);                       /* reset default values */
bool process_key(void);        /* read & process keyboard and mouse input */
void swap_screen( bool );      /* swap displayed screen and drawing screen */

/* translate all points */
void translate_data (coord Xchange, coord Ychange, coord Zchange);

#if 0
void get_view_details (void);                 /* get info for new view */
#endif

#if 0
float scale_to_screen( lid Huge **pplid );
#else
float scale_to_screen(lid Huge **pplid,lid Huge **pplid2);
#endif

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
static bool  fRotating;        /* flag indicating auto-rotation */
static bool  fNames = fFalse;  /* Draw station names? */
       bool  fAllNames = fFalse;  /* Draw all station names? */
static bool  fLegs  = fTrue;   /* Draw legs? */
static bool  fStns  = fFalse;  /* Draw crosses at stns? */
static bool  fAll   = fFalse;  /* Draw all during movement? */
static bool  fRevSense = fFalse; /* Movements relate to cave/observer */
static bool  fSlowMachine = fFalse; /* Controls slick tilt, etc */
static float elev;             /* angle of elevation of viewpoint */
static float nStepsize;        /* stepsize for movements */
static bool  f3D;              /* flag indicating red/green 3d view mode */
coord Xorg, Yorg, Zorg; /* position of centre of survey */
coord Xrad, Yrad, Zrad; /* "radii" */
static int   cMouseBut;        /* # of mouse buttons; -ve => mouse dead */
int   xcMac, ycMac;            /* screen size in plot units (==pixels usually) */
#if 1
#ifdef Y_UP
float y_stretch=1.0f;   /* factor to multiply y by to correct aspect ratio */
#else /* !defined(Y_UP) */
float y_stretch=-1.0f;  /* factor to multiply y by to correct aspect ratio */
#endif
#else
float y_stretch=-0.3f;  /* test that aspect ratio works */
#endif

sz    pthMe;            /* path executable was run from */

#if 0
static int   Stored_views;     /* number of defined stores */
static view * pView_store;     /* pointer to array of view stores */
#endif

static int locked; /* no data along this axis (1=>x, 2=>y, 3=>z, 0=>none) */

#ifdef ANIMATE
int cAnimate = 0;
int incAnimate = 1;
#endif

static lid Huge **ppLegs=NULL;
static lid Huge **ppStns=NULL;

#ifndef ANIMATE
static void draw_legs(void) {
   int c;
   for(c=0;ppLegs[c];c++) {
      set_gcolour(acolDraw[c]);
      draw_view_legs(ppLegs[c]);
   }
}
#else
static void draw_legs(void) {
   set_gcolour(acolDraw[0]);
   draw_view_legs(ppLegs[cAnimate]);
}
#endif

#ifndef ANIMATE
static void draw_stns(void) {
   int c;
   for(c=0;ppStns[c];c++) {
      set_gcolour(acolDraw[c]);
      draw_view_stns(ppStns[c]);
   }
}
#else
static void draw_stns(void) {
   set_gcolour(acolDraw[0]);
   draw_view_stns(ppStns[cAnimate]);
}
#endif

#ifndef ANIMATE
static void draw_names(void) {
   int c;
   set_tcolour(colText);
   for(c=0;ppStns[c];c++) {
      draw_view_labs(ppStns[c]);
   }
}
#else
static void draw_names(void) {
   set_tcolour(colText);
   draw_view_labs(ppStns[cAnimate]);
}
#endif

/*************************************************************************/
/*************************************************************************/

int main (int argc, sz argv[]) {
 enum { LEGS, STNS, LABS, DONE} item=LEGS;
 pthMe=ReadErrorFile( "Cave Rotator", "SURVEXHOME", "SURVEXLANG", argv[0],
                      MESSAGE_FILE );

 printf("Survex cave rotator v"VERSION"\n"
        "  Copyright (C) 1990,1993-"THIS_YEAR" Olly Betts\n"
        "  Portions Copyright (C) 1993 Wookey\n");

#include "expire.h"

 parse_command( argc, argv );

 /* init mouse driver (returns <0 for failure) */
 cMouseBut=init_mouse();
 if (cMouseBut<0) {
  puts(msg(100)); /* No mouse detected */
  putnl();
 }

 /* these aren't in set_defaults() 'cos we don't want DELETE to reset them */
 degView = 0;
 elev = 90.0f;       /* display plan by default */
 fRotating = fFalse; /* stationary to start with */
 f3D = fFalse;       /* red/green 3d view off by default */
 degStereoSep=2.5f;

 /* setup colour values for 3dview (RED_3D,GREEN_3D,BLUE_3D) or ??? */
#if 0
 Stored_views =9;
 pView_store = (viewstore*) osmalloc(Stored_views*sizeof(view))
 /* and initialise */
#endif

 /* initialise graphics h/ware & s/ware */
 init_graphics();
 init_map();

 /* display help screen to give user something to read while we do scaling */
 cleardevice();
 swap_screen( fTrue );
 show_help();

 /* can't do this until after we've initialised the graphics */
#if 0
 {
    float scTemp;
    scDefault = scale_to_screen(ppLegs);
    scTemp = scale_to_screen(ppStns);
    if (scTemp < scDefault)
      scDefault = scTemp;
 }
#else
 scDefault = scale_to_screen(ppLegs,ppStns);
#endif
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
   default:
    break;
 }

 /* set base step size according to screen size */
 nStepsize = min(xcMac,ycMac)/25.0f;
 set_defaults();

 {
  bool fRedraw=fTrue;
  sz szPlan,szElev;

  szPlan=msgPerm(101);
  szElev=msgPerm(102);

  while (fTrue) {
   char sz[80];

   if (fRedraw) {
    while (degView >= 360) degView -= 360;
    while (degView <0) degView += 360;
    clear_map();
    cleardevice();
    if (f3D) {
#if 0
     int c;
     set_gcolour(col3dL);
     set_view(sc,degView-degStereoSep,elev);
     for(c=0;ppLegs[c];c++) draw_view_legs(ppLegs[c]);
     set_gcolour(col3dR);
     set_view(sc,degView+degStereoSep,elev);
     for(c=0;ppLegs[c];c++) draw_view_legs(ppLegs[c]);
#endif
    } else {
     set_view(sc,degView,elev);
     if (fAll) {
      if (fLegs) draw_legs();
      if (fStns) draw_stns();
      if (fNames) draw_names();
      item=DONE;
     } else {
      if (fLegs) {
       draw_legs();
       item=STNS;
      } else {
       if (fStns) {
        draw_stns();
        item=LABS;
       } else {
        if (fNames) draw_names();
        item=DONE;
       }
      }
     }
    }
    set_tcolour(colText);
    sprintf(sz,(elev==90.0f)?szPlan:szElev,degView);
    text_xy(0,0,sz); /* outtextxy(8,0,sz); */
    draw_scale_bar();
/*    fprintf(fhDbug,"Drawing %s view\n", (elev==90.0f)?"plan":"side" ); */
    swap_screen( fTrue );
   } else {
    switch (item) {
     case STNS:
      if (fStns) {
       swap_screen(fFalse);
       draw_stns();
       bop_screen();
      }
      item++;
      break;
     case LABS:
      if (fNames) {
       swap_screen(fFalse);
       draw_names();
       bop_screen();
      }
      item++;
      break;
     default:
      break; /* suppress warning */
    }
   }

/*   if (fDemo)
      fRedraw=demo_step();
   else */
   fRedraw=process_key();
  }
 }
}

/***************************************************************************/

#define BIG_SCALE 1e3f
#if 0
float scale_to_screen( lid Huge **pplid ) {
   /* run through data to find max & min points */
   coord Xmin,Xmax,Ymin,Ymax,Zmin,Zmax; /* min & max values of co-ords */
   coord Radius; /* radius of plan */
   point Huge * p;
   lid Huge *plid;
   bool fData=0;

   if (pplid) {
      for(;*pplid;pplid++){
	 plid=*pplid;
	 p=plid->pData;

	 if (!fData) {
	    Xmin = Xmax = p->X;
	    Ymin = Ymax = p->Y;
	    Zmin = Zmax = p->Z;
	    fData=1;
	 }

	 for( ; plid ; plid=plid->next ) {
	    p=plid->pData;
	    for ( ; p->Option!=STOP && p->Option!=(coord)-1 ; p++ ) {
	       if (p->X < Xmin) Xmin = p->X; else if (p->X > Xmax) Xmax = p->X;
	       if (p->Y < Ymin) Ymin = p->Y; else if (p->Y > Ymax) Ymax = p->Y;
	       if (p->Z < Zmin) Zmin = p->Z; else if (p->Z > Zmax) Zmax = p->Z;
	    }
	 }
      }
   }

   /* if no data, return BIG_SCALE as scale factor */
   if (!fData)
     return (BIG_SCALE);

   /* centre survey in each (spatial) dimension */
   Xorg = (Xmin+Xmax)/2; Yorg = (Ymin+Ymax)/2; Zorg = (Zmin+Zmax)/2;
   Xrad = (Xmax-Xmin)/2; Yrad = (Ymax-Ymin)/2; Zrad = (Zmax-Zmin)/2;
   Radius = (coord)(SQRT(sqrd((double)Xrad) + sqrd((double)Yrad)));
   if (Radius==0 && Zrad==0)
     return (BIG_SCALE);

   return (0.5f*0.99f*min((float)xcMac,(float)ycMac/(float)fabs(y_stretch)))
     / (float) (max( Radius, Zrad ));
}
#else
float scale_to_screen( lid Huge **pplid, lid Huge **pplid2 ) {
   /* run through data to find max & min points */
   coord Xmin,Xmax,Ymin,Ymax,Zmin,Zmax; /* min & max values of co-ords */
   coord Radius; /* radius of plan */
   point Huge * p;
   lid Huge *plid;
   bool fData=0;

   /* if no data, return BIG_SCALE as scale factor */
   if (!pplid || !*pplid) {
      pplid=pplid2;
      pplid2=NULL;
   }

   if (!pplid || !*pplid)
      return (BIG_SCALE);

xxx:
   for(;*pplid;pplid++){
      plid=*pplid;
      p=plid->pData;

      if (!p || p->Option==STOP || p->Option==(coord)-1)
         continue;

      if (!fData) {
         Xmin = Xmax = p->X;
         Ymin = Ymax = p->Y;
         Zmin = Zmax = p->Z;
         fData=1;
      }

      for( ; plid ; plid=plid->next ) {
         p=plid->pData;
         for ( ; p->Option!=STOP && p->Option!=(coord)-1 ; p++ ) {
            if (p->X < Xmin) Xmin = p->X; else if (p->X > Xmax) Xmax = p->X;
            if (p->Y < Ymin) Ymin = p->Y; else if (p->Y > Ymax) Ymax = p->Y;
            if (p->Z < Zmin) Zmin = p->Z; else if (p->Z > Zmax) Zmax = p->Z;
         }
      }
   }
   if (pplid2) {
      pplid=pplid2;
      pplid2=NULL;
      goto xxx;
   }
   /* centre survey in each (spatial) dimension */
   Xorg = (Xmin+Xmax)/2; Yorg = (Ymin+Ymax)/2; Zorg = (Zmin+Zmax)/2;
   Xrad = (Xmax-Xmin)/2; Yrad = (Ymax-Ymin)/2; Zrad = (Zmax-Zmin)/2;
   Radius = (coord)(SQRT(sqrd((double)Xrad) + sqrd((double)Yrad)));
   if (Radius==0 && Zrad==0)
      return (BIG_SCALE);

   return (0.5f*0.99f*min((float)xcMac,(float)ycMac/(float)fabs(y_stretch)))
                              / (float) (max( Radius, Zrad ));
}
#endif

/***************************************************************************/

/* things in here get reset by DELETE key */
void set_defaults (void) {
   degViewStep = 3;
   sc = scDefault;
   translate_data (-Xorg,-Yorg,-Zorg); /* rotate about 'centre' of cave */
}

/***************************************************************************/

/*
bool demo_step(void) {
   static n=0;
   struct {
   };
} */
int dt;

bool process_key(void) /* and mouse! */ {
   float nStep, Accel;
   int iKeycode;
   static bool fRedraw=fFalse;
   double s=SIND(degView), c=COSD(degView);
   static int autotilt=0;
   static float autotilttarget;
   static clock_t time=0;
   clock_t old_time;
   static float tsc=1.0f; /* sane starting value */
#ifdef ANIMATE
   static clock_t last_animate=0;
#endif

   old_time=time;
   time=clock();
   if (fRedraw) {
      dt = (int)(time-old_time);
      /* better to be paranoid */
      if (dt<1)
         dt=1;
      tsc = (float)dt*(10.0f/CLOCKS_PER_SEC);
      if (tsc>1000.0f) { beep(); tsc=1000.0f; }
      fRedraw = fFalse;
   }

#ifdef ANIMATE
   if (time-last_animate > CLOCKS_PER_SEC/ANIMATE_FPS) {
      cAnimate += incAnimate;
      if (cAnimate == 0 || ppLegs[cAnimate+1]==NULL)
         incAnimate = -incAnimate;
      if (last_animate==0)
         last_animate=time;
      else
         last_animate += CLOCKS_PER_SEC/ANIMATE_FPS;
      fRedraw = fTrue;
   }
#endif

   if (shift_pressed()) {
      nStep = 5.0f * nStepsize/sc;
      ZoomFactor = BIG_MAGNIFY_FACTOR;
      Accel = 5.0f;
   } else {
      nStep = nStepsize/sc;
      ZoomFactor = LITTLE_MAGNIFY_FACTOR;
      Accel = 1.0f;
   }
   nStep *= tsc;
   ZoomFactor = pow(ZoomFactor,tsc);
   Accel *= tsc;

   if (fRevSense)
      nStep = -nStep;

   iKeycode = get_key();
   if (iKeycode>-1) {
      autotilt=0;
      if (iKeycode >= 'a' && iKeycode <= 'z')
       iKeycode -= 32; /* force lower case letters to upper case */
      if (locked==0 || locked==3) {
       /* no restriction, or plan only */
       switch (iKeycode) {
/*        case '3': case SHIFT_3:    f3D = !f3D; break; */
        case 'Z': if (degViewStep < 45) degViewStep *= 1.2f; break;
        case 'X': degViewStep /=1.2f;  break;
        case 'C': degView -= degViewStep*Accel; fRedraw=fTrue; break;
        case 'V': degView += degViewStep*Accel; fRedraw=fTrue; break;
        case 'R': degViewStep = -degViewStep; break;
        case ' ':            fRotating = fFalse; break;
        case RETURN_KEY:     fRotating = fTrue; break;
       }
      }
      if (locked==0) {
       switch (iKeycode) {
        case QUOTE: case SHIFT_QUOTE:
          if (fRevSense) goto tiltup;
          tiltdown:
          if (elev>-90.0f) {
             elev -= degViewStep*Accel;
             if (elev<-90.0f) elev=-90.0f;
             fRedraw=fTrue;
          }
          break;
        case '/': case'?':
          if (fRevSense) goto tiltdown;
          tiltup:
          if (elev<90.0f) {
             elev += degViewStep*Accel;
             if (elev>90.0f) elev=90.0f;
             fRedraw=fTrue;
          }
          break;
        case 'P':   if (elev!=90.0f) {
                       if (fSlowMachine)
                          elev=90.0f;
                       else {
/*fprintf(stderr,"P:tsc = %f\n",tsc);fflush(stderr);*/
                          autotilt=ceil(90.0/ceil(9.0/tsc));
/*fprintf(stderr,"P:autotilt = %d\n",autotilt);fflush(stderr);*/
                          if (autotilt>16.0f) {
                             autotilt=0;
                             elev=90.0f;
                          }
                       }
                       autotilttarget=90.0f;
                       fRedraw=fTrue;
                    }
                    break;
        case 'L':   if (elev!=0.0f) {
                       if (fSlowMachine)
                          elev=0.0f;
                       else {
/*fprintf(stderr,"L:tsc = %f\n",tsc);fflush(stderr);*/
                          autotilt = ceil(90.0/ceil(9.0/tsc));
/*fprintf(stderr,"L:autotilt = %d\n",autotilt);fflush(stderr);*/
                          if (autotilt>16.0f) {
                             elev=0.0f;
                             autotilt=0;
                          } else if (elev>0.0f)
                             autotilt = -autotilt;
	  	       }
                       autotilttarget=0.0f;
                       fRedraw=fTrue;
                    }
                    break;
       }
      }
      switch (iKeycode) {
       case ']': case'}':
         sc = fRevSense ? sc/ZoomFactor : sc*ZoomFactor;
         fRedraw=fTrue; break;
       case '[': case'{':
         sc = fRevSense ? sc*ZoomFactor : sc/ZoomFactor;
         fRedraw=fTrue; break;
       case 'U': translate_data(0,0, (coord)nStep); fRedraw=fTrue; break;
       case 'D': translate_data(0,0,(coord)-nStep); fRedraw=fTrue; break;
       case 'N': translate_data(0, (coord)nStep,0); fRedraw=fTrue; break;
       case 'S': translate_data(0,(coord)-nStep,0); fRedraw=fTrue; break;
       case 'E': translate_data( (coord)nStep,0,0); fRedraw=fTrue; break;
       case 'W': translate_data((coord)-nStep,0,0); fRedraw=fTrue; break;
       case DELETE_KEY:  set_defaults(); fRedraw=fTrue; break;
       case 'O':   fAllNames=!fAllNames; fRedraw=fNames; break;
/*       case 'I':   get_view_details(); fRedraw=fTrue; break; */
       case CURSOR_UP:
         if (elev==90.0f)
            translate_data( (coord)(nStep*s), (coord)(nStep*c), 0 );
         else
            translate_data( 0, 0, (coord)nStep );
         fRedraw=fTrue; break;
       case CURSOR_DOWN:
         if (elev==90.0f)
            translate_data( -(coord)(nStep*s), -(coord)(nStep*c), 0 );
         else
            translate_data( 0,0,(coord)-nStep);
         fRedraw=fTrue; break;
       case CURSOR_LEFT:
         translate_data( -(coord)(nStep*c), (coord)(nStep*s), 0 );
         fRedraw=fTrue; break;
       case CURSOR_RIGHT:
         translate_data(  (coord)(nStep*c), -(coord)(nStep*s), 0 );
         fRedraw=fTrue; break;
       case 'H': {
	  clock_t start_time;
	  start_time = clock();
	  fRedraw=fTrue;
	  show_help();
	  time += (clock() - start_time); /* ignore time user spends viewing help */
	  /* cruder: time = clock(); */
	  break;
       }
       case ('A'-'@'):
         fAll = !fAll; break; /* no need to redraw */
       case ('N'-'@'):
         fRedraw=fTrue; fNames = !fNames; break;
       case ('X'-'@'):
         fRedraw=fTrue; fStns = !fStns; break;
       case ('L'-'@'):
         fRedraw=fTrue; fLegs = !fLegs; break;
       case ('R'-'@'):
         fRevSense=!fRevSense; break;
       case ESCAPE_KEY:
	 exit_graphics();
         exit(EXIT_SUCCESS); break; /* That's all folks */
      }
   }

   if (cMouseBut>=0) {
      static fOldMBut=fFalse;
      int buttons=0; /* zero in case read_mouse() doesn't set */
      int dx=0, dy=0;
      read_mouse(&dx,&dy,&buttons);
      if (buttons&(LBUT|RBUT|MBUT))
         autotilt=0;
      if (buttons&LBUT) {
         if (fRevSense) {
            sc*=(float)pow(LITTLE_MAGNIFY_FACTOR,0.08*dy);
            if (locked==0 || locked==3) degView+=dx*0.16f;
         } else {
            sc*=(float)pow(LITTLE_MAGNIFY_FACTOR,-0.08*dy);
            if (locked==0 || locked==3) degView-=dx*0.16f;
         }
         fRedraw=fTrue;
      }
      if (buttons&RBUT) {
         nStep*=0.025f;
         if (elev==90.0f)
            translate_data( (coord)(nStep * ( dx*c + dy*s ) ),
                           -(coord)(nStep * ( dx*s - dy*c ) ), 0 );
         else
            translate_data( (coord)(nStep * (double)dx*c ),
                            -(coord)(nStep * (double)dx*s ),
                            (coord)(nStep * dy));
         fRedraw=fTrue;
      }
      if (locked ==0 && (buttons&MBUT) && !fOldMBut) {
         elev = (elev==90.0f) ? 0.0f : 90.0f;
         fRedraw=fTrue;
      }
      fOldMBut=((buttons&MBUT)>0);
   }

   if (autotilt) {
      elev += (float)autotilt;
      fRedraw=fTrue;
      if (autotilt>0) {
         if (elev>=autotilttarget) {
            elev=autotilttarget;
            autotilt=0;
         }
      } else {
         if (elev<=autotilttarget) {
            elev=autotilttarget;
            autotilt=0;
         }
      }
   }

   /* rotate by current stepsize */
   if (fRotating) {
      degView += degViewStep*tsc;
      fRedraw=fTrue; /* and force a redraw */
   }

   return (fRedraw);
}

/***************************************************************************/

static void show_help(void) {
 /* help text stored as static array of strings, and printed as graphics
  *  to the screen */
 static char *szHelpArray[]= {
  "                  Z,X : Faster/Slower rotation",
  "                    R : [R]everse direction of rotation",
  "          Enter,Space : Start/Stop auto-rotation",
  "                  C,V : Rotate cave one step clockwise/anti",
  "                  :,/ : Higher/Lower viewpoint",
  "                  ],[ : Zoom In/Out",
  "                  U,D : Cave [U]p/[D]own",
  "              N,S,E,W : Cave [N]orth/[S]outh/[E]ast/[W]est",
  "               Delete : Default scale, rotation rate, etc",
  "                  P,L : [P]lan/e[L]evation",
/*  "                    I : [I]nput values", */
/*  "                  3,# : Toggle [3]D goggles view", */
  "    Cursor Left,Right : Cave [Left]/[Right] (on screen)",
  "       Cursor Up,Down : Cave [Up]/[Down] (on screen)",
/*  "    Sft Copy + Letter : Store view in store <Letter>", */
/*  "        Copy + Letter : Recall view in store <Letter>", */
  "               Ctrl-N : Toggle display of station [N]ames",
  "               Ctrl-X : Toggle display of crosses ([X]s) at stations",
  "               Ctrl-L : Toggle display of survey [L]egs",
  "               Ctrl-A : Toggle display of [A]ll/skeleton when moving",
  "                    O : Toggle display of non-overlapping/all names",
  "               Ctrl-R : Reverse sense of controls",
  "               ESCAPE : Quit program",
  "             Shift accelerates all movement keys"
  "",
  "                    H : [H]elp screen (this!)",
  "",
  "Mouse controls:  LEFT : left/right rotates, up/down zooms",
  "  (if present) MIDDLE : toggles plan/elevation",
  "  (if present)  RIGHT : mouse moves cave",
  "",
  "              PRESS  ANY  KEY  TO  CONTINUE",
  NULL
 };
 int i, buttons=0;
#if 0
 printf("Mouse %s (%d buttons)\n",
                               (cMouseBut>=0)?"present":"absent",cMouseBut);
#endif

 /* display help text over currently displayed image */
 swap_screen( fFalse ); /* write to the displayed image */
 set_tcolour(colText);
 for( i=0 ; szHelpArray[i] ; i++ ) /* go through array until we find NULL */
    text_xy( 4, 2+i, szHelpArray[i] );
 /*  outtextxy( 48 *//*(xcMac/2)-210*//*, 24+i*12, szHelpArray[i] ); */

 bop_screen();
 /* clear keyboard buffer */
 get_key();

 /* wait until key pressed or mouse clicked */
 do {
    if (cMouseBut>0)
       read_mouse( &i, &i, &buttons );
 } while (get_key()<0 && !buttons);
/* swap_screen( fTrue ); */
}

/***************************************************************************/

#if 0
static void get_view_details(void) {
   /* input some numbers */
}
#endif

/***************************************************************************/

void translate_data (coord Xchange, coord Ychange, coord Zchange) {
   int c;
   Xorg+= Xchange; Yorg+= Ychange; Zorg+= Zchange;
   for(c=0;ppLegs[c];c++)
      do_translate( ppLegs[c], Xchange, Ychange, Zchange );
   for(c=0;ppStns[c];c++)
      do_translate( ppStns[c], Xchange, Ychange, Zchange );
}

/**************************************************************************/

static void parse_command (int argc, char *argv[] ) {
   int c;
   int start = 1;
   if (argc<2) {
      fatal(82,wr,msgPerm(103),0); /* syntax error */
   }
   if (argv[1][0]=='-' && argv[1][1]=='c') {
      char *p;
      int i;
      start++;
      p=argv[1]+2;
      i=0;
      while(*p && i<(sizeof(acolDraw)/sizeof(acolDraw[0]))) {
	 acolDraw[i++] = (unsigned char)strtol(p,&p,0);
	 if (*p!=',') {
	    break;
	 }
	 p++;
      }
   }
   ppLegs=osmalloc((argc-start+1)*sizeof(lid Huge *));
   ppStns=osmalloc((argc-start+1)*sizeof(lid Huge *));
   /* load data into memory */
   /* !HACK! load_data returns bool (but always fTrue currently) */
   for (c=start;c<argc;c++)
      load_data(argv[c],ppLegs+(c-start),ppStns+(c-start));
   ppLegs[argc-start]=NULL;
   ppStns[argc-start]=NULL;
}

/**************************************************************************/
/**************************************************************************/
