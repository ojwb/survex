//
//  Aven.h
//
//  A cave visualisation application for use in conjunction with Survex and
//  the GTK+ toolkit.
//
//  Copyright (C) 1999-2000, Mark R. Shinwell.
//  Portions from Survex Copyright (C) Olly Betts and others.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#ifndef aven_h
#define aven_h

#include <X11/Xlib.h>
#include <X11/extensions/Xdbe.h>
#include <gtk--.h>

extern "C" {
// Survex stuff:
#include "useful.h"
#include "filename.h"
#include "message.h"
#include "caverot.h" // for LITTLE_MAGNIFY_FACTOR, etc
#include "cvrotimg.h"
};

static const int NCOLS = 13;

class Aven : public Gtk::Window {
    Gtk::DrawingArea m_Cave;
    bool m_Moving;
    bool m_Scaling;
    int m_CurX;
    int m_CurY;
    int m_StartX;
    int m_StartY;
    int m_LastX;
    int m_LastY;
    GdkGC* m_LineGC;
    int m_XSize;
    int m_YSize;
    GdkWindow* m_CaveWin;
    float m_LastFactor;
    float factor;
    float m_Angle;
    GdkPixmap* m_DrawPixmap;
    bool m_Eleving;
    bool m_Rotating;
    float m_RotateAngle;
    bool m_SwitchToPlan;
    bool m_SwitchToElevation;

    lid* m_RawLegs[2];
    lid* m_RawStns[2];
    int m_NumLines[NCOLS];
    XdbeBackBuffer m_BackBuf;

    float elev_angle;

    GdkColor black, red;

    struct _line {
        coord* x0;
        coord* y0;
        coord* x1;
        coord* y1;
        coord* x0_orig;
        coord* y0_orig;
        coord* x1_orig;
        coord* y1_orig;
        coord* z0, *z1, *z0_orig, *z1_orig;
        bool* ignore;
        coord* px0, *py0, *pz0, *px1, *py1, *pz1;
    };

    _line m_Lines[NCOLS];

    bool m_ForceUseOrig;

    int total_xshift, total_yshift;
    float sx, cx, fx, fy, fz;
    float m_Scale, m_Scale2;
    coord x_mid, y_mid, z_mid;
    int m_CurOriginX, m_CurOriginY;
    int m_RotationTrigOffset;
    float z_col_scale;
    bool m_WorkProcEnabled;
    bool m_WorkProcStateBeforeDrag;
    bool m_Splash;
    int m_SplashStep;
    SigC::Connection m_WorkProc;

    Gtk::HBox m_StatusBar;
    Gtk::Label m_HeadingLabel;
    Gtk::Label m_LegsLabel;
    Gtk::Label m_MouseLabel;

    float m_AngleOrig;

    GdkColor cols[NCOLS];

    double RotationAngleFromXOffset(int);

    void AllocColour(GdkColor*, float, float, float);

public:
    Aven(string name);
    virtual ~Aven();

    bool Contributes(int x0, int y0, int x1, int y1,
                     int clip_x0, int clip_y0, int clip_x1, int clip_y1);

    void Open(void*);

    gint ExposeEvent(GdkEventExpose* event);
    gint ButtonPressEvent(GdkEventButton* event);
    gint ButtonReleaseEvent(GdkEventButton* event);
    gint PointerMotionEvent(GdkEventMotion* event);
    gint KeyPressEvent(GdkEventKey* event);
    gint Idle();

    void EnableWorkProc();
    void DisableWorkProc();

    int toscreen_x(coord X, coord Y, coord Z, float scale=-1.0)
    {
       if (scale==-1.0) scale = m_Scale;
       return int(((X - x_mid) * -cx + (Y - y_mid) * -sx) * scale);
    }

    int toscreen_y(coord X, coord Y, coord Z, float scale=-1.0)
    {
       if (scale==-1.0) scale = m_Scale;
       int y= int(((X - x_mid) * fx + (Y - y_mid) * fy
                      + (Z - z_mid) * fz) * scale);


       return -y;
    }

    void Draw(GdkPixmap* window, GdkGC* gc, int x0, int y0, int x1, int y1,
              float scale,
              int bx0, int by0, int bx1, int by1,
              bool = false, coord = 0, coord = 0, coord = 0);

    gint delete_event_impl();

    void Rotate();
    void Stop();
    void Reverse();
    void Plan();
    void Elevation();
    void Faster();
    void Slower();
};

#endif
