//
//  avendoc.cc
//
//  Document class for Aven.
//
//  Copyright (C) 2001, Mark R. Shinwell.
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

#include "avendefs.h"
#include "avendoc.h"

IMPLEMENT_DYNAMIC_CLASS(AvenDoc, wxDocument)

BEGIN_EVENT_TABLE(AvenDoc, wxDocument)
END_EVENT_TABLE()

AvenDoc::AvenDoc()
{
    m_Points = new list<PointInfo*>[NUM_DEPTH_COLOURS+1];
}

AvenDoc::~AvenDoc()
{
    ClearPointLists();
    delete[] m_Points;
}

bool AvenDoc::LoadData(const wxString& file)
{
    // Load survey data from file, centre the dataset around the origin,
    // chop legs such that no legs cross depth colour boundaries and prepare
    // the data for drawing.

    // Load the survey data.

    img* survey = img_open(file, NULL, NULL);
    if (!survey) {
	wxString m = wxString::Format(msg(img_error()), file.c_str());
	wxGetApp().ReportError(m);
    }
    else {
        // Create a list of all the leg vertices, counting them and finding the
        // extent of the survey at the same time.
 
	m_NumLegs = 0;
	m_NumPoints = 0;
	m_NumExtraLegs = 0;
	m_NumCrosses = 0;

	// Delete any existing list entries.
	ClearPointLists();

	double xmin = DBL_MAX;
	double xmax = -DBL_MAX;
	double ymin = DBL_MAX;
	double ymax = -DBL_MAX;
	m_ZMin = DBL_MAX;
	double zmax = -DBL_MAX;

	bool first = true;
	bool last_was_move = false;

	list<PointInfo*> points;

	int result;
	do {
	    img_point pt;
	    char buffer[256];
	    switch (result = img_read_item(survey, buffer, &pt)) {
	        case img_MOVE:
	        case img_LINE:
		{
		    // Delete <move1> where we get ... <move1> <move2> ... in the data.
		    if (result == img_MOVE) {
		        if (last_was_move) {
			    PointInfo* pt = points.back();
			    delete pt;
			    points.pop_back();
			    m_NumPoints--;
			}
			last_was_move = true;
		    }
		    else {
		        last_was_move = false;
		    }

		    m_NumPoints++;

		    // We must ensure that the resulting list has a move as it's first
		    // datum.
		    if (first && (result != img_MOVE)) {
		        PointInfo* info = new PointInfo;
			info->x = 0.0;
			info->y = 0.0;
			info->z = 0.0;
			info->isLine = false;
			points.push_back(info);
			m_NumPoints++;
		    }
		    
		    // Update survey extents.
		    if (pt.x < xmin) xmin = pt.x;
		    if (pt.x > xmax) xmax = pt.x;
		    if (pt.y < ymin) ymin = pt.y;
		    if (pt.y > ymax) ymax = pt.y;
		    if (pt.z < m_ZMin) m_ZMin = pt.z;
		    if (pt.z > zmax) zmax = pt.z;

		    PointInfo* info = new PointInfo;
		    info->x = pt.x;
		    info->y = pt.y;
		    info->z = pt.z;

		    // Set flags if this datum is a line rather than a move; update number of
		    // legs if it's a line.
		    if (result == img_LINE) {
		        m_NumLegs++;
		        info->isLine = true;
		    }
		    else {
		        info->isLine = false;
		    }
		    
		    info->isSurface = (survey->flags & img_FLAG_SURFACE);

		    // Store this point in the list.
		    points.push_back(info);
		    
		    break;
		}

	        case img_LABEL:
		{
		    LabelInfo* label = new LabelInfo;
		    label->text = buffer;
		    label->x = pt.x;
		    label->y = pt.y;
		    label->z = pt.z;
		    m_Labels.push_back(label);
		    m_NumCrosses++;

		    // Update survey extents (just in case there are no legs).
		    if (pt.x < xmin) xmin = pt.x;
		    if (pt.x > xmax) xmax = pt.x;
		    if (pt.y < ymin) ymin = pt.y;
		    if (pt.y > ymax) ymax = pt.y;
		    if (pt.z < m_ZMin) m_ZMin = pt.z;
		    if (pt.z > zmax) zmax = pt.z;

		    break;
		}
		
	        default:
	            break;
	    }

	    first = false;

	} while (result != img_BAD && result != img_STOP);

	// Check we've actually loaded some legs or stations!
	if (m_NumLegs == 0 && m_Labels.empty()) {
	    wxString m = wxString::Format(msg(/*No survey data in 3d file `%s'*/202), file.c_str());
	    wxGetApp().ReportError(m);
	    return false;
	}

	// Delete any trailing move.
	PointInfo* pt = points.back();
	if (!pt->isLine) {
	    m_NumPoints--;
	    points.pop_back();
	    delete pt;
	}

	m_XExt = xmax - xmin;
	m_YExt = ymax - ymin;
	m_ZExt = zmax - m_ZMin;

	// Sort out depth colouring boundaries (before centering dataset!)
	SortIntoDepthBands(points);

	// Centre the dataset around the origin.
	CentreDataset(xmin, ymin, m_ZMin);
    }

    return (survey != NULL);
}

void AvenDoc::CentreDataset(double xmin, double ymin, double zmin)
{
    // Centre the dataset around the origin.

    double xoff = xmin + (m_XExt / 2.0f);
    double yoff = ymin + (m_YExt / 2.0f);
    double zoff = zmin + (m_ZExt / 2.0f);
    
    for (int band = 0; band < NUM_DEPTH_COLOURS + 1; band++) {
        list<PointInfo*>::iterator pos = m_Points[band].begin();
	list<PointInfo*>::iterator end = m_Points[band].end();
	while (pos != end) {
	    PointInfo* point = *pos++;
	    point->x -= xoff;
	    point->y -= yoff;
	    point->z -= zoff;
	}
    }

    list<LabelInfo*>::iterator pos = m_Labels.begin();
    while (pos != m_Labels.end()) {
        LabelInfo* label = *pos++;
	label->x -= xoff;
	label->y -= yoff;
	label->z -= zoff;
    }
}

int AvenDoc::GetDepthColour(double z)
{
    // Return the (0-based) depth colour band index for a z-coordinate.
    return int(((z - m_ZMin) / (m_ZExt == 0.0f ? 1.0f : m_ZExt)) * (NUM_DEPTH_COLOURS - 1));
}

double AvenDoc::GetDepthBoundaryBetweenBands(int a, int b)
{
    // Return the z-coordinate of the depth colour boundary between
    // two adjacent depth colour bands (specified by 0-based indices).

    assert((a == b - 1) || (a == b + 1));

    int band = (a > b) ? a : b; // boundary N lies on the bottom of band N.
    return m_ZMin + (m_ZExt * band / (NUM_DEPTH_COLOURS - 1));
}

void AvenDoc::IntersectLineWithPlane(double x0, double y0, double z0,
				     double x1, double y1, double z1,
				     double z, double& x, double& y)
{
    // Find the intersection point of the line (x0, y0, z0) -> (x1, y1, z1) with
    // the plane parallel to the xy-plane with z-axis intersection z.

    double t = (z - z0) / (z1 - z0);
    x = x0 + t*(x1 - x0);
    y = y0 + t*(y1 - y0);
}

void AvenDoc::SortIntoDepthBands(list<PointInfo*>& points)
{
    // Split legs which cross depth colouring boundaries and classify all
    // points into the correct depth bands.

    list<PointInfo*>::iterator pos = points.begin();
    PointInfo* prev_point = NULL;
    while (pos != points.end()) {
        PointInfo* point = *pos++;
	assert(point);

	// If this is a leg, then check if it intersects a depth
	// colour boundary.
	if (prev_point && point->isLine/* && !point->isSurface*/) {
	    int col1 = GetDepthColour(prev_point->z);
	    int col2 = GetDepthColour(point->z);
	    if (col1 != col2) {
	        // The leg does cross at least one boundary, so split it as
		// many times as required...
	        int inc = (col1 > col2) ? -1 : 1;
		for (int band = col1; band != col2; band += inc) {
		    int next_band = band + inc;

		    // Determine the z-coordinate of the boundary being intersected.
		    double split_at_z = GetDepthBoundaryBetweenBands(band, next_band);
		    double split_at_x, split_at_y;

		    // Find the coordinates of the intersection point.
		    IntersectLineWithPlane(prev_point->x, prev_point->y, prev_point->z,
					   point->x, point->y, point->z,
					   split_at_z, split_at_x, split_at_y);
		    
		    // Create a new leg only as far as this point.
		    PointInfo* info = new PointInfo;
		    info->x = split_at_x;
		    info->y = split_at_y;
		    info->z = split_at_z;
		    info->isLine = true;
		    m_Points[band].push_back(info);

		    // Create a move to this point in the next band.
		    info = new PointInfo;
		    info->x = split_at_x;
		    info->y = split_at_y;
		    info->z = split_at_z;
		    info->isLine = false;
		    m_Points[next_band].push_back(info);
		    
		    m_NumExtraLegs++;
		    m_NumPoints += 2;
		}
	    }

	    // Add the last point of the (possibly split) leg.
	    m_Points[col2].push_back(point);
	}
	else {
	    // The first point, a surface point, or another move: put it in the correct list
	    // according to depth.
	    assert(point->isSurface || !point->isLine);
	    int band = GetDepthColour(point->z);
	    m_Points[band].push_back(point);
	}

	prev_point = point;
    }
}

bool AvenDoc::OnOpenDocument(const wxString& file)
{
    // Open a 3D file.

  //SetCursor(*wxHOURGLASS_CURSOR); //-tbs

    bool success = LoadData(file);

    if (success) {
        SetFilename(file, true);
	Modify(false);
	UpdateAllViews();
    }

    //SetCursor(*wxSTANDARD_CURSOR);

    return success;
}

void AvenDoc::ClearPointLists()
{
    // Free memory occupied by the contents of the point and label lists.

    for (int band = 0; band < NUM_DEPTH_COLOURS + 1; band++) {
        list<PointInfo*>::iterator pos = m_Points[band].begin();
	list<PointInfo*>::iterator end = m_Points[band].end();
	while (pos != end) {
	    PointInfo* point = *pos++;
	    delete point;
	}
	m_Points[band].clear();
    }

    list<LabelInfo*>::iterator pos = m_Labels.begin();
    while (pos != m_Labels.end()) {
        LabelInfo* label = *pos++;
	delete label;
    }
    m_Labels.clear();
}
