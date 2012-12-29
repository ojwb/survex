/* printwx.h */
/* Device dependent part of Survex wxWidgets driver */
/* Copyright (C) 2004 Philip Underwood
 * Copyright (C) 2004,2005,2006,2011,2012 Olly Betts
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "wx.h"
#include <wx/dialog.h>

#include "avenprcore.h"

// The Gnome print dialog provides its own preview, as does Mac OS X.
#if !defined __WXMAC__ && !defined wxUSE_LIBGNOMEPRINT
# define AVEN_PRINT_PREVIEW
#endif

class MainFrm;
class wxComboBox;
class wxStaticText;
class wxSpinCtrl;
class wxCheckBox;
class wxSpinEvent;

// This dialog is also use for Export as well as Print.
class svxPrintDlg : public wxDialog {
	layout m_layout;
	wxComboBox* m_scale;
	wxStaticText* m_printSize;
	wxSpinCtrl* m_bearing;
	wxSpinCtrl* m_tilt;
	wxCheckBox* m_legs;
	wxCheckBox* m_stations;
	wxCheckBox* m_names;
	wxCheckBox* m_xsect;
	wxCheckBox* m_walls;
	wxCheckBox* m_passages;
	wxCheckBox* m_borders;
//	wxCheckBox* m_blanks;
	wxCheckBox* m_infoBox;
	wxCheckBox* m_surface;
	wxString m_File;
	MainFrm* mainfrm;

	void LayoutToUI();
	void UIToLayout();
	void RecalcBounds();
	void SomethingChanged();
 public:
	svxPrintDlg(MainFrm* parent, const wxString & filename,
		    const wxString & title, const wxString & datestamp,
		    double angle, double tilt_angle,
		    bool labels, bool crosses, bool legs, bool surf,
		    bool tubes, bool printing);
	void OnPrint(wxCommandEvent& event);
	void OnExport(wxCommandEvent& event);
#ifdef AVEN_PRINT_PREVIEW
	void OnPreview(wxCommandEvent& event);
#endif
	void OnPlan(wxCommandEvent&);
	void OnElevation(wxCommandEvent&);
	void OnPlanUpdate(wxUpdateUIEvent& e);
	void OnElevationUpdate(wxUpdateUIEvent& e);
	void OnChangeSpin(wxSpinEvent& event);
	void OnChange(wxCommandEvent& event);
 private:
	DECLARE_EVENT_TABLE()
};
