//
//  model.cc
//
//  Cave survey model.
//
//  Copyright (C) 2000-2002,2005,2006 Mark R. Shinwell
//  Copyright (C) 2001-2003,2004,2005,2006,2010,2011,2012,2013,2014,2015,2016,2018,2019 Olly Betts
//  Copyright (C) 2005 Martin Green
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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "model.h"

#include "img_hosted.h"
#include "useful.h"

#include <cfloat>
#include <map>

using namespace std;

const static int img2aven_tab[] = {
#include "img2aven.h"
};

inline int
img2aven(int flags)
{
    flags &= (sizeof(img2aven_tab) / sizeof(img2aven_tab[0]) - 1);
    return img2aven_tab[flags];
}

int Model::Load(const wxString& file, const wxString& prefix)
{
    // Load the processed survey data.
    img* survey = img_read_stream_survey(wxFopen(file, wxT("rb")),
					 fclose,
					 file.c_str(),
					 prefix.utf8_str());
    if (!survey) {
	return img_error2msg(img_error());
    }

    m_IsExtendedElevation = survey->is_extended_elevation;

    // Create a list of all the leg vertices, counting them and finding the
    // extent of the survey at the same time.

    m_NumFixedPts = 0;
    m_NumExportedPts = 0;
    m_NumEntrances = 0;
    m_HasUndergroundLegs = false;
    m_HasSplays = false;
    m_HasDupes = false;
    m_HasSurfaceLegs = false;
    m_HasErrorInformation = false;

    // FIXME: discard existing presentation? ask user about saving if we do!

    // Delete any existing list entries.
    m_Labels.clear();

    double xmin = DBL_MAX;
    double xmax = -DBL_MAX;
    double ymin = DBL_MAX;
    double ymax = -DBL_MAX;
    double zmin = DBL_MAX;
    double zmax = -DBL_MAX;

    m_DepthMin = DBL_MAX;
    double depthmax = -DBL_MAX;

    m_DateMin = INT_MAX;
    int datemax = -1;
    complete_dateinfo = true;

    for (unsigned f = 0; f != sizeof(traverses) / sizeof(traverses[0]); ++f) {
	traverses[f].clear();
    }
    tubes.clear();

    // Ultimately we probably want different types (subclasses perhaps?) for
    // underground and surface data, so we don't need to store LRUD for surface
    // stuff.
    traverse * current_traverse = NULL;
    vector<XSect> * current_tube = NULL;

    map<wxString, LabelInfo *> labelmap;
    list<LabelInfo*>::const_iterator last_mapped_label = m_Labels.begin();

    int result;
    img_point prev_pt = {0,0,0};
    bool current_polyline_is_surface = false;
    int current_flags = 0;
    int current_style = 0;
    string current_label;
    bool pending_move = false;
    // When legs within a traverse have different surface/splay/duplicate
    // flags, we split it into contiguous traverses of each flag combination,
    // but we need to track these so we can assign the error statistics to all
    // of them.  So we keep counts of how many of each combination we've
    // generated for the current traverse.
    size_t n_traverses[8];
    memset(n_traverses, 0, sizeof(n_traverses));
    do {
#if 0
	if (++items % 200 == 0) {
	    long pos = ftell(survey->fh);
	    int progress = int((double(pos) / double(file_size)) * 100.0);
	    // SetProgress(progress);
	}
#endif

	img_point pt;
	result = img_read_item(survey, &pt);
	switch (result) {
	    case img_MOVE:
		memset(n_traverses, 0, sizeof(n_traverses));
		pending_move = true;
		prev_pt = pt;
		break;

	    case img_LINE: {
		// Update survey extents.
		if (pt.x < xmin) xmin = pt.x;
		if (pt.x > xmax) xmax = pt.x;
		if (pt.y < ymin) ymin = pt.y;
		if (pt.y > ymax) ymax = pt.y;
		if (pt.z < zmin) zmin = pt.z;
		if (pt.z > zmax) zmax = pt.z;

		int date = survey->days1;
		if (date != -1) {
		    date += (survey->days2 - date) / 2;
		    if (date < m_DateMin) m_DateMin = date;
		    if (date > datemax) datemax = date;
		} else {
		    complete_dateinfo = false;
		}

		int flags = survey->flags &
		    (img_FLAG_SURFACE|img_FLAG_SPLAY|img_FLAG_DUPLICATE);
		bool is_surface = (flags & img_FLAG_SURFACE);
		bool is_splay = (flags & img_FLAG_SPLAY);
		bool is_dupe = (flags & img_FLAG_DUPLICATE);

		if (!is_surface) {
		    if (pt.z < m_DepthMin) m_DepthMin = pt.z;
		    if (pt.z > depthmax) depthmax = pt.z;
		}
		if (is_splay)
		    m_HasSplays = true;
		if (is_dupe)
		    m_HasDupes = true;
		if (pending_move ||
		    current_flags != flags ||
		    current_label != survey->label ||
		    current_style != survey->style) {
		    if (!current_polyline_is_surface && current_traverse) {
			//FixLRUD(*current_traverse);
		    }

		    ++n_traverses[flags];
		    // Start new traverse (surface or underground).
		    if (is_surface) {
			m_HasSurfaceLegs = true;
		    } else {
			m_HasUndergroundLegs = true;
			// The previous point was at a surface->ug transition.
			if (current_polyline_is_surface) {
			    if (prev_pt.z < m_DepthMin) m_DepthMin = prev_pt.z;
			    if (prev_pt.z > depthmax) depthmax = prev_pt.z;
			}
		    }
		    traverses[flags].push_back(traverse(survey->label));
		    current_traverse = &traverses[flags].back();
		    current_traverse->flags = survey->flags;
		    current_traverse->style = survey->style;

		    current_polyline_is_surface = is_surface;
		    current_flags = flags;
		    current_label = survey->label;
		    current_style = survey->style;

		    if (pending_move) {
			// Update survey extents.  We only need to do this if
			// there's a pending move, since for a surface <->
			// underground transition, we'll already have handled
			// this point.
			if (prev_pt.x < xmin) xmin = prev_pt.x;
			if (prev_pt.x > xmax) xmax = prev_pt.x;
			if (prev_pt.y < ymin) ymin = prev_pt.y;
			if (prev_pt.y > ymax) ymax = prev_pt.y;
			if (prev_pt.z < zmin) zmin = prev_pt.z;
			if (prev_pt.z > zmax) zmax = prev_pt.z;
		    }

		    current_traverse->push_back(PointInfo(prev_pt));
		}

		current_traverse->push_back(PointInfo(pt, date));

		prev_pt = pt;
		pending_move = false;
		break;
	    }

	    case img_LABEL: {
		wxString s(survey->label, wxConvUTF8);
		if (s.empty()) {
		    // If label isn't valid UTF-8 then this conversion will
		    // give an empty string.  In this case, assume that the
		    // label is CP1252 (the Microsoft superset of ISO8859-1).
		    static wxCSConv ConvCP1252(wxFONTENCODING_CP1252);
		    s = wxString(survey->label, ConvCP1252);
		    if (s.empty()) {
			// Or if that doesn't work (ConvCP1252 doesn't like
			// strings with some bytes in) let's just go for
			// ISO8859-1.
			s = wxString(survey->label, wxConvISO8859_1);
		    }
		}
		int flags = img2aven(survey->flags);
		LabelInfo* label = new LabelInfo(pt, s, flags);
		if (label->IsEntrance()) {
		    m_NumEntrances++;
		}
		if (label->IsFixedPt()) {
		    m_NumFixedPts++;
		}
		if (label->IsExportedPt()) {
		    m_NumExportedPts++;
		}
		m_Labels.push_back(label);
		break;
	    }

	    case img_XSECT: {
		if (!current_tube) {
		    // Start new current_tube.
		    tubes.push_back(vector<XSect>());
		    current_tube = &tubes.back();
		}

		LabelInfo * lab;
		wxString label(survey->label, wxConvUTF8);
		map<wxString, LabelInfo *>::const_iterator p;
		p = labelmap.find(label);
		if (p != labelmap.end()) {
		    lab = p->second;
		} else {
		    // Initialise labelmap lazily - we may have no
		    // cross-sections.
		    list<LabelInfo*>::const_iterator i;
		    if (labelmap.empty()) {
			i = m_Labels.begin();
		    } else {
			i = last_mapped_label;
			++i;
		    }
		    while (i != m_Labels.end() && (*i)->GetText() != label) {
			labelmap[(*i)->GetText()] = *i;
			++i;
		    }
		    last_mapped_label = i;
		    if (i == m_Labels.end()) {
			// Unattached cross-section - ignore for now.
			printf("unattached cross-section\n");
			if (current_tube->size() <= 1)
			    tubes.resize(tubes.size() - 1);
			current_tube = NULL;
			if (!m_Labels.empty())
			    --last_mapped_label;
			break;
		    }
		    lab = *i;
		    labelmap[label] = lab;
		}

		int date = survey->days1;
		if (date != -1) {
		    date += (survey->days2 - date) / 2;
		    if (date < m_DateMin) m_DateMin = date;
		    if (date > datemax) datemax = date;
		}

		current_tube->emplace_back(lab, date, survey->l, survey->r, survey->u, survey->d);
		break;
	    }

	    case img_XSECT_END:
		// Finish off current_tube.
		// If there's only one cross-section in the tube, just
		// discard it for now.  FIXME: we should handle this
		// when we come to skinning the tubes.
		if (current_tube && current_tube->size() <= 1)
		    tubes.resize(tubes.size() - 1);
		current_tube = NULL;
		break;

	    case img_ERROR_INFO: {
		if (survey->E == 0.0) {
		    // Currently cavern doesn't spot all articulating traverses
		    // so we assume that any traverse with no error isn't part
		    // of a loop.  FIXME: fix cavern!
		    break;
		}
		m_HasErrorInformation = true;
		for (size_t f = 0; f != sizeof(traverses) / sizeof(traverses[0]); ++f) {
		    list<traverse>::reverse_iterator t = traverses[f].rbegin();
		    size_t n = n_traverses[f];
		    n_traverses[f] = 0;
		    while (n) {
			assert(t != traverses[f].rend());
			t->n_legs = survey->n_legs;
			t->length = survey->length;
			t->errors[traverse::ERROR_3D] = survey->E;
			t->errors[traverse::ERROR_H] = survey->H;
			t->errors[traverse::ERROR_V] = survey->V;
			--n;
			++t;
		    }
		}
		break;
	    }

	    case img_BAD: {
		m_Labels.clear();

		// FIXME: Do we need to reset all these? - Olly
		m_NumFixedPts = 0;
		m_NumExportedPts = 0;
		m_NumEntrances = 0;
		m_HasUndergroundLegs = false;
		m_HasSplays = false;
		m_HasSurfaceLegs = false;

		img_close(survey);

		return img_error2msg(img_error());
	    }

	    default:
		break;
	}
    } while (result != img_STOP);

    if (!current_polyline_is_surface && current_traverse) {
	//FixLRUD(*current_traverse);
    }

    // Finish off current_tube.
    // If there's only one cross-section in the tube, just
    // discard it for now.  FIXME: we should handle this
    // when we come to skinning the tubes.
    if (current_tube && current_tube->size() <= 1)
	tubes.resize(tubes.size() - 1);

    m_separator = survey->separator;
    m_Title = wxString(survey->title, wxConvUTF8);
    m_DateStamp_numeric = survey->datestamp_numeric;
    if (survey->cs) {
	m_cs_proj = wxString(survey->cs, wxConvUTF8);
    } else {
	m_cs_proj = wxString();
    }
    if (strcmp(survey->datestamp, "?") == 0) {
	/* TRANSLATORS: used a processed survey with no processing date/time info */
	m_DateStamp = wmsg(/*Date and time not available.*/108);
    } else if (survey->datestamp[0] == '@') {
	const struct tm * tm = localtime(&m_DateStamp_numeric);
	char buf[256];
	/* TRANSLATORS: This is the date format string used to timestamp .3d
	 * files internally.  Probably best to keep it the same for all
	 * translations. */
	strftime(buf, 256, msg(/*%a,%Y.%m.%d %H:%M:%S %Z*/107), tm);
	m_DateStamp = wxString(buf, wxConvUTF8);
    }
    if (m_DateStamp.empty()) {
	m_DateStamp = wxString(survey->datestamp, wxConvUTF8);
    }
    img_close(survey);

    // Check we've actually loaded some legs or stations!
    if (!m_HasUndergroundLegs && !m_HasSurfaceLegs && m_Labels.empty()) {
	return (/*No survey data in 3d file “%s”*/202);
    }

    if (traverses[0].empty() &&
	traverses[1].empty() &&
	traverses[2].empty() &&
	traverses[3].empty() &&
	traverses[4].empty() &&
	traverses[5].empty() &&
	traverses[6].empty() &&
	traverses[7].empty()) {
	// No legs, so get survey extents from stations
	list<LabelInfo*>::const_iterator i;
	for (i = m_Labels.begin(); i != m_Labels.end(); ++i) {
	    if ((*i)->GetX() < xmin) xmin = (*i)->GetX();
	    if ((*i)->GetX() > xmax) xmax = (*i)->GetX();
	    if ((*i)->GetY() < ymin) ymin = (*i)->GetY();
	    if ((*i)->GetY() > ymax) ymax = (*i)->GetY();
	    if ((*i)->GetZ() < zmin) zmin = (*i)->GetZ();
	    if ((*i)->GetZ() > zmax) zmax = (*i)->GetZ();
	}
    }

    m_Ext.assign(xmax - xmin, ymax - ymin, zmax - zmin);

    if (datemax < m_DateMin) m_DateMin = datemax;
    m_DateExt = datemax - m_DateMin;

    // Centre the dataset around the origin.
    CentreDataset(Vector3(xmin, ymin, zmin));

    if (depthmax < m_DepthMin) {
	m_DepthMin = 0;
	m_DepthExt = 0;
    } else {
	m_DepthExt = depthmax - m_DepthMin;
	m_DepthMin -= GetOffset().GetZ();
    }

#if 0
    printf("time to load = %.3f\n", (double)timer.Time());
#endif

    return 0; // OK
}

void Model::CentreDataset(const Vector3& vmin)
{
    // Centre the dataset around the origin.

    m_Offset = vmin + (m_Ext * 0.5);

    for (unsigned f = 0; f != sizeof(traverses) / sizeof(traverses[0]); ++f) {
	list<traverse>::iterator t = traverses[f].begin();
	while (t != traverses[f].end()) {
	    assert(t->size() > 1);
	    vector<PointInfo>::iterator pos = t->begin();
	    while (pos != t->end()) {
		Point & point = *pos++;
		point -= m_Offset;
	    }
	    ++t;
	}
    }

    list<LabelInfo*>::iterator lpos = m_Labels.begin();
    while (lpos != m_Labels.end()) {
	Point & point = **lpos++;
	point -= m_Offset;
    }
}

void
Model::do_prepare_tubes() const
{
    // Fill in "right_bearing" for each cross-section.
    for (auto&& tube : tubes) {
	assert(tube.size() > 1);
	Vector3 U[4];
	XSect* prev_pt_v = NULL;
	Vector3 last_right(1.0, 0.0, 0.0);

	vector<XSect>::iterator i = tube.begin();
	vector<XSect>::size_type segment = 0;
	while (i != tube.end()) {
	    // get the coordinates of this vertex
	    XSect & pt_v = *i++;

	    bool cover_end = false;

	    Vector3 right, up;

	    const Vector3 up_v(0.0, 0.0, 1.0);

	    if (segment == 0) {
		assert(i != tube.end());
		// first segment

		// get the coordinates of the next vertex
		const XSect & next_pt_v = *i;

		// calculate vector from this pt to the next one
		Vector3 leg_v = next_pt_v - pt_v;

		// obtain a vector in the LRUD plane
		right = leg_v * up_v;
		if (right.magnitude() == 0) {
		    right = last_right;
		    // Obtain a second vector in the LRUD plane,
		    // perpendicular to the first.
		    //up = right * leg_v;
		    up = up_v;
		} else {
		    last_right = right;
		    up = up_v;
		}

		cover_end = true;
	    } else if (segment + 1 == tube.size()) {
		// last segment

		// Calculate vector from the previous pt to this one.
		Vector3 leg_v = pt_v - *prev_pt_v;

		// Obtain a horizontal vector in the LRUD plane.
		right = leg_v * up_v;
		if (right.magnitude() == 0) {
		    right = Vector3(last_right.GetX(), last_right.GetY(), 0.0);
		    // Obtain a second vector in the LRUD plane,
		    // perpendicular to the first.
		    //up = right * leg_v;
		    up = up_v;
		} else {
		    last_right = right;
		    up = up_v;
		}

		cover_end = true;
	    } else {
		assert(i != tube.end());
		// Intermediate segment.

		// Get the coordinates of the next vertex.
		const XSect & next_pt_v = *i;

		// Calculate vectors from this vertex to the
		// next vertex, and from the previous vertex to
		// this one.
		Vector3 leg1_v = pt_v - *prev_pt_v;
		Vector3 leg2_v = next_pt_v - pt_v;

		// Obtain horizontal vectors perpendicular to
		// both legs, then normalise and average to get
		// a horizontal bisector.
		Vector3 r1 = leg1_v * up_v;
		Vector3 r2 = leg2_v * up_v;
		r1.normalise();
		r2.normalise();
		right = r1 + r2;
		if (right.magnitude() == 0) {
		    // This is the "mid-pitch" case...
		    right = last_right;
		}
		if (r1.magnitude() == 0) {
		    up = up_v;

		    // Rotate pitch section to minimise the
		    // "torsional stress" - FIXME: use
		    // triangles instead of rectangles?
		    int shift = 0;
		    double maxdotp = 0;

		    // Scale to unit vectors in the LRUD plane.
		    right.normalise();
		    up.normalise();
		    Vector3 vec = up - right;
		    for (int orient = 0; orient <= 3; ++orient) {
			Vector3 tmp = U[orient] - prev_pt_v->GetPoint();
			tmp.normalise();
			double dotp = dot(vec, tmp);
			if (dotp > maxdotp) {
			    maxdotp = dotp;
			    shift = orient;
			}
		    }
		    if (shift) {
			if (shift != 2) {
			    Vector3 temp(U[0]);
			    U[0] = U[shift];
			    U[shift] = U[2];
			    U[2] = U[shift ^ 2];
			    U[shift ^ 2] = temp;
			} else {
			    swap(U[0], U[2]);
			    swap(U[1], U[3]);
			}
		    }
#if 0
		    // Check that the above code actually permuted
		    // the vertices correctly.
		    shift = 0;
		    maxdotp = 0;
		    for (int j = 0; j <= 3; ++j) {
			Vector3 tmp = U[j] - *prev_pt_v;
			tmp.normalise();
			double dotp = dot(vec, tmp);
			if (dotp > maxdotp) {
			    maxdotp = dotp + 1e-6; // Add small tolerance to stop 45 degree offset cases being flagged...
			    shift = j;
			}
		    }
		    if (shift) {
			printf("New shift = %d!\n", shift);
			shift = 0;
			maxdotp = 0;
			for (int j = 0; j <= 3; ++j) {
			    Vector3 tmp = U[j] - *prev_pt_v;
			    tmp.normalise();
			    double dotp = dot(vec, tmp);
			    printf("    %d : %.8f\n", j, dotp);
			}
		    }
#endif
		} else {
		    up = up_v;
		}
		last_right = right;
	    }

	    // Scale to unit vectors in the LRUD plane.
	    right.normalise();
	    up.normalise();

	    double l = fabs(pt_v.GetL());
	    double r = fabs(pt_v.GetR());
	    double u = fabs(pt_v.GetU());
	    double d = fabs(pt_v.GetD());

	    // Produce coordinates of the corners of the LRUD "plane".
	    Vector3 v[4];
	    v[0] = pt_v.GetPoint() - right * l + up * u;
	    v[1] = pt_v.GetPoint() + right * r + up * u;
	    v[2] = pt_v.GetPoint() + right * r - up * d;
	    v[3] = pt_v.GetPoint() - right * l - up * d;

	    prev_pt_v = &pt_v;
	    U[0] = v[0];
	    U[1] = v[1];
	    U[2] = v[2];
	    U[3] = v[3];

	    // FIXME: Store rather than recomputing on each draw?
	    (void)cover_end;

	    pt_v.set_right_bearing(deg(atan2(right.GetX(), right.GetY())));

	    ++segment;
	}
    }
}

void
SurveyFilter::add(const wxString& name)
{
    auto it = filters.lower_bound(name);
    if (it != filters.end()) {
	// It's invalid to add a survey which is already present.
	assert(*it != name);
	// Check if a survey prefixing name is visible.
	if (name.StartsWith(*it) && name[it->size()] == separator) {
	    redundant_filters.insert(name);
	    return;
	}
    }
    while (it != filters.begin()) {
	--it;
	const wxString& s = *it;
	if (s.size() <= name.size()) break;
	if (s.StartsWith(name) && s[name.size()] == separator) {
	    redundant_filters.insert(s);
	    it = filters.erase(it);
	}
    }
    filters.insert(name);
}

void
SurveyFilter::remove(const wxString& name)
{
    if (filters.erase(name) == 0) {
	redundant_filters.erase(name);
	return;
    }
    if (redundant_filters.empty()) {
	return;
    }
    auto it = redundant_filters.upper_bound(name);
    while (it != redundant_filters.begin()) {
	--it;
	// Check if a survey prefixed by name should be made visible.
	const wxString& s = *it;
	if (s.size() <= name.size()) {
	    break;
	}
	if (!(s.StartsWith(name) && s[name.size()] == separator))
	    break;
	filters.insert(s);
	it = redundant_filters.erase(it);
    }
}

void
SurveyFilter::SetSeparator(wxChar separator_)
{
    if (separator_ == separator) return;

    separator = separator_;

    if (filters.empty()) {
	return;
    }

    // Move aside all the filters already set and re-add() them so they get
    // split into redundant_filters appropriately.
    std::set<wxString, std::greater<wxString>> old_filters;
    std::set<wxString, std::greater<wxString>> old_redundant_filters;
    swap(filters, old_filters);
    swap(redundant_filters, old_redundant_filters);
    for (auto& s : filters) {
	add(s);
    }
    for (auto& s : redundant_filters) {
	add(s);
    }
}

bool
SurveyFilter::CheckVisible(const wxString& name) const
{
    auto it = filters.lower_bound(name);
    if (it == filters.end()) {
	// There's no filter <= name so name is excluded.
	return false;
    }
    if (*it == name) {
	// Exact match.
	return true;
    }
    // Check if a survey prefixing name is visible.
    if (name.StartsWith(*it) && name[it->size()] == separator)
	return true;
    return false;
}
