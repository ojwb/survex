//
//  gla-gl.cc
//
//  OpenGL implementation for the GLA abstraction layer.
//
//  Copyright (C) 2002 Mark R. Shinwell.
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

#include "gla.h"
#include <GL/gl.h>

//
//  GLAPen
//  

GLAPen::GLAPen() :
    m_Red(0.0), m_Green(0.0), m_Blue(0.0), m_Alpha(0.0)
{
}

GLAPen::~GLAPen()
{
}

void GLAPen::SetColour(double red, double green, double blue)
{
    m_Red = red;
    m_Green = green;
    m_Blue = blue;
}

void GLAPen::SetAlpha(double alpha)
{
    m_Alpha = alpha;
}

double GLAPen::GetRed() const
{
    return m_Red;
}

double GLAPen::GetGreen() const
{
    return m_Green;
}

double GLAPen::GetBlue() const
{
    return m_Blue;
}

double GLAPen::GetAlpha() const
{
    return m_Alpha;
}

void GLAPen::Interpolate(const GLAPen& pen, double how_far)
{
    m_Red = ((pen.GetRed() - m_Red) * how_far) + m_Red;
    m_Green = ((pen.GetGreen() - m_Green) * how_far) + m_Green;
    m_Blue = ((pen.GetBlue() - m_Blue) * how_far) + m_Blue;
    m_Alpha = ((pen.GetAlpha() - m_Alpha) * how_far) + m_Alpha;
}

//
//  GLACanvas
//  

GLACanvas::GLACanvas(wxWindow* parent, int id, const wxPoint& posn, wxSize size) :
    wxGLCanvas(parent, id, posn, size)
{
    // Constructor.

    m_Rotation.setFromEulerAngles(0.0, 0.0, 0.0);
    m_Scale = 0.0;
    m_Translation.x = m_Translation.y = m_Translation.z = 0.0;
    m_SphereCreated = false;
}

GLACanvas::~GLACanvas()
{
    // Destructor.

    if (m_SphereCreated) {
        glDeleteLists(m_SphereList, 1);
    }
}

void GLACanvas::Clear()
{
    // Clear the canvas.
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

    GLdouble modelview_matrix[16];
    GLdouble projection_matrix[16];
    GLint viewport[4];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview_matrix);
    glGetDoublev(GL_PROJECTION_MATRIX, projection_matrix);
    glGetIntegerv(GL_VIEWPORT, viewport);

    Double x0, y0, z0;
    Double x, y, z;
    gluUnProject(0.0, 0.0, 0.0, modelview_matrix, projection_matrix, viewport, &x0, &y0, &z0);
    gluUnProject(dx, -dy, 0.0, modelview_matrix, projection_matrix, viewport, &x, &y, &z);

    // Apply the translation.
    AddTranslation(x - x0, y - y0, z - z0);
}

void GLACanvas::SetVolumeCoordinates(glaCoord left, glaCoord right, glaCoord front, glaCoord back,
                                     glaCoord bottom, glaCoord top)
{
    // Set the size of the data drawing volume by giving the coordinates relative to the origin
    // of the faces of the volume.

    m_Volume.left = left;
    m_Volume.right = right;
    m_Volume.front = front;
    m_Volume.back = back;
    m_Volume.bottom = bottom;
    m_Volume.top = top;
}

void GLACanvas::SetViewportAndProjection()
{
    // Set viewport.
    wxSize size = GetSize();
    double aspect = double(size.GetWidth()) / double(size.GetHeight());
    glViewport(0, 0, size.GetWidth(), size.GetHeight());

    // Set projection.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(m_Volume.left, m_Volume.right, m_Volume.bottom / aspect, m_Volume.top / aspect,
            m_Volume.front * 3.0 * m_Scale, m_Volume.back * 3.0 * m_Scale);
}

void GLACanvas::StartDrawing()
{
    // Prepare for a redraw operation.
    
    SetCurrent();
    SetViewportAndProjection();
    
    glEnable(GL_COLOR_MATERIAL);
    glShadeModel(GL_SMOOTH);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glColorMaterial(GL_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_DEPTH_TEST);
}

void GLACanvas::SetDataTransform()
{
    // Set the modelview transform for drawing data.
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glScaled(m_Scale, m_Scale, m_Scale);
    glRotated(-90.0, 1.0, 0.0, 0.0); // get axes the correct way around (z upwards, y into screen)
    m_Rotation.CopyToOpenGL();
    glTranslated(m_Translation.x, m_Translation.y, m_Translation.z);
    
    glEnable(GL_DEPTH_TEST);
}

void GLACanvas::SetIndicatorTransform()
{
    // Set the modelview transform for drawing indicators.
    
    wxSize size = GetSize();
    
    double aspect = double(size.GetWidth()) / double(size.GetHeight());
    double width = m_Volume.right - m_Volume.left;
    double height = (m_Volume.top / aspect) - (m_Volume.bottom / aspect);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslated(0.0, 0.0, -m_Volume.front * 2.5 * m_Scale);
    glScaled(width / size.GetWidth(), height / size.GetHeight(), 1.0);

    glDisable(GL_DEPTH_TEST);
}

void GLACanvas::FinishDrawing()
{
    // Complete a redraw operation.
    
    glFlush();
    SwapBuffers();
}

void GLACanvas::SetQuaternion(Quaternion& q)
{
    // Set the quaternion used for the modelling transformation.
    
    m_Rotation = q;
}

glaList GLACanvas::CreateList(GfxCore* obj, void (GfxCore::*generator)())
{
    // Create a new list to hold a sequence of drawing operations, and compile it.

    glaList l = glGenLists(1);
    
    SetCurrent();
    glNewList(l, GL_COMPILE);
    (obj->*generator)();
    glEndList();
    
    return l;
}

void GLACanvas::DeleteList(glaList l)
{
    // Delete an existing list.

    glDeleteLists(l, 1);
}

void GLACanvas::SetBackgroundColour(float red, float green, float blue)
{
    // Set the background colour of the canvas.
    
    glClearColor(red, green, blue, 1.0);
}

void GLACanvas::SetColour(const GLAPen& pen, bool set_transparency, double rgb_scale)
{
    // Set the colour for subsequent operations.
   
    if (set_transparency) {
        glColor4f(pen.GetRed() * rgb_scale, pen.GetGreen() * rgb_scale, pen.GetBlue() * rgb_scale, pen.GetAlpha());
    }
    else {
        glColor3f(pen.GetRed() * rgb_scale, pen.GetGreen() * rgb_scale, pen.GetBlue() * rgb_scale);
    }
}

void GLACanvas::SetBlendColour(const GLAPen& pen, bool set_transparency, double rgb_scale)
{
    // Set the blend colour for subsequent operations.
   
    glBlendColor(pen.GetRed() * rgb_scale, pen.GetGreen() * rgb_scale, pen.GetBlue() * rgb_scale,
                 set_transparency ? pen.GetAlpha() : 1.0);
}

void GLACanvas::SetPolygonColour(GLAPen& pen, bool front, bool set_transparency)
{
    float col[4];
    col[0] = pen.GetRed();
    col[1] = pen.GetGreen();
    col[2] = pen.GetBlue();
    col[3] = pen.GetAlpha();

    glMaterialfv(front ? GL_FRONT : GL_BACK, GL_AMBIENT_AND_DIFFUSE, col);
}

void GLACanvas::DrawText(glaCoord x, glaCoord y, glaCoord z, const wxString& str)
{
    // Draw a text string on the current buffer in the current font.
 
    glRasterPos3d(x, y, z);
    
    for (int pos = 0; pos < str.Length(); pos++) {
        glutBitmapCharacter(m_Font, int(str[pos]));
    }
}

void GLACanvas::DrawIndicatorText(glaCoord x, glaCoord y, const wxString& str)
{
    DrawText(x, y, 0.0, str);
}

void GLACanvas::GetTextExtent(const wxString& str, glaCoord* x_ext, glaCoord* y_ext)
{
    assert(x_ext);
    assert(y_ext);

    *x_ext = 0;

    for (int pos = 0; pos < str.Length(); pos++) {
        *x_ext += glutBitmapWidth(m_Font, str[pos]);
    }

    *y_ext = m_FontSize + 2;
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

    glVertex3d(x, y, z);
}

void GLACanvas::PlaceIndicatorVertex(glaCoord x, glaCoord y)
{
    // Place a vertex for the current indicator object being drawn.

    PlaceVertex(x, y, 0.0);
}

void GLACanvas::DrawSphere(GLAPen& pen, glaCoord x, glaCoord y, glaCoord z, glaCoord radius, int divisions)
{
    // Draw a sphere centred on a particular point.

    SetColour(pen);

    if (!m_SphereCreated) {
        m_SphereCreated = true;

        m_SphereList = glGenLists(1);
        glNewList(m_SphereList, GL_COMPILE);
    
        GLUquadric* quadric = gluNewQuadric();
        assert(quadric);
        gluSphere(quadric, 1.0, divisions, divisions);

        glEndList();
    }

    glTranslated(x, y, z);
    glScalef(radius, radius, radius);
    glCallList(m_SphereList);
    glScalef(1.0 / radius, 1.0 / radius, 1.0 / radius);
    glTranslated(-x, -y, -z);
}

void GLACanvas::DrawRectangle(GLAPen& edge, GLAPen& fill, GLAPen& top,
                              glaCoord x0, glaCoord y0, glaCoord w, glaCoord h,
                              bool draw_lines)
{
    // Draw a filled rectangle with an edge in the indicator plane.
    // (x0, y0) specify the bottom-left corner of the rectangle and (w, h) the size.

    BeginQuadrilaterals();
    SetColour(fill);
    PlaceIndicatorVertex(x0, y0);
    PlaceIndicatorVertex(x0 + w, y0);
    SetColour(top);
    PlaceIndicatorVertex(x0 + w, y0 + h);
    PlaceIndicatorVertex(x0, y0 + h);
    EndQuadrilaterals();

    if (draw_lines) {
        SetColour(edge);
        BeginLines();
        PlaceIndicatorVertex(x0, y0);
        PlaceIndicatorVertex(x0 + w, y0);
        PlaceIndicatorVertex(x0 + w, y0 + h);
        PlaceIndicatorVertex(x0, y0 + h);
        EndLines();
    }
}

void GLACanvas::DrawCircle(GLAPen& edge, GLAPen& fill, glaCoord cx, glaCoord cy, glaCoord radius)
{
    // Draw a filled circle with an edge.
    
    GLUquadric* quadric = gluNewQuadric();

    assert(quadric);

    cx += radius;
    cy -= radius;
    
    SetColour(fill);
    glTranslated(cx, cy, 0.0);
    gluDisk(quadric, 0.0, radius, 36, 1);
    SetColour(edge);
    gluDisk(quadric, radius - 1.0, radius, 36, 1);
    glTranslated(-cx, -cy, 0.0);
}

void GLACanvas::DrawSemicircle(GLAPen& edge, GLAPen& fill, glaCoord cx, glaCoord cy, glaCoord radius, glaCoord start)
{
    // Draw a filled semicircle with an edge.
    // The semicircle extends from "start" deg to "start"+180 deg (increasing clockwise, 0 deg upwards).
    
    GLUquadric* quadric = gluNewQuadric();

    assert(quadric);

    cx += radius;
    cy -= radius;

    SetColour(fill);
    glTranslated(cx, cy, 0.0);
    gluPartialDisk(quadric, 0.0, radius, 36, 1, start, 180.0);
    SetColour(edge);
    gluPartialDisk(quadric, radius - 1.0, radius, 36, 1, start, 180.0);
    glTranslated(-cx, -cy, 0.0);
}

void GLACanvas::DrawTriangle(GLAPen& edge, GLAPen& fill, GLAPoint* points)
{
    // Draw a filled triangle with an edge.  The fill is in the z=0 plane whilst the edge is in
    // the z=5 plane.
    
    SetColour(fill);
    BeginTriangles();
    PlaceVertex(points[0].GetX(), points[0].GetY(), 0.0);
    PlaceVertex(points[1].GetX(), points[1].GetY(), 0.0);
    PlaceVertex(points[2].GetX(), points[2].GetY(), 0.0);
    EndTriangles();

    SetColour(edge);
    BeginPolyline();
    PlaceVertex(points[0].GetX(), points[0].GetY(), 0.0);
    PlaceVertex(points[1].GetX(), points[1].GetY(), 0.0);
    PlaceVertex(points[2].GetX(), points[2].GetY(), 0.0);

    EndPolyline();
}

void GLACanvas::DrawList(glaList l)
{
    // Perform the operations specified by a display list.
    
    glCallList(l);
}

void GLACanvas::EnableDashedLines()
{
    // Enable dashed lines, and start drawing in them.

    glLineStipple(1, 0xaaaa);
    glEnable(GL_LINE_STIPPLE);
}

void GLACanvas::DisableDashedLines()
{
    glDisable(GL_LINE_STIPPLE);
}

void GLACanvas::Transform(Double x, Double y, Double z, Double* x_out, Double* y_out, Double* z_out)
{
    // Convert from data coordinates to screen coordinates.
    
    // Get transformation matrices, etc.
    GLdouble modelview_matrix[16];
    GLdouble projection_matrix[16];
    GLint viewport[4];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview_matrix);
    glGetDoublev(GL_PROJECTION_MATRIX, projection_matrix);
    glGetIntegerv(GL_VIEWPORT, viewport);

    // Perform the projection.
    gluProject(x, y, z, modelview_matrix, projection_matrix, viewport, x_out, y_out, z_out);
}

void GLACanvas::ReverseTransform(Double x, Double y, Double* x_out, Double* y_out, Double* z_out)
{
    // Convert from screen coordinates to data coordinates.
    
    // Get transformation matrices, etc.
    GLdouble modelview_matrix[16];
    GLdouble projection_matrix[16];
    GLint viewport[4];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview_matrix);
    glGetDoublev(GL_PROJECTION_MATRIX, projection_matrix);
    glGetIntegerv(GL_VIEWPORT, viewport);

    // Perform the projection.
    gluUnProject(x, y, 0.0, modelview_matrix, projection_matrix, viewport, x_out, y_out, z_out);
}

Double GLACanvas::SurveyUnitsAcrossViewport()
{
    // Measure the current viewport in survey units, taking into account the current
    // display scale.

    return (m_Volume.right - m_Volume.left) / m_Scale;
}

