/***************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net>        *
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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <QAction>
# include <QMenu>
# include <QMessageBox>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopExp.hxx>
#endif

#include "ViewProviderDraft.h"
#include "TaskDraftParameters.h"
#include <Mod/PartDesign/App/FeatureDraft.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>


using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDraft,PartDesignGui::ViewProvider)

ViewProviderDraft::ViewProviderDraft()
{
    sPixmap = "PartDesign_Draft.svg";
}

ViewProviderDraft::~ViewProviderDraft()
{
}


void ViewProviderDraft::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit draft"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartGui::ViewProviderPart::setupContextMenu(menu, receiver, member);
}

bool ViewProviderDraft::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
        // When double-clicking on the item for this fillet the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgDraftParameters *draftDlg = qobject_cast<TaskDlgDraftParameters *>(dlg);
        if (draftDlg && draftDlg->getDraftView() != this)
            draftDlg = 0; // another pad left open its task panel
        if (dlg && !draftDlg) {
            QMessageBox msgBox;
            msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
            msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes)
                Gui::Control().reject();
            else
                return false;
        }

        // clear the selection (convenience)
        Gui::Selection().clearSelection();

        // always change to PartDesign WB, remember where we come from
        oldWb = Gui::Command::assureWorkbench("PartDesignWorkbench");

        // start the edit dialog
        if (draftDlg)
            Gui::Control().showDialog(draftDlg);
        else
            Gui::Control().showDialog(new TaskDlgDraftParameters(this));

        return true;
    }
    else {
        return PartGui::ViewProviderPart::setEdit(ModNum);
    }
}

bool ViewProviderDraft::onDelete(const std::vector<std::string> &s)
{
    return ViewProvider::onDelete(s);
}

void ViewProviderDraft::highlightReferences(const bool on)
{
    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(getObject());
    Part::Feature* base = static_cast<Part::Feature*>(pcDraft->Base.getValue());
    if (base == NULL) return;
    PartGui::ViewProviderPart* vp = dynamic_cast<PartGui::ViewProviderPart*>(
                Gui::Application::Instance->getViewProvider(base));
    if (vp == NULL) return;

    if (on) {
        std::vector<std::string> SubVals = pcDraft->Base.getSubValuesStartsWith("Face");
        if (SubVals.size() == 0) return;

        TopTools_IndexedMapOfShape fMap;
        TopExp::MapShapes(base->Shape.getValue(), TopAbs_FACE, fMap);

        originalColors = vp->DiffuseColor.getValues();
        std::vector<App::Color> colors = originalColors;
        colors.resize(fMap.Extent(), ShapeColor.getValue());

        for (std::vector<std::string>::const_iterator f = SubVals.begin(); f != SubVals.end(); f++) {
            int idx = atoi(f->substr(4).c_str()) - 1;
            // TODO: Find a better colour
            colors[idx] = App::Color(0.2,1,0.2);
        }
        vp->DiffuseColor.setValues(colors);
    } else {
        vp->DiffuseColor.setValues(originalColors);
    }
}

