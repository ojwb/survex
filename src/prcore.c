/* > prindept.c
 * Printer independent parts of Survex printer drivers
 * Copyright (C) 1993-1997 Olly Betts
 */

/*
1993.04.30 Started revision history
           Added expiry code
1993.05.01 General fiddling
1993.05.03 Added fNoBorder hack
1993.05.04 Added (C) symbol & adjusted info box a little
           getline() moved to useful.c
1993.05.05 froody scale bar code added
1993.05.22 (W) changed to use filelist.h
           (W) added code to use 'image.3d' if no filename given
1993.05.27 fettled
1993.06.04 added #define CRAP_SCALE_BAR 1 option for old-style scale bar
           FALSE -> fFalse
           added hack for GCC sprintf
1993.06.05 fixed Unix v ANSI sprintf() problem
           added code to catch empty .3d files
           recoded DescribeLayout()
1993.06.07 view bearing now defaults to 000
1993.06.10 name changed
           added code to read binary .3d files
           fixed newish bug in DescribeLayout()
1993.06.11 fixed very new bug in DescribeLayout
           view defaults to plan if newline pressed
1993.06.16 malloc() -> osmalloc()
1993.06.28 Sun math.h has 'double pow()' etc - must pass doubles (MF)
1993.06.29 Turned border on
1993.06.30 killed a couple of Norcroft warnings introduced by math.h fix
1993.07.02 VIEW/UP PAGE bearing now always 3 digits, also 'Elev on' bearings
           fixed bug which meant view was always on 000 degrees
1993.07.18 tidied up scale bar code, so I can quote it in my dissertation
1993.07.20 extracted image file reading code to readimg.c
1993.07.27 converted from readimg.c -> img.c
1993.08.10 default scale used is now 1:<default_scale>
           now print version number (from version.h)
           now print default rotation if return pressed
           fettled a little
1993.08.13 fettled header
           corrected plan rotation (rotated clockwise, not a/cwise)
1993.11.03 changed error routines
1993.11.05 changed error numbers so we can use same error list as survex
1993.11.06 extracted messages
1993.11.07 improved plurals in messages
1993.11.08 error #80 merged with #106
1993.11.09 fettled in these changes: [& debugged (missing breaks, etc)]
>>1993.10.20 (W/BP) added command line options for crosses & labels
1993.11.14 removed 'sz lfImagefile=IMAGE_FILE;'
1993.11.15 created print_malloc & print_free to claim & release all memory
           added code to process plot in passes if need be
1993.11.16 fixed bug which caused 1st char of filename to be lost
1993.11.17 added -B option to control whether border is printed
           added -!<option> to turn it off
           fixed code so fLabels and fCrosses actually work
           added -S option to control survey legs (default on)
1993.11.21 PRNERRS_FILE -> MESSAGE_FILE
1993.11.30 sorted out error vs fatal
1993.12.01 return error_summary(); instead of EXIT_SUCCESS
1993.12.05 removed extern from bool fBorder (gcc -Wall warns it)
           -!P skips blank pages (border is counted, alignment marks aren't)
1993.12.06 -[!]P gets errored as invalid (this is rubbish I think ?!?)
1993.12.16 msg # 161 -> 105
1994.01.05 replying 4 to 'Which pages?' prints only page 4
           printXX foo now gives expected error if foo not found
1994.01.17 patched for systems with tolower/toupper as a macro
1994.03.15 corrected direction of North arrow (wrongly changed on 1993.08.13)
           internationalised ynq, etc
1994.03.17 coords now long rather than int to increase coord range for DOS
1994.03.20 added img_error
           sorted out replies to "Which pages?"
1994.04.07 img_CROSS is now deprecated
1994.04.27 lfMessages gone
1994.06.09 added SURVEXHOME
1994.06.20 created prindept.h and suppressed -fh warnings; readnum moved here
           added int argument to warning, error and fatal
1994.07.12 sizeof() -> ossizeof(); fixed arithmetic error with -L and/or -C
1994.08.15 nothing much
1994.09.10 corrected FLT_MIN (~= 0) to -(FLT_MAX/10.0f)
           Now says something like "This will need 6 pages (2x3)."
1994.10.05 added language arg to ReadErrorFile
1994.10.08 osnew() added
1994.10.28 converted use of gets to fgets
           removed <y> bit from "(ynq) <y> :" style prompts
1995.01.28 getanswer handles (ynq) prompts - copes with uppercase initials
1995.01.31 minor fettle
1995.03.25 added THIS_YEAR; reformatted; osstrdup
1995.04.22 reindented
1995.06.04 now use device struct for device dependent calls
           fixed "'\0' squeezed out" warning
1995.06.05 pr.szDesc -> pr.Name()
1995.06.23 removed readnum
1995.10.06 fixed bearing to default to 000
1995.10.07 changed "(ynq) :" style prompts to "(y/n/q) :"
1995.10.10 added dotted borders as requested by AndyA
1995.12.20 added as_xxx functions to simplify reading of .ini files
1996.02.09 scale bar is now limited to 20cm
           pfont is now in iso-8859-1, so COPYRIGHT and DEG changed
1996.02.13 fixed typo
1996.02.15 Removed (C) statement and added version number
1996.02.23 added tilt option
1996.02.26 message 161 added for tilt prompt
1996.02.29 undid debug change which stopped more than one page being printed
1996.03.03 finished code to print lists of ranges of pages
1996.03.10 as_xxx now error on NULL string pointer
1996.03.26 fixed problem which meant all pages always got printed
1996.04.03 as_escstring() now reports syntax errors it spots
1996.07.12 fixed bug in as_escstring (skipped too much/little)
1997.01.19 minor tweaks
1997.01.20 added code to work out scale to fit on given page layout
           trimmed 2 superfluous elements from powers[] and si_mods[]
           added command line options to allow non-interactive use
1997.01.22 added code for separate printer charset(s)
1997.01.23 fettled charset code more
           fixed /0 error when .3d file is 0 width and/or height
1998.03.21 fixed up to compile cleanly on Linux
           expiry code turned off
*/

/* !HACK! ought to provide more explanation when reporting errors in print.ini */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <float.h>

#include "getopt.h"

#include "useful.h"
#include "filename.h"
#include "message.h"
#include "version.h"
#include "filelist.h"
#include "img.h"
#include "prindept.h"
#include "debug.h"

#define MOVEMM(X,Y) pr->MoveTo((long)((X)*scX),(long)((Y)*scY));
#define DRAWMM(X,Y) pr->DrawTo((long)((X)*scX),(long)((Y)*scY));

/* degree sign is 176, (C) is 169 in iso-8859-1 */
#define DEG "\xB0" /* specially defined degree symbol */
#define COPYRIGHT "\xA9" /* specially defined degree symbol */
/* also #define COPYRIGHT "(C)" will work */

static long default_scale=500; /* 1:<default_scale> is the default */

static char *szDesc;

typedef struct LI {
   struct LI *pliNext;
   int tag;
   union {
      struct { float x,y; } to;
      sz szLabel;
   } d;
} li;

static li *pliHead,**ppliEnd=&pliHead;
static bool fPlan = fTrue;
static bool fTilt;

static bool fLabels   = fFalse;
static bool fCrosses  = fFalse;
static bool fShots    = fTrue;
       bool fNoBorder = fFalse;
static bool fSkipBlank= fFalse;
       bool fBlankPage= fFalse;

static img *pimg;

/*extern device hpgl;
static device *pr=&hpgl;*/
extern device printer;
static device *pr=&printer;

#define lenSzDateStamp 40
static char  szTitle[256],szDateStamp[lenSzDateStamp];

static int   rot = 0, tilt = 0;
static float xMin, xMax, yMin, yMax;

static float COS,SIN;
static float COST,SINT;

static float scX,scY;
static float xOrg,yOrg;
static int pagesX,pagesY;

static char szTmp[256];

float PaperWidth, PaperDepth;

/* draw fancy scale bar with bottom left at (x,y) (both in mm) and at most */
/* MaxLength mm long. The scaling in use is 1:scale */
static void draw_scale_bar( double x, double y, double MaxLength, double scale );

#define DEF_RATIO (1.0/(double)default_scale)
/* return a scale which will make it fit in the desired size */
static float PickAScale( int x, int y ) {
   double Sc_x, Sc_y;
#if 0
   double E;
#endif
   /*    pagesY = ceil((image_dy+61.0)/PaperDepth)
    * so (image_dy+61.0)/PaperDepth <= pagesY < (image_dy+61.0)/PaperDepth+1
    * so image_dy <= pagesY*PaperDepth-61 < image_dy+PaperDepth
    * and Sc = image_dy / (yMax-yMin)
    * so Sc <= (pagesY*PaperDepth-61)/(yMax-yMin) < Sc+PaperDepth/(yMax-yMin)
    */
   Sc_x = (PaperWidth>0.0 && xMax>xMin) ? (x*PaperWidth-19.0f)/(xMax-xMin) : DEF_RATIO;
   Sc_y = (PaperDepth>0.0 && yMax>yMin) ? (y*PaperDepth-61.0f)/(yMax-yMin) : DEF_RATIO;
   Sc_x = min( Sc_x, Sc_y )*0.99; /* shrink by 1% so we don't cock up */
#if 0 /* this picks a nice (in some sense) ratio, but is too stingy */
   E = pow( 10.0, floor( log10(Sc_x) ) );
   Sc_x = floor( Sc_x / E ) * E;
#endif
   return Sc_x;
}

static void PagesRequired( float Sc ) {
   float image_dx, image_dy;
   float image_centre_x, image_centre_y;
   float paper_centre_x,paper_centre_y;

   image_dx = (xMax-xMin)*Sc;
   if (PaperWidth>0.0)
      pagesX=(int)ceil((image_dx+19.0)/PaperWidth);
   else { /* paperwidth not fixed (eg window or roll printer/plotter) */
      pagesX=1;
      PaperWidth=image_dx+19.0f;
   }
   paper_centre_x=(pagesX*PaperWidth)/2;
   image_centre_x=Sc*(xMax+xMin)/2;
   xOrg=paper_centre_x-image_centre_x;

   image_dy = (yMax-yMin)*Sc;
   if (PaperDepth>0.0)
      pagesY=(int)ceil((image_dy+61.0)/PaperDepth);
   else { /* paperdepth not fixed (eg window or roll printer/plotter) */
      pagesY=1;
      PaperDepth=image_dy+61.0f;
   }
   paper_centre_y=20+(pagesY*PaperDepth)/2;
   image_centre_y=Sc*(yMax+yMin)/2;
   yOrg=paper_centre_y-image_centre_y;
}

static void DrawInfoBox( float num, float denom ) {
   MOVEMM(  0, 0); DRAWMM(  0,40); DRAWMM(100,40);
                   DRAWMM(100, 0); DRAWMM(  0, 0);
   MOVEMM( 60,40); DRAWMM( 60, 0);

   if (fPlan) {
      long ax,ay,bx,by,cx,cy,dx,dy;
      int c;

      MOVEMM( 62,36);
      pr->WriteString(msg(115));

      ax=(long)((80-15*sin(rad(000.0+rot)))*scX);
      ay=(long)((20+15*cos(rad(000.0+rot)))*scY);
      bx=(long)((80-07*sin(rad(180.0+rot)))*scX);
      by=(long)((20+07*cos(rad(180.0+rot)))*scY);
      cx=(long)((80-15*sin(rad(160.0+rot)))*scX);
      cy=(long)((20+15*cos(rad(160.0+rot)))*scY);
      dx=(long)((80-15*sin(rad(200.0+rot)))*scX);
      dy=(long)((20+15*cos(rad(200.0+rot)))*scY);

      pr->MoveTo(ax,ay);
      pr->DrawTo(bx,by);
      pr->DrawTo(cx,cy);
      pr->DrawTo(ax,ay);
      pr->DrawTo(dx,dy);
      pr->DrawTo(bx,by);

#define RADIUS 16.0

      MOVEMM(80.0,20.0+RADIUS);
      for( c=3 ; c<=360 ; c+=3 )
       DRAWMM(80.0+RADIUS*sin(rad(c)),20.0+RADIUS*cos(rad(c)));
   } else {
      MOVEMM( 62,33);
      pr->WriteString(msg(116));

      MOVEMM(65,15); DRAWMM(70,12); DRAWMM(68,15); DRAWMM(70,18);

      DRAWMM(65,15); DRAWMM(95,15);

      DRAWMM(90,18); DRAWMM(92,15); DRAWMM(90,12); DRAWMM(95,15);

      MOVEMM(80,13); DRAWMM(80,17);

      sprintf(szTmp,"%03d"DEG,(rot+270)%360);
      MOVEMM(65,20); pr->WriteString(szTmp);
      sprintf(szTmp,"%03d"DEG,(rot+90)%360);
      MOVEMM(85,20); pr->WriteString(szTmp);
   }

   MOVEMM( 5,33); pr->WriteString(szTitle);

   MOVEMM( 0,30); DRAWMM(60,30);

   MOVEMM( 5,23);
   pr->WriteString(msg(fPlan?117:118));

   MOVEMM( 0,20); DRAWMM(60,20);

   sprintf(szTmp,msg(119),num,denom);
   MOVEMM( 5, 13); pr->WriteString(szTmp);

   MOVEMM( 0,10); DRAWMM(60,10);

   sprintf(szTmp,msg( fPlan ? 150 : 151 ),rot,DEG);
   MOVEMM( 5,  3); pr->WriteString(szTmp);

   MOVEMM(102, 8); pr->WriteString("Survex "VERSION);
   pr->WriteString(szDesc); pr->WriteString(msg(152));

/* Copyright line has been mis-interpreted as refering to survey */
/* MOVEMM(102, 2); pr->WriteString(COPYRIGHT" Olly Betts 1993-"THIS_YEAR); */
/* !HACK! rewrite message so it's clear... */

   draw_scale_bar( 110.0, 17.0, PaperWidth-118.0, (double)denom/num );

}

/* Draw fancy scale bar with bottom left at (x,y) (both in mm) and at most */
/* MaxLength mm long. The scaling in use is 1:scale */
static void draw_scale_bar( double x, double y, double MaxLength,
                            double scale ) {
   double StepEst, d;
   int    E, Step, n, l, c;
   char   szUnits[3], szTmp[256];
   static signed char powers[] = {
      12,9,9,9,6,6,6,3,2,2,0,0,0,-3,-3,-3,-6,-6,-6,-9,
   };
   static char si_mods[sizeof(powers)] = {
      'p','n','n','n','u','u','u','m','c','c','\0','\0','\0',
      'k','k','k','M','M','M','G'
   };

   /* Limit to 20cm to stop people with A0 plotters complaining */
   if (MaxLength>200.0)
      MaxLength=200.0;

#define dmin 10.0      /* each division >= dmin mm long */
#define StepMax 5      /* number in steps of at most StepMax (x 10^N) */
#define epsilon (1e-4) /* fudge factor to prevent rounding problems */

   E = (int)ceil( log10( (dmin*0.001*scale)/StepMax ) );
   StepEst = pow( 10.0, -(double)E ) * (dmin*0.001) * scale - epsilon;

   /* Force labelling to be in multiples of 1, 2, or 5 */
   Step = ( StepEst<=1.0 ? 1 : ( StepEst<=2.0 ? 2 : 5 ) );

   /* Work out actual length of each scale bar division */
   d = Step * pow( 10.0, (double)E ) / scale * 1000.0;

   /* Choose appropriate units, s.t. if possible E is >=0 and minimized */
   /* Range of units is a little extreme, but it doesn't hurt... */
   n = min( E, 9 );
   n = max( n, -10 ) + 10;
   E += (int)powers[n];
   szUnits[0]=si_mods[n];
   strcat( szUnits, "m" );

   /* Work out how many divisions there will be */
   n=(int)(MaxLength/d);

   /* Print units used - eg. "Scale (10m)" */
   sprintf(szTmp,msg(153),(float)pow( 10.0, (double)E ), szUnits );
   MOVEMM( x    , y+4 ); pr->WriteString(szTmp);

   /* Draw top and bottom sides of scale bar */
   MOVEMM( x    , y+3 ); DRAWMM( x+n*d, y+3 );
   MOVEMM( x+n*d, y   ); DRAWMM( x    , y   );

   /* Draw divisions and label them */
   for( c=0; c<=n; c++) {
      MOVEMM( x+c*d, y ); DRAWMM( x+c*d, y+3 );
#if 0
      /* Gives a "zebra crossing" scale bar if we've a FillBox function */
      if (c<n && (c&1)==0)
         FillBox( x+c*d, y,  x+(c+1)*d, y+3 );
#endif
      /* ANSI sprintf returns length of formatted string, but Unix (SUN */
      /*  libraries at least) sprintf returns char* (ptr to '\0'?) */
      sprintf(szTmp,"%d",c*Step);
      l=strlen(szTmp);
      MOVEMM( x+c*d-l, y-4 ); pr->WriteString(szTmp);
   }
}

/* Handle a "(ynq) : " prompt */
static int getanswer( char *szReplies ) {
   char *reply;
   int ch;
   ASSERT2(szReplies,"NULL pointer passed");
   ASSERT2(*szReplies,"list of possible replies is empty");
   putchar('(');
   putchar(*szReplies);
   for( reply=szReplies+1; *reply ; reply++ ) {
      putchar('/');
      putchar(*reply);
   }
   fputs(") : ",stdout);
   /* !HACK! this isn't what we want, as buffered IO means we wait until */
   /* return is pressed, then if nothing is recognised, return is taken */
   /* to mean default */
   do {
      ch=getchar();
      /* first try the letter as typed */
      reply=strchr(szReplies,ch);
      /* if that isn't there, then try toggling the case of the letter */
      if (!reply)
         reply=strchr(szReplies,isupper(ch)?tolower(ch):toupper(ch));
   } while (!reply && ch!='\n');
   if (ch=='\n')
      return 0; /* default to first answer */
   while (getchar()!='\n')
      {} /* twiddle(thumbs); */
   return (reply-szReplies);
}

static bool DescribeLayout(int x, int y) {
   char szReplies[4];
   int answer;
   if ((x*y)==1)
      printf(msg(171)); /* 1 page */
   else
      printf(msg(170),x*y,x,y); /* x*y pages */
   szReplies[0]=*msg(180); /* yes */
   szReplies[1]=*msg(181); /* no */
   szReplies[2]=*msg(182); /* quit */
   szReplies[3]='\0';
   /* " Continue (ynq) : " */
   printf("\n %s ",msg(155));
   answer=getanswer(szReplies);
   if (answer==2) /* quit */ {
      putnl();
      puts(msg(156));
      exit(EXIT_FAILURE);
   }
   return (answer!=1); /* no */
}

static void stack( int tag, float x, float y, float z ) {
   li *pli;
   pli=osnew(li);
   pli->tag=tag;
   pli->d.to.x = x*COS-y*SIN;
   pli->d.to.y = ( fPlan ? x*SIN+y*COS :
    (fTilt ? (x*SIN+y*COS)*SINT+z*COST : z) );
   if (pli->d.to.x>xMax) xMax=pli->d.to.x;
   if (pli->d.to.x<xMin) xMin=pli->d.to.x;
   if (pli->d.to.y>yMax) yMax=pli->d.to.y;
   if (pli->d.to.y<yMin) yMin=pli->d.to.y;
   *ppliEnd=pli;
   ppliEnd=&(pli->pliNext);
}

static void stackSz( sz sz ) {
   li *pli;
   pli=osnew(li);
   pli->tag=img_LABEL;
   pli->d.szLabel=osstrdup(sz);
   *ppliEnd=pli;
   ppliEnd=&(pli->pliNext);
}

static void ReadInData( void ) {
   float x, y, z;
   char sz[256];
   int result;

   do {
      result=img_read_datum( pimg, sz, &x, &y, &z );
      switch (result) {
       case img_BAD:
         fatal(106,NULL,NULL,0);
         /* break; */
       case img_MOVE:
       case img_LINE:
         stack(result,x,y,z);
         break;
       case img_CROSS:
         /* use img_LABEL to posn CROSS - newer .3d files don't have
          * img_CROSS anyway */
         break;
       case img_LABEL:
         stack(img_CROSS,x,y,z);
         stackSz(sz);
         break;
      }
   } while (result!=img_BAD && result!=img_STOP);

   *ppliEnd=NULL;

}

static int next_page( int *pstate, char **q, int pageLim ) {
   char *p;
   int page;
   int c;
   p=*q;
   if (*pstate>0) {
      /* doing a range */
      (*pstate)++;
      ASSERT(*p=='-');
      p++;
      while (isspace(*p))
         p++;
      if (sscanf(p,"%u%n",&page,&c)>0) {
         p+=c;
      } else {
         page=pageLim;
      }
      if (*pstate>page)
         goto err;
      if (*pstate<page)
         return *pstate;
      *q=p;
      *pstate=0;
      return page;
   }
   while (isspace(*p) || *p==',')
      p++;
   if (!*p) {
      return 0; /* done */
   }
   if (*p=='-') {
      *q=p;
      *pstate=1;
      return 1; /* range with initial parameter omitted */
   }
   if (sscanf(p,"%u%n",&page,&c)>0) {
      p+=c;
      while (isspace(*p))
         p++;
      *q=p;
      if (0<page && page<=pageLim) {
         if (*p=='-')
            *pstate=page; /* range with start */
         return page;
      }
   }
   err:
   *pstate=-1;
   return 0;
}

int main( int argc, sz argv[] ) {
   bool fOk;
   float N_Scale = 1, D_Scale = 500, Sc = 0;
   unsigned int page,pages;
   sz fnm;
   const char *pthMe;
   int j;
   int cPasses, pass;
   unsigned int cPagesPrinted;
   int state;
   char *p;
   char *szPages=NULL;
   int pageLim;
   bool fInteractive = fTrue;
   const char *msg166, *msg167;
   int old_charset;

   static const struct option opts[] = {
      /* const char *name; int has_arg (0 no_argument, 1 required_*, 2 optional_*); int *flag; int val; */
      "elevation", no_argument, 0, 'e',
      "elev", no_argument, 0, 'e', /* FIXME superfluous? */
      "plan", no_argument, 0, 'p',
      "bearing", required_argument, 0, 'b',
      "tilt", required_argument, 0, 't',
      "scale", required_argument, 0, 's',
      "station-names", no_argument, 0, 'n',
      "crosses", no_argument, 0, 'c',
      "no-border", no_argument, 0, 'B',
      "no-legs", no_argument, 0, 'l',
      "skip-blanks", no_argument, 0, 'k',
      0, 0, 0, 0
   };

   pthMe = ReadErrorFile("Printer driver", "SURVEXHOME", "SURVEXLANG",
			 argv[0], MESSAGE_FILE);

   szDesc = pr->Name();

   printf("Survex %s%s v"VERSION
          "\n  (C) Copyright Olly Betts 1993-"THIS_YEAR"\n\n",
          szDesc, msg(152));

/*#include "expire.h"*/

   fnm = NULL;
   
   /* No arguments given */
   if (argc < 2) fatal(82, NULL /*display_syntax_message*/, NULL, 0); /*FIXME*/

   while (1) {
      int opt = getopt_long(argc, argv, "epbt:s:ncBlk", opts, NULL);
      if (opt == EOF) break;
      switch (opt) {
       case 0:
	 break; /* long option processed - ignore */
       case ':':
	  /* missing param - fall through */
       case '?':
	 /* FIXME unknown option */
	 /* or ambiguous match or extraneous param */
	 fatal(82, NULL /*display_syntax_message*/, NULL, 0); /* FIXME */
	 break;
       case 'n': /* Labels */
	 fLabels = 1;
	 break;
       case 'c': /* Crosses */
	 fCrosses = 1;
	 break;
       case 'B': /* Border */
	 fNoBorder = 1;
	 break;
       case 'l': /* legs */
	 fShots = 0;
	 break;
       case 'k': /* Print blank pages */
	 fSkipBlank = 1;
	 break;
       case 'e':
	 fPlan = fFalse;
	 fInteractive = fFalse;
	 break;
       case 'p':
	 fPlan = fTrue;
	 fInteractive = fFalse;
	 break;
       case 'b':
	 rot = atoi(optarg); /* FIXME check for trailing garbage... */
	 fInteractive = fFalse;
	 break;
       case 't':
	 tilt = atoi(optarg); /* FIXME check for trailing garbage... */
	 fInteractive = fFalse;
	 break;
       case 's':
	 /* FIXME check for trailing garbage... */
	 if (sscanf(optarg, "%f:%f", &N_Scale, &D_Scale) == 2) {
	    fInteractive = fFalse;
	    break;
	 }
	 /* fall through */
       default: /* FIXME default ? */
	 fatal(82, wr, argv[j], 0); /* bad command line options */
      }
   }

   fnm = argv[optind++];
   if (!fnm) {
      /* no .3d files given */
      printf(msg(157), szAppNameCopy);
      putnl();
      exit(EXIT_FAILURE);
   }

   /* Try to open first image file and check it has correct header,
    * rather than asking lots of questions then failing */
   pimg = img_open(fnm, szTitle, szDateStamp);
   if (!pimg) fatal(img_error(), wr, fnm, 0);
   
   if (pr->Init) {
      FILE *fh;
      fh = fopenWithPthAndExt(pthMe, PRINT_INI, NULL, "rb", NULL);
      if (!fh) fatal(24, wr, fnm, 0);
      pr->Init(fh, pthMe, &scX, &scY);
      fclose(fh);
   }

   if (fInteractive) {
      char szReplies[3];
      szReplies[0] = *msg(183); /* plan */
      szReplies[1] = *msg(184); /* elevation */
      szReplies[2] = '\0';
      /* "Plan or Elevation (pe) : " */
      printf("%s ", msg(158));
      fPlan = ( getanswer(szReplies) != 1 ); /* elevation */
   }

   if (fInteractive) {
      do {
         printf( msg( fPlan ? 159:160) );
         fgets( szTmp, sizeof(szTmp), stdin );
      } while (!(*szTmp<32) && sscanf( szTmp, "%d", &rot )<1);
      if (*szTmp<32) {
         /* if just return, default view to North up page/looking North */
         rot=0;
         printf("%d\n",rot);
      }
   }
   SIN = (float)sin(rad(rot));
   COS = (float)cos(rad(rot));

   if (!fPlan) {
      if (fInteractive) {
         do {
            printf(msg(161));
            fgets(szTmp,sizeof(szTmp),stdin);
         } while (!(*szTmp<32) && sscanf( szTmp, "%d", &tilt )<1);
         if (*szTmp<32) {
            /* if just return, default to no tilt */
            tilt=0;
            printf("%d\n",tilt);
         }
      }
      fTilt=(tilt!=0);
      SINT=(float)sin(rad(tilt));
      COST=(float)cos(rad(tilt));
   }

   if (fInteractive) {
      putnl();
      puts(msg(105));
   }

   xMax = yMax = -(FLT_MAX/10.0f); /* any (sane) value will beat this */
   xMin = yMin = FLT_MAX; /* ditto */
   
   while (fnm) {
      /* first time around pimg is already open... */
      if (!pimg) {
	 /* FIXME for multiple files, which title and datestamp to use...? */
	 pimg = img_open(fnm, szTitle, szDateStamp);
	 if (!pimg) fatal(img_error(), wr, fnm, 0);
      }
      ReadInData();
      img_close(pimg);
      pimg = NULL;
      fnm = argv[optind++];
   }

   /* can't have been any data */
   if (xMax<xMin || yMax<yMin) fatal(86, NULL, NULL, 0);
   
   {
      float x;
      x = 1000.0/PickAScale(1,1);
      printf("Scale to fit to 1 page = 1:%f\n",x);
      if (N_Scale==0.0) {
         N_Scale = 1;
         D_Scale = x;
         Sc = N_Scale*1000/D_Scale;
         PagesRequired(Sc);
      } else if (!fInteractive) {
         Sc = N_Scale*1000/D_Scale;
         PagesRequired(Sc);
      }
   }

   if (fInteractive) { do {
      putnl();
      printf(msg(162),default_scale);
      fgets(szTmp,sizeof(szTmp),stdin);
      if (sscanf(szTmp,"%f:%f",&N_Scale,&D_Scale)<2) {
         N_Scale=1;
         D_Scale=(float)default_scale;
      }
      putnl();
      printf(msg(163),N_Scale,D_Scale);
      putnl();
      Sc = N_Scale*1000/D_Scale;
      PagesRequired(Sc);
      fOk=DescribeLayout( pagesX, pagesY );
   } while (!fOk); }

   pageLim=pagesX*pagesY;
   if (pageLim==1) {
      pages=1;
   } else {
      int fOk;
      do {
         fOk=fTrue;
         printf(msg(164));
         if (fInteractive)
            fgets( szTmp, sizeof(szTmp), stdin );
         else
            *szTmp = '\0'; /* default to all if non-interactive */
         for( p=szTmp; isspace(*p) ; p++ )
            {}
         szPages=osstrdup(p);
         if (*szPages) {
            pages=0;
            for( page=0, state=0, p=szPages ; ; ) {
               page=next_page( &state, &p, pageLim );
               if (state<0) {
                  printf("error\n");
                  fOk=fFalse;
                  break;
               }
               if (page==0)
                  break;
/*               printf("page %d\n",page);*/  /*!HACK!*/
               pages++;
            }
         } else {
            pages=pageLim;
         }
      } while (!fOk);

#if 0
         for( p=szPages ; *p ;  ) {
            while (isspace(*p))
               p++;
            if (p!=szPages && *p==',')
               p++;
            while (isspace(*p))
               p++;
            if (*p) {
               int c;
               int fNoNum=fTrue;
               page=1;
               pageMax=pageLim;
               if (*p!='-') {
                  if (sscanf(p,"%u%n",&page,&c)>0) {
                     fNoNum=fFalse;
                     p+=c;
                     while (isspace(*p))
                        p++;
                  }
               }
               if (*p!='-')
                  pageMax=page;
               else {
                  unsigned int v;
                  p++;
                  if (sscanf(p,"%u%n",&v,&c)>0) {
/*                   printf("v %u, c %d\n",v,c);*/
                     pageMax=v;
                     fNoNum=fFalse;
                     p+=c;
                     while (isspace(*p))
                        p++;
                  }
               }
               if (fNoNum || pageMax<page || page<1 || pageMax>pageLim)
                  fOk=fFalse;
               pages+=(pageMax-page+1);
               printf("%d-%d%c\n",page,pageMax,fOk?' ':'*');
            }
         }
      } while (!fOk);
#endif

   }

   /* if no explicit Alloc, default to one pass */
   cPasses=(pr->Pre?pr->Pre(pages):1);

   puts(msg(165));
   msg166 = msgPerm(166); /* note down so we can switch to printer charset */
   select_charset( 0 /*CHARSET_USASCII*/ ); /* !HACK! could do better and find out what charset actually is */
   msg167 = msgPerm(167); /* used in printer's native charset in footer */
   old_charset = select_charset( 1 /*CHARSET_ISO8859_1*/ ); /* !HACK! */
   cPagesPrinted=0;
   for( page=0, state=0, p=szPages ; ; ) {
      if (pageLim==1) {
         if (page==0)
            page=1;
         else
            page=0; /* we've already printed the only page */
      } else if (!*szPages) {
         page++;
         if (page>pageLim)
            page=0; /* all pages printed */
      } else {
         page=next_page( &state, &p, pageLim );
      }
      ASSERT( state>=0 ); /* errors should have been caught above */
      if (page==0)
         break;
      cPagesPrinted++;
      if (pages>1) {
         putchar('\r');
         printf( msg166, (int)cPagesPrinted, pages );
      }
      /* don't skip the page with the info box on */
      if (fSkipBlank && (int)page!=(pagesY-1)*pagesX+1)
         pass=-1;
      else
         pass=0;
      fBlankPage=fFalse;
      for( ; pass<cPasses ; pass++ ) {
         li *pli;
         if (pr->NewPage)
            pr->NewPage( (int)page, pass, pagesX, pagesY );
         if ((int)page==(pagesY-1)*pagesX+1)
            DrawInfoBox( N_Scale, D_Scale );
         for( pli=pliHead ; pli ; pli=pli->pliNext )
            switch (pli->tag) {
             case img_MOVE:
               if (fShots)
                  pr->MoveTo((long)((pli->d.to.x*Sc+xOrg)*scX),
                            (long)((pli->d.to.y*Sc+yOrg)*scY));
               break;
             case img_LINE:
               if (fShots)
                  pr->DrawTo((long)((pli->d.to.x*Sc+xOrg)*scX),
                            (long)((pli->d.to.y*Sc+yOrg)*scY));
               break;
             case img_CROSS:
               if (fCrosses) {
                  pr->DrawCross((long)((pli->d.to.x*Sc+xOrg)*scX),
                               (long)((pli->d.to.y*Sc+yOrg)*scY));
                  break;
               }
               if (fLabels) {
                  pr->MoveTo((long)((pli->d.to.x*Sc+xOrg)*scX),
                            (long)((pli->d.to.y*Sc+yOrg)*scY));
               }
               break;
             case img_LABEL:
               if (fLabels) {
                  pr->WriteString(pli->d.szLabel);
               }
               break;
            }
         if (pass==-1) {
            if (fBlankPage) {
               /* !HACK! printf("\rSkipping blank page\n"); */
               break;
            }
         } else {
            if (pr->ShowPage) {
               sprintf(szTmp, msg167, szTitle, (int)page, pagesX*pagesY,
                       szDateStamp );
               pr->ShowPage(szTmp);
            }
         }
      }
   }

   if (pr->Post)
      pr->Post();

   if (pr->Quit)
      pr->Quit();

   select_charset(old_charset);
   putnl();
   putnl();
   puts(msg(144));
   return error_summary();
}

#define fDotBorders 1
/* Draws in alignment marks on each page or borders on edge pages */
void drawticks( border clip, int tick_size, int x, int y ) {
   long i;
   int s=tick_size*4;
   int o=s/8;
   pr->MoveTo(clip.x_min, clip.y_min);
   if (fNoBorder || x) {
      pr->DrawTo(clip.x_min, clip.y_min+tick_size);
      if (fDotBorders) {
         i = (clip.y_max-clip.y_min) -
             ( tick_size + ( (clip.y_max-clip.y_min-tick_size*2L)%s )/2 );
         for( ; i>tick_size ; i-=s ) {
            pr->MoveTo(clip.x_min, clip.y_max-(i+o));
            pr->DrawTo(clip.x_min, clip.y_max-(i-o));
         }
      }
      pr->MoveTo(clip.x_min, clip.y_max-tick_size);
   }
   pr->DrawTo(clip.x_min, clip.y_max);
   if (fNoBorder || y<pagesY-1) {
      pr->DrawTo(clip.x_min+tick_size, clip.y_max);
      if (fDotBorders) {
         i = (clip.x_max-clip.x_min) -
             ( tick_size + ( (clip.x_max-clip.x_min-tick_size*2L)%s )/2 );
         for( ; i>tick_size ; i-=s ) {
            pr->MoveTo(clip.x_max-(i+o), clip.y_max );
            pr->DrawTo(clip.x_max-(i-o), clip.y_max );
         }
      }
      pr->MoveTo(clip.x_max-tick_size, clip.y_max);
   }
   pr->DrawTo(clip.x_max, clip.y_max);
   if (fNoBorder || x<pagesX-1) {
      pr->DrawTo(clip.x_max, clip.y_max-tick_size);
      if (fDotBorders) {
         i = (clip.y_max-clip.y_min) -
             ( tick_size + ( (clip.y_max-clip.y_min-tick_size*2L)%s )/2 );
         for( ; i>tick_size ; i-=s ) {
            pr->MoveTo(clip.x_max, clip.y_min+(i+o));
            pr->DrawTo(clip.x_max, clip.y_min+(i-o));
         }
      }
      pr->MoveTo(clip.x_max, clip.y_min+tick_size);
   }
   pr->DrawTo(clip.x_max, clip.y_min);
   if (fNoBorder || y) {
      pr->DrawTo(clip.x_max-tick_size, clip.y_min);
      if (fDotBorders) {
         i = (clip.x_max-clip.x_min) -
             ( tick_size + ( (clip.x_max-clip.x_min-tick_size*2L)%s )/2 );
         for( ; i>tick_size ; i-=s ) {
            pr->MoveTo(clip.x_min+(i+o), clip.y_min);
            pr->DrawTo(clip.x_min+(i-o), clip.y_min);
         }
      }
      pr->MoveTo(clip.x_min+tick_size, clip.y_min);
   }
   pr->DrawTo(clip.x_min, clip.y_min);
}

char *as_string(char *p) {
   if (!p) fatal(85, NULL, NULL, 0); /* bad printer config file */
   return p;
}

int as_int( char *p, int min_val, int max_val ) {
   long val;
   char *pEnd;
   if (!p) fatal(85, NULL, NULL, 0); /* bad printer config file */
   val=strtol( p, &pEnd, 10 );
   if (pEnd==p || val<(long)min_val || val>(long)max_val)
      fatal(85,NULL,NULL,0); /* bad printer config file */
   osfree(p);
   return (int)val;
}

int as_bool( char *p ) {
   return as_int( p, 0, 1 );
}

float as_float( char *p, float min_val, float max_val ) {
   double val;
   char *pEnd;
   if (!p) fatal(85, NULL, NULL, 0); /* bad printer config file */
   val=strtod( p, &pEnd );
   if (pEnd==p || val<(double)min_val || val>(double)max_val)
      fatal(85,NULL,NULL,0); /* bad printer config file */
   osfree(p);
   return (float)val;
}

/* Converts '0'-'9' to 0-9, 'A'-'F' to 10-15 and 'a'-'f' to 10-15.
 * Undefined on other values */
#define CHAR2HEX(C) (((C)+((C)>64?9:0))&15)

/*
Codes:
\\ -> '\'
\xXX -> char with hex value XX
\0, \n, \r, \t -> nul (0), newline (10), return (13), tab (9)
\[ -> Esc (27)
\? -> delete (127)
\A - \Z -> (1) to (26)
*/

/* Takes a string, converts escape sequences in situ, and returns length
 * of result (needed since converted string may contain '\0' */
int as_escstring( char *s ) {
   char *p, *q;
   char c;
   char *t;
   static char *escFr="[nrt?0"; /* 0 is last so maps to the implicit \0 */
   static char *escTo="\x1b\n\r\t\?";
   bool fSyntax=fFalse;
   if (!s)
      fatal(85,NULL,NULL,0); /* bad printer config file */
   for( p=s, q=s ; *p ; ) {
      c=*p++;
      if (c=='\\') {
         c=*p++;
         switch (c) {
           case '\\': /* literal "\" */
            break;
           case 'x': /* hex digits */
            if (isxdigit(*p) && isxdigit(p[1])) {
               c=(CHAR2HEX(*p)<<4) | CHAR2HEX(p[1]);
               p+=2;
               break;
            }
            /* \x not followed by 2 hex digits */
            /* !!! FALLS THROUGH !!! */
	   case '\0': /* trailing \ is meaningless */
	    fSyntax=1;
	    break;
	   default:
	    t=strchr(escFr,c);
	    if (t) {
	       c=escTo[t-escFr];
	       break;
	    }
	    /* \<capital letter> -> Ctrl-<letter> */
	    if (isupper(c)) {
	       c-='@';
	       break;
	    }
	    /* otherwise don't do anything to c (?) */
	    break;
	 }
      }
      *q++=c;
   }
   if (fSyntax) fatal(85, NULL, NULL, 0); /* bad printer config file */
   return (q-s);
}
