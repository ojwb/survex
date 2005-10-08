//
//  gla-gl.cc
//
//  OpenGL implementation for the GLA abstraction layer.
//
//  Copyright (C) 2002-2003,2005 Mark R. Shinwell
//  Copyright (C) 2003,2004,2005 Olly Betts
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
#include <wx/image.h>

#include <algorithm>

#include "gla.h"
#include "message.h"
#include "useful.h"

#include <GL/glext.h>

#ifndef GL_POINT_SPRITE_ARB
#define GL_POINT_SPRITE_ARB 0x8861
#endif
#ifndef GL_COORD_REPLACE_ARB
#define GL_COORD_REPLACE_ARB 0x8862
#endif

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

const double BLOB_DIAMETER = 5.0;

wxString GetGLSystemDescription()
{
    const char *p = (const char*)glGetString(GL_VERSION);
    // If OpenGL isn't initialised, p may be NULL.
    if (!p) return "No OpenGL information available!";

    wxString info;
    info += "OpenGL ";
    info += p;
    info += '\n';
    info += (const char*)glGetString(GL_VENDOR);
    info += '\n';
    info += (const char*)glGetString(GL_RENDERER);
    info += '\n';

    const GLubyte* gl_extensions = glGetString(GL_EXTENSIONS);
    if (*gl_extensions) {
	info += gl_extensions;
	info += '\n';
    }
    GLint red, green, blue;
    glGetIntegerv(GL_RED_BITS, &red);
    glGetIntegerv(GL_GREEN_BITS, &green);
    glGetIntegerv(GL_BLUE_BITS, &blue);
    GLint max_texture_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    GLint max_viewport[2];
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, max_viewport);
    GLdouble point_size_range[2];
    glGetDoublev(GL_POINT_SIZE_RANGE, point_size_range);
    GLdouble point_size_granularity;
    glGetDoublev(GL_POINT_SIZE_GRANULARITY, &point_size_granularity);
    wxString s;
    s.Printf("R%dG%dB%d\n"
	     "Max Texture size: %dx%d\n"
	     "Max Viewport size: %dx%d\n"
	     "Point Size %.3f-%.3f (granularity %.3f)",
	     (int)red, (int)green, (int)blue,
	     (int)max_texture_size, (int)max_texture_size,
	     (int)max_viewport[0], (int)max_viewport[1],
	     point_size_range[0], point_size_range[1],
	     point_size_granularity);
    info += s;
    return info;
}

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
    { 40, 40, 255 },   // blue
};

void GLAList::DrawList() const {
    glCallList(gl_list);
    CHECK_GL_ERROR("GLAList::DrawList", "glCallList");
}

void GLAList::InvalidateList() {
    glDeleteLists(gl_list, 1);
    CHECK_GL_ERROR("GLAList::InvalidateList", "glDeleteLists");

    // And flag this list as requiring generation before use.
    gl_list = 0;
}

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
    m_Texture = 0;
    m_Textured = false;
    m_Perspective = false;
    m_Fog = false;
    m_AntiAlias = false;
    list_flags = 0;
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

    // Clear any cached OpenGL lists.
    vector<GLAList>::iterator i;
    for (i = drawing_lists.begin(); i != drawing_lists.end(); ++i) {
	if (*i) i->InvalidateList();
    }
    drawing_lists.resize(0);

    static bool once_only = true;
    if (!once_only) return;
    once_only = false;

    m_Quadric = gluNewQuadric();
    CHECK_GL_ERROR("FirstShow", "gluNewQuadric");
    if (!m_Quadric) {
	abort(); // FIXME need to cope somehow
    }

    glShadeModel(GL_FLAT);
    CHECK_GL_ERROR("FirstShow", "glShadeModel");
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    CHECK_GL_ERROR("FirstShow", "glPolygonMode");
    //glAlphaFunc(GL_GREATER, 0.5f);
    //CHECK_GL_ERROR("FirstShow", "glAlphaFunc");

    // Grey fog effect.
    GLfloat fogcolour[4] = { 0.5, 0.5, 0.5, 1.0 };
    glFogfv(GL_FOG_COLOR, fogcolour);

    // Linear fogging.
    glFogi(GL_FOG_MODE, GL_LINEAR);

    // Optimise for speed (compute fog per vertex).
    glHint(GL_FOG_HINT, GL_FASTEST);

    // No padding on pixel packing and unpacking (default is to pad each
    // line to a multiple of 4 bytes).
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // For setting texture maps.
    CHECK_GL_ERROR("FirstShow", "glPixelStorei GL_UNPACK_ALIGNMENT");
    glPixelStorei(GL_PACK_ALIGNMENT, 1); // For screengrabs and movies.
    CHECK_GL_ERROR("FirstShow", "glPixelStorei GL_PACK_ALIGNMENT");

#ifdef USE_FNT
    // Load font
    wxString path(msg_cfgpth());
    path += wxCONFIG_PATH_SEPARATOR;
    path += "aven.txf";
    m_Font.load(path.c_str());
#endif

    // Check if we can use GL_POINTS to plot blobs at stations.
    GLdouble point_size_range[2];
    glGetDoublev(GL_POINT_SIZE_RANGE, point_size_range);
    CHECK_GL_ERROR("FirstShow", "glGetDoublev GL_POINT_SIZE_RANGE");
    glpoint_ok = (point_size_range[0] <= BLOB_DIAMETER &&
		  point_size_range[1] >= BLOB_DIAMETER);
    if (glpoint_ok) {
	glPointSize(BLOB_DIAMETER);
	CHECK_GL_ERROR("FirstShow", "glPointSize");
    }

    // See if we have GL_POINT_SPRITE_ARB - if so use it to draw crosses
    // by texture mapping GL points.
    const char * gl_extensions = (const char *)glGetString(GL_EXTENSIONS);
    glpoint_sprite = strstr(gl_extensions, "GL_ARB_point_sprite");
    if (glpoint_sprite) {
	float maxSize = 0.0f;
	glGetFloatv(GL_POINT_SIZE_MAX_ARB, &maxSize);
	if (maxSize < 8) glpoint_sprite = false;
    }
    if (glpoint_sprite) {
	glGenTextures(1, &m_CrossTexture);
	glBindTexture(GL_TEXTURE_2D, m_CrossTexture);
	const unsigned char crossteximage[128] = {
	    255,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,255,
	      0,  0,255,255,  0,  0,  0,  0,  0,  0,  0,  0,255,255,  0,  0,
	      0,  0,  0,  0,255,255,  0,  0,  0,  0,255,255,  0,  0,  0,  0,
	      0,  0,  0,  0,  0,  0,255,255,255,255,  0,  0,  0,  0,  0,  0,
	      0,  0,  0,  0,  0,  0,255,255,255,255,  0,  0,  0,  0,  0,  0,
	      0,  0,  0,  0,255,255,  0,  0,  0,  0,255,255,  0,  0,  0,  0,
	      0,  0,255,255,  0,  0,  0,  0,  0,  0,  0,  0,255,255,  0,  0,
	    255,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,255
	};
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, 8, 8, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, (GLvoid *)crossteximage);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
}

void GLACanvas::Clear()
{
    // Clear the canvas.

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CHECK_GL_ERROR("Clear", "glClear");
}

void GLACanvas::SetRotation(const Quaternion& q)
{
    m_Rotation = q;
}

void GLACanvas::SetScale(Double scale)
{
    if (scale != m_Scale) {
	vector<GLAList>::iterator i;
	for (i = drawing_lists.begin(); i != drawing_lists.end(); ++i) {
	    if (*i && i->test_flag(INVALIDATE_ON_SCALE)) i->InvalidateList();
	}

	m_Scale = scale;
    }
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
    // Set viewport.  The width and height go to zero when the panel is dragged
    // right across so we clamp them to be at least 1 to avoid errors from the
    // opengl calls below.
    int window_width;
    int window_height;
    GetSize(&window_width, &window_height);
    if (window_height < 1) window_height = 1;
    if (window_width < 1) window_width = 1;
    double aspect = double(window_height) / double(window_width);

    glViewport(0, 0, window_width, window_height);
    CHECK_GL_ERROR("SetDataTransform", "glViewport");

    // Set projection.
    glMatrixMode(GL_PROJECTION);
    CHECK_GL_ERROR("SetDataTransform", "glMatrixMode");
    glLoadIdentity();
    CHECK_GL_ERROR("SetDataTransform", "glLoadIdentity");

    Double near_plane = 1.0;
    if (m_Perspective) {
	Double lr = near_plane * tan(rad(25.0));
	Double far_plane = m_VolumeDiameter * 5 + near_plane; // FIXME: work out properly
	Double tb = lr * aspect;
	glFrustum(-lr, lr, -tb, tb, near_plane, far_plane);
	CHECK_GL_ERROR("SetViewportAndProjection", "glFrustum");
    } else {
	near_plane = 0.0;
	assert(m_Scale != 0.0);
	Double lr = m_VolumeDiameter / m_Scale * 0.5;
	Double far_plane = m_VolumeDiameter + near_plane;
	Double tb = lr * aspect;
	glOrtho(-lr, lr, -tb, tb, near_plane, far_plane);
	CHECK_GL_ERROR("SetViewportAndProjection", "glOrtho");
    }

    // Set the modelview transform for drawing data.
    glMatrixMode(GL_MODELVIEW);
    CHECK_GL_ERROR("SetDataTransform", "glMatrixMode");
    glLoadIdentity();
    CHECK_GL_ERROR("SetDataTransform", "glLoadIdentity");
    if (m_Perspective) {
	glTranslated(0.0, 0.0, -near_plane);
    } else {
	glTranslated(0.0, 0.0, -0.5 * m_VolumeDiameter);
    }
    CHECK_GL_ERROR("SetDataTransform", "glTranslated");
    // Get axes the correct way around (z upwards, y into screen)
    glRotated(-90.0, 1.0, 0.0, 0.0);
    CHECK_GL_ERROR("SetDataTransform", "glRotated");
    m_Rotation.CopyToOpenGL();
    CHECK_GL_ERROR("SetDataTransform", "CopyToOpenGL");
    if (m_Perspective) {
	glTranslated(m_Translation.x, m_Translation.y, m_Translation.z);
	CHECK_GL_ERROR("SetDataTransform", "glTranslated");
    }

    // Save projection matrix.
    glGetDoublev(GL_PROJECTION_MATRIX, projection_matrix);
    CHECK_GL_ERROR("SetDataTransform", "glGetDoublev");

    // Save viewport coordinates.
    glGetIntegerv(GL_VIEWPORT, viewport);
    CHECK_GL_ERROR("SetDataTransform", "glGetIntegerv");

    // Save modelview matrix.
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview_matrix);
    CHECK_GL_ERROR("SetDataTransform", "glGetDoublev");

    if (!m_Perspective) {
	// Adjust the translation so we don't change the Z position of the model
	Double X, Y, Z;
	gluProject(m_Translation.x, m_Translation.y, m_Translation.z,
		   modelview_matrix, projection_matrix, viewport,
		   &X, &Y, &Z);
	Double Tx, Ty, Tz;
	gluUnProject(X, Y, 0.5, modelview_matrix, projection_matrix, viewport,
		     &Tx, &Ty, &Tz);
	glMatrixMode(GL_MODELVIEW);
	glTranslated(Tx, Ty, Tz);
	CHECK_GL_ERROR("SetDataTransform", "glTranslated");
	glGetDoublev(GL_MODELVIEW_MATRIX, modelview_matrix);
    }

    glEnable(GL_DEPTH_TEST);
    CHECK_GL_ERROR("SetDataTransform", "glEnable GL_DEPTH_TEST");

    if (m_Textured) {
	glBindTexture(GL_TEXTURE_2D, m_Texture);
	glEnable(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	CHECK_GL_ERROR("ToggleTextured", "glTexParameteri GL_TEXTURE_WRAP_S");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	CHECK_GL_ERROR("ToggleTextured", "glTexParameteri GL_TEXTURE_WRAP_T");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	CHECK_GL_ERROR("ToggleTextured", "glTexParameteri GL_TEXTURE_MAG_FILTER");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			GL_LINEAR_MIPMAP_LINEAR);
	CHECK_GL_ERROR("ToggleTextured", "glTexParameteri GL_TEXTURE_MIN_FILTER");
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    } else {
	glDisable(GL_TEXTURE_2D);
    }
    if (m_Fog) {
	glFogf(GL_FOG_START, near_plane);
	glFogf(GL_FOG_END, near_plane + m_VolumeDiameter);
	glEnable(GL_FOG);
    } else {
	glDisable(GL_FOG);
    }

    if (m_AntiAlias) {
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
	glDisable(GL_LINE_SMOOTH);
    }
}

void GLACanvas::SetIndicatorTransform()
{
    list_flags |= NEVER_CACHE;

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
    glDisable(GL_FOG);
    CHECK_GL_ERROR("SetIndicatorTransform", "glDisable GL_FOG");

    // And just a simple 2D projection
    glMatrixMode(GL_PROJECTION);
    CHECK_GL_ERROR("SetIndicatorTransform", "glMatrixMode");
    glLoadIdentity();
    CHECK_GL_ERROR("SetIndicatorTransform", "glLoadIdentity (2)");
    gluOrtho2D(0, window_width, 0, window_height);
    CHECK_GL_ERROR("SetIndicatorTransform", "gluOrtho2D");

    glDisable(GL_TEXTURE_2D);
    CHECK_GL_ERROR("SetIndicatorTransform", "glDisable GL_TEXTURE_2D");
    glDisable(GL_BLEND);
    CHECK_GL_ERROR("SetIndicatorTransform", "glDisable GL_BLEND");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glAlphaFunc(GL_GREATER, 0.5f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    CHECK_GL_ERROR("SetIndicatorTransform", "various");
}

void GLACanvas::FinishDrawing()
{
    // Complete a redraw operation.

//    glFlush();
//    CHECK_GL_ERROR("FinishDrawing", "glFlush");
    SwapBuffers();
}

void GLACanvas::DrawList(unsigned int l)
{
    // FIXME: uncomment to disable use of lists for debugging:
    // GenerateList(l); return;
    if (l >= drawing_lists.size()) drawing_lists.resize(l + 1);

    // We generate the OpenGL lists lazily to minimise delays on startup.
    // So check if we need to generate the OpenGL list now.
    if (!drawing_lists[l] && !drawing_lists[l].test_flag(NEVER_CACHE)) {
	// Create a new OpenGL list to hold this sequence of drawing
	// operations.
	GLuint list = glGenLists(1);
#ifdef GLA_DEBUG
	printf("new list #%d: %d... ", l, list);
	m_Vertices = 0;
#endif
	CHECK_GL_ERROR("CreateList", "glGenLists");
	if (list == 0) {
	    // If we can't create a list, fall back to just drawing directly.
	    GenerateList(l);
	    return;
	}

	// http://www.opengl.org/resources/faq/technical/displaylist.htm
	// advises:
	// "Stay away from GL_COMPILE_AND_EXECUTE mode. Instead, create the
	// list using GL_COMPILE mode, then execute it with glCallList()."
	glNewList(list, GL_COMPILE);
	CHECK_GL_ERROR("CreateList", "glNewList");
	// Clear list_flags so that we can note what conditions to invalidate
	// the cached OpenGL list on.
	list_flags = 0;
	GenerateList(l);
	glEndList();
#ifdef GLA_DEBUG
	printf("done (%d vertices)\n", m_Vertices);
#endif
	CHECK_GL_ERROR("CreateList", "glEndList");
	drawing_lists[l] = GLAList(list, list_flags);
    }

    if (drawing_lists[l].test_flag(NEVER_CACHE)) {
	// That list can't be cached.
	GenerateList(l);
    } else {
	// Perform the operations specified by the OpenGL display list.
	drawing_lists[l].DrawList();
    }
}

void GLACanvas::DrawList2D(unsigned int l, glaCoord x, glaCoord y, Double rotation)
{
    glMatrixMode(GL_PROJECTION);
    CHECK_GL_ERROR("DrawList2D", "glMatrixMode");
    glPushMatrix();
    CHECK_GL_ERROR("DrawList2D", "glPushMatrix");
    glTranslated(x, y, 0);
    CHECK_GL_ERROR("DrawList2D", "glTranslated");
    if (rotation != 0.0) {
	glRotated(rotation, 0, 0, -1);
	CHECK_GL_ERROR("DrawList2D", "glRotated");
    }
    DrawList(l);
    glMatrixMode(GL_PROJECTION);
    CHECK_GL_ERROR("DrawList2D", "glMatrixMode 2");
    glPopMatrix();
    CHECK_GL_ERROR("DrawList2D", "glPopMatrix");
}

void GLACanvas::InvalidateList(unsigned int l)
{
    if (l < drawing_lists.size() && drawing_lists[l]) {
	// Delete any existing OpenGL list.
	drawing_lists[l].InvalidateList();
    }
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
    // Only draw text which is in front of the viewer.
    if (Z > 0) DrawIndicatorText((int)X, (int)Y, str);
#else
    glRasterPos3d(x, y, z);
    CHECK_GL_ERROR("DrawText", "glRasterPos3d");

#ifdef FREEGLUT
    glutBitmapString(m_Font, (const unsigned char *)str.c_str());
    CHECK_GL_ERROR("DrawText", "glutBitmapString");
#else
    for (size_t pos = 0; pos < str.length(); pos++) {
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
#ifdef USE_FNT
    m_Font.getTextExtent(str.c_str(), x_ext, y_ext);
#else
    if (x_ext) {
#if 1
	*x_ext = glutBitmapLength(m_Font, (const unsigned char *)str.c_str());
	CHECK_GL_ERROR("GetTextExtent", "glutBitmapLength");
#else
	*x_ext = 0;
	for (size_t pos = 0; pos < str.length(); pos++) {
	    x_ext += glutBitmapWidth(m_Font, int((unsigned char)str[pos]));
	    CHECK_GL_ERROR("GetTextExtent", "glutBitmapWidth");
	}
#endif
    }
    if (y_ext) *y_ext = m_FontSize + 2;
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
    if (glpoint_ok) {
	// Commence drawing of a set of blobs.
	glEnable(GL_ALPHA_TEST);
	CHECK_GL_ERROR("BeginBlobs", "glEnable GL_ALPHA_TEST");
	glEnable(GL_POINT_SMOOTH);
	CHECK_GL_ERROR("BeginBlobs", "glEnable GL_POINT_SMOOTH");
	glBegin(GL_POINTS);
    } else {
	glPushAttrib(GL_TRANSFORM_BIT|GL_VIEWPORT_BIT);
	CHECK_GL_ERROR("BeginBlobs", "glPushAttrib");
	SetIndicatorTransform();
    }
}

void GLACanvas::EndBlobs()
{
    if (glpoint_ok) {
	// Finish drawing of a set of blobs.
	glEnd();
	CHECK_GL_ERROR("EndBlobs", "GL_POINTS");
	glDisable(GL_POINT_SMOOTH);
	CHECK_GL_ERROR("EndBlobs", "glDisable GL_POINT_SMOOTH");
	glDisable(GL_ALPHA_TEST);
	CHECK_GL_ERROR("EndBlobs", "glDisable GL_ALPHA_TEST");
    } else {
	glPopAttrib();
	CHECK_GL_ERROR("EndBlobs", "glPopAttrib");
    }
}

void GLACanvas::DrawBlob(glaCoord x, glaCoord y, glaCoord z)
{
    if (glpoint_ok) {
	// Draw a marker.
	PlaceVertex(x, y, z);
    } else {
	Double X, Y, Z;
	if (!Transform(x, y, z, &X, &Y, &Z)) {
	    printf("bad transform\n");
	    return;
	}
	// Stuff behind us (in perspective view) will get clipped,
	// but we can save effort with a cheap check here.
	if (Z <= 0) return;

	// Draw an filled circle.
	assert(m_Quadric);
	glTranslated(X, Y, Z);
	CHECK_GL_ERROR("glTranslated 1", "DrawBlob");
	gluDisk(m_Quadric, 0, BLOB_DIAMETER * 0.5, 8, 1);
	CHECK_GL_ERROR("gluDisk", "DrawBlob");
	glTranslated(-X, -Y, -Z);
	CHECK_GL_ERROR("glTranslated 2", "DrawBlob");
#ifdef GLA_DEBUG
	m_Vertices++;
#endif
    }
}

void GLACanvas::BeginCrosses()
{
    // Plot crosses.
    if (glpoint_sprite) {
	glPushAttrib(GL_ENABLE_BIT|GL_POINT_BIT);
	glBindTexture(GL_TEXTURE_2D, m_CrossTexture);
	glEnable(GL_ALPHA_TEST);
	glPointSize(8);
	glTexEnvf(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_POINT_SPRITE_ARB);
	glBegin(GL_POINTS);
    } else {
	// To get the crosses to appear at a constant size and orientation on
	// screen, we plot them in the Indicator transform coordinates (which
	// unfortunately means they can't be usefully put in an opengl display
	// list).
	glPushAttrib(GL_TRANSFORM_BIT|GL_VIEWPORT_BIT);
	CHECK_GL_ERROR("BeginCrosses", "glPushAttrib");
	SetIndicatorTransform();
	glBegin(GL_LINES);
    }
}

void GLACanvas::EndCrosses()
{
    glEnd();
    glPopAttrib();
    CHECK_GL_ERROR("EndCrosses", "glPopAttrib");
}

void GLACanvas::DrawCross(glaCoord x, glaCoord y, glaCoord z)
{
    if (glpoint_sprite) {
	PlaceVertex(x, y, z);
    } else {
	Double X, Y, Z;
	if (!Transform(x, y, z, &X, &Y, &Z)) {
	    printf("bad transform\n");
	    return;
	}
	// Stuff behind us (in perspective view) will get clipped,
	// but we can save effort with a cheap check here.
	if (Z > 0) {
	    // Round to integers before adding on the offsets for the
	    // cross arms to avoid uneven crosses.
	    X = rint(X);
	    Y = rint(Y);
	    PlaceVertex(X - 3, Y - 3, Z);
	    PlaceVertex(X + 3, Y + 3, Z);
	    PlaceVertex(X - 3, Y + 3, Z);
	    PlaceVertex(X + 3, Y - 3, Z);
	}
    }
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

    SetColour(fill);
    BeginQuadrilaterals();
    PlaceIndicatorVertex(x0, y0);
    PlaceIndicatorVertex(x0 + w, y0);
    PlaceIndicatorVertex(x0 + w, y0 + h);
    PlaceIndicatorVertex(x0, y0 + h);
    EndQuadrilaterals();

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

    glShadeModel(GL_SMOOTH);
    BeginQuadrilaterals();
    SetColour(fill_bot);
    PlaceIndicatorVertex(x0, y0);
    PlaceIndicatorVertex(x0 + w, y0);
    SetColour(fill_top);
    PlaceIndicatorVertex(x0 + w, y0 + h);
    PlaceIndicatorVertex(x0, y0 + h);
    EndQuadrilaterals();
    glShadeModel(GL_FLAT);
}

void GLACanvas::DrawCircle(gla_colour edge, gla_colour fill,
			   glaCoord cx, glaCoord cy, glaCoord radius)
{
    // Draw a filled circle with an edge.
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

bool GLACanvas::Transform(Double x, Double y, Double z,
                          Double* x_out, Double* y_out, Double* z_out) const
{
    // Convert from data coordinates to screen coordinates.

    // Perform the projection.
    return gluProject(x, y, z, modelview_matrix, projection_matrix, viewport,
		      x_out, y_out, z_out);
}

void GLACanvas::ReverseTransform(Double x, Double y,
                                 Double* x_out, Double* y_out, Double* z_out) const
{
    // Convert from screen coordinates to data coordinates.

    // Perform the projection.
    gluUnProject(x, y, 0.0, modelview_matrix, projection_matrix, viewport,
                 x_out, y_out, z_out);
    CHECK_GL_ERROR("ReverseTransform", "gluUnProject");
}

Double GLACanvas::SurveyUnitsAcrossViewport() const
{
    // Measure the current viewport in survey units, taking into account the
    // current display scale.

    assert(m_Scale != 0.0);
    list_flags |= INVALIDATE_ON_SCALE;
    return m_VolumeDiameter / m_Scale;
}

void GLACanvas::ToggleTextured()
{
    m_Textured = !m_Textured;
    if (m_Textured && m_Texture == 0) {
	glGenTextures(1, &m_Texture);
	CHECK_GL_ERROR("ToggleTextured", "glGenTextures");

	glBindTexture(GL_TEXTURE_2D, m_Texture);
	CHECK_GL_ERROR("ToggleTextured", "glBindTexture");

	::wxInitAllImageHandlers();
	wxImage img;
	if (!img.LoadFile(wxString(msg_cfgpth()) + wxCONFIG_PATH_SEPARATOR +
			  wxString("icons") + wxCONFIG_PATH_SEPARATOR +
			  wxString("texture.png"), wxBITMAP_TYPE_PNG)) {
	    // FIXME
	    fprintf(stderr, "Couldn't load image.\n");
	    exit(1);
	}
	// Generate mipmaps.
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, // was GL_LUMINANCE
			  img.GetWidth(), img.GetHeight(),
			  GL_RGB, GL_UNSIGNED_BYTE, img.GetData());
	CHECK_GL_ERROR("ToggleTextured", "gluBuild2DMipmaps");

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	CHECK_GL_ERROR("ToggleTextured", "glTexEnvi");
    }
}

bool GLACanvas::SaveScreenshot(const wxString & fnm, int type) const
{
    int width;
    int height;
    GetSize(&width, &height);
    unsigned char *pixels = (unsigned char *)malloc(width * height * 3);
    if (!pixels) return false;
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid *)pixels);
    wxImage grab(width, height, pixels);
    // FIXME: might be slow to create new image, and uses twice the memory.
    // Perhaps flip image inplace ourselves using memcpy?
    return grab.Mirror(false).SaveFile(fnm, type);
}
