/* > dosrot.c
 * Survex cave rotator plot & translate routines for MS-DOS
 * Also for Atari ST
 * Copyright (C) 1993-1997 Olly Betts
 */

/*
1993.08.02 split from "dosrot.h"
1993.08.11 added extern int X_size, Y_size;
           removed #include "dosrot.h"
           made stop() a far pointer
           hacked not to use pointers
1993.08.12 pRaw_data -> pData
1993.08.13 fettled header
           added NO_FUNC_PTRS and SETJMP -> INIT
1993.08.14 tidied up after bug-fix sesh with MF
           eliminated counter c and nasty DATA() macro
1993.08.16 Y being down screen is now dealt with via Y_UP macro in caverot.c
1993.12.12 Y_size -> ycMac; X_size -> xcMac
           brackets removed from (Option)
1993.12.14 (W) DOS point pointers made Huge to stop data wrapping at 64K
1993.12.15 minor fettles
1993.01.19 hacked for msc
1994.02.06 tidy up msc hacks for release
1994.03.19 added crosses
1994.03.23 corrected _drawto() to _lineto()
           added CROSS_SIZE
           added code to cope with BC probably not having _moveto and _lineto
1994.03.26 data is now in blocks
1994.03.29 started to update for finalised new scheme
           added splot*() and lplot*()
1993.03.30 fettled for Borland C
1994.04.06 and again
           fixed NULL label problem
1994.04.07 tidied up a little
1994.04.10 added TOS
1994.04.14 cross size reduced from 4 to 3
1994.04.16 use BLK() macro
1994.04.17 throw away labels that are off screen
1994.04.18 fixed centring of clip rectangle for labels
1994.04.19 stripped DOS eols
1994.09.23 implemented sliding point
1995.03.17 GRX-ed
1995.03.21 fettled indenting
1995.03.24 fixed for fancy labels
1995.03.28 centring in lplot*() removed and now done in fancy_label
1995.10.10 __GO32__ -> __DJGPP__
1996.04.04 fixed 3 warnings
1997.01.30 kludged in colour for JLIB
1997.02.01 fettled INIT, COND and DO_PLOT to improve code clarity
1998.03.22 autoconf-ed
*/

#include "caverot.h"
#include "useful.h"
#include "labels.h"

extern point Huge * pData;

/* Use macros to simplify code  */
# define drawcross(x,y) BLK( _moveto((x)-CS,(y)-CS); _lineto((x)+CS,(y)+CS); _moveto((x)-CS,(y)+CS); _lineto((x)+CS,(y)-CS); )

#if (OS==MSDOS)
# ifndef MSC
#  define _moveto(X,Y) moveto((X),(Y))
#  define _lineto(X,Y) lineto((X),(Y))
# endif
#elif (OS==TOS)
static int last_x=0,last_y=0;
# define _moveto(X,Y) BLK( last_x=(X); last_y=(Y); )
# define _lineto(X,Y) BLK( int x=(X),y=(Y); v_line(last_x,last_y,x,y); last_x=x; last_y=y; )
#endif

/* Cross size */
#define CS 3

#ifdef NO_FUNC_PTRS

/* really plebby version (needed if fn pointers won't fit in a coord) */
# define INIT() NOP /* do nothing */
# define COND(p) ((p)->Option!=STOP)

# define DO_PLOT(p,X,Y) if ((p)->Option==DRAW) _lineto((X),(Y)); else _moveto((X),(Y))

#elif defined(HAVE_SETJMP)

/* uses function pointers and setjmp (fastest for Borland C) */
# include <setjmp.h>

# define INIT() if (!setjmp(jbEnd)) NOP; else return /* store env; return after jmp */
# define COND(p) fTrue /* never exit loop by condition failing */
# define DO_PLOT(p,X,Y) (((void(*)(int,int))((p)->Option))( (X), (Y) ))

jmp_buf jbEnd; /* store for environment for exiting plot loop */

extern void far stop( int X, int Y ) {
   X=X; Y=Y; /* suppress compiler warnings */
   longjmp(jbEnd,1); /* return to setjmp() and return 1 & so exit function */
}

#else

/* uses function pointers, but not setjmp */
# define INIT() NOP /* do nothing */
# define COND(p) ((p)->Option!=STOP)
# define DO_PLOT(p,X,Y) (((void(*)(int,int))((p)->Option))( (X), (Y) ))

#endif

extern int xcMac, ycMac; /* defined in caverot.c */

/* general plot, used when neither of the special cases below applies */
void plot (point Huge *p,
     	   coord x1, coord x2, coord y1, coord y2, coord y3, int fixpt ) {
   int X,Y;   /* screen plot position */
   INIT();
   for ( ; COND(p) ; p++ ) {
      /* calc positions and shift right get screen co-ords */
      X = (int)((p->X*x1 + p->Y*x2) >> fixpt)           + (xcMac>>1);
      Y = (int)((p->X*y1 + p->Y*y2 + p->Z*y3) >> fixpt) + (ycMac>>1);
      DO_PLOT(p,X,Y);
   }
}

/**************************************************************************/

/* plot elevation, with view height set at zero */
void plot_no_tilt (point Huge *p, coord x1, coord x2, coord y3, int fixpt ) {
   int X,Y;   /* screen plot position */
   INIT();
   for ( ; COND(p) ; p++ ) {
      /* calc positions and shift right get screen co-ords */
      X = (int)((p->X*x1 + p->Y*x2) >> fixpt) + (xcMac>>1);
      Y = (int)((p->Z*y3) >> fixpt)           + (ycMac>>1);
      DO_PLOT(p,X,Y);
   }
}

/**************************************************************************/

/* plot plan only - ie Z co-ord and view height are totally ignored */
void plot_plan (point Huge *p,
     		coord x1, coord x2, coord y1, coord y2, int fixpt ) {
   int X,Y;   /* screen plot position */
   INIT();
   for ( ; COND(p) ; p++ ) {
      /* calc positions and shift right get screen co-ords */
      X = (int)((p->X*x1 + p->Y*x2) >> fixpt) + (xcMac>>1);
      Y = (int)((p->X*y1 + p->Y*y2) >> fixpt) + (ycMac>>1);
      DO_PLOT(p,X,Y);
   }
}

/**************************************************************************/

void do_translate ( lid Huge *plid, coord dX, coord dY, coord dZ ) {
   point Huge *p;
   for ( ; plid ; plid=plid->next ) {
      p=plid->pData;
      for ( ; p->Option!=STOP && p->Option!=(coord)-1 ; p++ ) {
      	 p->X += dX;
         p->Y += dY;
   	 p->Z += dZ;
      }
   }
}

/* general plot, used when neither of the special cases below applies */
void splot (point Huge *p,
     	    coord x1, coord x2, coord y1, coord y2, coord y3, int fixpt ) {
   int X,Y;   /* screen plot position */
   for ( ; p->Option!=(coord)-1 ; p++ ) {
      /* calc positions and shift right get screen co-ords */
      X = (int)((p->X*x1 + p->Y*x2) >> fixpt)           + (xcMac>>1);
      Y = (int)((p->X*y1 + p->Y*y2 + p->Z*y3) >> fixpt) + (ycMac>>1);
      drawcross(X,Y);
   }
}

/* plot elevation, with view height set at zero */
void splot_no_tilt (point Huge *p, coord x1, coord x2, coord y3,
                    int fixpt ) {
   int X,Y;   /* screen plot position */
   for ( ; p->Option!=(coord)-1 ; p++ ) {
      /* calc positions and shift right get screen co-ords */
      X = (int)((p->X*x1 + p->Y*x2) >> fixpt) + (xcMac>>1);
      Y = (int)((p->Z*y3) >> fixpt)           + (ycMac>>1);
      drawcross(X,Y);
   }
}

/* plot plan only - ie Z co-ord and view height are totally ignored */
void splot_plan (point Huge *p,
     		 coord x1, coord x2, coord y1, coord y2, int fixpt ) {
   int X,Y;   /* screen plot position */
   for ( ; p->Option!=(coord)-1 ; p++ ) {
      /* calc positions and shift right get screen co-ords */
      X = (int)((p->X*x1 + p->Y*x2) >> fixpt) + (xcMac>>1);
      Y = (int)((p->X*y1 + p->Y*y2) >> fixpt) + (ycMac>>1);
      drawcross(X,Y);
   }
}

/* general plot, used when neither of the special cases below applies */
void lplot (point Huge *p,
     	    coord x1, coord x2, coord y1, coord y2, coord y3, int fixpt ) {
   int X,Y;   /* screen plot position */
   for ( ; p->Option!=(coord)-1 ; p++ ) {
      /* calc positions and shift right get screen co-ords */
      X = (int)((p->X*x1 + p->Y*x2) >> fixpt);
      if ( (X<0 ? -X : X) > (xcMac>>1) ) continue;
      Y = (int)((p->X*y1 + p->Y*y2 + p->Z*y3) >> fixpt);
      if ( (Y<0 ? -Y : Y) > (ycMac>>1) ) continue;
      if (p->Option)
         fancy_label( (char*)(p->Option), X, Y );
/*         outtextxy( X, Y, (char*)(p->Option) ); */
   }
}

/* plot elevation, with view height set at zero */
void lplot_no_tilt (point Huge *p, coord x1, coord x2, coord y3,
                    int fixpt ) {
   int X,Y;   /* screen plot position */
   for ( ; p->Option!=(coord)-1 ; p++ ) {
      /* calc positions and shift right get screen co-ords */
      X = (int)((p->X*x1 + p->Y*x2) >> fixpt);
      if ( (X<0 ? -X : X) > (xcMac>>1) ) continue;
      Y = (int)((p->Z*y3) >> fixpt);
      if ( (Y<0 ? -Y : Y) > (ycMac>>1) ) continue;
      if (p->Option)
         fancy_label( (char*)(p->Option), X, Y );
/*         outtextxy( X, Y, (char*)(p->Option) ); */
   }
}

/* plot plan only - ie Z co-ord and view height are totally ignored */
void lplot_plan (point Huge *p,
     		 coord x1, coord x2, coord y1, coord y2, int fixpt ) {
   int X,Y;   /* screen plot position */
   for ( ; p->Option!=(coord)-1 ; p++ ) {
      /* calc positions and shift right get screen co-ords */
      X = (int)((p->X*x1 + p->Y*x2) >> fixpt);
      if ( (X<0 ? -X : X) > (xcMac>>1) ) continue;
      Y = (int)((p->X*y1 + p->Y*y2) >> fixpt);
      if ( (Y<0 ? -Y : Y) > (ycMac>>1) ) continue;
      if (p->Option)
         fancy_label( (char*)(p->Option), X, Y );
/*         outtextxy( X, Y, (char*)(p->Option) ); */
   }
}

#ifdef __DJGPP__
static int last_x=0, last_y=0;
extern void moveto( int X, int Y ) {
   last_x=X;
   last_y=Y;
}

extern void lineto( int X, int Y ) {
#ifdef JLIB
   extern int colDraw; /*!HACK! */
   buff_draw_line(BitMap,last_x,last_y,X,Y,colDraw);
#else
   GrLine(last_x,last_y,X,Y,15); /* 15 is colour !HACK! */
#endif
   last_x=X; last_y=Y;
}
#endif
