/* prcore.c
 * Printer independent parts of Survex printer drivers
 * Copyright (C) 1993-2001 Olly Betts
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

/* FIXME provide more explanation when reporting errors in print.ini */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <float.h>

#include "cmdline.h"
#include "useful.h"
#include "filename.h"
#include "message.h"
#include "filelist.h"
#include "img.h"
#include "prcore.h"
#include "debug.h"

#define MOVEMM(X, Y) pr->MoveTo((long)((X) * scX), (long)((Y) * scY));
#define DRAWMM(X, Y) pr->DrawTo((long)((X) * scX), (long)((Y) * scY));

#define DEG "\xB0" /* degree symbol in iso-8859-1 */

/* 1:<DEFAULT_SCALE> is the default scale */
#define DEFAULT_SCALE 500

static const char *szDesc;

typedef struct LI {
   struct LI *pliNext;
   int tag;
   struct { double x, y; } to;
   char *label;
} li;

static li *pliHead, **ppliEnd = &pliHead;
static enum {PLAN, ELEV, TILT, EXTELEV} view = PLAN;

static bool fLabels = fFalse;
static bool fCrosses = fFalse;
static bool fShots = fTrue;
static bool fSurface = fFalse;
static bool fSkipBlank = fFalse;

bool fNoBorder = fFalse;
bool fBlankPage = fFalse;

static img *pimg;

#if 0
extern device hpgl;
static device *pr = &hpgl;
#endif
static device *pr = &printer;

static char *title, *datestamp;

static int rot = 0, tilt = 0;
static double xMin, xMax, yMin, yMax;

static double COS, SIN;
static double COST, SINT;

static double scX, scY;
static double xOrg, yOrg;
static int pagesX, pagesY;

static char szTmp[256];

double PaperWidth, PaperDepth;

/* draw fancy scale bar with bottom left at (x,y) (both in mm) and at most */
/* MaxLength mm long. The scaling in use is 1:scale */
static void draw_scale_bar(double x, double y, double MaxLength,
       	    		   double scale);

#define DEF_RATIO (1.0/(double)DEFAULT_SCALE)
/* return a scale which will make it fit in the desired size */
static double
pick_scale(int x, int y)
{
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
   Sc_x = Sc_y = DEF_RATIO;
   if (PaperWidth > 0.0 && xMax > xMin)
      Sc_x = (x * PaperWidth - 19.0) / (xMax - xMin);
   if (PaperDepth > 0.0 && yMax > yMin)
      Sc_y = (y * PaperDepth - 61.0) / (yMax - yMin);

   Sc_x = min(Sc_x, Sc_y) * 0.99; /* shrink by 1% so we don't cock up */
#if 0 /* this picks a nice (in some sense) ratio, but is too stingy */
   E = pow(10.0, floor(log10(Sc_x)));
   Sc_x = floor(Sc_x / E) * E;
#endif
   return Sc_x;
}

static void
pages_required(double Sc)
{
   double image_dx, image_dy;
   double image_centre_x, image_centre_y;
   double paper_centre_x, paper_centre_y;

   image_dx = (xMax - xMin) * Sc;
   if (PaperWidth > 0.0) {
      pagesX = (int)ceil((image_dx + 19.0) / PaperWidth);
   } else {
      /* paperwidth not fixed (eg window or roll printer/plotter) */
      pagesX = 1;
      PaperWidth = image_dx + 19.0;
   }
   paper_centre_x = (pagesX * PaperWidth) / 2;
   image_centre_x = Sc * (xMax + xMin) / 2;
   xOrg = paper_centre_x - image_centre_x;

   image_dy = (yMax - yMin) * Sc;
   if (PaperDepth > 0.0) {
      pagesY = (int)ceil((image_dy + 61.0) / PaperDepth);
   } else {
      /* paperdepth not fixed (eg window or roll printer/plotter) */
      pagesY = 1;
      PaperDepth = image_dy + 61.0;
   }
   paper_centre_y = 20 + (pagesY * PaperDepth) / 2;
   image_centre_y = Sc * (yMax + yMin) / 2;
   yOrg = paper_centre_y - image_centre_y;
}

static void
draw_info_box(double num, double denom)
{
   char *p;
   int boxwidth = 60;
   int boxheight = 30;
    
   if (view != EXTELEV) {
      boxwidth = 100;
      boxheight = 40;
      MOVEMM(60,40);
      DRAWMM(60, 0);
      MOVEMM(0, 30); DRAWMM(60, 30);
   }
   
   MOVEMM(0, 0);
   DRAWMM(0, boxheight);
   DRAWMM(boxwidth, boxheight);
   DRAWMM(boxwidth, 0);
   DRAWMM(0, 0);

   MOVEMM(0, 20); DRAWMM(60, 20);
   MOVEMM(0, 10); DRAWMM(60, 10);

   switch (view) {
    case PLAN: {
      long ax, ay, bx, by, cx, cy, dx, dy;
      int c;

      MOVEMM(62, 36);
      pr->WriteString(msg(/*North*/115));

      ax = (long)((80 - 15 * sin(rad(000.0 + rot))) * scX);
      ay = (long)((20 + 15 * cos(rad(000.0 + rot))) * scY);
      bx = (long)((80 - 07 * sin(rad(180.0 + rot))) * scX);
      by = (long)((20 + 07 * cos(rad(180.0 + rot))) * scY);
      cx = (long)((80 - 15 * sin(rad(160.0 + rot))) * scX);
      cy = (long)((20 + 15 * cos(rad(160.0 + rot))) * scY);
      dx = (long)((80 - 15 * sin(rad(200.0 + rot))) * scX);
      dy = (long)((20 + 15 * cos(rad(200.0 + rot))) * scY);

      pr->MoveTo(ax, ay);
      pr->DrawTo(bx, by);
      pr->DrawTo(cx, cy);
      pr->DrawTo(ax, ay);
      pr->DrawTo(dx, dy);
      pr->DrawTo(bx, by);

#define RADIUS 16.0

      if (pr->DrawCircle && scX == scY) {
	 pr->DrawCircle((long)(80.0 * scX), (long)(20.0 * scY),
			(long)(RADIUS * scX));
      } else {
	 MOVEMM(80.0, 20.0 + RADIUS);
	 for (c = 3; c <= 360; c += 3)
	    DRAWMM(80.0 + RADIUS * sin(rad(c)), 20.0 + RADIUS * cos(rad(c)));
      }

      MOVEMM(5, 23);
      pr->WriteString(msg(/*Plan view*/117));
      break;
    }
    case ELEV: case TILT:
      MOVEMM(62, 33);
      pr->WriteString(msg(/*Elevation on*/116));

      MOVEMM(65, 15); DRAWMM(70, 12); DRAWMM(68, 15); DRAWMM(70, 18);

      DRAWMM(65, 15); DRAWMM(95, 15);

      DRAWMM(90, 18); DRAWMM(92, 15); DRAWMM(90, 12); DRAWMM(95, 15);

      MOVEMM(80, 13); DRAWMM(80, 17);

      sprintf(szTmp, "%03d"DEG, (rot + 270) % 360);
      MOVEMM(65, 20); pr->WriteString(szTmp);
      sprintf(szTmp, "%03d"DEG, (rot + 90) % 360);
      MOVEMM(85, 20); pr->WriteString(szTmp);

      MOVEMM(5, 23);
      pr->WriteString(msg(/*Elevation*/118));
      break;
    case EXTELEV:
      MOVEMM(5, 13);
      pr->WriteString(msg(/*Extended elevation*/191));
      break;
   }

   MOVEMM(5, boxheight - 7); pr->WriteString(title);

   strcpy(szTmp, msg(/*Scale*/154));
   p = szTmp + strlen(szTmp);
   sprintf(p, " %.0f:%.0f", num, denom);
   MOVEMM(5, boxheight - 27); pr->WriteString(szTmp);

   if (view != EXTELEV) {
      strcpy(szTmp, msg(view == PLAN ? /*Up page*/168 : /*View*/169));
      p = szTmp + strlen(szTmp);
      sprintf(p, " %03d"DEG, rot);
      MOVEMM(5, 3); pr->WriteString(szTmp);
   }

   sprintf(szTmp, "Survex "VERSION" %s %s", szDesc, msg(/*Driver*/152));
   MOVEMM(boxwidth + 2, 8); pr->WriteString(szTmp);

   /* This used to be a copyright line, but it was occasionally
    * mis-interpreted as us claiming copyright on the survey, so let's
    * give the website URL instead */
   MOVEMM(boxwidth + 2, 2); pr->WriteString("http://www.survex.com/");

   draw_scale_bar(boxwidth + 10.0, 17.0, PaperWidth - boxwidth - 18.0,
		  denom / num);
}

/* Draw fancy scale bar with bottom left at (x,y) (both in mm) and at most */
/* MaxLength mm long. The scaling in use is 1:scale */
static void
draw_scale_bar(double x, double y, double MaxLength, double scale)
{
   double StepEst, d;
   int E, Step, n, l, c;
   char u_buf[3], buf[256];
   char *p;
   static signed char powers[] = {
      12, 9, 9, 9, 6, 6, 6, 3, 2, 2, 0, 0, 0, -3, -3, -3, -6, -6, -6, -9,
   };
   static char si_mods[sizeof(powers)] = {
      'p', 'n', 'n', 'n', 'u', 'u', 'u', 'm', 'c', 'c', '\0', '\0', '\0',
      'k', 'k', 'k', 'M', 'M', 'M', 'G'
   };

   /* Limit scalebar to 20cm to stop people with A0 plotters complaining */
   if (MaxLength > 200.0) MaxLength = 200.0;

#define dmin 10.0      /* each division >= dmin mm long */
#define StepMax 5      /* number in steps of at most StepMax (x 10^N) */
#define epsilon (1e-4) /* fudge factor to prevent rounding problems */

   E = (int)ceil(log10((dmin * 0.001 * scale) / StepMax));
   StepEst = pow(10.0, -(double)E) * (dmin * 0.001) * scale - epsilon;

   /* Force labelling to be in multiples of 1, 2, or 5 */
   Step = (StepEst <= 1.0 ? 1 : (StepEst <= 2.0 ? 2 : 5));

   /* Work out actual length of each scale bar division */
   d = Step * pow(10.0, (double)E) / scale * 1000.0;

   /* Choose appropriate units, s.t. if possible E is >=0 and minimized */
   /* Range of units is a little extreme, but it doesn't hurt... */
   n = min(E, 9);
   n = max(n, -10) + 10;
   E += (int)powers[n];

   u_buf[0] = si_mods[n];
   u_buf[1] = '\0';
   strcat(u_buf, "m");

   strcpy(buf, msg(/*Scale*/154));

   /* Add units used - eg. "Scale (10m)" */
   p = buf + strlen(buf);
   sprintf(p, " (%.0f%s)", (double)pow(10.0, (double)E), u_buf);

   MOVEMM(x, y + 4); pr->WriteString(buf);

   /* Work out how many divisions there will be */
   n = (int)(MaxLength / d);

   /* Draw top and bottom sides of scale bar */
   MOVEMM(x, y + 3); DRAWMM(x + n * d, y + 3);
   MOVEMM(x + n * d, y); DRAWMM(x, y);

   /* Draw divisions and label them */
   for (c = 0; c <= n; c++) {
      MOVEMM(x + c * d, y); DRAWMM(x + c * d, y + 3);
#if 0
      /* Gives a "zebra crossing" scale bar if we've a FillBox function */
      if (c < n && (c & 1) == 0)
	 FillBox(x + c * d, y, x + (c + 1) * d, y + 3);
#endif
      /* ANSI sprintf returns length of formatted string, but some pre-ANSI Unix
       * implementations return char* (ptr to end of written string I think) */
      sprintf(buf, "%d", c * Step);
      l = strlen(buf);
      MOVEMM(x + c * d - l, y - 4);
      pr->WriteString(buf);
   }
}

/* Handle a "(ynq) : " prompt */
static int
getanswer(char *szReplies)
{
   int ch;
   char *reply;
   ASSERT2(szReplies, "NULL pointer passed");
   ASSERT2(*szReplies, "list of possible replies is empty");
   putchar('(');
   putchar(*szReplies);
   for (reply = szReplies + 1; *reply; reply++) {
      putchar('/');
      putchar(*reply);
   }
   fputs(") : ", stdout);
   /* Switching to non-line based input is tricky to do portably, so we'll
    * just take a line and look at the first character */
   do {
      ch = getchar();
      
      if (ch == '\n') return 0; /* default to first answer */
      
      /* skip rest of line entered */
      while (getchar() != '\n') {}

      /* first try the letter as typed */
      reply = strchr(szReplies, ch);
      /* if that isn't there, then try toggling the case of the letter */
      if (!reply)
         reply = strchr(szReplies, isupper(ch) ? tolower(ch) : toupper(ch));
   } while (!reply);
   return (reply - szReplies);
}

static bool
describe_layout(int x, int y)
{
   char szReplies[4];
   int answer;
   if ((x * y) == 1)
      printf(msg(/*This will need 1 page.*/171));
   else
      printf(msg(/*This will need %d pages (%dx%d).*/170), x*y, x, y);
   szReplies[0] = *msg(/*yes*/180);
   szReplies[1] = *msg(/*no*/181);
   szReplies[2] = *msg(/*quit*/182);
   szReplies[3] = '\0';
   /* " Continue (ynq) : " */
   printf("\n %s ", msg(/*Continue*/155));
   answer = getanswer(szReplies);
   if (answer == 2) {
      /* quit */
      putnl();
      puts(msg(/*Exiting.*/156));
      exit(EXIT_FAILURE);
   }
   return (answer != 1); /* no */
}

static void
stack(int tag, const char *s, const img_point *p)
{
   li *pli;
   pli = osnew(li);
   pli->tag = tag;
   pli->to.x = p->x * COS - p->y * SIN;
   switch (view) {
    case PLAN:
       pli->to.y = p->x * SIN + p->y * COS;
       break;
    case TILT:
       pli->to.y = (p->x * SIN + p->y * COS) * SINT + p->z * COST;
       break;
    case ELEV: case EXTELEV:
       pli->to.y = p->z;
       break;
   }
   if (pli->to.x > xMax) xMax = pli->to.x;
   if (pli->to.x < xMin) xMin = pli->to.x;
   if (pli->to.y > yMax) yMax = pli->to.y;
   if (pli->to.y < yMin) yMin = pli->to.y;
   pli->label = s ? osstrdup(s) : NULL;
   *ppliEnd = pli;
   ppliEnd = &(pli->pliNext);
}

static int
read_in_data(void)
{
   bool fMove = fFalse;
   img_point p, p_move;
   int result;

   do {
      result = img_read_item(pimg, &p);
      switch (result) {
       case img_BAD:
	 return 0;
       case img_LINE:
	 /* if we're plotting surface legs or this isn't a surface leg */
         if (fSurface || !(pimg->flags & img_FLAG_SURFACE)) {
	    if (fMove) {
	       fMove = fFalse;
	       stack(img_MOVE, NULL, &p_move);
	    }
	    stack(img_LINE, NULL, &p);
	    break;
	 }
	 /* FALLTHRU */
       case img_MOVE:
	 p_move = p;
	 fMove = fTrue;
         break;
       case img_LABEL:
	 if (fSurface || (pimg->flags & img_SFLAG_UNDERGROUND))
	    stack(img_LABEL, pimg->label, &p);
         break;
      }
   } while (result != img_STOP);

   *ppliEnd = NULL;
   return 1;
}

static int
next_page(int *pstate, char **q, int pageLim)
{
   char *p;
   int page;
   int c;
   p = *q;
   if (*pstate > 0) {
      /* doing a range */
      (*pstate)++;
      ASSERT(*p == '-');
      p++;
      while (isspace(*p)) p++;
      if (sscanf(p, "%u%n", &page, &c) > 0) {
         p += c;
      } else {
         page = pageLim;
      }
      if (*pstate > page) goto err;
      if (*pstate < page) return *pstate;
      *q = p;
      *pstate = 0;
      return page;
   }

   while (isspace(*p) || *p == ',') p++;

   if (!*p) return 0; /* done */

   if (*p == '-') {
      *q = p;
      *pstate = 1;
      return 1; /* range with initial parameter omitted */
   }
   if (sscanf(p, "%u%n", &page, &c) > 0) {
      p += c;
      while (isspace(*p)) p++;
      *q = p;
      if (0 < page && page <= pageLim) {
         if (*p == '-') *pstate = page; /* range with start */
         return page;
      }
   }
   err:
   *pstate = -1;
   return 0;
}

static double N_Scale = 1, D_Scale = DEFAULT_SCALE;

static bool
read_scale(const char *s)
{
   char *p;
   double val;

   val = strtod(s, &p);
   if (val > 0 && p != s) {
      while (isspace(*p)) p++;
      if (*p == '\0') {
	 /* accept "<number>" as meaning "1:<number>" if number > 1
	  * or "<number>:1" is number < 1 - so all these are the same scale:
	  * 1:1000
	  * 1000
	  * 0.001
	  */
	 if (val > 1) {
	    N_Scale = 1;
	    D_Scale = val;
	 } else {
	    N_Scale = val;
	    D_Scale = 1;
	 }
	 return fTrue;
      }
      if (*p == ':') {
	 double val2;
	 optarg = p + 1;
	 val2 = strtod(optarg, &p);
	 if (val2 > 0 && p != optarg) {
	    while (isspace(*p)) p++;
	    if (*p == '\0') {
	       N_Scale = val;
	       D_Scale = val2;
	       return fTrue;
	    }
	 }
      }
   }
   return fFalse;
}

int
main(int argc, char **argv)
{
   bool fOk;
   double Sc = 0;
   int page, pages;
   char *fnm;
   int cPasses, pass;
   unsigned int cPagesPrinted;
   int state;
   char *p;
   char *szPages = NULL;
   int pageLim;
   bool fInteractive = fTrue;
   const char *msg166, *msg167;
   int old_charset;
   const char *output_fnm = NULL;
   const char *survey = NULL;
   bool fCalibrate = 0;

   /* TRANSLATE */
   static const struct option long_opts[] = {
      /* const char *name;
       * int has_arg (0 no_argument, 1 required_*, 2 optional_*);
       * int *flag;
       * int val; */
      {"survey", required_argument, 0, 1},
      {"elevation", no_argument, 0, 'e'},
      {"plan", no_argument, 0, 'p'},
      {"bearing", required_argument, 0, 'b'},
      {"tilt", required_argument, 0, 't'},
      {"scale", required_argument, 0, 's'},
      {"station-names", no_argument, 0, 'n'},
      {"crosses", no_argument, 0, 'c'},
      {"no-border", no_argument, 0, 'B'},
      {"no-legs", no_argument, 0, 'l'},
      {"surface", no_argument, 0, 'S'},
      {"skip-blanks", no_argument, 0, 'k'},
      {"output", required_argument, 0, 'o'},
      {"calibrate", no_argument, 0, 'C'},
      {"help", no_argument, 0, HLP_HELP},
      {"version", no_argument, 0, HLP_VERSION},
      {0, 0, 0, 0}
   };

#define short_opts "epb:t:s:ncBlSko:C"

   /* TRANSLATE */
   static struct help_msg help[] = {
/*				<-- */
      {HLP_ENCODELONG(0),       "only load the sub-survey with this prefix"},
      {HLP_ENCODELONG(1),       "select elevation"},
      {HLP_ENCODELONG(2),       "select plan view"},
      {HLP_ENCODELONG(3),       "set bearing"},
      {HLP_ENCODELONG(4),       "set tilt"},
      {HLP_ENCODELONG(5),       "set scale"},
      {HLP_ENCODELONG(6),       "display station names"},
      {HLP_ENCODELONG(7),       "display crosses at stations"},
      {HLP_ENCODELONG(8),       "turn off page border"},
      {HLP_ENCODELONG(9),       "turn off display of survey legs"},
      {HLP_ENCODELONG(10),      "turn on display of surface survey legs"},
      {HLP_ENCODELONG(11),      "don't output blank pages (implies --no-border)"},
      {HLP_ENCODELONG(12),      "set output file"},
      {HLP_ENCODELONG(13),      "print out calibration page"},
      {0, 0}
   };

   msg_init(argv);

   ASSERT(help[12].opt == HLP_ENCODELONG(12));
   ASSERT(help[13].opt == HLP_ENCODELONG(13));
   ASSERT(help[14].opt == 0);
   ASSERT(!(pr->flags & PR_FLAG_CALIBRATE && pr->flags & PR_FLAG_NOFILEOUTPUT));

   if (pr->flags & PR_FLAG_NOFILEOUTPUT) help[12] = help[14];
   if (!(pr->flags & PR_FLAG_CALIBRATE)) help[13] = help[14];
   
   fnm = NULL;

   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 0, -1);
   while (1) {
      int opt = cmdline_getopt();
      if (opt == EOF) break;
      switch (opt) {
       case 1:
	 survey = optarg;
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
       case 'S': /* Surface */
	 fSurface = 1;
	 break;
       case 'l': /* legs */
	 fShots = 0;
	 break;
       case 'k': /* Print blank pages */
	 fSkipBlank = 1;
	 fNoBorder = 1;
	 break;
       case 'e':
	 view = ELEV;
	 fInteractive = fFalse;
	 break;
       case 'p':
	 view = PLAN;
	 fInteractive = fFalse;
	 break;
       case 'b':
	 rot = cmdline_int_arg();
	 fInteractive = fFalse;
	 break;
       case 't':
	 tilt = cmdline_int_arg();
	 fInteractive = fFalse;
	 break;
       case 's':
	 if (!read_scale(optarg)) {
	    fatalerror(/*Bad scale `%s' (expecting e.g. `1:500', `500', or `0.002')*/217,
		       optarg);
	 }
	 fInteractive = fFalse;
	 break;
       case 'o':
	 output_fnm = optarg;
	 break;
       case 'C':
	 fCalibrate = 1;
	 fInteractive = fFalse;
	 break;
      }
   }

   /* at least one argument must be given unless -C is specified
    * - then no arguments may be given */
   if (fCalibrate) {
      time_t tm;

      if (argv[optind]) cmdline_too_many_args();

      fCrosses = fNoBorder = fSurface = fSkipBlank = fNoBorder = 0;
      fLabels = fShots = 1;
      view = PLAN;
      rot = tilt = 0;
      N_Scale = D_Scale = 1.0;
      title = osstrdup("calibration plot");

      tm = time(NULL);
      if (tm == (time_t)-1) {
	 datestamp = osstrdup(msg(/*Date and time not available.*/108));
      } else {
	 char date[256];
	 /* output current date and time in format specified */
	 strftime(date, 256, msg(/*%a,%Y.%m.%d %H:%M:%S %Z*/107),
		  localtime(&tm));
	 datestamp = osstrdup(date);
      }
   } else {
      if (!argv[optind]) cmdline_too_few_args();
   }

   szDesc = pr->Name();

   printf("Survex %s %s v"VERSION"\n  "COPYRIGHT_MSG"\n\n",
          szDesc, msg(/*Driver*/152));

   if (!fCalibrate) {      
      fnm = argv[optind++];

      /* Try to open first image file and check it has correct header,
       * rather than asking lots of questions then failing */
      pimg = img_open_survey(fnm, survey);
      if (!pimg) fatalerror(img_error(), fnm);

      /* Copy strings so they're valid after the 3d file is closed... */
      title = osstrdup(pimg->title);
      datestamp = osstrdup(pimg->datestamp);

      if (strlen(title) > 11 &&
	  strcmp(title + strlen(title) - 11, " (extended)") == 0) {
	 title[strlen(title) - 11] = '\0';
	 view = EXTELEV;
	 rot = 0;
	 tilt = 0;
      }
   }

   if (pr->Init) {
      if (pr->flags & PR_FLAG_NOINI) {
	 pr->Init(NULL, msg_cfgpth(), output_fnm, &scX, &scY, 0);
      } else {
	 FILE *fh_list[4];
	 FILE **pfh = fh_list;
	 FILE *fh;
	 const char *pth_cfg;
	 char *print_ini;

	 /* ini files searched in this order:
	  * ~/.survex/print.ini [unix only]
	  * /etc/survex/print.ini [unix only]
	  * $SURVEXHOME/myprint.ini [not unix]
	  * $SURVEXHOME/print.ini [must exist]
	  */

#if (OS==UNIX)
	 pth_cfg = getenv("HOME");
	 if (pth_cfg) {
	    fh = fopenWithPthAndExt(pth_cfg, ".survex/print."EXT_INI, NULL,
				    "rb", NULL);
	    if (fh) *pfh++ = fh;
	 }
	 pth_cfg = msg_cfgpth();
	 fh = fopenWithPthAndExt(NULL, "/etc/survex/print."EXT_INI, NULL, "rb",
				 NULL);
	 if (fh) *pfh++ = fh;
#else
	 pth_cfg = msg_cfgpth();
	 print_ini = add_ext("myprint", EXT_INI);
	 fh = fopenWithPthAndExt(pth_cfg, print_ini, NULL, "rb", NULL);
	 if (fh) *pfh++ = fh;
#endif
	 print_ini = add_ext("print", EXT_INI);
	 fh = fopenWithPthAndExt(pth_cfg, print_ini, NULL, "rb", NULL);      
	 if (!fh) fatalerror(/*Couldn't open data file `%s'*/24, print_ini);
	 *pfh++ = fh;
	 *pfh = NULL;
	 pr->Init(fh_list, pth_cfg, output_fnm, &scX, &scY, fCalibrate);
	 for (pfh = fh_list; *pfh; pfh++) (void)fclose(*pfh);
      }
   }

   if (fInteractive && view != EXTELEV) {
      char szReplies[3];
      szReplies[0] = *msg(/*plan*/183);
      szReplies[1] = *msg(/*elevation*/184);
      szReplies[2] = '\0';
      /* "Plan or Elevation (pe) : " */
      fputs(msg(/*Plan or Elevation*/158), stdout);
      putchar(' ');
      view = PLAN;
      if (getanswer(szReplies) == 1) view = ELEV;

      do {
         printf(msg(view == PLAN ? /*Bearing up page (degrees): */159:
		    /*View towards: */160));
         fgets(szTmp, sizeof(szTmp), stdin);
      } while (*szTmp >= 32 && sscanf(szTmp, "%d", &rot) < 1);
      if (*szTmp < 32) {
         /* if just return, default view to North up page/looking North */
         rot = 0;
         printf("%d\n", rot);
      }
   }

   SIN = sin(rad(rot));
   COS = cos(rad(rot));

   if (view == ELEV) {
      if (fInteractive) {
         do {
            printf(msg(/*Tilt (degrees): */161));
            fgets(szTmp, sizeof(szTmp), stdin);
         } while (*szTmp >= 32 && sscanf(szTmp, "%d", &tilt) < 1);
         if (*szTmp < 32) {
            /* if just return, default to no tilt */
            tilt = 0;
            printf("%d\n", tilt);
         }
      }
      if (tilt != 0) view = TILT;
      SINT = sin(rad(tilt));
      COST = cos(rad(tilt));
   }

   if (fInteractive) {
      char *q;
      if (survey == NULL) {
	 fputs(msg(/*Only load the sub-survey with prefix*/199), stdout);
	 puts(":");
	 fgets(szTmp, sizeof(szTmp), stdin);
	 if (szTmp[0] >= 32) {
	    size_t len = strlen(szTmp);
	    if (szTmp[len - 1] == '\n') szTmp[len - 1] = '\0';	 
	    survey = osstrdup(szTmp);
	 }
      }

      q = szTmp;
      if (fLabels) *q++ = 'n';
      if (fCrosses) *q++ = 'c';
      if (fShots) *q++ = 'l';
      if (fSurface) *q++ = 's';
      *q = '\0';
      printf(msg(/*Plot what (n = station names, c = crosses, l = legs, s = surface) (default &quot;%s&quot;)&#10;: */58), szTmp);
      fgets(szTmp, sizeof(szTmp), stdin);
      if (szTmp[0] >= 32) {
	  fLabels = (strchr(szTmp, 'n') != NULL);
	  fCrosses = (strchr(szTmp, 'c') != NULL);
	  fShots = (strchr(szTmp, 'l') != NULL);
	  fSurface = (strchr(szTmp, 's') != NULL);
      }

      putnl();
      puts(msg(/*Reading in data - please wait...*/105));
   }

   if (fCalibrate) {
      img_point pt = { 0.0, 0.0, 0.0 };
      xMax = yMax = 0.1;
      xMin = yMin = 0;

      stack(img_MOVE, NULL, &pt);
      pt.x = 0.1;
      stack(img_LINE, NULL, &pt);
      pt.y = 0.1;
      stack(img_LINE, NULL, &pt);
      pt.x = 0.0;
      stack(img_LINE, NULL, &pt);
      pt.y = 0.0;
      stack(img_LINE, NULL, &pt);
      pt.x = 0.05;
      pt.y = 0.001;
      stack(img_LABEL, "10cm", &pt);
      pt.x = 0.001;
      pt.y = 0.05;
      stack(img_LABEL, "10cm", &pt);

      *ppliEnd = NULL;
      
      Sc = 1000.0;
      pages_required(Sc);
   } else {
      double w, x;

      xMax = yMax = -DBL_MAX; /* any (sane) value will beat this */
      xMin = yMin = DBL_MAX; /* ditto */

      while (fnm) {
	 /* first time around pimg is already open... */
	 if (!pimg) {
	    /* for multiple files use title and datestamp from the first */
	    pimg = img_open_survey(fnm, survey);
	    if (!pimg) fatalerror(img_error(), fnm);
	 }
	 if (!read_in_data()) fatalerror(img_error(), fnm);
	 img_close(pimg);
	 pimg = NULL;
	 fnm = argv[optind++];
      }

      /* can't have been any data */
      if (xMax < xMin || yMax < yMin) fatalerror(/*No data in 3d Image file*/86);

      x = 1000.0 / pick_scale(1, 1);

      /* trim to 2 s.f. (rounding up) */
      w = pow(10.0, floor(log10(x) - 1.0));
      x = ceil(x / w) * w;

      fputs(msg(/*Scale to fit on 1 page*/83), stdout);
      printf(" = 1:%.0f\n", x);
      if (N_Scale == 0.0) {
         N_Scale = 1;
         D_Scale = x;
         Sc = N_Scale * 1000 / D_Scale;
         pages_required(Sc);
      } else if (!fInteractive) {
         Sc = N_Scale * 1000 / D_Scale;
         pages_required(Sc);
      }
   }

   if (fInteractive) do {
      putnl();
      printf(msg(/*Please enter Map Scale = X:Y (default 1:%d)&#10;: */162),
	     DEFAULT_SCALE);
      fgets(szTmp, sizeof(szTmp), stdin);
      putnl();
      if (*szTmp == '\n') {
         N_Scale = 1.0;
         D_Scale = (double)DEFAULT_SCALE;
      } else if (!read_scale(szTmp)) {
	 size_t len = strlen(szTmp);
	 if (len && szTmp[len - 1] == '\n') szTmp[len - 1] = '\0';	 
	 printf(msg(/*Bad scale `%s' (expecting e.g. `1:500', `500', or `0.002')*/217),
		szTmp);
	 fOk = fFalse;
	 continue;
      }
      printf(msg(/*Using scale %.0f:%.0f*/163), N_Scale, D_Scale);
      putnl();
      Sc = N_Scale * 1000 / D_Scale;
      pages_required(Sc);
      fOk = describe_layout(pagesX, pagesY);
   } while (!fOk);

   pageLim = pagesX * pagesY;
   if (pageLim == 1) {
      pages = 1;
   } else {
      do {
         fOk = fTrue;
         if (fInteractive) {
	    fputs(msg(/*Print which pages?&#10;(RETURN for all; 'n' for one page, 'm-n', 'm-', '-n' for a range)&#10;: */164), stdout);
            fgets(szTmp, sizeof(szTmp), stdin);
	 } else {
            *szTmp = '\0'; /* default to all if non-interactive */
	 }

	 p = szTmp;
	 while (isspace(*p)) p++;
         szPages = osstrdup(p);
         if (*szPages) {
            pages = page = state = 0;
	    p = szPages + strlen(szPages) - 1;
	    if (*p == '\n') *p = '\0';
	    p = szPages;
	    while (1) {
               page = next_page(&state, &p, pageLim);
               if (state < 0) {
                  printf(msg(/*Bad list of pages to print `%s'*/218), szPages);
		  putnl();
                  fOk = fFalse;
                  break;
               }
               if (page == 0) break;
               pages++;
            }
         } else {
            pages = pageLim;
         }
      } while (!fOk);
   }

   /* if no explicit Alloc, default to one pass */
   cPasses = (pr->Pre ? pr->Pre(pages, title) : 1);

   puts(msg(/*Generating images...*/165));

   /* note down so we can switch to printer charset */
   msg166 = msgPerm(/*Page %d of %d*/166);
   select_charset(pr->Charset ? pr->Charset() : CHARSET_USASCII);

   /* used in printer's native charset in footer */
   msg167 = msgPerm(/*Survey `%s'   Page %d (of %d)   Processed on %s*/167);

   old_charset = select_charset(CHARSET_ISO_8859_1);
   cPagesPrinted = 0;
   page = state = 0;
   p = szPages;
   while (1) {
      if (pageLim == 1) {
         if (page == 0)
            page = 1;
         else
            page = 0; /* we've already printed the only page */
      } else if (!*szPages) {
         page++;
         if (page > pageLim) page = 0; /* all pages printed */
      } else {
         page = next_page(&state, &p, pageLim);
      }
      ASSERT(state >= 0); /* errors should have been caught above */
      if (page == 0) break;
      cPagesPrinted++;
      if (pages > 1) {
         putchar('\r');
         printf(msg166, (int)cPagesPrinted, pages);
      }
      /* don't skip the page with the info box on */
      if (fSkipBlank && (int)page != (pagesY - 1) * pagesX + 1)
         pass = -1;
      else
         pass = 0;
      fBlankPage = fFalse;
      for ( ; pass < cPasses; pass++) {
         li *pli;
	 long x, y;
	 int pending_move = 0;

	 x = y = INT_MAX;

         if (pr->NewPage)
            pr->NewPage((int)page, pass, pagesX, pagesY);

         if ((int)page == (pagesY - 1) * pagesX + 1) {
	    if (pr->SetFont) pr->SetFont(PR_FONT_DEFAULT);
            draw_info_box(N_Scale, D_Scale);
	 }

	 if (pr->SetFont) pr->SetFont(PR_FONT_LABELS);
         for (pli = pliHead; pli; pli = pli->pliNext) {
	    if (pass == -1 && !fBlankPage) break;
            switch (pli->tag) {
             case img_MOVE:
               if (fShots) {
		  long xnew, ynew;
		  xnew = (long)((pli->to.x * Sc + xOrg) * scX);
	          ynew = (long)((pli->to.y * Sc + yOrg) * scY);

                  /* avoid superfluous moves */
                  if (xnew != x || ynew != y) {
		     x = xnew;
		     y = ynew;
		     pending_move = 1;
		  }
	       }
               break;
             case img_LINE:
               if (fShots) {
		  long xnew, ynew;
		  xnew = (long)((pli->to.x * Sc + xOrg) * scX);
	          ynew = (long)((pli->to.y * Sc + yOrg) * scY);

		  if (pending_move) {
		     pr->MoveTo(x, y);
		     pending_move = 0;
		  }

                  /* avoid drawing superfluous lines */
                  if (xnew != x || ynew != y) {
		     x = xnew;
		     y = ynew;
		     pr->DrawTo(x, y);
		  }
               }
               break;
             case img_LABEL:
               if (fCrosses || fLabels) {
                  x = (long)((pli->to.x * Sc + xOrg) * scX);
	          y = (long)((pli->to.y * Sc + yOrg) * scY);
	       }
               if (fCrosses) pr->DrawCross(x, y);
               if (fLabels) {
                  pr->MoveTo(x, y);
		  pr->WriteString(pli->label);
	       }
	       if (fCrosses || fLabels) x = y = INT_MAX;
               break;
            }	    
	 }

         if (pass == -1) {
            if (fBlankPage) break;
         } else {
            if (pr->ShowPage) {
               sprintf(szTmp, msg167, title, (int)page, pagesX * pagesY,
                       datestamp);
               pr->ShowPage(szTmp);
            }
         }
      }
   }

   if (pr->Post) pr->Post();

   if (pr->Quit) pr->Quit();

   select_charset(old_charset);
   putnl();
   putnl();
   puts(msg(/*Done.*/144));
   return EXIT_SUCCESS;
}

#define fDotBorders 1
/* Draws in alignment marks on each page or borders on edge pages */
/* There's nothing like proper code reuse, and this is nothing like... */
void
drawticks(border clip, int tsize, int x, int y)
{
   long i;
   int s = tsize * 4;
   int o = s / 8;
   pr->MoveTo(clip.x_min, clip.y_min);
   if (fNoBorder || x) {
      pr->DrawTo(clip.x_min, clip.y_min + tsize);
      if (fDotBorders) {
         i = (clip.y_max - clip.y_min) -
	     (tsize + ((clip.y_max - clip.y_min - tsize * 2L) % s) / 2);
         for ( ; i > tsize; i -= s) {
            pr->MoveTo(clip.x_min, clip.y_max - (i + o));
            pr->DrawTo(clip.x_min, clip.y_max - (i - o));
         }
      }
      pr->MoveTo(clip.x_min, clip.y_max - tsize);
   }
   pr->DrawTo(clip.x_min, clip.y_max);
   if (fNoBorder || y < pagesY - 1) {
      pr->DrawTo(clip.x_min + tsize, clip.y_max);
      if (fDotBorders) {
         i = (clip.x_max - clip.x_min) -
             (tsize + ((clip.x_max - clip.x_min - tsize * 2L) % s) / 2);
         for ( ; i > tsize; i -= s) {
            pr->MoveTo(clip.x_max - (i + o), clip.y_max);
            pr->DrawTo(clip.x_max - (i - o), clip.y_max);
         }
      }
      pr->MoveTo(clip.x_max - tsize, clip.y_max);
   }
   pr->DrawTo(clip.x_max, clip.y_max);
   if (fNoBorder || x < pagesX - 1) {
      pr->DrawTo(clip.x_max, clip.y_max - tsize);
      if (fDotBorders) {
         i = (clip.y_max - clip.y_min) -
             (tsize + ((clip.y_max - clip.y_min - tsize * 2L) % s) / 2);
         for ( ; i > tsize; i -= s) {
            pr->MoveTo(clip.x_max, clip.y_min + (i + o));
            pr->DrawTo(clip.x_max, clip.y_min + (i - o));
         }
      }
      pr->MoveTo(clip.x_max, clip.y_min + tsize);
   }
   pr->DrawTo(clip.x_max, clip.y_min);
   if (fNoBorder || y) {
      pr->DrawTo(clip.x_max - tsize, clip.y_min);
      if (fDotBorders) {
         i = (clip.x_max - clip.x_min) -
             (tsize + ((clip.x_max - clip.x_min - tsize * 2L) % s) / 2);
         for ( ; i > tsize; i -= s) {
            pr->MoveTo(clip.x_min + (i + o), clip.y_min);
            pr->DrawTo(clip.x_min + (i - o), clip.y_min);
         }
      }
      pr->MoveTo(clip.x_min + tsize, clip.y_min);
   }
   pr->DrawTo(clip.x_min, clip.y_min);
}

static void setting_missing(const char *v)
{
   fatalerror(/*Parameter `%s' missing in printer configuration file*/85, v);
}

static void setting_bad_value(const char *v, const char *p)
{
   fatalerror(/*Parameter `%s' has invalid value `%s' in printer configuration file*/82,
	      v, p);
}

char *
as_string(const char *v, char *p)
{
   if (!p) setting_missing(v);
   return p;
}

int
as_int(const char *v, char *p, int min_val, int max_val)
{
   long val;
   char *pEnd;
   if (!p) setting_missing(v);
   val = strtol(p, &pEnd, 10);
   if (pEnd == p || val < (long)min_val || val > (long)max_val)
      setting_bad_value(v, p);
   osfree(p);
   return (int)val;
}

int
as_bool(const char *v, char *p)
{
   return as_int(v, p, 0, 1);
}

double
as_double(const char *v, char *p, double min_val, double max_val)
{
   double val;
   char *pEnd;
   if (!p) setting_missing(v);
   val = strtod(p, &pEnd);
   if (pEnd == p || val < min_val || val > max_val)
      setting_bad_value(v, p);
   osfree(p);
   return val;
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
int
as_escstring(const char *v, char *s)
{
   char *p, *q;
   char c;
   int pass;
   static const char *escFr = "[nrt?0"; /* 0 is last so maps to the implicit \0 */
   static const char *escTo = "\x1b\n\r\t\?";
   bool fSyntax = fFalse;
   if (!s) setting_missing(v);
   for (pass = 0; pass <= 1; pass++) {
      p = q = s;
      while (*p) {
	 c = *p++;
	 if (c == '\\') {
	    c = *p++;
	    switch (c) {
	     case '\\': /* literal "\" */
	       break;
	     case 'x': /* hex digits */
	       if (isxdigit(*p) && isxdigit(p[1])) {
		  if (pass) c = (CHAR2HEX(*p) << 4) | CHAR2HEX(p[1]);
		  p += 2;
		  break;
	       }
	       /* \x not followed by 2 hex digits */
	       /* !!! FALLS THROUGH !!! */
	     case '\0': /* trailing \ is meaningless */
	       fSyntax = 1;
	       break;
	     default: 
	       if (pass) {
		  const char *t = strchr(escFr, c);
		  if (t) {
		     c = escTo[t - escFr];
		     break;
		  }
		  /* \<capital letter> -> Ctrl-<letter> */
		  if (isupper(c)) {
		     c -= '@';
		     break;
		  }
		  /* otherwise don't do anything to c (?) */
		  break;
	       }
	    }
	 }
	 if (pass) *q++ = c;
      }
      if (fSyntax) {
	 ASSERT(pass == 0);
	 setting_bad_value(v, s);
      }
   }
   return (q - s);
}
