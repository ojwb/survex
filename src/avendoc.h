//
//  avendoc.h
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

#ifndef avendoc_h
#define avendoc_h

#include "wx.h"
#include "avendefs.h"
#include "img.h"
#include "aven.h"

#include <float.h>
#include <list>

using namespace std;

class PointInfo {
    friend class AvenDoc;
    double x, y, z;
    bool isLine; // false => move, true => draw line
    bool isSurface;

public:
    double GetX() const { return x; }
    double GetY() const { return y; }
    double GetZ() const { return z; }
    bool IsLine() const { return isLine; }
    bool IsSurface() const { return isSurface; }
};

class LabelInfo {
    friend class AvenDoc;
    double x, y, z;
    wxString text;

public:
    double GetX() const { return x; }
    double GetY() const { return y; }
    double GetZ() const { return z; }
    wxString GetText() const { return text; }
};

class AvenDoc : public wxDocument {
    DECLARE_DYNAMIC_CLASS(AvenDoc)

    list<PointInfo*>* m_Points;
    list<LabelInfo*> m_Labels;
    double m_XExt;
    double m_YExt;
    double m_ZExt;
    double m_ZMin;
    int m_NumLegs;
    int m_NumPoints;
    int m_NumCrosses;
    int m_NumExtraLegs;

    void ClearPointLists();
    bool LoadData(const wxString& file);
    void SortIntoDepthBands(list<PointInfo*>& points);
    void IntersectLineWithPlane(double x0, double y0, double z0,
				double x1, double y1, double z1,
				double z, double& x, double& y);
    double GetDepthBoundaryBetweenBands(int a, int b);
    int GetDepthColour(double z);
    void CentreDataset(double xmin, double ymin, double zmin);

public:
    AvenDoc();
    ~AvenDoc();

    bool OnOpenDocument(const wxString& file);

    double GetXExtent() { return m_XExt; }
    double GetYExtent() { return m_YExt; }
    double GetZExtent() { return m_ZExt; }
    double GetZMin()    { return m_ZMin; }

    int GetNumLegs()   { return m_NumLegs; }
    int GetNumPoints() { return m_NumPoints; }
    int GetNumCrosses() { return m_NumCrosses; }

    int GetNumDepthBands() { return NUM_DEPTH_COLOURS; }

    list<PointInfo*>::const_iterator GetPoints(int band) {
        assert(band >= 0 && band < NUM_DEPTH_COLOURS);
        return m_Points[band].begin();
    }

    list<LabelInfo*>::const_iterator GetLabels() {
        return m_Labels.begin();
    }

    list<PointInfo*>::const_iterator GetPointsEnd(int band) {
        assert(band >= 0 && band < NUM_DEPTH_COLOURS);
        return m_Points[band].end();
    }

    list<LabelInfo*>::const_iterator GetLabelsEnd() {
        return m_Labels.end();
    }

private:
    DECLARE_EVENT_TABLE()
};

#endif
