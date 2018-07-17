/* buttontaghandler.cc
 * Handle avenbutton tags
 *
 * Copyright (C) 2010 Olly Betts
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "aven.h"
#include <wx/html/htmlwin.h>
#include <wx/html/m_templ.h>
#include <wx/html/forcelnk.h>

FORCE_LINK_ME(buttontaghandler)

TAG_HANDLER_BEGIN(AVENBUTTON, "AVENBUTTON")
    TAG_HANDLER_PROC(tag) {
	int id;
	if (!tag.GetParamAsInt(wxT("id"), &id))
	    id = -1;
	wxHtmlContainerCell * cells = m_WParser->GetContainer();
	cells->SetAlignHor(wxHTML_ALIGN_RIGHT);
	wxWindow * win = m_WParser->GetWindowInterface()->GetHTMLWindow();
	wxButton * but = new wxButton(win, id, tag.GetParam(wxT("name")));
	if (tag.HasParam(wxT("default")))
	    but->SetDefault();
	cells->InsertCell(new wxHtmlWidgetCell(but));
	return false;
    }
TAG_HANDLER_END(TITLE)

TAGS_MODULE_BEGIN(CavernLog)
    TAGS_MODULE_ADD(AVENBUTTON)
TAGS_MODULE_END(CavernLog)
