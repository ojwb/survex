//
//  gla-gl.cc
//
//  OpenGL implementation for the GLA abstraction layer.
//
//  Copyright (C) 2002-2003 Mark R. Shinwell
//  Copyright (C) 2003,2004 Olly Betts
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
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <wx/confbase.h>

#include <algorithm>

#include "gla.h"
#include "message.h"

#ifndef USE_FNT
// Some WIN32 stupidity which causes mingw to fail to link for some reason;
// doing this probably means we can't safely use atexit() - see the comments in
// glut.h if you can take the full horror...
#define GLUT_DISABLE_ATEXIT_HACK
// For glutBitmapLength()
#define GLUT_API_VERSION 4
#include <GL/glut.h>
#ifdef FREEGLUT
#include <GL/freeglut_ext.h>
#endif
#endif

using namespace std;

#ifdef GLA_DEBUG
// Important: CHECK_GL_ERROR must not be called within a glBegin()/glEnd() pair
//            (thus it must not be called from BeginLines(), etc., or within a
//             BeginLines()/EndLines() block etc.)
#define CHECK_GL_ERROR(f, m) \
    do { \
        GLenum err = glGetError(); \
        if (err != GL_NO_ERROR) { \
            fprintf(stderr, "OpenGL error: %s: call %s in function %s\n", \
                    gluErrorString(err), m, f); \
            /* abort();*/ \
        } \
    } \
    while (0)
#else
#define CHECK_GL_ERROR(f, m) do {} while (0)
#endif

//
//  GLAPen
//  

GLAPen::GLAPen()
{
    components[0] = components[1] = components[2] = 0.0;
}

GLAPen::~GLAPen()
{
}

void GLAPen::SetColour(double red, double green, double blue)
{
    components[0] = red;
    components[1] = green;
    components[2] = blue;
}

double GLAPen::GetRed() const
{
    return components[0];
}

double GLAPen::GetGreen() const
{
    return components[1];
}

double GLAPen::GetBlue() const
{
    return components[2];
}

void GLAPen::Interpolate(const GLAPen& pen, double how_far)
{
    components[0] = how_far * (pen.GetRed() - components[0]) + components[0];
    components[1] = how_far * (pen.GetGreen() - components[1]) + components[1];
    components[2] = how_far * (pen.GetBlue() - components[2]) + components[2];
}

struct ColourTriple {
    // RGB triple: values are from 0-255 inclusive for each component.
    unsigned char r, g, b;
};

// These must be in the same order as the entries in COLOURS[] below.
const ColourTriple COLOURS[] = {
    { 0, 0, 0 },       // black
    { 100, 100, 100 }, // grey
    { 180, 180, 180 }, // light grey
    { 140, 140, 140 }, // light grey 2
    { 90, 90, 90 },    // dark grey
    { 255, 255, 255 }, // white
    { 0, 100, 255},    // turquoise
    { 0, 255, 40 },    // green
    { 150, 205, 224 }, // indicator 1
    { 114, 149, 160 }, // indicator 2
    { 255, 255, 0 },   // yellow
    { 255, 0, 0 },     // red
};

//
//  GLACanvas
//  

#ifndef USE_FNT
void* const GLACanvas::m_Font = GLUT_BITMAP_HELVETICA_10;
const int GLACanvas::m_FontSize = 10;
#endif

GLACanvas::GLACanvas(wxWindow* parent, int id, const wxPoint& posn, wxSize size) :
    wxGLCanvas(parent, id, posn, size)
{
    // Constructor.

    m_Quadric = NULL;
    m_Rotation.setFromSphericalPolars(0.0, 0.0, 0.0);
    m_Scale = 0.0;
    m_Translation.x = m_Translation.y = m_Translation.z = 0.0;
    m_VolumeDiameter = 1.0;
    m_Perspective = false;
}

GLACanvas::~GLACanvas()
{
    // Destructor.

    if (m_Quadric) {
        gluDeleteQuadric(m_Quadric);
        CHECK_GL_ERROR("~GLACanvas", "gluDeleteQuadric");
    }
}

void GLACanvas::FirstShow()
{
    SetCurrent();
    m_Quadric = gluNewQuadric();
    CHECK_GL_ERROR("FirstShow", "gluNewQuadric");
    if (!m_Quadric) {
	abort(); // FIXME need to cope somehow
    }
 
    glShadeModel(GL_FLAT);
    CHECK_GL_ERROR("FirstShow", "glShadeModel");
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    CHECK_GL_ERROR("FirstShow", "glPolygonMode");
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    CHECK_GL_ERROR("FirstShow", "glColorMaterial GL_FRONT");
    glColorMaterial(GL_BACK, GL_AMBIENT_AND_DIFFUSE);
    CHECK_GL_ERROR("FirstShow", "glColorMaterial GL_BACK");
    //glEnable(GL_POINT_SMOOTH);
    //CHECK_GL_ERROR("FirstShow", "glEnable GL_POINT_SMOOTH");
    glPointSize(4);
    CHECK_GL_ERROR("FirstShow", "glPointSize");
    //glAlphaFunc(GL_GREATER, 0.5f);
    //CHECK_GL_ERROR("FirstShow", "glAlphaFunc");

#ifdef USE_FNT
    // Load font
    wxString path(msg_cfgpth());
    path += wxCONFIG_PATH_SEPARATOR;
    path += "aven.txf";
    m_Font.load(path.c_str());
#endif
}

void GLACanvas::Clear()
{
    // Clear the canvas.

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CHECK_GL_ERROR("Clear", "glClear");
}

void GLACanvas::SetRotation(Quaternion& q)
{
    m_Rotation = q;
}

void GLACanvas::SetScale(Double scale)
{
    m_Scale = scale;
}

void GLACanvas::SetTranslation(Double x, Double y, Double z)
{
    m_Translation.x = x;
    m_Translation.y = y;
    m_Translation.z = z;
}

void GLACanvas::AddTranslation(Double x, Double y, Double z)
{
    m_Translation.x += x;
    m_Translation.y += y;
    m_Translation.z += z;
}

void GLACanvas::AddTranslationScreenCoordinates(int dx, int dy)
{
    // Translate the data by a given amount, specified in screen coordinates.
    
    // Find out how far the translation takes us in data coordinates.
    SetViewportAndProjection();
    SetDataTransform();

    Double x0, y0, z0;
    Double x, y, z;
    gluUnProject(0.0, 0.0, 0.0, modelview_matrix, projection_matrix, viewport,
                 &x0, &y0, &z0);
    CHECK_GL_ERROR("AddTranslationScreenCoordinates", "gluUnProject");
    gluUnProject(dx, -dy, 0.0, modelview_matrix, projection_matrix, viewport,
                 &x, &y, &z);
    CHECK_GL_ERROR("AddTranslationScreenCoordinates", "gluUnProject (2)");

    // Apply the translation.
    AddTranslation(x - x0, y - y0, z - z0);
}

void GLACanvas::SetVolumeDiameter(glaCoord diameter)
{
    // Set the size of the data drawing volume by giving the diameter of the
    // smallest sphere containing it.

    m_VolumeDiameter = diameter;
}

Double GLACanvas::SetViewportAndProjection()
{
    // Set viewport.  The width and height go to zero when the panel is dragged
    // right across so we clamp them to be at least 1 to avoid errors from the
    // opengl calls below.
    wxSize size = GetSize();
    int window_width = max(size.GetWidth(), 1);
    int window_height = max(size.GetHeight(), 1);
    double aspect = double(window_height) / double(window_width);

    glViewport(0, 0, window_width, window_height);
    CHECK_GL_ERROR("SetViewportAndProjection", "glViewport");

    // Set projection.
    glMatrixMode(GL_PROJECTION);
    CHECK_GL_ERROR("SetViewportAndProjection", "glMatrixMode");
    glLoadIdentity();
    CHECK_GL_ERROR("SetViewportAndProjection", "glLoadIdentity");

    assert(m_Scale != 0.0);
#ifndef FLYFREE
    Double lr = m_VolumeDiameter / m_Scale * 0.5;
    Double near_plane = lr / tan(25.0 * M_PI / 180.0);
    Double far_plane = m_VolumeDiameter + near_plane;
#else
    Double near_plane = 0.5;
    Double lr = near_plane * tan(25.0 * M_PI / 180.0);
    Double far_plane = m_VolumeDiameter * 5 + near_plane; // FIXME: work out properly
#endif
    Double tb = lr * aspect;
    if (m_Perspective) {
	glFrustum(-lr, lr, -tb, tb, near_plane, far_plane);
	CHECK_GL_ERROR("SetViewportAndProjection", "glFrustum");
    } else {
	glOrtho(-lr, lr, -tb, tb, near_plane, far_plane);
	CHECK_GL_ERROR("SetViewportAndProjection", "glOrtho");
    }

    glEnable(GL_DEPTH_TEST);
    CHECK_GL_ERROR("SetViewportAndProjection", "glEnable GL_DEPTH_TEST");

    // Save projection info.
    glGetDoublev(GL_PROJECTION_MATRIX, projection_matrix);
    CHECK_GL_ERROR("SetViewportAndProjection", "glGetDoublev");

    // Save viewport info.
    glGetIntegerv(GL_VIEWPORT, viewport);
    CHECK_GL_ERROR("SetViewportAndProjection", "glGetIntegerv");

    return near_plane;
}

void GLACanvas::StartDrawing()
{
    // Prepare for a redraw operation.
 
    SetCurrent();
    glDepthMask(true);
}

void GLACanvas::EnableSmoothPolygons()
{
    // Prepare for drawing smoothly-shaded polygons.
    // Only use this when required (in particular lines in lists may not be
    // coloured correctly when this is enabled).
    
    glShadeModel(GL_SMOOTH);
    //glEnable(GL_COLOR_MATERIAL);
 
    //GLfloat diffuseMaterial[] = { 0.3, 0.3, 0.3, 1.0 };
    //GLfloat mat_specular[] = { 0.5, 0.5, 0.5, 1.0 };
    //wxSize size = GetSize();
    //double aspect = double(size.GetWidth()) / double(size.GetHeight());
#if 0
    GLfloat light_position[] = { m_VolumeDiameter * 0.5 - 5.0,
                                 m_VolumeDiameter * 0.5 / aspect - 5.0,
                                 m_VolumeDiameter * 0.5 + 5.0,
                                 0.0 };
#endif
    
  //  glMaterialfv(GL_FRONT, GL_AMBIENT, diffuseMaterial);
//    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
//    glMaterialf(GL_FRONT, GL_SHININESS, 10.0);
/*
    GLfloat light_ambient[] = { 1.0, 1.0, 1.0, 1.0 };
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    //glColorMaterial(GL_BACK, GL_AMBIENT);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);*/
}

void GLACanvas::DisableSmoothPolygons()
{
    glShadeModel(GL_FLAT);
  //  glDisable(GL_LIGHT0);
//    glDisable(GL_LIGHTING);
//    glDisable(GL_COLOR_MATERIAL);
}

void GLACanvas::PlaceNormal(glaCoord x, glaCoord y, glaCoord z)
{
    // Add a normal (for polygons etc.)

    glNormal3d(x, y, z);
}

void GLACanvas::SetDataTransform()
{
    // Set the modelview transform for drawing data.
    
    Double near_plane = SetViewportAndProjection();
    glMatrixMode(GL_MODELVIEW);
    CHECK_GL_ERROR("SetDataTransform", "glMatrixMode");
    glLoadIdentity();
    CHECK_GL_ERROR("SetDataTransform", "glLoadIdentity");
#ifndef FLYFREE
    glTranslated(0.0, 0.0, -0.5 * m_VolumeDiameter - near_plane);
#else
    glTranslated(0.0, 0.0, - near_plane);
#endif
    CHECK_GL_ERROR("SetDataTransform", "glTranslated");
    // Get axes the correct way around (z upwards, y into screen)
    glRotated(-90.0, 1.0, 0.0, 0.0);
    CHECK_GL_ERROR("SetDataTransform", "glRotated");
    m_Rotation.CopyToOpenGL();
    CHECK_GL_ERROR("SetDataTransform", "CopyToOpenGL");
    glTranslated(m_Translation.x, m_Translation.y, m_Translation.z);
    CHECK_GL_ERROR("SetDataTransform", "glTranslated");

    // Save matrices.
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview_matrix);
    CHECK_GL_ERROR("SetDataTransform", "glGetDoublev");
}

void GLACanvas::SetIndicatorTransform()
{
    // Set the modelview transform and projection for drawing indicators.
    wxSize size = GetSize();
    int window_width = max(size.GetWidth(), 1);
    int window_height = max(size.GetHeight(), 1);

    // No modelview transform
    glMatrixMode(GL_MODELVIEW);
    CHECK_GL_ERROR("SetIndicatorTransform", "glMatrixMode");
    glLoadIdentity();
    CHECK_GL_ERROR("SetIndicatorTransform", "glLoadIdentity");

    glDisable(GL_DEPTH_TEST);
    CHECK_GL_ERROR("SetIndicatorTransform", "glDisable GL_DEPTH_TEST");

    // And just a simple 2D projection
    glMatrixMode(GL_PROJECTION);
    CHECK_GL_ERROR("SetIndicatorTransform", "glMatrixMode");
    glLoadIdentity();
    CHECK_GL_ERROR("SetIndicatorTransform", "glLoadIdentity (2)");
    gluOrtho2D(0, window_width, 0, window_height);
    CHECK_GL_ERROR("SetIndicatorTransform", "gluOrtho2D");
}

void GLACanvas::FinishDrawing()
{
    // Complete a redraw operation.
    
//    glFlush();
//    CHECK_GL_ERROR("FinishDrawing", "glFlush");
    SwapBuffers();
}

void GLACanvas::SetQuaternion(Quaternion& q)
{
    // Set the quaternion used for the modelling transformation.
    
    m_Rotation = q;
}

glaList GLACanvas::CreateList(GfxCore* obj, void (GfxCore::*generator)())
{
    // Create a new list to hold a sequence of drawing operations, and compile
    // it.

    glaList l = glGenLists(1);
//    printf("new list: %d... ", l);
    CHECK_GL_ERROR("CreateList", "glGenLists");
    assert(l != 0);
    
    glNewList(l, GL_COMPILE);
    CHECK_GL_ERROR("CreateList", "glNewList");
#ifdef GLA_DEBUG
    m_Vertices = 0;
#endif
    (obj->*generator)();
    glEndList();
#ifdef GLA_DEBUG
    //printf("done (%d vertices)\n", m_Vertices);
#endif
    CHECK_GL_ERROR("CreateList", "glEndList");

    return l;
}

void GLACanvas::DeleteList(glaList l)
{
    // Delete an existing list.

    assert(l != 0);
    SetCurrent();
    glDeleteLists(l, 1);
    CHECK_GL_ERROR("DeleteList", "glDeleteLists");
}

void GLACanvas::SetBackgroundColour(float red, float green, float blue)
{
    // Set the background colour of the canvas.
    
    glClearColor(red, green, blue, 1.0);
    CHECK_GL_ERROR("SetBackgroundColour", "glClearColor");
}

void GLACanvas::SetColour(const GLAPen& pen, double rgb_scale)
{
    // Set the colour for subsequent operations.
    glColor3f(pen.GetRed() * rgb_scale, pen.GetGreen() * rgb_scale,
	      pen.GetBlue() * rgb_scale);
}

void GLACanvas::SetColour(const GLAPen& pen)
{
    // Set the colour for subsequent operations.
    glColor3dv(pen.components);
}

void GLACanvas::SetColour(gla_colour colour)
{
    // Set the colour for subsequent operations.
    glColor3ubv(&COLOURS[colour].r);
}

void GLACanvas::DrawText(glaCoord x, glaCoord y, glaCoord z, const wxString& str)
{
    // Draw a text string on the current buffer in the current font.
#ifdef USE_FNT
    GLdouble X, Y, Z;
    if (!gluProject(x, y, z, modelview_matrix, projection_matrix, viewport,
		    &X, &Y, &Z)) return;
    DrawIndicatorText((int)X, (int)Y, str);
#else
    glRasterPos3d(x, y, z);
    CHECK_GL_ERROR("DrawText", "glRasterPos3d");

#ifdef FREEGLUT
    glutBitmapString(m_Font, (const unsigned char *)str.c_str());
    CHECK_GL_ERROR("DrawText", "glutBitmapString");
#else
    for (size_t pos = 0; pos < str.Length(); pos++) {
        glutBitmapCharacter(m_Font, int((unsigned char)str[pos]));
        CHECK_GL_ERROR("DrawText", "glutBitmapCharacter");
    }
#endif
#endif
}

void GLACanvas::DrawIndicatorText(int x, int y, const wxString& str)
{
#ifdef USE_FNT
    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    m_Font.puts(x, y, str.c_str());
    glPopAttrib();
#else
    DrawText(x, y, 0.0, str);
#endif
}

void GLACanvas::GetTextExtent(const wxString& str, int * x_ext, int * y_ext)
{
    assert(x_ext);
    assert(y_ext);

#ifdef USE_FNT
    m_Font.getTextExtent(str.c_str(), x_ext, y_ext);
#else
#if 1
    *x_ext = glutBitmapLength(m_Font, (const unsigned char *)str.c_str());
    CHECK_GL_ERROR("GetTextExtent", "glutBitmapLength");
#else
    *x_ext = 0;
    for (size_t pos = 0; pos < str.Length(); pos++) {
        x_ext += glutBitmapWidth(m_Font, int((unsigned char)str[pos]));
        CHECK_GL_ERROR("DrawText", "glutBitmapWidth");
    }
#endif
    *y_ext = m_FontSize + 2;
#endif
}

void GLACanvas::BeginQuadrilaterals()
{
    // Commence drawing of quadrilaterals.
    
    glBegin(GL_QUADS);
}

void GLACanvas::EndQuadrilaterals()
{
    // Finish drawing of quadrilaterals.
    
    glEnd();
}

void GLACanvas::BeginLines()
{
    // Commence drawing of a set of lines.

    glBegin(GL_LINES);
}

void GLACanvas::EndLines()
{
    // Finish drawing of a set of lines.

    glEnd();
}

void GLACanvas::BeginTriangles()
{
    // Commence drawing of a set of triangles.

    glBegin(GL_TRIANGLES);
}

void GLACanvas::EndTriangles()
{
    // Finish drawing of a set of triangles.

    glEnd();
}

void GLACanvas::BeginTriangleStrip()
{
    // Commence drawing of a triangle strip.

    glBegin(GL_TRIANGLE_STRIP);
}

void GLACanvas::EndTriangleStrip()
{
    // Finish drawing of a triangle strip.

    glEnd();
}

void GLACanvas::BeginPolyline()
{
    // Commence drawing of a polyline.

    glBegin(GL_LINE_STRIP);
}

void GLACanvas::EndPolyline()
{
    // Finish drawing of a polyline.
    
    glEnd();
}

void GLACanvas::BeginPolygon()
{
    // Commence drawing of a polygon.

    glBegin(GL_POLYGON);
}

void GLACanvas::EndPolygon()
{
    // Finish drawing of a polygon.
    
    glEnd();
}

void GLACanvas::PlaceVertex(glaCoord x, glaCoord y, glaCoord z)
{
    // Place a vertex for the current object being drawn.

#ifdef GLA_DEBUG
    m_Vertices++;
#endif
    glVertex3d(x, y, z);
}

void GLACanvas::PlaceIndicatorVertex(glaCoord x, glaCoord y)
{
    // Place a vertex for the current indicator object being drawn.

    PlaceVertex(x, y, 0.0);
}

void GLACanvas::BeginBlobs()
{
    // Commence drawing of a set of blobs.
    //glEnable(GL_ALPHA_TEST);
    //CHECK_GL_ERROR("BeginBlobs", "glEnable GL_ALPHA_TEST");
    glBegin(GL_POINTS);
}

void GLACanvas::EndBlobs()
{
    // Finish drawing of a set of blobs.
    glEnd();
    CHECK_GL_ERROR("EndBlobs", "GL_POINTS");
    //glDisable(GL_ALPHA_TEST);
    //CHECK_GL_ERROR("EndBlobs", "glDisable GL_ALPHA_TEST");
}

void GLACanvas::DrawBlob(glaCoord x, glaCoord y, glaCoord z)
{
    // Draw a marker (ideally a circle, but a square for now).
    PlaceVertex(x, y, z);
}

void GLACanvas::DrawRing(glaCoord x, glaCoord y)
{
    // Draw an unfilled circle
    const Double radius = 4; 
    assert(m_Quadric);
    glTranslated(x, y, 0.0);
    CHECK_GL_ERROR("DrawRing", "glTranslated");
    gluDisk(m_Quadric, radius - 1.0, radius, 12, 1);
    CHECK_GL_ERROR("DrawRing", "gluDisk");
    glTranslated(-x, -y, 0.0);
    CHECK_GL_ERROR("DrawRing", "glTranslated");
}

void GLACanvas::DrawRectangle(gla_colour edge, gla_colour fill,
                              glaCoord x0, glaCoord y0, glaCoord w, glaCoord h)
{
    // Draw a filled rectangle with an edge in the indicator plane.
    // (x0, y0) specify the bottom-left corner of the rectangle and (w, h) the
    // size.

    EnableSmoothPolygons();
    SetColour(fill);
    BeginQuadrilaterals();
    PlaceIndicatorVertex(x0, y0);
    PlaceIndicatorVertex(x0 + w, y0);
    PlaceIndicatorVertex(x0 + w, y0 + h);
    PlaceIndicatorVertex(x0, y0 + h);
    EndQuadrilaterals();
    DisableSmoothPolygons();

    if (edge != fill) {
        SetColour(edge);
        BeginLines();
        PlaceIndicatorVertex(x0, y0);
        PlaceIndicatorVertex(x0 + w, y0);
        PlaceIndicatorVertex(x0 + w, y0 + h);
        PlaceIndicatorVertex(x0, y0 + h);
        EndLines();
    }
}

void
GLACanvas::DrawShadedRectangle(const GLAPen & fill_bot, const GLAPen & fill_top,
			       glaCoord x0, glaCoord y0,
			       glaCoord w, glaCoord h)
{
    // Draw a graduated filled rectangle in the indicator plane.
    // (x0, y0) specify the bottom-left corner of the rectangle and (w, h) the
    // size.

    EnableSmoothPolygons();
    BeginQuadrilaterals();
    SetColour(fill_bot);
    PlaceIndicatorVertex(x0, y0);
    PlaceIndicatorVertex(x0 + w, y0);
    SetColour(fill_top);
    PlaceIndicatorVertex(x0 + w, y0 + h);
    PlaceIndicatorVertex(x0, y0 + h);
    EndQuadrilaterals();
    DisableSmoothPolygons();
}

void GLACanvas::DrawCircle(gla_colour edge, gla_colour fill,
			   glaCoord cx, glaCoord cy, glaCoord radius)
{
    // Draw a filled circle with an edge.
    
    cx += radius;
    cy -= radius;
    
    SetColour(fill);
    glTranslated(cx, cy, 0.0);
    CHECK_GL_ERROR("DrawCircle", "glTranslated");
    assert(m_Quadric);
    gluDisk(m_Quadric, 0.0, radius, 36, 1);
    CHECK_GL_ERROR("DrawCircle", "gluDisk");
    SetColour(edge);
    gluDisk(m_Quadric, radius - 1.0, radius, 36, 1);
    CHECK_GL_ERROR("DrawCircle", "gluDisk (2)");
    glTranslated(-cx, -cy, 0.0);
    CHECK_GL_ERROR("DrawCircle", "glTranslated (2)");
}

void GLACanvas::DrawSemicircle(gla_colour edge, gla_colour fill,
                               glaCoord cx, glaCoord cy,
                               glaCoord radius, glaCoord start)
{
    // Draw a filled semicircle with an edge.
    // The semicircle extends from "start" deg to "start"+180 deg (increasing
    // clockwise, 0 deg upwards).
    
    cx += radius;
    cy -= radius;

    SetColour(fill);
    glTranslated(cx, cy, 0.0);
    CHECK_GL_ERROR("DrawCircle", "glTranslated");
    assert(m_Quadric);
    gluPartialDisk(m_Quadric, 0.0, radius, 36, 1, start, 180.0);
    CHECK_GL_ERROR("DrawCircle", "gluPartialDisk");
    SetColour(edge);
    gluPartialDisk(m_Quadric, radius - 1.0, radius, 36, 1, start, 180.0);
    CHECK_GL_ERROR("DrawCircle", "gluPartialDisk (2)");
    glTranslated(-cx, -cy, 0.0);
    CHECK_GL_ERROR("DrawCircle", "glTranslated (2)");
}

void GLACanvas::DrawTriangle(gla_colour edge, gla_colour fill, GLAPoint* points)
{
    // Draw a filled triangle with an edge.
    
    SetColour(fill);
    BeginTriangles();
    PlaceIndicatorVertex(points[0].GetX(), points[0].GetY());
    PlaceIndicatorVertex(points[1].GetX(), points[1].GetY());
    PlaceIndicatorVertex(points[2].GetX(), points[2].GetY());
    EndTriangles();

    SetColour(edge);
    BeginPolyline();
    PlaceIndicatorVertex(points[0].GetX(), points[0].GetY());
    PlaceIndicatorVertex(points[1].GetX(), points[1].GetY());
    PlaceIndicatorVertex(points[2].GetX(), points[2].GetY());
    EndPolyline();
}

void GLACanvas::DrawList(glaList l)
{
    // Perform the operations specified by a display list.
    
    glCallList(l);
    CHECK_GL_ERROR("DrawList", "glCallList");
}

void GLACanvas::EnableDashedLines()
{
    // Enable dashed lines, and start drawing in them.

    glLineStipple(1, 0x3333);
    CHECK_GL_ERROR("EnableDashedLines", "glLineStipple");
    glEnable(GL_LINE_STIPPLE);
    CHECK_GL_ERROR("EnableDashedLines", "glEnable GL_LINE_STIPPLE");
}

void GLACanvas::DisableDashedLines()
{
    glDisable(GL_LINE_STIPPLE);
    CHECK_GL_ERROR("DisableDashedLines", "glDisable GL_LINE_STIPPLE");
}

void GLACanvas::Transform(Double x, Double y, Double z,
                          Double* x_out, Double* y_out, Double* z_out)
{
    // Convert from data coordinates to screen coordinates.
    
    // Perform the projection.
    gluProject(x, y, z, modelview_matrix, projection_matrix, viewport,
               x_out, y_out, z_out);
    CHECK_GL_ERROR("Transform", "gluProject");
}

void GLACanvas::ReverseTransform(Double x, Double y,
                                 Double* x_out, Double* y_out, Double* z_out)
{
    // Convert from screen coordinates to data coordinates.
    
    // Perform the projection.
    gluUnProject(x, y, 0.0, modelview_matrix, projection_matrix, viewport,
                 x_out, y_out, z_out);
    CHECK_GL_ERROR("ReverseTransform", "gluUnProject");
}

Double GLACanvas::SurveyUnitsAcrossViewport()
{
    // Measure the current viewport in survey units, taking into account the
    // current display scale.

    assert(m_Scale != 0.0);
    return m_VolumeDiameter / m_Scale;
}
