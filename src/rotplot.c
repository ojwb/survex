/* rotplot.c */

/* Copyright (C) Olly Betts 1994-1997
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
# include <config.h>
#endif

#include "useful.h"
#include "caverot.h"
#include "rotplot.h"
#include "cvrotgfx.h"

static coord x1 = 0, x2 = 0, y1_sigh = 0, y2 = 0, y3 = 0;
static float sc_last, theta_last, elev_last;
static bool fPlan;

static int fixed_pt_pos = FIXED_PT_POS;

/* looking towards theta from height z, with zoom factor sc */
extern void
set_view(float sc, float theta, float elev)
{
   double sine, cosine;
   double scale;
   double X1, X2, Y1, Y2, Y3;
   double max, t;
   double phi;
   phi = rad(elev);

   sc_last = sc;
   theta_last = theta;
   elev_last = elev;
   fPlan = (elev == 90.0);

   sine = (float)SIND(theta);
   cosine = (float)COSD(theta);

   X1 = cosine * sc;
   X2 = -sine * sc;
#define CORNER(XP, YP) fabs((coord)(Xorg XP Xrad) * X1 + \
                            (coord)(Yorg YP Yrad) * X2)
   max = CORNER(+, +);
   t = CORNER(-, +);
   if (t > max) max = t;
   t = CORNER(+, -);
   if (t > max) max = t;
   t = CORNER(-, -);
   if (t > max) max = t;
#undef CORNER

   if (fPlan) {
      Y1 = sine * sc * y_stretch;
      Y2 = cosine * sc * y_stretch;
      Y3 = 0.0;
#define CORNER(XP, YP) fabs((coord)(Xorg XP Xrad) * Y1 + \
                            (coord)(Yorg YP Yrad) * Y2)
      t = CORNER(+, +);
      if (t > max) max = t;
      t = CORNER(-, +);
      if (t > max) max = t;
      t = CORNER(+, -);
      if (t > max) max = t;
      t = CORNER(-, -);
      if (t > max) max = t;
#undef CORNER
   } else {
      /* work out y scale factor for whole survey */
      Y3 = (double)sc * y_stretch * cos(phi);
      Y1 = sine * tan(phi) * Y3;
      Y2 = cosine * tan(phi) * Y3;
#define CORNER(XP, YP, ZP) fabs((coord)(Xorg XP Xrad) * Y1 + \
                                (coord)(Yorg YP Yrad) * Y2 + \
                                (coord)(Zorg ZP Zrad) * Y3)
      t = CORNER(+, +, +);
      if (t > max) max = t;
      t = CORNER(-, +, +);
      if (t > max) max = t;
      t = CORNER(+, -, +);
      if (t > max) max = t;
      t = CORNER(+, +, -);
      if (t > max) max = t;
      t = CORNER(-, -, +);
      if (t > max) max = t;
      t = CORNER(+, -, -);
      if (t > max) max = t;
      t = CORNER(-, +, -);
      if (t > max) max = t;
      t = CORNER(-, +, -);
      if (t > max) max = t;
      t = CORNER(-, -, -);
      if (t > max) max = t;
#undef CORNER
   }

   if (max >= 0.1)
      fixed_pt_pos = 30 - (int)ceil(log10(max) / log10(2));
   else
      fixed_pt_pos = 30;

   if (fixed_pt_pos < 0)
      fixed_pt_pos = 0;
   else if (fixed_pt_pos > 30)
      fixed_pt_pos = 30;

#if 0
   xos_writec(os_VDU_GRAPH_TEXT_OFF);
   xos_writec(31), xos_writec(0), xos_writec(10);
   printf("%d %f", fixed_pt_pos, scDefault);
   xos_writec(os_VDU_GRAPH_TEXT_ON);
#endif

   scale = (ulong)(1UL << fixed_pt_pos);
   x1 = (coord)(scale * X1);
   x2 = (coord)(scale * X2);
   y1_sigh = (coord)(scale * Y1);
   y2 = (coord)(scale * Y2);
   y3 = (coord)(scale * Y3);
}

extern void
draw_view_legs(lid Huge *plid)
{
   if (fPlan) {
      for ( ; plid; plid = plid->next)
	 plot_plan(plid->pData, x1, x2, y1_sigh, y2, fixed_pt_pos);
   } else if (y1_sigh == 0 && y2 == 0) {
      for ( ; plid; plid = plid->next)
	 plot_no_tilt(plid->pData, x1, x2, y3, fixed_pt_pos);
   } else {
      for ( ; plid; plid = plid->next)
	 plot(plid->pData, x1, x2, y1_sigh, y2, y3, fixed_pt_pos);
   }
}

extern void
draw_view_stns(lid Huge *plid)
{
   if (fPlan) {
      for ( ; plid; plid = plid->next)
	 splot_plan(plid->pData, x1, x2, y1_sigh, y2, fixed_pt_pos);
   } else if (y1_sigh == 0 && y2 == 0) {
      for ( ; plid; plid = plid->next)
	 splot_no_tilt(plid->pData, x1, x2, y3, fixed_pt_pos);
   } else {
      for ( ; plid; plid = plid->next)
	 splot(plid->pData, x1, x2, y1_sigh, y2, y3, fixed_pt_pos);
   }
}

extern void
draw_view_labs(lid Huge *plid)
{
   if (fPlan) {
      for ( ; plid; plid = plid->next)
	 lplot_plan(plid->pData, x1, x2, y1_sigh, y2, fixed_pt_pos);
   } else if (y1_sigh == 0 && y2 == 0) {
      for ( ; plid; plid = plid->next)
	 lplot_no_tilt(plid->pData, x1, x2, y3, fixed_pt_pos);
   } else {
      for ( ; plid; plid = plid->next)
	 lplot(plid->pData, x1, x2, y1_sigh, y2, y3, fixed_pt_pos);
   }
}

#define XTRA 4
void
draw_scale_bar(void)
{
   static coord pScaleBar[] = {
      (coord)MOVE, (coord)-49, (coord)0, (coord)-53,
      (coord)DRAW, (coord)-49, (coord)0, (coord) 47,
      (coord)STOP
   };
   static coord pCompass[] = {
      (coord)MOVE, (coord)450, (coord)0, (coord)450,
      (coord)DRAW, (coord)450, (coord)0, (coord)450,
      (coord)DRAW, (coord)450, (coord)0, (coord)450,
      (coord)DRAW, (coord)450, (coord)0, (coord)450,
      (coord)STOP
   };
   char sz[80];
   double len, maxlen;
   int r;
   /* length in metres of scale bar */
   maxlen = .96 * ycMac / (sc_last * fabs((double)y_stretch));
   /* fprintf(stderr, "maxlen = %g\n", maxlen); */

   /* The (double) cast would seem to be totally superfluous,
    * but seems to cure a DJGPP bug
    * (hmm, this could be the DJGPP floor()/ceil() bug) */
   len = pow(10.0, floor((double)log10(maxlen)));
   /* fprintf(stderr, "len = %g\n", len); */
   r = (int)floor(maxlen / len);
   /* fprintf(stderr, "r = %d\n", r); */
   if (r >= 5)
      len *= 5.0;
   else if (r >= 2)
      len *= 2.0;

   len *= 0.01;
   pScaleBar[3] = (coord)((1L << XTRA) * (-.5) * maxlen / len);
   pScaleBar[7] = (coord)(pScaleBar[3] + (100L << XTRA));

   if (len < 0.01)
      sprintf(sz, "%gmm", len * 1000.0);
   else if (len < 1.0)
      sprintf(sz, "%gcm", len * 100.0);
   else if (len < 1000.0)
      sprintf(sz, "%gm", len);
   else
      sprintf(sz, "%gkm", len / 1000.0);
#if 0
  { /* display the time slice */
     extern int dt; /* now static in caverot.c */
     sprintf(sz + strlen(sz), " %d", dt);
  }
#endif
   text_xy(0, 1, sz);
   /* outtextxy(8, 12, sz); */
   plot_no_tilt((point Huge *)pScaleBar,
		(coord)(xcMac * .01 * (float)(1L << FIXED_PT_POS)),
		(coord)0,
		(coord)(len * sc_last * y_stretch *
			(float)(1L << (FIXED_PT_POS-XTRA))),
		FIXED_PT_POS );

     {
#define EDGE .49
#define RADIUS .03
	double x_org, y_org, x_rad, y_rad;
	x_rad = RADIUS * xcMac;
	y_rad = x_rad * y_stretch;
	x_org = EDGE * xcMac - x_rad;
	y_org = EDGE * ycMac;
	if (y_stretch < 0) y_org = -y_org;
	y_org -= y_rad;
	pCompass[1] = (coord)(x_org - x_rad * SIND(theta_last - 160.0));
	pCompass[3] = (coord)(y_org + y_rad * COSD(theta_last - 160.0));
	pCompass[5] = (coord)(x_org - x_rad * SIND(theta_last));
	pCompass[7] = (coord)(y_org + y_rad * COSD(theta_last));
	pCompass[9] = (coord)(x_org - x_rad * SIND(theta_last + 160.0));
	pCompass[11] = (coord)(y_org + y_rad * COSD(theta_last + 160.0));
	pCompass[13] = pCompass[1];
	pCompass[15] = pCompass[3];

	plot_no_tilt((point Huge *)pCompass,
		     (coord)(1L << FIXED_PT_POS),
		     (coord)0,
		     (coord)(1L << FIXED_PT_POS),
		     FIXED_PT_POS);

	y_org = -y_org;
	pCompass[1] = (coord)(x_org + x_rad * COSD(elev_last - 160.0));
	pCompass[3] = (coord)(y_org - y_rad * SIND(elev_last - 160.0));
	pCompass[5] = (coord)(x_org + x_rad * COSD(elev_last));
	pCompass[7] = (coord)(y_org - y_rad * SIND(elev_last));
	pCompass[9] = (coord)(x_org + x_rad * COSD(elev_last + 160.0));
	pCompass[11] = (coord)(y_org - y_rad * SIND(elev_last + 160.0));
	pCompass[13] = pCompass[1];
	pCompass[15] = pCompass[3];

	plot_no_tilt((point Huge *)pCompass,
		     (coord)(1L << FIXED_PT_POS),
		     (coord)0,
		     (coord)(1L << FIXED_PT_POS),
		     FIXED_PT_POS);
     }
}
