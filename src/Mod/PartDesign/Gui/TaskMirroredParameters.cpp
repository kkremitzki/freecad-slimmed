/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
# include <QMessageBox>
#endif

#include "ui_TaskMirroredParameters.h"
#include "TaskMirroredParameters.h"
#include "TaskMultiTransformParameters.h"
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/PartDesign/App/FeatureMirrored.h>
#include <Mod/Sketcher/App/SketchObject.h>


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskMirroredParameters */

TaskMirroredParameters::TaskMirroredParameters(ViewProviderTransformed *TransformedView, QWidget *parent)
        : TaskTransformedParameters(TransformedView, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskMirroredParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    ui->buttonOK->hide();
    ui->checkBoxUpdateView->setEnabled(true);

    referenceSelectionMode = false;

    updateUIinProgress = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

TaskMirroredParameters::TaskMirroredParameters(TaskMultiTransformParameters *parentTask, QLayout *layout)
        : TaskTransformedParameters(parentTask)
{
    proxy = new QWidget(parentTask);
    ui = new Ui_TaskMirroredParameters();
    ui->setupUi(proxy);
    connect(ui->buttonOK, SIGNAL(pressed()),
            parentTask, SLOT(onSubTaskButtonOK()));
    QMetaObject::connectSlotsByName(this);

    layout->addWidget(proxy);

    ui->buttonOK->setEnabled(true);
    ui->labelOriginal->hide();
    ui->lineOriginal->hide();
    ui->checkBoxUpdateView->hide();

    referenceSelectionMode = false;

    updateUIinProgress = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

void TaskMirroredParameters::setupUI()
{
    connect(ui->buttonXY, SIGNAL(pressed()),
            this, SLOT(onButtonXY()));
    connect(ui->buttonXZ, SIGNAL(pressed()),
            this, SLOT(onButtonXZ()));
    connect(ui->buttonYZ, SIGNAL(pressed()),
            this, SLOT(onButtonYZ()));
    connect(ui->buttonReference, SIGNAL(toggled(bool)),
            this, SLOT(onButtonReference(bool)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    // Get the feature data
    PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
    std::vector<App::DocumentObject*> originals = pcMirrored->Originals.getValues();

    // Fill data into dialog elements
    ui->lineOriginal->setEnabled(false);
    for (std::vector<App::DocumentObject*>::const_iterator i = originals.begin(); i != originals.end(); i++)
    {
        if ((*i) != NULL) { // find the first valid original
            ui->lineOriginal->setText(QString::fromAscii((*i)->getNameInDocument()));
            break;
        }
    }
    // ---------------------

    ui->buttonXY->setEnabled(true);
    ui->buttonXZ->setEnabled(true);
    ui->buttonYZ->setEnabled(true);
    ui->buttonReference->setEnabled(true);
    ui->lineReference->setEnabled(false); // This is never enabled since it is for optical feed-back only
    updateUI();
}

void TaskMirroredParameters::updateUI()
{
    if (updateUIinProgress)
        return;
    updateUIinProgress = true;
    PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
    App::DocumentObject* mirrorPlaneFeature = pcMirrored->MirrorPlane.getValue();
    std::vector<std::string> mirrorPlanes = pcMirrored->MirrorPlane.getSubValues();
    std::string stdMirrorPlane = pcMirrored->StdMirrorPlane.getValue();

    ui->buttonReference->setChecked(referenceSelectionMode);
    if (!stdMirrorPlane.empty())
    {
        ui->buttonXY->setAutoExclusive(true);
        ui->buttonXZ->setAutoExclusive(true);
        ui->buttonYZ->setAutoExclusive(true);
        ui->buttonXY->setChecked(stdMirrorPlane == "XY");
        ui->buttonXZ->setChecked(stdMirrorPlane == "XZ");
        ui->buttonYZ->setChecked(stdMirrorPlane == "YZ");
        ui->lineReference->setText(tr(""));
    } else if (mirrorPlaneFeature != NULL && !mirrorPlanes.empty()) {
        ui->buttonXY->setAutoExclusive(false);
        ui->buttonXZ->setAutoExclusive(false);
        ui->buttonYZ->setAutoExclusive(false);
        ui->buttonXY->setChecked(false);
        ui->buttonXZ->setChecked(false);
        ui->buttonYZ->setChecked(false);
        ui->lineReference->setText(QString::fromAscii(mirrorPlanes.front().c_str()));
    } else {
        // Error message?
        ui->lineReference->setText(tr(""));
    }
    if (referenceSelectionMode)
        ui->lineReference->setText(tr("Select a plane"));

    updateUIinProgress = false;
}

void TaskMirroredParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        std::string subName(msg.pSubName);
        PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
        if (originalSelectionMode) {
            App::DocumentObject* selectedObject = pcMirrored->getDocument()->getActiveObject();
            if ((selectedObject == NULL) || !selectedObject->isDerivedFrom(Part::Feature::getClassTypeId()))
                return;
            if (originalSelected(msg))
                ui->lineOriginal->setText(QString::fromAscii(selectedObject->getNameInDocument()));
        } else if (referenceSelectionMode &&
                   (subName.size() > 4 && subName.substr(0,4) == "Face")) {

            std::vector<std::string> mirrorPlanes;
            mirrorPlanes.push_back(subName.c_str());
            pcMirrored->MirrorPlane.setValue(getOriginalObject(), mirrorPlanes);
            pcMirrored->StdMirrorPlane.setValue("");

            if (updateView())
                recomputeFeature();

            exitSelectionMode();
            updateUI();
        }
    }
}

void TaskMirroredParameters::onStdMirrorPlane(const std::string &plane) {
    if (updateUIinProgress)
        return;
    PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
    pcMirrored->StdMirrorPlane.setValue(plane.c_str());
    pcMirrored->MirrorPlane.setValue(NULL);

    exitSelectionMode();
    updateUI();
    if (updateView())
        recomputeFeature();
}

void TaskMirroredParameters::onButtonXY() {
    onStdMirrorPlane("XY");
}

void TaskMirroredParameters::onButtonXZ() {
    onStdMirrorPlane("XZ");
}

void TaskMirroredParameters::onButtonYZ() {
    onStdMirrorPlane("YZ");
}

void TaskMirroredParameters::onButtonReference(bool checked)
{
    if (updateUIinProgress)
        return;
    if (checked ) {
        hideObject();
        showOriginals();
        referenceSelectionMode = true;
        Gui::Selection().clearSelection();
    } else {
        exitSelectionMode();
    }
    updateUI();
}

void TaskMirroredParameters::onUpdateView(bool on)
{
    ui->buttonXY->blockSignals(!on);
    ui->buttonYZ->blockSignals(!on);
    ui->buttonXZ->blockSignals(!on);
}

const std::string TaskMirroredParameters::getStdMirrorPlane(void) const
{
    std::string stdMirrorPlane;

    if (ui->buttonXY->isChecked())
        stdMirrorPlane = "XY";
    else if (ui->buttonYZ->isChecked())
        stdMirrorPlane = "YZ";
    else if (ui->buttonXZ->isChecked())
        stdMirrorPlane = "XZ";

    if (!stdMirrorPlane.empty())
        return std::string("\"") + stdMirrorPlane + "\"";
    else
        return std::string("");
}

const QString TaskMirroredParameters::getMirrorPlane(void) const
{
    PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
    App::DocumentObject* feature = pcMirrored->MirrorPlane.getValue();
    if (feature == NULL)
        return QString::fromUtf8("");
    std::vector<std::string> mirrorPlanes = pcMirrored->MirrorPlane.getSubValues();
    QString buf;

    if ((feature != NULL) && !mirrorPlanes.empty()) {
        buf = QString::fromUtf8("(App.ActiveDocument.%1,[\"%2\"])");
        buf = buf.arg(QString::fromUtf8(feature->getNameInDocument()));
        buf = buf.arg(QString::fromUtf8(mirrorPlanes.front().c_str()));
    }
    else
        buf = QString::fromUtf8("");

    return buf;
}

const bool TaskMirroredParameters::updateView() const
{
    if (insideMultiTransform)
        return parentTask->updateView();
    else
        return ui->checkBoxUpdateView->isChecked();
}

void TaskMirroredParameters::exitSelectionMode()
{
    originalSelectionMode = false;
    referenceSelectionMode = false;
    showObject();
    hideOriginals();
}


TaskMirroredParameters::~TaskMirroredParameters()
{
    delete ui;
    if (proxy)
        delete proxy;
}

void TaskMirroredParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgMirroredParameters::TaskDlgMirroredParameters(ViewProviderMirrored *MirroredView)
    : TaskDlgTransformedParameters(MirroredView)
{
    parameter = new TaskMirroredParameters(MirroredView);

    Content.push_back(parameter);
}
//==== calls from the TaskView ===============================================================

bool TaskDlgMirroredParameters::accept()
{
    std::string name = TransformedView->getObject()->getNameInDocument();

    try {
        //Gui::Command::openCommand("Mirrored changed");
        // Handle Originals
        if (!TaskDlgTransformedParameters::accept())
            return false;

        TaskMirroredParameters* mirrorParameter = static_cast<TaskMirroredParameters*>(parameter);
        std::string mirrorPlane = mirrorParameter->getMirrorPlane().toStdString();
        if (!mirrorPlane.empty())
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.MirrorPlane = %s", name.c_str(), mirrorPlane.c_str());
        std::string stdMirrorPlane = mirrorParameter->getStdMirrorPlane();
        if (!stdMirrorPlane.empty())
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.StdMirrorPlane = %s",name.c_str(),stdMirrorPlane.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        if (!TransformedView->getObject()->isValid())
            throw Base::Exception(TransformedView->getObject()->getStatusString());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromAscii(e.what()));
        return false;
    }

    return true;
}

#include "moc_TaskMirroredParameters.cpp"
