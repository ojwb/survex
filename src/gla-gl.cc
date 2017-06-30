//
//  gla-gl.cc
//
//  OpenGL implementation for the GLA abstraction layer.
//
//  Copyright (C) 2002-2003,2005 Mark R. Shinwell
//  Copyright (C) 2003,2004,2005,2006,2007,2010,2011,2012,2013,2014,2015,2017 Olly Betts
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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <wx/confbase.h>
#include <wx/image.h>

#include <algorithm>

#include "aven.h"
#include "gla.h"
#include "message.h"
#include "useful.h"

#ifdef HAVE_GL_GL_H
# include <GL/gl.h>
#elif defined HAVE_OPENGL_GL_H
# include <OpenGL/gl.h>
#endif

#ifdef HAVE_GL_GLEXT_H
# include <GL/glext.h>
#elif defined HAVE_OPENGL_GLEXT_H
# include <OpenGL/glext.h>
#endif

#ifndef GL_POINT_SIZE_MAX
#define GL_POINT_SIZE_MAX 0x8127
#endif
#ifndef GL_POINT_SPRITE
#define GL_POINT_SPRITE 0x8861
#endif
#ifndef GL_COORD_REPLACE
#define GL_COORD_REPLACE 0x8862
#endif
// GL_POINT_SIZE_RANGE is deprecated in OpenGL 1.2 and later, and replaced by
// GL_SMOOTH_POINT_SIZE_RANGE.
#ifndef GL_SMOOTH_POINT_SIZE_RANGE
#define GL_SMOOTH_POINT_SIZE_RANGE GL_POINT_SIZE_RANGE
#endif
// GL_POINT_SIZE_GRANULARITY is deprecated in OpenGL 1.2 and later, and
// replaced by GL_SMOOTH_POINT_SIZE_GRANULARITY.
#ifndef GL_SMOOTH_POINT_SIZE_GRANULARITY
#define GL_SMOOTH_POINT_SIZE_GRANULARITY GL_POINT_SIZE_GRANULARITY
#endif
// GL_ALIASED_POINT_SIZE_RANGE was added in OpenGL 1.2.
#ifndef GL_ALIASED_POINT_SIZE_RANGE
#define GL_ALIASED_POINT_SIZE_RANGE 0x846D
#endif

using namespace std;

const int BLOB_DIAMETER = 5;

#define BLOB_TEXTURE \
	    o, o, o, o, o, o, o, o,\
	    o, o, o, o, o, o, o, o,\
	    o, o, I, I, I, o, o, o,\
	    o, I, I, I, I, I, o, o,\
	    o, I, I, I, I, I, o, o,\
	    o, I, I, I, I, I, o, o,\
	    o, o, I, I, I, o, o, o,\
	    o, o, o, o, o, o, o, o

#define CROSS_TEXTURE \
	    o, o, o, o, o, o, o, o,\
	    I, o, o, o, o, o, I, o,\
	    o, I, o, o, o, I, o, o,\
	    o, o, I, o, I, o, o, o,\
	    o, o, o, I, o, o, o, o,\
	    o, o, I, o, I, o, o, o,\
	    o, I, o, o, o, I, o, o,\
	    I, o, o, o, o, o, I, o

static bool opengl_initialised = false;

string GetGLSystemDescription()
{
    // If OpenGL isn't initialised we may get a SEGV from glGetString.
    if (!opengl_initialised)
	return "No OpenGL information available yet - try opening a file.";
    const char *p = (const char*)glGetString(GL_VERSION);
    if (!p)
	return "Couldn't read OpenGL version!";

    string info;
    info += "OpenGL ";
    info += p;
    info += '\n';
    info += (const char*)glGetString(GL_VENDOR);
    info += '\n';
    info += (const char*)glGetString(GL_RENDERER);
#if defined __WXGTK__ || defined __WXX11__ || defined __WXMOTIF__
    info += string_format("\nGLX %0.1f\n", wxGLCanvas::GetGLXVersion() * 0.1);
#else
    info += '\n';
#endif

    GLint red, green, blue;
    glGetIntegerv(GL_RED_BITS, &red);
    glGetIntegerv(GL_GREEN_BITS, &green);
    glGetIntegerv(GL_BLUE_BITS, &blue);
    GLint max_texture_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    GLint max_viewport[2];
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, max_viewport);
    GLdouble point_size_range[2];
    glGetDoublev(GL_SMOOTH_POINT_SIZE_RANGE, point_size_range);
    GLdouble point_size_granularity;
    glGetDoublev(GL_SMOOTH_POINT_SIZE_GRANULARITY, &point_size_granularity);
    info += string_format("R%dG%dB%d\n"
	     "Max Texture size: %dx%d\n"
	     "Max Viewport size: %dx%d\n"
	     "Smooth Point Size %.3f-%.3f (granularity %.3f)",
	     (int)red, (int)green, (int)blue,
	     (int)max_texture_size, (int)max_texture_size,
	     (int)max_viewport[0], (int)max_viewport[1],
	     point_size_range[0], point_size_range[1],
	     point_size_granularity);
    glGetDoublev(GL_ALIASED_POINT_SIZE_RANGE, point_size_range);
    if (glGetError() != GL_INVALID_ENUM) {
	info += string_format("\nAliased point size %.3f-%.3f",
			      point_size_range[0], point_size_range[1]);
    }

    info += "\nDouble buffered: ";
    if (double_buffered)
	info += "true";
    else
	info += "false";

    const GLubyte* gl_extensions = glGetString(GL_EXTENSIONS);
    if (*gl_extensions) {
	info += '\n';
	info += (const char*)gl_extensions;
    }
    return info;
}

static bool
glpoint_sprite_works()
{
    // Point sprites provide an easy, fast way for us to draw crosses by
    // texture mapping GL points.
    //
    // If we have OpenGL >= 2.0 then we definitely have GL_POINT_SPRITE.
    // Otherwise see if we have the GL_ARB_point_sprite or GL_NV_point_sprite
    // extensions.
    //
    // The symbolic constants GL_POINT_SPRITE, GL_POINT_SPRITE_ARB, and
    // GL_POINT_SPRITE_NV all give the same number so it doesn't matter
    // which we use.
    static bool glpoint_sprite = false;
    static bool checked = false;
    if (!checked) {
	float maxSize = 0.0f;
	glGetFloatv(GL_POINT_SIZE_MAX, &maxSize);
	if (maxSize >= 8) {
	    glpoint_sprite = (atoi((const char *)glGetString(GL_VERSION)) >= 2);
	    if (!glpoint_sprite) {
		const char * p = (const char *)glGetString(GL_EXTENSIONS);
		while (true) {
		    size_t l = 0;
		    if (memcmp(p, "GL_ARB_point_sprite", 19) == 0) {
			l = 19;
		    } else if (memcmp(p, "GL_NV_point_sprite", 18) == 0) {
			l = 18;
		    }
		    if (l) {
			p += l;
			if (*p == '\0' || *p == ' ') {
			    glpoint_sprite = true;
			    break;
			}
		    }
		    p = strchr(p + 1, ' ');
		    if (!p) break;
		    ++p;
		}
	    }
	}
	checked = true;
    }
    return glpoint_sprite;
}

static void
log_gl_error(const wxChar * str, GLenum error_code)
{
    const char * e = reinterpret_cast<const char *>(gluErrorString(error_code));
    wxLogError(str, wxString(e, wxConvUTF8).c_str());
}

// Important: CHECK_GL_ERROR must not be called within a glBegin()/glEnd() pair
//            (thus it must not be called from BeginLines(), etc., or within a
//             BeginLines()/EndLines() block etc.)
#define CHECK_GL_ERROR(M, F) do { \
    if (!opengl_initialised) { \
	wxLogError(wxT(__FILE__ ":" STRING(__LINE__) ": OpenGL not initialised before (call " F " in method " M ")")); \
    } \
    GLenum error_code_ = glGetError(); \
    if (error_code_ != GL_NO_ERROR) { \
	log_gl_error(wxT(__FILE__ ":" STRING(__LINE__) ": OpenGL error: %s " \
			 "(call " F " in method " M ")"), error_code_); \
    } \
} while (0)

//
//  GLAPen
//

GLAPen::GLAPen()
{
    components[0] = components[1] = components[2] = 0.0;
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
    components[0] += how_far * (pen.GetRed() - components[0]);
    components[1] += how_far * (pen.GetGreen() - components[1]);
    components[2] += how_far * (pen.GetBlue() - components[2]);
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

bool GLAList::need_to_generate() {
    // Bail out if the list is already cached, or can't usefully be cached.
    if (flags & (GLACanvas::CACHED|GLACanvas::NEVER_CACHE))
	return false;

    // Create a new OpenGL list to hold this sequence of drawing
    // operations.
    if (gl_list == 0) {
	gl_list = glGenLists(1);
	CHECK_GL_ERROR("GLAList::need_to_generate", "glGenLists");
#ifdef GLA_DEBUG
	printf("glGenLists(1) returned %u\n", (unsigned)gl_list);
#endif
	if (gl_list == 0) {
	    // If we can't create a list for any reason, fall back to just
	    // drawing directly, and flag the list as NEVER_CACHE as there's
	    // unlikely to be much point calling glGenLists() again.
	    flags = GLACanvas::NEVER_CACHE;
	    return false;
	}

	// We should have 256 lists for font drawing and a dozen or so for 2D
	// and 3D lists.  So something is amiss if we've generated 1000 lists,
	// probably a infinite loop in the lazy list mechanism.
	assert(gl_list < 1000);
    }
    // https://www.opengl.org/resources/faq/technical/displaylist.htm advises:
    //
    // "Stay away from GL_COMPILE_AND_EXECUTE mode. Instead, create the
    // list using GL_COMPILE mode, then execute it with glCallList()."
    glNewList(gl_list, GL_COMPILE);
    CHECK_GL_ERROR("GLAList::need_to_generate", "glNewList");
    return true;
}

void GLAList::finalise(unsigned int list_flags)
{
    glEndList();
    CHECK_GL_ERROR("GLAList::finalise", "glEndList");
    if (list_flags & GLACanvas::NEVER_CACHE) {
	glDeleteLists(gl_list, 1);
	CHECK_GL_ERROR("GLAList::finalise", "glDeleteLists");
	gl_list = 0;
	flags = GLACanvas::NEVER_CACHE;
    } else {
	flags = list_flags | GLACanvas::CACHED;
    }
}

bool GLAList::DrawList() const {
    if ((flags & GLACanvas::CACHED) == 0)
	return false;
    glCallList(gl_list);
    CHECK_GL_ERROR("GLAList::DrawList", "glCallList");
    return true;
}

//
//  GLACanvas
//

BEGIN_EVENT_TABLE(GLACanvas, wxGLCanvas)
    EVT_SIZE(GLACanvas::OnSize)
END_EVENT_TABLE()

static const int wx_gl_window_attribs[] = {
    WX_GL_DOUBLEBUFFER,
    WX_GL_RGBA,
    WX_GL_DEPTH_SIZE, 16,
    0
};

// Pass wxWANTS_CHARS so that the window gets cursor keys on MS Windows.
GLACanvas::GLACanvas(wxWindow* parent, int id)
    : wxGLCanvas(parent, id, wx_gl_window_attribs, wxDefaultPosition,
		 wxDefaultSize, wxWANTS_CHARS),
      ctx(this), m_Translation(), blob_method(UNKNOWN), cross_method(UNKNOWN),
      x_size(0), y_size(0)
{
    // Constructor.

    m_Quadric = NULL;
    m_Pan = 0.0;
    m_Tilt = 0.0;
    m_Scale = 0.0;
    m_VolumeDiameter = 1.0;
    m_SmoothShading = false;
    m_Texture = 0;
    m_Textured = false;
    m_Perspective = false;
    m_Fog = false;
    m_AntiAlias = false;
    list_flags = 0;
    alpha = 1.0;
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
    // Update our record of the client area size and centre.
    GetClientSize(&x_size, &y_size);
    if (x_size < 1) x_size = 1;
    if (y_size < 1) y_size = 1;

    ctx.SetCurrent(*this);
    opengl_initialised = true;

    // Set the background colour of the canvas to black.
    glClearColor(0.0, 0.0, 0.0, 1.0);
    CHECK_GL_ERROR("FirstShow", "glClearColor");

    // Set viewport.
    glViewport(0, 0, x_size, y_size);
    CHECK_GL_ERROR("FirstShow", "glViewport");

    save_hints = false;

    vendor = wxString((const char *)glGetString(GL_VENDOR), wxConvUTF8);
    renderer = wxString((const char *)glGetString(GL_RENDERER), wxConvUTF8);
    {
	wxConfigBase * cfg = wxConfigBase::Get();
	wxString s;
	if (cfg->Read(wxT("opengl_survex"), &s, wxString()) && s == wxT(VERSION) &&
	    cfg->Read(wxT("opengl_vendor"), &s, wxString()) && s == vendor &&
	    cfg->Read(wxT("opengl_renderer"), &s, wxString()) && s == renderer) {
	    // The survex version, vendor and renderer are the same as those
	    // we cached hints for, so use those hints.
	    int v;
	    if (cfg->Read(wxT("blob_method"), &v, 0) &&
		(v == SPRITE || v == POINT || v == LINES)) {
		// How to draw blobs.
		blob_method = v;
	    }
	    if (cfg->Read(wxT("cross_method"), &v, 0) &&
		(v == SPRITE || v == LINES)) {
		// How to draw crosses.
		cross_method = v;
	    }
	}
    }

    if (m_Quadric) return;
    // One time initialisation follows.

    m_Quadric = gluNewQuadric();
    CHECK_GL_ERROR("FirstShow", "gluNewQuadric");
    if (!m_Quadric) {
	abort(); // FIXME need to cope somehow
    }

    glShadeModel(GL_FLAT);
    CHECK_GL_ERROR("FirstShow", "glShadeModel");
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // So text works.
    CHECK_GL_ERROR("FirstShow", "glPolygonMode");
    //glAlphaFunc(GL_GREATER, 0.5f);
    //CHECK_GL_ERROR("FirstShow", "glAlphaFunc");

    // We want glReadPixels() to read from the front buffer (which is the
    // default for single-buffered displays).
    if (double_buffered) {
	glReadBuffer(GL_FRONT);
	CHECK_GL_ERROR("FirstShow", "glReadBuffer");
    }

    // Grey fog effect.
    GLfloat fogcolour[4] = { 0.5, 0.5, 0.5, 1.0 };
    glFogfv(GL_FOG_COLOR, fogcolour);
    CHECK_GL_ERROR("FirstShow", "glFogfv");

    // Linear fogging.
    glFogi(GL_FOG_MODE, GL_LINEAR);
    CHECK_GL_ERROR("FirstShow", "glFogi");

    // Optimise for speed (compute fog per vertex).
    glHint(GL_FOG_HINT, GL_FASTEST);
    CHECK_GL_ERROR("FirstShow", "glHint");

    // No padding on pixel packing and unpacking (default is to pad each
    // line to a multiple of 4 bytes).
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // For setting texture maps.
    CHECK_GL_ERROR("FirstShow", "glPixelStorei GL_UNPACK_ALIGNMENT");
    glPixelStorei(GL_PACK_ALIGNMENT, 1); // For screengrabs and movies.
    CHECK_GL_ERROR("FirstShow", "glPixelStorei GL_PACK_ALIGNMENT");

    // Load font
    wxString path = wmsg_cfgpth();
    path += wxCONFIG_PATH_SEPARATOR;
    path += wxT("unifont.pixelfont");
    if (!m_Font.load(path)) {
	// FIXME: do something better.
	// We have this message available: Error in format of font file “%s”
	fprintf(stderr, "Failed to parse compiled-in font data\n");
	exit(1);
    }

    if (blob_method == UNKNOWN) {
	// Check if we can use GL_POINTS to plot blobs at stations.
	GLdouble point_size_range[2];
	glGetDoublev(GL_SMOOTH_POINT_SIZE_RANGE, point_size_range);
	CHECK_GL_ERROR("FirstShow", "glGetDoublev GL_SMOOTH_POINT_SIZE_RANGE");
	if (point_size_range[0] <= BLOB_DIAMETER &&
	    point_size_range[1] >= BLOB_DIAMETER) {
	    blob_method = POINT;
	} else {
	    blob_method = glpoint_sprite_works() ? SPRITE : LINES;
	}
	save_hints = true;
    }

    if (blob_method == POINT) {
	glPointSize(BLOB_DIAMETER);
	CHECK_GL_ERROR("FirstShow", "glPointSize");
    }

    if (cross_method == UNKNOWN) {
	cross_method = glpoint_sprite_works() ? SPRITE : LINES;
	save_hints = true;
    }

    if (cross_method == SPRITE) {
	glGenTextures(1, &m_CrossTexture);
	CHECK_GL_ERROR("FirstShow", "glGenTextures");
	glBindTexture(GL_TEXTURE_2D, m_CrossTexture);
	CHECK_GL_ERROR("FirstShow", "glBindTexture");
	// Cross image for drawing crosses using texture mapped point sprites.
	const unsigned char crossteximage[128] = {
#define o 0,0
#define I 255,255
	    CROSS_TEXTURE
#undef o
#undef I
	};
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	CHECK_GL_ERROR("FirstShow", "glPixelStorei");
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	CHECK_GL_ERROR("FirstShow", "glTexEnvi");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	CHECK_GL_ERROR("FirstShow", "glTexParameteri GL_TEXTURE_WRAP_S");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	CHECK_GL_ERROR("FirstShow", "glTexParameteri GL_TEXTURE_WRAP_T");
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, 8, 8, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, (GLvoid *)crossteximage);
	CHECK_GL_ERROR("FirstShow", "glTexImage2D");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	CHECK_GL_ERROR("FirstShow", "glTexParameteri GL_TEXTURE_MAG_FILTER");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	CHECK_GL_ERROR("FirstShow", "glTexParameteri GL_TEXTURE_MIN_FILTER");
    }

    if (blob_method == SPRITE) {
	glGenTextures(1, &m_BlobTexture);
	CHECK_GL_ERROR("FirstShow", "glGenTextures");
	glBindTexture(GL_TEXTURE_2D, m_BlobTexture);
	CHECK_GL_ERROR("FirstShow", "glBindTexture");
	// Image for drawing blobs using texture mapped point sprites.
	const unsigned char blobteximage[128] = {
#define o 0,0
#define I 255,255
	    BLOB_TEXTURE
#undef o
#undef I
	};
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	CHECK_GL_ERROR("FirstShow", "glPixelStorei");
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	CHECK_GL_ERROR("FirstShow", "glTexEnvi");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	CHECK_GL_ERROR("FirstShow", "glTexParameteri GL_TEXTURE_WRAP_S");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	CHECK_GL_ERROR("FirstShow", "glTexParameteri GL_TEXTURE_WRAP_T");
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, 8, 8, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, (GLvoid *)blobteximage);
	CHECK_GL_ERROR("FirstShow", "glTexImage2D");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	CHECK_GL_ERROR("FirstShow", "glTexParameteri GL_TEXTURE_MAG_FILTER");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	CHECK_GL_ERROR("FirstShow", "glTexParameteri GL_TEXTURE_MIN_FILTER");
    }
}

void GLACanvas::Clear()
{
    // Clear the canvas.

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CHECK_GL_ERROR("Clear", "glClear");
}

void GLACanvas::SetScale(Double scale)
{
    if (scale != m_Scale) {
	vector<GLAList>::iterator i;
	for (i = drawing_lists.begin(); i != drawing_lists.end(); ++i) {
	    i->invalidate_if(INVALIDATE_ON_SCALE);
	}

	m_Scale = scale;
    }
}

void GLACanvas::OnSize(wxSizeEvent & event)
{
    wxSize size = event.GetSize();

    unsigned int mask = 0;
    if (size.GetWidth() != x_size) mask |= INVALIDATE_ON_X_RESIZE;
    if (size.GetHeight() != y_size) mask |= INVALIDATE_ON_Y_RESIZE;
    if (mask) {
	vector<GLAList>::iterator i;
	for (i = drawing_lists.begin(); i != drawing_lists.end(); ++i) {
	    i->invalidate_if(mask);
	}

	// The width and height go to zero when the panel is dragged right
	// across so we clamp them to be at least 1 to avoid problems.
	x_size = size.GetWidth();
	y_size = size.GetHeight();
	if (x_size < 1) x_size = 1;
	if (y_size < 1) y_size = 1;
    }

    event.Skip();

    if (!opengl_initialised) return;

    // Set viewport.
    glViewport(0, 0, x_size, y_size);
    CHECK_GL_ERROR("OnSize", "glViewport");
}

void GLACanvas::AddTranslationScreenCoordinates(int dx, int dy)
{
    // Translate the data by a given amount, specified in screen coordinates.

    // Find out how far the translation takes us in data coordinates.
    SetDataTransform();

    double x0, y0, z0;
    double x, y, z;
    gluUnProject(0.0, 0.0, 0.0, modelview_matrix, projection_matrix, viewport,
		 &x0, &y0, &z0);
    CHECK_GL_ERROR("AddTranslationScreenCoordinates", "gluUnProject");
    gluUnProject(dx, -dy, 0.0, modelview_matrix, projection_matrix, viewport,
		 &x, &y, &z);
    CHECK_GL_ERROR("AddTranslationScreenCoordinates", "gluUnProject (2)");

    // Apply the translation.
    AddTranslation(Vector3(x - x0, y - y0, z - z0));
}

void GLACanvas::SetVolumeDiameter(glaCoord diameter)
{
    // Set the size of the data drawing volume by giving the diameter of the
    // smallest sphere containing it.

    m_VolumeDiameter = max(glaCoord(1.0), diameter);
}

void GLACanvas::StartDrawing()
{
    // Prepare for a redraw operation.

    ctx.SetCurrent(*this);
    glDepthMask(GL_TRUE);

    if (!save_hints) return;

    // We want to check on the second redraw.
    static int draw_count = 2;
    if (--draw_count != 0) return;

    if (cross_method != LINES) {
	SetColour(col_WHITE);
	Clear();
	SetDataTransform();
	BeginCrosses();
	DrawCross(-m_Translation.GetX(), -m_Translation.GetY(), -m_Translation.GetZ());
	EndCrosses();
	static const unsigned char expected_cross[64 * 3] = {
#define o 0,0,0
#define I 255,255,255
	    CROSS_TEXTURE
#undef o
#undef I
	};
	if (!CheckVisualFidelity(expected_cross)) {
	    cross_method = LINES;
	    save_hints = true;
	}
    }

    if (blob_method != LINES) {
	SetColour(col_WHITE);
	Clear();
	SetDataTransform();
	BeginBlobs();
	DrawBlob(-m_Translation.GetX(), -m_Translation.GetY(), -m_Translation.GetZ());
	EndBlobs();
	static const unsigned char expected_blob[64 * 3] = {
#define o 0,0,0
#define I 255,255,255
	    BLOB_TEXTURE
#undef o
#undef I
	};
	if (!CheckVisualFidelity(expected_blob)) {
	    blob_method = LINES;
	    save_hints = true;
	}
    }

    wxConfigBase * cfg = wxConfigBase::Get();
    cfg->Write(wxT("opengl_survex"), wxT(VERSION));
    cfg->Write(wxT("opengl_vendor"), vendor);
    cfg->Write(wxT("opengl_renderer"), renderer);
    cfg->Write(wxT("blob_method"), blob_method);
    cfg->Write(wxT("cross_method"), cross_method);
    cfg->Flush();
    save_hints = false;
}

void GLACanvas::EnableSmoothPolygons(bool filled)
{
    // Prepare for drawing smoothly-shaded polygons.
    // Only use this when required (in particular lines in lists may not be
    // coloured correctly when this is enabled).

    glPushAttrib(GL_ENABLE_BIT|GL_LIGHTING_BIT|GL_POLYGON_BIT);
    if (filled) {
	glShadeModel(GL_SMOOTH);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    } else {
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    CHECK_GL_ERROR("EnableSmoothPolygons", "glPolygonMode");

    if (filled && m_SmoothShading) {
	static const GLfloat mat_specular[] = { 0.2, 0.2, 0.2, 1.0 };
	static const GLfloat light_position[] = { -1.0, -1.0, -1.0, 0.0 };
	static const GLfloat light_ambient[] = { 0.3, 0.3, 0.3, 1.0 };
	static const GLfloat light_diffuse[] = { 0.7, 0.7, 0.7, 1.0 };
	glEnable(GL_COLOR_MATERIAL);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 10.0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
    }
}

void GLACanvas::DisableSmoothPolygons()
{
    glPopAttrib();
}

void GLACanvas::PlaceNormal(const Vector3 &v)
{
    // Add a normal (for polygons etc.)

    glNormal3d(v.GetX(), v.GetY(), v.GetZ());
}

void GLACanvas::SetDataTransform()
{
    // Set projection.
    glMatrixMode(GL_PROJECTION);
    CHECK_GL_ERROR("SetDataTransform", "glMatrixMode");
    glLoadIdentity();
    CHECK_GL_ERROR("SetDataTransform", "glLoadIdentity");

    double aspect = double(y_size) / double(x_size);

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
    glRotated(-m_Tilt, 1.0, 0.0, 0.0);
    CHECK_GL_ERROR("SetDataTransform", "glRotated");
    glRotated(m_Pan, 0.0, 0.0, 1.0);
    CHECK_GL_ERROR("SetDataTransform", "CopyToOpenGL");
    if (m_Perspective) {
	glTranslated(m_Translation.GetX(),
		     m_Translation.GetY(),
		     m_Translation.GetZ());
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
	double X, Y, Z;
	gluProject(m_Translation.GetX(),
		   m_Translation.GetY(),
		   m_Translation.GetZ(),
		   modelview_matrix, projection_matrix, viewport,
		   &X, &Y, &Z);
	double Tx, Ty, Tz;
	gluUnProject(X, Y, 0.5, modelview_matrix, projection_matrix, viewport,
		     &Tx, &Ty, &Tz);
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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (m_AntiAlias) {
	glEnable(GL_LINE_SMOOTH);
    } else {
	glDisable(GL_LINE_SMOOTH);
    }
}

void GLACanvas::SetIndicatorTransform()
{
    list_flags |= NEVER_CACHE;

    // Set the modelview transform and projection for drawing indicators.

    glDisable(GL_DEPTH_TEST);
    CHECK_GL_ERROR("SetIndicatorTransform", "glDisable GL_DEPTH_TEST");
    glDisable(GL_FOG);
    CHECK_GL_ERROR("SetIndicatorTransform", "glDisable GL_FOG");

    // Just a simple 2D projection.
    glMatrixMode(GL_PROJECTION);
    CHECK_GL_ERROR("SetIndicatorTransform", "glMatrixMode");
    glLoadIdentity();
    CHECK_GL_ERROR("SetIndicatorTransform", "glLoadIdentity (2)");
    gluOrtho2D(0, x_size, 0, y_size);
    CHECK_GL_ERROR("SetIndicatorTransform", "gluOrtho2D");

    // No modelview transform.
    glMatrixMode(GL_MODELVIEW);
    CHECK_GL_ERROR("SetIndicatorTransform", "glMatrixMode");
    glLoadIdentity();
    CHECK_GL_ERROR("SetIndicatorTransform", "glLoadIdentity");

    glDisable(GL_TEXTURE_2D);
    CHECK_GL_ERROR("SetIndicatorTransform", "glDisable GL_TEXTURE_2D");
    glDisable(GL_BLEND);
    CHECK_GL_ERROR("SetIndicatorTransform", "glDisable GL_BLEND");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    CHECK_GL_ERROR("SetIndicatorTransform", "glTexParameteri GL_TEXTURE_WRAP_S");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    CHECK_GL_ERROR("SetIndicatorTransform", "glTexParameteri GL_TEXTURE_WRAP_T");
    glAlphaFunc(GL_GREATER, 0.5f);
    CHECK_GL_ERROR("SetIndicatorTransform", "glAlphaFunc");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    CHECK_GL_ERROR("SetIndicatorTransform", "glTexParameteri GL_TEXTURE_MAG_FILTER");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    CHECK_GL_ERROR("SetIndicatorTransform", "glTexParameteri GL_TEXTURE_MIN_FILTER");
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    CHECK_GL_ERROR("SetIndicatorTransform", "glHint");
}

void GLACanvas::FinishDrawing()
{
    // Complete a redraw operation.

    if (double_buffered) {
	SwapBuffers();
    } else {
	glFlush();
	CHECK_GL_ERROR("FinishDrawing", "glFlush");
    }
}

void GLACanvas::DrawList(unsigned int l)
{
    // FIXME: uncomment to disable use of lists for debugging:
    // GenerateList(l); return;
    if (l >= drawing_lists.size()) drawing_lists.resize(l + 1);

    // We generate the OpenGL lists lazily to minimise delays on startup.
    // So check if we need to generate the OpenGL list now.
    if (drawing_lists[l].need_to_generate()) {
	// Clear list_flags so that we can note what conditions to invalidate
	// the cached OpenGL list on.
	list_flags = 0;

#ifdef GLA_DEBUG
	printf("generating list #%u... ", l);
	m_Vertices = 0;
#endif
	GenerateList(l);
#ifdef GLA_DEBUG
	printf("done (%d vertices)\n", m_Vertices);
#endif
	drawing_lists[l].finalise(list_flags);
    }

    if (!drawing_lists[l].DrawList()) {
	// That list isn't cached (which means it probably can't usefully be
	// cached).
	GenerateList(l);
    }
}

void GLACanvas::DrawListZPrepass(unsigned int l)
{
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    DrawList(l);
    glDepthMask(GL_FALSE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthFunc(GL_EQUAL);
    DrawList(l);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
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

void GLACanvas::SetColour(const GLAPen& pen, double rgb_scale)
{
    // Set the colour for subsequent operations.
    glColor4f(pen.GetRed() * rgb_scale, pen.GetGreen() * rgb_scale,
	      pen.GetBlue() * rgb_scale, alpha);
}

void GLACanvas::SetColour(const GLAPen& pen)
{
    // Set the colour for subsequent operations.
    glColor4d(pen.components[0], pen.components[1], pen.components[2], alpha);
}

void GLACanvas::SetColour(gla_colour colour, double rgb_scale)
{
    // Set the colour for subsequent operations.
    rgb_scale /= 255.0;
    glColor4f(COLOURS[colour].r * rgb_scale,
	      COLOURS[colour].g * rgb_scale,
	      COLOURS[colour].b * rgb_scale,
	      alpha);
}

void GLACanvas::SetColour(gla_colour colour)
{
    // Set the colour for subsequent operations.
    if (alpha == 1.0) {
	glColor3ubv(&COLOURS[colour].r);
    } else {
	glColor4ub(COLOURS[colour].r,
		   COLOURS[colour].g,
		   COLOURS[colour].b,
		   (unsigned char)(255 * alpha));
    }
}

void GLACanvas::DrawText(glaCoord x, glaCoord y, glaCoord z, const wxString& str)
{
    // Draw a text string on the current buffer in the current font.
    glRasterPos3d(x, y, z);
    CHECK_GL_ERROR("DrawText", "glRasterPos3d");
    m_Font.write_string(str.data(), str.size());
}

void GLACanvas::DrawIndicatorText(int x, int y, const wxString& str)
{
    glRasterPos2d(x, y);
    CHECK_GL_ERROR("DrawIndicatorText", "glRasterPos2d");
    m_Font.write_string(str.data(), str.size());
}

void GLACanvas::GetTextExtent(const wxString& str, int * x_ext, int * y_ext) const
{
    m_Font.get_text_extent(str.data(), str.size(), x_ext, y_ext);
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
    CHECK_GL_ERROR("EndQuadrilaterals", "glEnd GL_QUADS");
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
    CHECK_GL_ERROR("EndLines", "glEnd GL_LINES");
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
    CHECK_GL_ERROR("EndTriangles", "glEnd GL_TRIANGLES");
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
    CHECK_GL_ERROR("EndTriangleStrip", "glEnd GL_TRIANGLE_STRIP");
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
    CHECK_GL_ERROR("EndPolyline", "glEnd GL_LINE_STRIP");
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
    CHECK_GL_ERROR("EndPolygon", "glEnd GL_POLYGON");
}

void GLACanvas::PlaceVertex(glaCoord x, glaCoord y, glaCoord z)
{
    // Place a vertex for the current object being drawn.

#ifdef GLA_DEBUG
    m_Vertices++;
#endif
    glVertex3d(x, y, z);
}

void GLACanvas::PlaceVertex(glaCoord x, glaCoord y, glaCoord z,
			    glaTexCoord tex_x, glaTexCoord tex_y)
{
    // Place a vertex for the current object being drawn.

#ifdef GLA_DEBUG
    m_Vertices++;
#endif
    glTexCoord2f(tex_x, tex_y);
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
    if (blob_method == SPRITE) {
	glPushAttrib(GL_ENABLE_BIT|GL_POINT_BIT);
	CHECK_GL_ERROR("BeginBlobs", "glPushAttrib");
	glBindTexture(GL_TEXTURE_2D, m_BlobTexture);
	CHECK_GL_ERROR("BeginBlobs", "glBindTexture");
	glEnable(GL_ALPHA_TEST);
	CHECK_GL_ERROR("BeginBlobs", "glEnable GL_ALPHA_TEST");
	glPointSize(8);
	CHECK_GL_ERROR("BeginBlobs", "glPointSize");
	glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
	CHECK_GL_ERROR("BeginBlobs", "glTexEnvi GL_POINT_SPRITE");
	glEnable(GL_TEXTURE_2D);
	CHECK_GL_ERROR("BeginBlobs", "glEnable GL_TEXTURE_2D");
	glEnable(GL_POINT_SPRITE);
	CHECK_GL_ERROR("BeginBlobs", "glEnable GL_POINT_SPRITE");
	glBegin(GL_POINTS);
    } else if (blob_method == POINT) {
	glPushAttrib(GL_ENABLE_BIT);
	CHECK_GL_ERROR("BeginBlobs", "glPushAttrib");
	glEnable(GL_ALPHA_TEST);
	CHECK_GL_ERROR("BeginBlobs", "glEnable GL_ALPHA_TEST");
	glEnable(GL_POINT_SMOOTH);
	CHECK_GL_ERROR("BeginBlobs", "glEnable GL_POINT_SMOOTH");
	glBegin(GL_POINTS);
    } else {
	glPushAttrib(GL_TRANSFORM_BIT|GL_VIEWPORT_BIT|GL_ENABLE_BIT);
	CHECK_GL_ERROR("BeginBlobs", "glPushAttrib");
	SetIndicatorTransform();
	glEnable(GL_DEPTH_TEST);
	CHECK_GL_ERROR("BeginBlobs", "glEnable GL_DEPTH_TEST");
	glBegin(GL_LINES);
    }
}

void GLACanvas::EndBlobs()
{
    // Finish drawing of a set of blobs.
    glEnd();
    if (blob_method != LINES) {
	CHECK_GL_ERROR("EndBlobs", "glEnd GL_POINTS");
    } else {
	CHECK_GL_ERROR("EndBlobs", "glEnd GL_LINES");
    }
    glPopAttrib();
    CHECK_GL_ERROR("EndBlobs", "glPopAttrib");
}

void GLACanvas::DrawBlob(glaCoord x, glaCoord y, glaCoord z)
{
    if (blob_method != LINES) {
	// Draw a marker.
	PlaceVertex(x, y, z);
    } else {
	double X, Y, Z;
	if (!Transform(Vector3(x, y, z), &X, &Y, &Z)) {
	    printf("bad transform\n");
	    return;
	}
	// Stuff behind us (in perspective view) will get clipped,
	// but we can save effort with a cheap check here.
	if (Z <= 0) return;

	X -= BLOB_DIAMETER * 0.5;
	Y -= BLOB_DIAMETER * 0.5;

	PlaceVertex(X, Y + 1, Z);
	PlaceVertex(X, Y + (BLOB_DIAMETER - 1), Z);

	for (int i = 1; i < (BLOB_DIAMETER - 1); ++i) {
	    PlaceVertex(X + i, Y, Z);
	    PlaceVertex(X + i, Y + BLOB_DIAMETER, Z);
	}

	PlaceVertex(X + (BLOB_DIAMETER - 1), Y + 1, Z);
	PlaceVertex(X + (BLOB_DIAMETER - 1), Y + (BLOB_DIAMETER - 1), Z);
    }
#ifdef GLA_DEBUG
    m_Vertices++;
#endif
}

void GLACanvas::DrawBlob(glaCoord x, glaCoord y)
{
    if (blob_method != LINES) {
	// Draw a marker.
	PlaceVertex(x, y, 0);
    } else {
	x -= BLOB_DIAMETER * 0.5;
	y -= BLOB_DIAMETER * 0.5;

	PlaceVertex(x, y + 1, 0);
	PlaceVertex(x, y + (BLOB_DIAMETER - 1), 0);

	for (int i = 1; i < (BLOB_DIAMETER - 1); ++i) {
	    PlaceVertex(x + i, y, 0);
	    PlaceVertex(x + i, y + BLOB_DIAMETER, 0);
	}

	PlaceVertex(x + (BLOB_DIAMETER - 1), y + 1, 0);
	PlaceVertex(x + (BLOB_DIAMETER - 1), y + (BLOB_DIAMETER - 1), 0);
    }
#ifdef GLA_DEBUG
    m_Vertices++;
#endif
}

void GLACanvas::BeginCrosses()
{
    // Plot crosses.
    if (cross_method == SPRITE) {
	glPushAttrib(GL_ENABLE_BIT|GL_POINT_BIT);
	CHECK_GL_ERROR("BeginCrosses", "glPushAttrib");
	glBindTexture(GL_TEXTURE_2D, m_CrossTexture);
	CHECK_GL_ERROR("BeginCrosses", "glBindTexture");
	glEnable(GL_ALPHA_TEST);
	CHECK_GL_ERROR("BeginCrosses", "glEnable GL_ALPHA_TEST");
	glPointSize(8);
	CHECK_GL_ERROR("BeginCrosses", "glPointSize");
	glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
	CHECK_GL_ERROR("BeginCrosses", "glTexEnvi GL_POINT_SPRITE");
	glEnable(GL_TEXTURE_2D);
	CHECK_GL_ERROR("BeginCrosses", "glEnable GL_TEXTURE_2D");
	glEnable(GL_POINT_SPRITE);
	CHECK_GL_ERROR("BeginCrosses", "glEnable GL_POINT_SPRITE");
	glBegin(GL_POINTS);
    } else {
	// To get the crosses to appear at a constant size and orientation on
	// screen, we plot them in the Indicator transform coordinates (which
	// unfortunately means they can't be usefully put in an opengl display
	// list).
	glPushAttrib(GL_TRANSFORM_BIT|GL_VIEWPORT_BIT|GL_ENABLE_BIT);
	CHECK_GL_ERROR("BeginCrosses", "glPushAttrib 2");
	SetIndicatorTransform();
	glEnable(GL_DEPTH_TEST);
	CHECK_GL_ERROR("BeginCrosses", "glEnable GL_DEPTH_TEST");
	glBegin(GL_LINES);
    }
}

void GLACanvas::EndCrosses()
{
    glEnd();
    if (cross_method == SPRITE) {
	CHECK_GL_ERROR("EndCrosses", "glEnd GL_POINTS");
    } else {
	CHECK_GL_ERROR("EndCrosses", "glEnd GL_LINES");
    }
    glPopAttrib();
    CHECK_GL_ERROR("EndCrosses", "glPopAttrib");
}

void GLACanvas::DrawCross(glaCoord x, glaCoord y, glaCoord z)
{
    if (cross_method == SPRITE) {
	// Draw a marker.
	PlaceVertex(x, y, z);
    } else {
	double X, Y, Z;
	if (!Transform(Vector3(x, y, z), &X, &Y, &Z)) {
	    printf("bad transform\n");
	    return;
	}
	// Stuff behind us (in perspective view) will get clipped,
	// but we can save effort with a cheap check here.
	if (Z <= 0) return;

	// Round to integers before adding on the offsets for the
	// cross arms to avoid uneven crosses.
	X = rint(X);
	Y = rint(Y);
	PlaceVertex(X - 3, Y - 3, Z);
	PlaceVertex(X + 3, Y + 3, Z);
	PlaceVertex(X - 3, Y + 3, Z);
	PlaceVertex(X + 3, Y - 3, Z);
    }
#ifdef GLA_DEBUG
    m_Vertices++;
#endif
}

void GLACanvas::DrawRing(glaCoord x, glaCoord y)
{
    // Draw an unfilled circle
    const Double radius = 4;
    assert(m_Quadric);
    glMatrixMode(GL_MODELVIEW);
    CHECK_GL_ERROR("DrawRing", "glMatrixMode");
    glPushMatrix();
    CHECK_GL_ERROR("DrawRing", "glPushMatrix");
    glTranslated(x, y, 0.0);
    CHECK_GL_ERROR("DrawRing", "glTranslated");
    gluDisk(m_Quadric, radius - 1.0, radius, 12, 1);
    CHECK_GL_ERROR("DrawRing", "gluDisk");
    glPopMatrix();
    CHECK_GL_ERROR("DrawRing", "glPopMatrix");
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
    CHECK_GL_ERROR("DrawShadedRectangle", "glShadeModel GL_SMOOTH");
    BeginQuadrilaterals();
    SetColour(fill_bot);
    PlaceIndicatorVertex(x0, y0);
    PlaceIndicatorVertex(x0 + w, y0);
    SetColour(fill_top);
    PlaceIndicatorVertex(x0 + w, y0 + h);
    PlaceIndicatorVertex(x0, y0 + h);
    EndQuadrilaterals();
    glShadeModel(GL_FLAT);
    CHECK_GL_ERROR("DrawShadedRectangle", "glShadeModel GL_FLAT");
}

void GLACanvas::DrawCircle(gla_colour edge, gla_colour fill,
			   glaCoord cx, glaCoord cy, glaCoord radius)
{
    // Draw a filled circle with an edge.
    SetColour(fill);
    glMatrixMode(GL_MODELVIEW);
    CHECK_GL_ERROR("DrawCircle", "glMatrixMode");
    glPushMatrix();
    CHECK_GL_ERROR("DrawCircle", "glPushMatrix");
    glTranslated(cx, cy, 0.0);
    CHECK_GL_ERROR("DrawCircle", "glTranslated");
    assert(m_Quadric);
    gluDisk(m_Quadric, 0.0, radius, 36, 1);
    CHECK_GL_ERROR("DrawCircle", "gluDisk");
    SetColour(edge);
    gluDisk(m_Quadric, radius - 1.0, radius, 36, 1);
    CHECK_GL_ERROR("DrawCircle", "gluDisk (2)");
    glPopMatrix();
    CHECK_GL_ERROR("DrawCircle", "glPopMatrix");
}

void GLACanvas::DrawSemicircle(gla_colour edge, gla_colour fill,
			       glaCoord cx, glaCoord cy,
			       glaCoord radius, glaCoord start)
{
    // Draw a filled semicircle with an edge.
    // The semicircle extends from "start" deg to "start"+180 deg (increasing
    // clockwise, 0 deg upwards).
    SetColour(fill);
    glMatrixMode(GL_MODELVIEW);
    CHECK_GL_ERROR("DrawSemicircle", "glMatrixMode");
    glPushMatrix();
    CHECK_GL_ERROR("DrawSemicircle", "glPushMatrix");
    glTranslated(cx, cy, 0.0);
    CHECK_GL_ERROR("DrawSemicircle", "glTranslated");
    assert(m_Quadric);
    gluPartialDisk(m_Quadric, 0.0, radius, 36, 1, start, 180.0);
    CHECK_GL_ERROR("DrawSemicircle", "gluPartialDisk");
    SetColour(edge);
    gluPartialDisk(m_Quadric, radius - 1.0, radius, 36, 1, start, 180.0);
    CHECK_GL_ERROR("DrawSemicircle", "gluPartialDisk (2)");
    glPopMatrix();
    CHECK_GL_ERROR("DrawSemicircle", "glPopMatrix");
}

void
GLACanvas::DrawTriangle(gla_colour edge, gla_colour fill,
			const Vector3 &p0, const Vector3 &p1, const Vector3 &p2)
{
    // Draw a filled triangle with an edge.

    SetColour(fill);
    BeginTriangles();
    PlaceIndicatorVertex(p0.GetX(), p0.GetY());
    PlaceIndicatorVertex(p1.GetX(), p1.GetY());
    PlaceIndicatorVertex(p2.GetX(), p2.GetY());
    EndTriangles();

    SetColour(edge);
    glBegin(GL_LINE_STRIP);
    PlaceIndicatorVertex(p0.GetX(), p0.GetY());
    PlaceIndicatorVertex(p1.GetX(), p1.GetY());
    PlaceIndicatorVertex(p2.GetX(), p2.GetY());
    glEnd();
    CHECK_GL_ERROR("DrawTriangle", "glEnd GL_LINE_STRIP");
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

bool GLACanvas::Transform(const Vector3 & v,
			  double* x_out, double* y_out, double* z_out) const
{
    // Convert from data coordinates to screen coordinates.

    // Perform the projection.
    return gluProject(v.GetX(), v.GetY(), v.GetZ(),
		      modelview_matrix, projection_matrix, viewport,
		      x_out, y_out, z_out);
}

void GLACanvas::ReverseTransform(Double x, Double y,
				 double* x_out, double* y_out, double* z_out) const
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

void GLACanvas::ToggleSmoothShading()
{
    m_SmoothShading = !m_SmoothShading;
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
	wxString texture(wmsg_cfgpth());
	texture += wxCONFIG_PATH_SEPARATOR;
	texture += wxT("images");
	texture += wxCONFIG_PATH_SEPARATOR;
	texture += wxT("texture.png");
	if (!img.LoadFile(texture, wxBITMAP_TYPE_PNG)) {
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

bool GLACanvas::SaveScreenshot(const wxString & fnm, wxBitmapType type) const
{
    const int width = x_size;
    const int height = y_size;
    unsigned char *pixels = (unsigned char *)malloc(3 * width * (height + 1));
    if (!pixels) return false;
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid *)pixels);
    CHECK_GL_ERROR("SaveScreenshot", "glReadPixels");
    unsigned char * tmp_row = pixels + 3 * width * height;
    // We need to flip the image vertically - this approach should be more
    // efficient than using wxImage::Mirror(false) as that creates a new
    // wxImage object.
    for (int y = height / 2 - 1; y >= 0; --y) {
	unsigned char * upper = pixels + 3 * width * y;
	unsigned char * lower = pixels + 3 * width * (height - y - 1);
	memcpy(tmp_row, upper, 3 * width);
	memcpy(upper, lower, 3 * width);
	memcpy(lower, tmp_row, 3 * width);
    }
    // NB wxImage constructor calls free(pixels) for us.
    wxImage grab(width, height, pixels);
    return grab.SaveFile(fnm, type);
}

bool GLACanvas::CheckVisualFidelity(const unsigned char * target) const
{
    unsigned char pixels[3 * 8 * 8];
    if (double_buffered) {
	glReadBuffer(GL_BACK);
	CHECK_GL_ERROR("FirstShow", "glReadBuffer");
    }
    glReadPixels(x_size / 2 - 4, y_size / 2 - 5, 8, 8,
		 GL_RGB, GL_UNSIGNED_BYTE, (GLvoid *)pixels);
    CHECK_GL_ERROR("CheckVisualFidelity", "glReadPixels");
    if (double_buffered) {
	glReadBuffer(GL_FRONT);
	CHECK_GL_ERROR("FirstShow", "glReadBuffer");
    }
#if 0
    // Show what got drawn and what was expected for debugging.
    for (int y = 0; y < 8; ++y) {
	for (int x = 0; x < 8; ++x) {
	    int o = (y * 8 + x) * 3;
	    printf("%c", pixels[o] ? 'X' : '.');
	}
	printf(" ");
	for (int x = 0; x < 8; ++x) {
	    int o = (y * 8 + x) * 3;
	    printf("%c", target[o] ? 'X' : '.');
	}
	printf("\n");
    }
#endif
    return (memcmp(pixels, target, sizeof(pixels)) == 0);
}

void GLACanvas::ReadPixels(int width, int height, unsigned char * buf) const
{
    CHECK_GL_ERROR("ReadPixels", "glReadPixels");
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid *)buf);
}

void GLACanvas::PolygonOffset(bool on) const
{
    if (on) {
	glPolygonOffset(1.0, 1.0);
	glEnable(GL_POLYGON_OFFSET_FILL);
    } else {
	glDisable(GL_POLYGON_OFFSET_FILL);
    }
}
