/***************************************************************************
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef DRAWINGGUI_VIEWPROVIDERTEMPLATE_H
#define DRAWINGGUI_VIEWPROVIDERTEMPLATE_H

#include <Gui/ViewProviderFeature.h>

namespace TechDraw{
    class DrawTemplate;
}

namespace TechDrawGui {
class QGITemplate;
class MDIViewPage;

class TechDrawGuiExport ViewProviderTemplate : public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDrawGui::ViewProviderTemplate);

public:
    /// constructor
    ViewProviderTemplate();
    /// destructor
    virtual ~ViewProviderTemplate();

    virtual void attach(App::DocumentObject *) override;
    virtual void setDisplayMode(const char* ModeName) override;
    virtual bool useNewSelectionModel(void) const override {return false;}
    /// returns a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const override;
    virtual void updateData(const App::Property*) override;
    virtual void onChanged(const App::Property *prop) override;
    virtual void hide(void) override;
    virtual void show(void) override;
    virtual bool isShow(void) const override;
    QGITemplate* getQTemplate(void);
    TechDraw::DrawTemplate* getTemplate() const;
    MDIViewPage* getMDIViewPage(void) const;
    virtual Gui::MDIView *getMDIView() const override;

    void setMarkers(bool state);
};

} // namespace TechDrawGui


#endif // DRAWINGGUI_VIEWPROVIDERTEMPLATE_H

