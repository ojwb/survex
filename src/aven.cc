//
//  Aven.cc
//
//  A cave visualisation application for Survex.
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

// Vastly improved speed is obtained with this #defined...
#define NASTY_SPEED_HACK

#include "aven.h"

#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>

extern "C" {
// X-specific GTK+ interface
#include <gdk/gdkx.h>
};

#define STOP 0
#define MOVE 1
#define DRAW 2

// Key bindings:
static const char key_ROTATION_1 = ' ';
static const char key_ROTATION_2 = '\n';
static const char key_REVERSE_ROTATION = 'R';
static const char key_SPEED_UP = 'Z';
static const char key_SLOW_DOWN = 'X';
static const char key_PLAN = 'P';
static const char key_ELEVATION = 'L';

// Various constants:
static const int WIDTH = 800;
static const int HEIGHT = 600;
static const double SPEED = 1.5;
static const int SCREEN_WIDTH = 1024;
static const double PIBY180 = 0.017453293;
static const int SPLASH_STEPS = 10;

Aven::Aven(string name) :
    m_Eleving(false), m_Rotating(false), m_Splash(true), m_SplashStep(1),
    m_Moving(false), m_CurX(0), m_CurY(0), m_CaveWin(NULL), m_LastFactor(1.0),
    m_Scale2(1.0), m_RotateAngle(PI / 180.0),
    m_CurOriginX(0), m_CurOriginY(0), m_ForceUseOrig(false), m_Angle(0),
    elev_angle(90.0), total_xshift(0), total_yshift(0), m_XSize(WIDTH), m_YSize(HEIGHT),
    m_SwitchToPlan(false), m_SwitchToElevation(false), m_WorkProcEnabled(false)
{
    if (!load_data(name.c_str(), m_RawLegs, m_RawStns)) {
        assert(0);
    }
    
    m_RawLegs[1] = NULL;
    m_RawStns[1] = NULL;
    
    int nl = 0;
    lid* ptr = m_RawLegs[0];
    while (ptr) {
        point *p;
        for (p = ptr->pData; p->_.action != STOP; p++) {
            switch (p->_.action) {
                case DRAW:
                    nl++;
                    break;
            }
        }
        
        ptr = ptr->next;
    }

    m_Scale = scale_to_screen(m_RawLegs, m_RawStns, WIDTH, HEIGHT, 1.0) * 1.5;
    m_Scale /= pow(1.5, SPLASH_STEPS);

    z_col_scale = ((float) (NCOLS - 1)) / (2 * Zrad);
    
    x_mid = Xorg;
    y_mid = Yorg;
    z_mid = Zorg;
    
    for (int depth = 0; depth < NCOLS; depth++) {
        m_NumLines[depth] = 0;
        m_Lines[depth].x0 = new int[nl];
        m_Lines[depth].x1 = new int[nl];
        m_Lines[depth].y0 = new int[nl];
        m_Lines[depth].y1 = new int[nl];
        m_Lines[depth].z0 = new int[nl];
        m_Lines[depth].z1 = new int[nl];
        m_Lines[depth].x0_orig = new int[nl];
        m_Lines[depth].x1_orig = new int[nl];
        m_Lines[depth].y0_orig = new int[nl];
        m_Lines[depth].y1_orig = new int[nl];
        m_Lines[depth].z0_orig = new int[nl];
        m_Lines[depth].z1_orig = new int[nl];
        m_Lines[depth].ignore = new bool[nl];
        m_Lines[depth].px0 = new coord[nl];
        m_Lines[depth].py0 = new coord[nl];
        m_Lines[depth].pz0 = new coord[nl];
        m_Lines[depth].px1 = new coord[nl];
        m_Lines[depth].py1 = new coord[nl];
        m_Lines[depth].pz1 = new coord[nl];
    }

    coord x1, y1, z1;
       
    ptr = m_RawLegs[0];
    
    float angle = m_Angle;
    
    sx = sin((angle)*PIBY180);
    cx = cos((angle)*PIBY180);
    fz = cos(elev_angle*PIBY180);
    fx = sin(elev_angle*PIBY180) * sx;
    fy = sin(elev_angle*PIBY180) * -cx;

    coord _px1, _py1, _pz1;
    
    while (ptr) {
        point *p;
        for (p = ptr->pData; p->_.action != STOP; p++) {
        switch (p->_.action) {
            case MOVE:
                x1 = toscreen_x(p->X, p->Y, p->Z);
                y1 = toscreen_y(p->X, p->Y, p->Z);
                z1 = p->Z;
                _px1 = p->X; _py1 = p->Y; _pz1 = p->Z;
                break;

            case DRAW: {
                int depth = (int) ((float) (- p->Z + Zorg + Zrad) * z_col_scale);
                int leg = m_NumLines[depth];
                assert(leg < nl);
                    
                m_Lines[depth].px0[leg] = _px1; 
                m_Lines[depth].py0[leg] = _py1; 
                m_Lines[depth].pz0[leg] = _pz1;
                m_Lines[depth].px1[leg] = p->X; 
                m_Lines[depth].py1[leg] = p->Y; 
                m_Lines[depth].pz1[leg] = p->Z;
                m_Lines[depth].x0[leg] = x1;
                m_Lines[depth].y0[leg] = y1;
                m_Lines[depth].x1[leg] = toscreen_x(p->X, p->Y, p->Z);
                m_Lines[depth].y1[leg] = toscreen_y(p->X, p->Y, p->Z);
     
                m_Lines[depth].x0_orig[leg] = m_Lines[depth].x0[leg];
                m_Lines[depth].x1_orig[leg] = m_Lines[depth].x1[leg];
                m_Lines[depth].y0_orig[leg] = m_Lines[depth].y0[leg];
                m_Lines[depth].y1_orig[leg] = m_Lines[depth].y1[leg];
                m_Lines[depth].z0_orig[leg] = m_Lines[depth].z0[leg];
                m_Lines[depth].z1_orig[leg] = m_Lines[depth].z1[leg];
                    
                m_Lines[depth].ignore[leg] = 
                    (m_Lines[depth].x0[leg] == m_Lines[depth].x1[leg]) &&
                    (m_Lines[depth].y0[leg] == m_Lines[depth].y1[leg]);
                    
                x1 = toscreen_x(p->X, p->Y, p->Z);
                y1 = toscreen_y(p->X, p->Y, p->Z);

                _px1 = p->X; _py1 = p->Y; _pz1 = p->Z;
                m_NumLines[depth]++;

                break;
            }
            }
        }
        
        ptr = ptr->next;
    }

    set_usize(WIDTH, HEIGHT);
    set_wmclass("Aven", "Aven");
    string title = string("Aven - [") + name + string("]");
    set_title(title.c_str());

    add(m_Cave);

    // Register event handlers.
    m_Cave.expose_event.connect(slot(this, &Aven::ExposeEvent));
    m_Cave.button_press_event.connect(slot(this, &Aven::ButtonPressEvent));
    m_Cave.button_release_event.connect(slot(this, &Aven::ButtonReleaseEvent));
    m_Cave.motion_notify_event.connect(slot(this, &Aven::PointerMotionEvent));
    m_Cave.key_press_event.connect(slot(this, &Aven::KeyPressEvent));

    AllocColour(&red, 1.0, 0.0, 0.0);
    AllocColour(&black, 0.0, 0.0, 0.0); 

    float reds[]   = {190, 155, 110, 18, 0, 124, 48, 117, 163, 182, 224, 237, 255};
    float greens[] = {218, 205, 177, 153, 178, 211, 219, 224, 224, 193, 190, 117, 0};
    float blues[]  = {247, 255, 244, 237, 169, 175, 139, 40, 40, 17, 40, 18, 0};

    for (int c = 0; c < NCOLS; c++) AllocColour(&cols[c], reds[c] / 255.0, greens[c] / 255.0,
						blues[c] / 255.0);

    m_Cave.set_events(m_Cave.get_events() | GDK_BUTTON_PRESS_MASK |
              GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
	      GDK_EXPOSURE_MASK);
    set_events(get_events() | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_EXPOSURE_MASK);



    EnableWorkProc();
}

Aven::~Aven()
{
    delete[] m_RawLegs;
    delete[] m_RawStns;
}

gint delete_event_impl()
{
    exit(0);

    return 1;
}

void Aven::EnableWorkProc()
{
    if (!m_WorkProcEnabled) {
        // Register the periodic timeout handler.
        m_WorkProc = Gtk::Main::timeout.connect(slot(this, &Aven::Idle), 100);

        m_WorkProcEnabled = true;
    }
}

void Aven::Open(void*)
{

}
   
void Aven::DisableWorkProc()
{
    if (m_WorkProcEnabled) {
        // Deregister the periodic timeout handler.
        m_WorkProc.disconnect();
        m_WorkProcEnabled = false;
    }
}

double Aven::RotationAngleFromXOffset(int x)
{
    return ((x / (SCREEN_WIDTH * 2)) * 2.0 * PI) - PI;
}

void Aven::AllocColour(GdkColor* colour,
                           float r, float g, float b /* components all 0.0 - 1.0 */)
{
    // Allocate a colour specified as RGB.
 
    colour->red = short(r * 65535);
    colour->green = short(g * 65535);
    colour->blue = short(b * 65535);
    gdk_colormap_alloc_color(gdk_colormap_get_system(), colour, false, true);
}

bool Aven::Contributes(int x0, int y0, int x1, int y1,
                           int clip_x0, int clip_y0, int clip_x1, int clip_y1)
{
    // Simplified Cohen-Sutherland clipper (see a computer graphics textbook!)

    int c0 = (y0 > clip_y1) + ((y0 < clip_y0) << 1) + ((x0 > clip_x1) << 2) +
             ((x0 < clip_x0) << 3);
    int c1 = (y1 > clip_y1) + ((y1 < clip_y0) << 1) + ((x1 > clip_x1) << 2) +
             ((x1 < clip_x0) << 3);
    
    return (c0 == c1 == 0) || ((c0 & c1) == 0);
}

void Aven::Draw(GdkPixmap* window, GdkGC* gc, int x0, int y0, int x1, int y1,
		float scale, int bx0, int by0, int bx1, int by1,
		bool translate, coord xshift, coord yshift, coord zshift)
{
    bool scale_and_rotate = (scale != m_Scale2 || m_ForceUseOrig);
  
/*    if (scale_and_rotate) {
        char buf[30];
        sprintf(buf, " Heading: %03ddeg", int(m_Angle * 180.0 / PI));
        m_HeadingLabel.set_text(buf);
    }
*/
    GtkWidget* widget = GTK_WIDGET(m_Cave.gtkobj());
    gdk_window_get_size(widget->window, &m_XSize, &m_YSize);
    int halfxs = m_XSize / 2, halfys = m_YSize / 2;

    gdk_gc_set_foreground(gc, &black);
    gdk_draw_rectangle(window, gc, true, x0, y0, x1 - x0, y1 - y0);
    if (bx0 >= 0) {
        gdk_draw_rectangle(window, gc, true, bx0, by0, bx1 - bx0, by1 - by0);
    }
    
    static GdkSegment* segs = new GdkSegment[10000 /* UGH! */];
    static int num_to_draw;
    static double cx0, cx1, cy0, cy1, cz0, cz1;
    static GdkSegment* cur_seg;

    for (int depth = 0; depth < NCOLS; depth++) {
        num_to_draw = 0;
        gdk_gc_set_foreground(gc, &cols[depth]);
    
        for (int i = 0; i < m_NumLines[depth]; i++) { // THE speed-critical loop!
            if (translate) {
	      //   printf("old coord x's: %d/%d\n", m_Lines[depth].px0[i], m_Lines[depth].px1[i]);

  	        m_Lines[depth].px0[i] += xshift;
  	        m_Lines[depth].px1[i] += xshift;
  	        m_Lines[depth].py0[i] += yshift;
  	        m_Lines[depth].py1[i] += yshift;
  	        m_Lines[depth].pz0[i] += zshift;
  	        m_Lines[depth].pz1[i] += zshift;
		//      printf("new coord x's: %d/%d\n", m_Lines[depth].px0[i], m_Lines[depth].px1[i]);
		scale_and_rotate = true; // next "if" gets executed now
	    }

            if (scale_and_rotate) {
                m_ForceUseOrig = false;
//       printf("requested %f, override %f\n",scale,m_Scale); 
                m_Scale = scale;
                m_Lines[depth].x0[i] = toscreen_x(m_Lines[depth].px0[i], m_Lines[depth].py0[i], m_Lines[depth].pz0[i], scale);
                m_Lines[depth].y0[i] = toscreen_y(m_Lines[depth].px0[i], m_Lines[depth].py0[i], m_Lines[depth].pz0[i], scale);
                m_Lines[depth].x1[i] = toscreen_x(m_Lines[depth].px1[i], m_Lines[depth].py1[i], m_Lines[depth].pz1[i], scale);
                m_Lines[depth].y1[i] = toscreen_y(m_Lines[depth].px1[i], m_Lines[depth].py1[i], m_Lines[depth].pz1[i], scale);
        
                m_Lines[depth].ignore[i] = (m_Lines[depth].x0[i] == m_Lines[depth].x1[i]) &&
                                           (m_Lines[depth].y0[i] == m_Lines[depth].y1[i]);
            }
            
            if (m_Lines[depth].ignore[i]) {
                continue;
            }
    
            cur_seg = &segs[num_to_draw];
        
            cur_seg->x1 = m_Lines[depth].x0[i] + halfxs;
            cur_seg->x2 = m_Lines[depth].x1[i] + halfxs;
            cur_seg->y1 = m_Lines[depth].y0[i] + halfys;
            cur_seg->y2 = m_Lines[depth].y1[i] + halfys;
                
            if (Contributes(cur_seg->x1, cur_seg->y1, cur_seg->x2, cur_seg->y2,
                            x0, y0, x1, y1) ||
                (bx0 >= 0 && Contributes(cur_seg->x1, cur_seg->y1, 
                                         cur_seg->x2, cur_seg->y2, bx0, by0, bx1, by1))) {
                num_to_draw++;
            }
        }
    
        gdk_draw_segments(window, gc, segs, num_to_draw);
    }

    if (window != m_CaveWin) {
        // Swap buffers.
        XdbeSwapInfo info;
        info.swap_window = GDK_WINDOW_XWINDOW(m_CaveWin);
        info.swap_action = XdbeUndefined;
        XdbeSwapBuffers(GDK_WINDOW_XDISPLAY(m_CaveWin), &info, 1);
    }
}

gint Aven::ExposeEvent(GdkEventExpose* event)
{
    GtkWidget* widget = GTK_WIDGET(m_Cave.gtkobj());

    if (!m_CaveWin) { // first expose event...
        m_CaveWin = widget->window;

	// Create back buffer for double-buffering.
        m_BackBuf = XdbeAllocateBackBufferName(GDK_WINDOW_XDISPLAY(m_CaveWin),
					       GDK_WINDOW_XWINDOW(m_CaveWin),
					       XdbeBackground);
	assert(m_BackBuf);

	m_DrawPixmap = gdk_pixmap_foreign_new((guint32) m_BackBuf);
	assert(m_DrawPixmap);

        m_LineGC = gdk_gc_new(m_DrawPixmap);
    }
 
    // Establish clipping regions.
    //    gdk_window_get_size(widget->window, &m_XSize, &m_YSize);

    // Establish clipping regions.
    GdkRectangle rect;
    rect.x = rect.y = 0;
    rect.width = m_XSize;
    rect.height = m_YSize;
    gdk_gc_set_clip_rectangle(m_LineGC, &rect);
    gdk_gc_set_clip_origin(m_LineGC, 0, 0);
    int x1 = event->area.x + event->area.width;
    int y1 = event->area.y + event->area.height;

    // Draw.
    Draw(m_DrawPixmap, m_LineGC, event->area.x, event->area.y, x1, y1, m_Scale2,
         -1, -1, -1, -1);

    return false;
}

gint Aven::ButtonPressEvent(GdkEventButton* event)
{
    if (event->button == 3) {
        m_Moving = true;
    }
    else if (event->button == 2) {
        m_AngleOrig = elev_angle;
        m_Eleving = true;
    }
    else {
        m_Scaling = true;
        m_LastFactor = m_Scale;

        m_XSize = m_Cave.width();
        m_YSize = m_Cave.height();

        m_AngleOrig = m_Angle;
        
        m_RotationTrigOffset = -int(event->x) + SCREEN_WIDTH;
    }

    m_WorkProcStateBeforeDrag = m_WorkProcEnabled;
    DisableWorkProc();
    
    m_StartX = m_LastX = int(event->x);
    m_StartY = m_LastY = int(event->y);    

    GtkWidget* widget = GTK_WIDGET(m_Cave.gtkobj());
    gdk_window_get_size(widget->window, &m_XSize, &m_YSize);

    // Establish clipping regions.
    GdkRectangle rect;
    rect.x = rect.y = 0;
    rect.width = m_XSize;
    rect.height = m_YSize;
    gdk_gc_set_clip_rectangle(m_LineGC, &rect);
    gdk_gc_set_clip_origin(m_LineGC, 0, 0);

    return false;
}

gint Aven::Idle()
{
    // Perform automatic rotation of the display.

    assert (m_Rotating || m_SwitchToPlan || m_SwitchToElevation || m_Splash);
    assert (!m_Scaling && !m_Eleving && !m_Moving);

    if (!m_CaveWin) return 1;

    if (m_Splash) {
        elev_angle = 90.0 - (m_SplashStep * (60.0 / SPLASH_STEPS));
	m_Angle = 1.5*PI - (m_SplashStep * 1.5*PI / SPLASH_STEPS);
	m_Scale *= 1.5;
        sx = sin(m_Angle);
        cx = cos(m_Angle);
	fz = cos(elev_angle*PIBY180);
        // other trig terms get updated below...
	if (m_SplashStep++ == SPLASH_STEPS) {
  	    m_Splash = false;
	    DisableWorkProc();
	}
    }

    // Handle switching to plan or elevation.
    if (m_SwitchToPlan) {
        elev_angle += 5.0;

        if (elev_angle > 90.0) {
            elev_angle = 90.0;
        }

        fz = cos(elev_angle*PIBY180);
        // other trig terms get updated below...

        if (elev_angle == 90.0) {
            m_SwitchToPlan = false;
            if (!m_Rotating) {
		DisableWorkProc();
	    }
        }
    }
    else if (m_SwitchToElevation) {
        elev_angle -= 5.0;
        if (elev_angle < 0.0) {
            elev_angle = 0.0;
        }

        fz = cos(elev_angle*PIBY180);
        // other trig terms get updated below...

        if (elev_angle == 0.0) {
            m_SwitchToElevation = false;
	    if (!m_Rotating) {
		DisableWorkProc();
	    }
        }
    }

    if (m_Rotating) {
        // Calculate rotation.
        m_Angle += m_RotateAngle;
        while (m_Angle >= (2*PI)) m_Angle -= (2*PI);
        while (m_Angle < 0.0) m_Angle += 2*PI;
        sx = sin(m_Angle);
        cx = cos(m_Angle);
    }

    fx = sin(elev_angle*PIBY180) * sx;
    fy = sin(elev_angle*PIBY180) * -cx;

    m_ForceUseOrig = true;
    Draw(m_DrawPixmap, m_LineGC, 0, 0, m_XSize, m_YSize, m_Scale,
         -1, -1, -1, -1);

    // Swap buffers.
    XdbeSwapInfo info;
    info.swap_window = GDK_WINDOW_XWINDOW(m_CaveWin);
    info.swap_action = XdbeUndefined;
    XdbeSwapBuffers(GDK_WINDOW_XDISPLAY(m_CaveWin), &info, 1);

    return 1;
}

gint Aven::PointerMotionEvent(GdkEventMotion* event)
{
    if (!m_Moving && !m_Scaling && !m_Eleving) return true;

#ifndef NASTY_SPEED_HACK
    static guint32 min_time_accepted = 0;
    guint32 tm = gdk_event_get_time((GdkEvent*) event);

    if (tm <= min_time_accepted) {
        return false;
    }
#endif

    int mask2;

    if (m_Moving) {
        mask2 = GDK_BUTTON3_MASK;
    
        int dx = -int((int(event->x) - m_LastX));
        int dy = -int((int(event->y) - m_LastY));

	float dxp = (float(dx)*cx) / m_Scale;
	float dyp = (float(dx)*sx) / m_Scale;
	float dzp = float(dy) / m_Scale;

	coord xshift = coord(dxp);
	coord yshift = coord(dyp);
	coord zshift = coord(dzp);
        
        m_LastX = int(event->x);
        m_LastY = int(event->y);

	Draw(m_DrawPixmap, m_LineGC, 0, 0, m_XSize, m_YSize, m_Scale,
	     -1, -1, -1, -1, true, xshift, yshift, zshift);
    }
    else if (m_Eleving) {
        mask2 = GDK_BUTTON2_MASK;

        // Calculate rotation.
        elev_angle = m_AngleOrig + (180.0 * float(event->y - m_StartY) / 500.0);

        sx = sin(m_Angle);
        cx = cos(m_Angle);
        fz = cos(elev_angle*PIBY180);
        fx = sin(elev_angle*PIBY180) * sx;
        fy = sin(elev_angle*PIBY180) * -cx;

        m_ForceUseOrig = true;

        if (elev_angle != m_AngleOrig) {
            Draw(m_DrawPixmap, m_LineGC, 0, 0, m_XSize, m_YSize, m_Scale2,
                 -1, -1, -1, -1);
        }
    }
    else if (m_Scaling) {
        mask2 = GDK_BUTTON1_MASK;

        float dy = float(event->y - m_StartY) / 2.0;
        
        float old_factor = factor;
        float old_angle = m_Angle;
        
        factor = (float) pow(LITTLE_MAGNIFY_FACTOR, -0.08 * dy);

        // Calculate rotation.
        m_Angle = m_AngleOrig + (PI * float(event->x - m_StartX) / 512.0);

        while (m_Angle < 0.0) m_Angle += 2*PI;
        while (m_Angle >= 2*PI) m_Angle -= 2*PI;

        sx = sin(m_Angle);
        cx = cos(m_Angle);
        fz = cos(elev_angle*PIBY180);
        fx = sin(elev_angle*PIBY180) * sx;
        fy = sin(elev_angle*PIBY180) * -cx;

        if (factor != old_factor || m_Angle != old_angle) {
            Draw(m_DrawPixmap, m_LineGC, 0, 0, m_XSize, m_YSize, factor * m_LastFactor,
                 -1, -1, -1, -1);
        }
    }

#ifdef NASTY_SPEED_HACK
    // GDK is useless at efficient event handling.  Thus we bypass it to flush out unnecessary
    // pointer motion events received during drawing.
    // This may break with future releases of GDK.

    Display* dpy = GDK_WINDOW_XDISPLAY(m_CaveWin);

    // Discard all events.
    XSync(dpy, True);

    // Sometimes button release events seem to get lost after poking the X event queue...
    static double mx, my, pressure, xtilt, ytilt;
    static GdkModifierType mask;
    gdk_input_window_get_pointer(m_CaveWin, event->deviceid, &mx, &my, &pressure,
                                 &xtilt, &ytilt, &mask);
    if (!(mask & mask2)) {
        ButtonReleaseEvent(NULL);
    }
#else
    gettimeofday(&tv2, NULL);

    int ms1 = (tv1.tv_sec * 1000) + int(float(tv1.tv_usec) / 1000.0);
    int ms2 = (tv2.tv_sec * 1000) + int(float(tv2.tv_usec) / 1000.0);
    
    min_time_accepted = tm + (ms2 - ms1) + 1;
#endif

    return false;
}

gint Aven::ButtonReleaseEvent(GdkEventButton* event)
{
    if (m_Scaling) {
        m_LastFactor *= factor;
        m_Scale2 = m_LastFactor;
    }
    
    m_Moving = m_Eleving = m_Scaling = false;

    if (m_WorkProcStateBeforeDrag) {
        EnableWorkProc();
    }

    return false;
}

void Aven::Rotate()
{
    if (!m_Rotating) {
	EnableWorkProc();
	m_Rotating = true;
    }
}

void Aven::Stop()
{
    if (m_Rotating) {
	DisableWorkProc();
	m_Rotating = false;
    }
}

void Aven::Reverse()
{
    m_RotateAngle = -m_RotateAngle;
}

void Aven::Plan()
{
    m_SwitchToPlan = true;
    m_SwitchToElevation = false;
    EnableWorkProc();
}

void Aven::Elevation()
{
    m_SwitchToPlan = false;
    m_SwitchToElevation = true;
    EnableWorkProc();
}

void Aven::Slower()
{
    if (fabs(m_RotateAngle) > (PI / 180.0)) {
	if (m_RotateAngle > 0) {
	   m_RotateAngle -= (PI / 360.0);
	} else {
	   m_RotateAngle += (PI / 360.0);
	}
    }
}

void Aven::Faster()
{
    if (fabs(m_RotateAngle) < (10 * PI / 180.0)) {
	if (m_RotateAngle > 0) {
	   m_RotateAngle += (PI / 360.0);
	} else {
	   m_RotateAngle -= (PI / 360.0);
	}
    }
}

gint Aven::KeyPressEvent(GdkEventKey* event)
{
    // Process a key press.

    switch (toupper(event->string[0])) {
        case key_ROTATION_1:
        case key_ROTATION_2:
            m_Rotating = !m_Rotating;
            if (m_Rotating) {
                EnableWorkProc();
            }
            else {
                DisableWorkProc();
            }
            break;

        case key_REVERSE_ROTATION:
            Reverse();
            break;

        case key_SPEED_UP:
            Faster();
            break;

        case key_SLOW_DOWN:
            Slower();
            break;

        case key_PLAN:
            Plan();
            break;

        case key_ELEVATION:
            Elevation();
            break;
    }

    return true;
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "syntax: %s <.3d file>\n", argv[0]);
        return 1;
    }
    
    set_codes(MOVE, DRAW, STOP);
   
    Gtk::Main m(argc, argv);
    
    gdk_set_use_xshm(true);
    
    Aven* win = new Aven(string(argv[1]));
    win->show_all();
    m.run();
}
