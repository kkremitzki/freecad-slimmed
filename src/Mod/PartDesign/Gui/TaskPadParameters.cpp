/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
# include <sstream>
# include <QRegExp>
# include <QTextStream>
# include <Precision.hxx>
#endif

#include "ui_TaskPadParameters.h"
#include "TaskPadParameters.h"
#include <App/Application.h>
#include <App/Document.h>
#include <App/Origin.h>
#include <Base/Console.h>
#include <Base/UnitsApi.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeaturePad.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include "ReferenceSelection.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskPadParameters */

TaskPadParameters::TaskPadParameters(ViewProviderPad *PadView, QWidget *parent, bool newObj)
    : TaskExtrudeParameters(PadView, parent, "PartDesign_Pad", tr("Pad parameters"))
{
    ui->offsetEdit->setToolTip(tr("Offset from face at which pad will end"));
    ui->checkBoxReversed->setToolTip(tr("Reverses pad direction"));

    // set the history path
    ui->lengthEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PadLength"));
    ui->lengthEdit2->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PadLength2"));
    ui->offsetEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PadOffset"));

    setupDialog(newObj);
}

TaskPadParameters::~TaskPadParameters()
{
}

void TaskPadParameters::translateModeList(int index)
{
    ui->changeMode->clear();
    ui->changeMode->addItem(tr("Dimension"));
    ui->changeMode->addItem(tr("To last"));
    ui->changeMode->addItem(tr("To first"));
    ui->changeMode->addItem(tr("Up to face"));
    ui->changeMode->addItem(tr("Two dimensions"));
    ui->changeMode->setCurrentIndex(index);
}

void TaskPadParameters::updateUI(int index)
{
    // update direction combobox
    fillDirectionCombo();
    
    // disable/hide everything unless we are sure we don't need it
    // exception: the direction parameters are in any case visible
    bool isLengthEditVisible  = false;
    bool isLengthEdit2Visible = false;
    bool isOffsetEditVisible  = false;
    bool isMidplaneEnabled    = false;
    bool isMidplaneVisible    = false;
    bool isReversedEnabled    = false;
    bool isReversedVisible    = false;
    bool isFaceEditEnabled    = false;

    // dimension
    if (index == 0) {
        isLengthEditVisible = true;
        ui->lengthEdit->selectNumber();
        // Make sure that the spin box has the focus to get key events
        // Calling setFocus() directly doesn't work because the spin box is not
        // yet visible.
        QMetaObject::invokeMethod(ui->lengthEdit, "setFocus", Qt::QueuedConnection);
        isMidplaneEnabled = !ui->checkBoxReversed->isChecked();
        isMidplaneVisible = true;
        // Reverse only makes sense if Midplane is not true
        isReversedEnabled = !ui->checkBoxMidplane->isChecked();
        isReversedVisible = true;
    }
    // up to first/last
    else if (index == 1 || index == 2) {
        isOffsetEditVisible = true;
        isReversedEnabled = true;
        isReversedVisible = true;
    }
    // up to face
    else if (index == 3) {
        isOffsetEditVisible = true;
        isFaceEditEnabled   = true;
        QMetaObject::invokeMethod(ui->lineFaceName, "setFocus", Qt::QueuedConnection);
        // Go into reference selection mode if no face has been selected yet
        if (ui->lineFaceName->property("FeatureName").isNull())
            onButtonFace(true);
        isReversedEnabled = true;
        isReversedVisible = true;
    }
    // two dimensions
    else {
        isLengthEditVisible  = true;
        isLengthEdit2Visible = true;
        isMidplaneEnabled    = !ui->checkBoxReversed->isChecked();
        isMidplaneVisible    = true;
        isReversedEnabled    = !ui->checkBoxMidplane->isChecked();
        isReversedVisible    = true;
    }

    ui->lengthEdit->setVisible( isLengthEditVisible );
    ui->lengthEdit->setEnabled( isLengthEditVisible );
    ui->labelLength->setVisible( isLengthEditVisible );
    ui->checkBoxAlongDirection->setVisible( isLengthEditVisible );

    ui->offsetEdit->setVisible( isOffsetEditVisible );
    ui->offsetEdit->setEnabled( isOffsetEditVisible );
    ui->labelOffset->setVisible( isOffsetEditVisible );

    ui->checkBoxMidplane->setEnabled( isMidplaneEnabled );
    ui->checkBoxMidplane->setVisible( isMidplaneVisible );

    ui->checkBoxReversed->setEnabled( isReversedEnabled );
    ui->checkBoxReversed->setVisible( isReversedVisible );

    ui->lengthEdit2->setVisible( isLengthEdit2Visible );
    ui->lengthEdit2->setEnabled( isLengthEdit2Visible );
    ui->labelLength2->setVisible( isLengthEdit2Visible );

    ui->buttonFace->setEnabled( isFaceEditEnabled );
    ui->lineFaceName->setEnabled( isFaceEditEnabled );
    if (!isFaceEditEnabled) {
        onButtonFace(false);
    }
}

void TaskPadParameters::onModeChanged(int index)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(vp->getObject());

    switch (index) {
        case 0:
            pcPad->Type.setValue("Length");
            // Avoid error message
            if (ui->lengthEdit->value() < Base::Quantity(Precision::Confusion(), Base::Unit::Length))
                ui->lengthEdit->setValue(5.0);
            break;
        case 1: pcPad->Type.setValue("UpToLast"); break;
        case 2: pcPad->Type.setValue("UpToFirst"); break;
        case 3: pcPad->Type.setValue("UpToFace"); break;
        default: pcPad->Type.setValue("TwoLengths");
    }

    updateUI(index);
    recomputeFeature();
}

void TaskPadParameters::apply()
{
    auto obj = vp->getObject();

    ui->lengthEdit->apply();
    ui->lengthEdit2->apply();
    FCMD_OBJ_CMD(obj, "UseCustomVector = " << (getCustom() ? 1 : 0));
    FCMD_OBJ_CMD(obj, "Direction = ("
        << getXDirection() << ", " << getYDirection() << ", " << getZDirection() << ")");
    FCMD_OBJ_CMD(obj, "ReferenceAxis = " << getReferenceAxis());
    FCMD_OBJ_CMD(obj, "AlongSketchNormal = " << (getAlongSketchNormal() ? 1 : 0));
    FCMD_OBJ_CMD(obj,"Type = " << getMode());
    QString facename = getFaceName();
    FCMD_OBJ_CMD(obj,"UpToFace = " << facename.toLatin1().data());
    FCMD_OBJ_CMD(obj,"Reversed = " << (getReversed()?1:0));
    FCMD_OBJ_CMD(obj,"Midplane = " << (getMidplane()?1:0));
    FCMD_OBJ_CMD(obj,"Offset = " << getOffset());
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgPadParameters::TaskDlgPadParameters(ViewProviderPad *PadView, bool /*newObj*/)
    : TaskDlgSketchBasedParameters(PadView)
{
    assert(vp);
    Content.push_back ( new TaskPadParameters(PadView ) );
}

//==== calls from the TaskView ===============================================================

#include "moc_TaskPadParameters.cpp"
