/* printing.h */
/* Aven printing code */
/* Copyright (C) 2004 Philip Underwood
 * Copyright (C) 2004,2005,2006,2011,2012,2013,2014,2015 Olly Betts
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
#include <wx/spinctrl.h>

#include <time.h>

#include "avenprcore.h"

// The libgnomeprint and GtkPrint dialogs provide their own preview, as does
// Mac OS X.
#if !(defined wxUSE_LIBGNOMEPRINT && wxUSE_LIBGNOMEPRINT) && \
    !(defined wxUSE_GTKPRINT && wxUSE_GTKPRINT) && \
    !defined __WXMAC__
# define AVEN_PRINT_PREVIEW
#endif

class MainFrm;
class wxComboBox;
class wxStaticText;
class wxSpinCtrlDouble;
class wxSpinEvent;

// This dialog is also use for Export as well as Print.
class svxPrintDlg : public wxDialog {
	layout m_layout;
	wxComboBox* m_scale;
	wxBoxSizer* m_scalebox;
	wxBoxSizer* m_viewbox;
	wxChoice* m_format;
	wxStaticText* m_printSize;
	wxSpinCtrlDouble* m_bearing;
	wxSpinCtrlDouble* m_tilt;
//	wxCheckBox* m_blanks;
	wxString m_File;
	MainFrm* mainfrm;
	bool close_after;

	void LayoutToUI();
	void UIToLayout();
	void RecalcBounds();
	void SomethingChanged(int control_id);
 public:
	svxPrintDlg(MainFrm* parent, const wxString & filename,
		    const wxString & title, const wxString & cs_proj,
		    const wxString & datestamp, time_t datestamp_numeric,
		    double angle, double tilt_angle,
		    bool labels, bool crosses, bool legs, bool surf,
		    bool splays, bool tubes, bool ents, bool fixes,
		    bool exports, bool printing, bool close_after_ = false);

	~svxPrintDlg() {
	    if (close_after) mainfrm->Close();
	}

	void OnPrint(wxCommandEvent& event);
	void OnExport(wxCommandEvent& event);
#ifdef AVEN_PRINT_PREVIEW
	void OnPreview(wxCommandEvent& event);
#endif
	void OnPlan(wxCommandEvent&);
	void OnElevation(wxCommandEvent&);
	void OnPlanUpdate(wxUpdateUIEvent& e);
	void OnElevationUpdate(wxUpdateUIEvent& e);
	void OnChangeSpin(wxSpinDoubleEvent& event);
	void OnChange(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
 private:
	DECLARE_EVENT_TABLE()
};
