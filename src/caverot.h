/* > caverot.h
 * Data structures and #defines for cave rotator
 * Copyright (C) 1993-1995,1997 Olly Betts
 */

/*
1993.02.08 (W) Conversion from Ol's BASIC/Arm code
1993.04.06 Fettled some of the most awful bits ;)
           Added some sort of RISCOS #defines, etc
1993.04.07 Debugged - now runs under DOS
1993.04.22 modified to use "useful.h"
1993.04.24 hacked about a lot to make porting possible
1993.04.28 (W) new version fettled so DOS works again
1993.04.30 Unfettled a bit so it works on RISCOS
1993.05.01 added riscos code to tidy up on exit()
           now set infinite mouse bounding box
           flush keyboard buffer on RISCOS (& DOS hopefully)
1993.05.03 Minor fettle
1993.05.11 lessened the getmaxx() hack
1993.05.13 ensure RISCOS mouse pointer is turned off
1993.05.21 (W) add MOUSE_SUPPORT for when mouse.lib not available
1993.05.27 Changed unsupported mouse functions to macros and tidied
1993.05.28 MS driver middle mouse button works, so removed ?
1993.05.30 macro Y_UP isn't used anymore, but leave it for now...
1993.06.04 added () to #if
           removed nasty hacky SCREEN_SIZE macro
1993.06.08 under DOS, restore screen to mode on entry
           added automatic screen mode choice for DOS
           corected one fScreenSwap -> fSwapScreen in RISCOS code
1993.06.09 got bank switching working as wanted again (problem with help)
           under RISCOS, restore screen to mode on entry too
1993.06.14 moved OS specific stuff from caverot.c to here
1993.06.16 moved MOVE, DRAW and STOP #defs to dosrot.h and armrot.h
           removed some debugging code
1993.06.19 moved an OS specific #include back to caverot.c :(
1993.06.30 LF only eols
           minor fettle
1993.07.14 added quick hack to try mode 39 on the archie
1993.07.16 added clever code to choose highest-res mode under RISC OS
           introduce colDraw to determine colour used for drawing
1993.07.18 added macro OS_SPECIFIC_HEADER to make caverot.c OS independent
           define NO_MOUSE_SUPPORT to *prevent* it being compiled in - this
            means it can be defined externally (eg by makefile)
1993.08.02 removed functions to cvrotfns.h
1993.08.11 tidied a mite
1993.08.12 added a #include "whichos.h"
1993.08.15 DOS -> MSDOS
1993.10.26 Tidied
1994.03.26 added DATA_LEGS and DATA_STNS
1994.03.27 added datatype lid (List Item Data)
1994.03.29 ?plot*() moved here
1994.03.30 got rid of OS_SPECIFIC_HEADER
1994.04.06 added Huge-s for BC
1994.04.10 added TOS support
1994.04.14 fiddled a little
1994.08.11 added a few externs to kill warnings
1994.09.23 implemented sliding point
1994.11.13 added some externs to kill warnings
1995.03.24 added text_xy prototype
1995.03.28 fettled
1995.10.23 removed fPlan
1997.03.01 added #include "cvrotgfx.h"
1997.05.08 moved SIN(), etc here
1997.05.29 altered point structure
1997.06.03 now used by xcaverot.c
*/

/* these are common to all systems: */
#define BIG_MAGNIFY_FACTOR     (1.1236f) /* SHIFT-ed zoom in/out factor */
#define LITTLE_MAGNIFY_FACTOR  (1.06f)   /* standard zoom in/out factor */

#include "whichos.h"
#include "useful.h"
#include <limits.h>

#if (OS!=UNIX)
#include "cvrotgfx.h"
#endif

/* avoid problems with eg cos(10) where cos prototype is double cos(); */
#define SIN(X) (sin((double)(X)))
#define COS(X) (cos((double)(X)))
#define SQRT(X) (sqrt((double)(X)))

/* SIND(A)/COSD(A) give sine/cosine of angle A degrees */
#define SIND(X) (sin(rad((double)(X))))
#define COSD(X) (cos(rad((double)(X))))

/* machine specific stuff */
#if (OS==RISCOS)
# include "armrot.h"
#elif ((OS==MSDOS) || (OS==TOS))
# include "dosrot.h"
#elif (OS==UNIX)
# include "xrot.h"
#else
# error Operating System not known
#endif

extern coord Xorg, Yorg, Zorg; /* position of centre of survey */
extern coord Xrad, Yrad, Zrad; /* "radii" */
extern float scDefault;

/* Data structures */
typedef struct {
   union {
      coord action; /* plot option for this point - draw/move/endofdata */
      char *str; /* string for a label */
   } _;
   coord X,Y,Z;  /* coordinates */
} point;

typedef struct LID {
   int datatype;
   point Huge *pData;
   struct LID Huge *next;
} lid; /* List Item Data */

/* These are the datatype-s */
#define DATA_LEGS 0
#define DATA_STNS 1

#if 0
typedef struct {
   float nView_dir;               /* direction of view */
   float nView_dir_step;          /* step size of change in view */
   float nHeight;                 /* current height of view position */
   float nScale;                  /* current scale */
   coord iXcentr,iYcentr,iZcentr; /* centre of rotation of survey */
   bool  fRotating;               /* flag for rotating or not */
} view;
#endif

/* general plot (used if neither of the special cases applies */
void plot (point Huge *pData,
           coord x1, coord x2, coord y1, coord y2, coord y3, int fixpt );
void splot(point Huge *pData,
           coord x1, coord x2, coord y1, coord y2, coord y3, int fixpt );
void lplot(point Huge *pData,
           coord x1, coord x2, coord y1, coord y2, coord y3, int fixpt );

/* plot with viewheight=0 */
void plot_no_tilt (point Huge *pData,
                   coord x1, coord x2, coord y3, int fixpt );
void splot_no_tilt(point Huge *pData,
                   coord x1, coord x2, coord y3, int fixpt );
void lplot_no_tilt(point Huge *pData,
                   coord x1, coord x2, coord y3, int fixpt );

/* plot plan */
void plot_plan (point Huge *pData,
                coord x1, coord x2, coord y1, coord y2, int fixpt );
void splot_plan(point Huge *pData,
                coord x1, coord x2, coord y1, coord y2, int fixpt );
void lplot_plan(point Huge *pData,
                coord x1, coord x2, coord y1, coord y2, int fixpt );

/* translate whole cave */
void do_translate( lid Huge *plid, coord dX, coord dY, coord dZ );
#if (OS==RISCOS)
#define do_translate_stns do_translate
#else
void do_translate_stns( lid Huge *plid, coord dX, coord dY, coord dZ );
#endif

extern int   xcMac, ycMac;/* screen size in plot units (==pixels usually) */
extern float y_stretch;   /* multiplier for y to correct aspect ratio */

/* plot text x chars *right* and y *down* */
extern void (text_xy)(int x, int y, sz sz);
