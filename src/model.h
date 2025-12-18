//
//  model.h
//
//  Cave survey model.
//
//  Copyright (C) 2000-2003,2005 Mark R. Shinwell
//  Copyright (C) 2001-2003,2004,2005,2006,2010,2011,2012,2013,2014,2015,2016,2018 Olly Betts
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

#ifndef model_h
#define model_h

#include <wx/wx.h>

#include "labelinfo.h"
#include "vector3.h"

#include <ctime>
#include <list>
#include <set>
#include <vector>

using namespace std;

class MainFrm;

class PointInfo : public Point {
    int date = -1;

public:
    PointInfo() : Point() { }
    explicit PointInfo(const img_point & pt) : Point(pt) { }
    PointInfo(const img_point & pt, int date_) : Point(pt), date(date_) { }
    PointInfo(const Point & p, int date_) : Point(p), date(date_) { }
    int GetDate() const { return date; }
};

class XSect {
    friend class MainFrm;
    const LabelInfo* stn;
    int date;
    double l, r, u, d;
    double right_bearing = 0.0;

public:
    XSect(const LabelInfo* stn_, int date_,
	  double l_, double r_, double u_, double d_)
	: stn(stn_), date(date_), l(l_), r(r_), u(u_), d(d_) { }
    double GetL() const { return l; }
    double GetR() const { return r; }
    double GetU() const { return u; }
    double GetD() const { return d; }
    double get_right_bearing() const { return right_bearing; }
    void set_right_bearing(double right_bearing_) {
	right_bearing = right_bearing_;
    }
    int GetDate() const { return date; }
    const wxString& GetLabel() const { return stn->GetText(); }
    const Point& GetPoint() const { return *stn; }
    double GetX() const { return stn->GetX(); }
    double GetY() const { return stn->GetY(); }
    double GetZ() const { return stn->GetZ(); }
    friend Vector3 operator-(const XSect& a, const XSect& b);
};

inline Vector3 operator-(const XSect& a, const XSect& b) {
    return *(a.stn) - *(b.stn);
}

class traverse : public vector<PointInfo> {
  public:
    int n_legs = 0;
    // Bitmask of img_FLAG_SURFACE, img_FLAG_SPLAY and img_FLAG_DUPLICATE.
    int flags = 0;
    // An img_STYLE_* value.
    int style = 0;
    double length = 0.0;
    enum { ERROR_3D = 0, ERROR_H = 1, ERROR_V = 2 };
    double errors[3] = {-1, -1, -1};
    wxString name;

    explicit
    traverse(const char* name_) : name(name_, wxConvUTF8) {
	if (name.empty() && !name_[0]) {
	    // If name isn't valid UTF-8 then this conversion will
	    // give an empty string.  In this case, assume that the
	    // label is CP1252 (the Microsoft superset of ISO8859-1).
	    static wxCSConv ConvCP1252(wxFONTENCODING_CP1252);
	    name = wxString(name_, ConvCP1252);
	    if (name.empty()) {
		// Or if that doesn't work (ConvCP1252 doesn't like
		// strings with some bytes in) let's just go for
		// ISO8859-1.
		name = wxString(name_, wxConvISO8859_1);
	    }
	}
    }
};

class SurveyFilter {
    std::set<wxString, std::greater<wxString>> filters;
    std::set<wxString, std::greater<wxString>> redundant_filters;
    // Default to the Survex standard separator - then a filter created before
    // the survey separator is known is likely to not need rebuilding.
    wxChar separator = '.';

  public:
    SurveyFilter() {}

    void add(const wxString& survey);

    void remove(const wxString& survey);

    void clear() { filters.clear(); redundant_filters.clear(); }

    bool empty() const { return filters.empty(); }

    void SetSeparator(wxChar separator_);

    bool CheckVisible(const wxString& name) const;
};

/// Cave model.
class Model {
    list<traverse> traverses[8];
    mutable list<vector<XSect>> tubes;

  private:
    list<LabelInfo*> m_Labels;

    Vector3 m_Ext;
    double m_DepthMin, m_DepthExt;
    int m_DateMin, m_DateExt;
    bool complete_dateinfo;
    int m_NumEntrances = 0;
    int m_NumFixedPts = 0;
    int m_NumExportedPts = 0;
    bool m_HasUndergroundLegs = false;
    bool m_HasSplays = false;
    bool m_HasDupes = false;
    bool m_HasSurfaceLegs = false;
    bool m_HasErrorInformation = false;
    bool m_IsExtendedElevation = false;
    mutable bool m_TubesPrepared = false;

    // Character separating survey levels (often '.')
    wxChar m_separator;

    wxString m_Title, m_cs_proj, m_DateStamp;

    time_t m_DateStamp_numeric;

    Vector3 m_Offset;

    // We lazily set the higher bits of LabelInfo::flags to a value to give us
    // the sort order we want via integer subtraction.  This is done the first
    // time this sort happens after loading a file.
    bool added_plot_order_keys = false;

    void do_prepare_tubes() const;

    void CentreDataset(const Vector3& vmin);

  public:
    int Load(const wxString& file, const wxString& prefix);

    const Vector3& GetExtent() const { return m_Ext; }

    const wxString& GetSurveyTitle() const { return m_Title; }

    const wxString& GetDateString() const { return m_DateStamp; }

    time_t GetDateStamp() const { return m_DateStamp_numeric; }

    double GetDepthExtent() const { return m_DepthExt; }
    double GetDepthMin() const { return m_DepthMin; }

    bool HasCompleteDateInfo() const { return complete_dateinfo; }
    int GetDateExtent() const { return m_DateExt; }
    int GetDateMin() const { return m_DateMin; }

    int GetNumFixedPts() const { return m_NumFixedPts; }
    int GetNumExportedPts() const { return m_NumExportedPts; }
    int GetNumEntrances() const { return m_NumEntrances; }

    bool HasUndergroundLegs() const { return m_HasUndergroundLegs; }
    bool HasSplays() const { return m_HasSplays; }
    bool HasDupes() const { return m_HasDupes; }
    bool HasSurfaceLegs() const { return m_HasSurfaceLegs; }
    bool HasTubes() const { return !tubes.empty(); }
    bool HasErrorInformation() const { return m_HasErrorInformation; }

    bool IsExtendedElevation() const { return m_IsExtendedElevation; }

    wxChar GetSeparator() const { return m_separator; }

    const wxString& GetCSProj() const { return m_cs_proj; }

    const Vector3& GetOffset() const { return m_Offset; }

    list<traverse>::const_iterator
    traverses_begin(unsigned flags, const SurveyFilter* filter) const {
	if (flags >= sizeof(traverses)) return traverses[0].end();
	auto it = traverses[flags].begin();
	if (filter) {
	    while (it != traverses[flags].end() &&
		   !filter->CheckVisible(it->name)) {
		++it;
	    }
	}
	return it;
    }

    list<traverse>::const_iterator
    traverses_next(unsigned flags, const SurveyFilter* filter,
		   list<traverse>::const_iterator it) const {
	++it;
	if (filter) {
	    while (it != traverses[flags].end() &&
		   !filter->CheckVisible(it->name)) {
		++it;
	    }
	}
	return it;
    }

    list<traverse>::const_iterator traverses_end(unsigned flags) const {
	if (flags >= sizeof(traverses)) flags = 0;
	return traverses[flags].end();
    }

    list<vector<XSect>>::const_iterator tubes_begin() const {
	prepare_tubes();
	return tubes.begin();
    }

    list<vector<XSect>>::const_iterator tubes_end() const {
	return tubes.end();
    }

    list<vector<XSect>>::iterator tubes_begin() {
	prepare_tubes();
	return tubes.begin();
    }

    list<vector<XSect>>::iterator tubes_end() {
	return tubes.end();
    }

    list<LabelInfo*>::const_iterator GetLabels() const {
	return m_Labels.begin();
    }

    list<LabelInfo*>::const_iterator GetLabelsEnd() const {
	return m_Labels.end();
    }

    list<LabelInfo*>::const_reverse_iterator GetRevLabels() const {
	return m_Labels.rbegin();
    }

    list<LabelInfo*>::const_reverse_iterator GetRevLabelsEnd() const {
	return m_Labels.rend();
    }

    list<LabelInfo*>::iterator GetLabelsNC() {
	return m_Labels.begin();
    }

    list<LabelInfo*>::iterator GetLabelsNCEnd() {
	return m_Labels.end();
    }

    void SortLabelsByName();

    void SortLabelsByPlotOrder();

    void prepare_tubes() const {
	if (!m_TubesPrepared) {
	    do_prepare_tubes();
	    m_TubesPrepared = true;
	}
    }
};

#endif
