/* printwx.h */
/* Device dependent part of Survex wxWindows driver */
/* Copyright (C) 2004 Philip Underwood
 * Copyright (C) 2004 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "wx.h"
#include <wx/dialog.h>

enum {
	svx_PRINT = 1200,
	svx_PREVIEW,
	svx_SCALE,
	svx_BEARING,
	svx_TILT,
	svx_LEGS,
	svx_STATIONS,
	svx_NAMES,
	svx_BORDERS,
	svx_BLANKS,
	svx_INFOBOX,
	svx_SCALEBAR,
	svx_PLAN,
	svx_ELEV
};

class MainFrm;
struct layout;
class wxComboBox;
class wxStaticText;
class wxSpinCtrl;
class wxCheckBox;
class wxSpinEvent;

class svxPrintDlg : public wxDialog {
protected:
	layout* m_layout;
	wxComboBox* m_scale;
	wxStaticText* m_printSize;
	wxStaticText* m_tilttext;
	wxSpinCtrl* m_bearing;
	wxSpinCtrl* m_tilt;
	wxCheckBox* m_legs;
	wxCheckBox* m_stations;
	wxCheckBox* m_names;
	wxCheckBox* m_borders;
//	wxCheckBox* m_blanks;
	wxCheckBox* m_infoBox;
	wxCheckBox* m_surface;
	wxString m_File;
	MainFrm* m_parent;

	void LayoutToUI();
	void UIToLayout();
	void RecalcBounds();
 public:
	svxPrintDlg(MainFrm* parent, const wxString & filename,
		    const wxString & title, const wxString & datestamp,
		    double angle, double tilt_angle,
		    bool labels, bool crosses, bool legs, bool surf);
	~svxPrintDlg();
	void OnPrint(wxCommandEvent& event); 
	void OnPreview(wxCommandEvent& event);
	void OnPlan(wxCommandEvent&);
	void OnElevation(wxCommandEvent&);
	void OnChangeSpin(wxSpinEvent& event); 
	void OnChange(wxCommandEvent& event); 
	void SomethingChanged();
 private:
	DECLARE_EVENT_TABLE()
};
