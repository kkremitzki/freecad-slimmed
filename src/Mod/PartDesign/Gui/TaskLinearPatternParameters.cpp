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

#include "ui_TaskLinearPatternParameters.h"
#include "TaskLinearPatternParameters.h"
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
#include <Mod/PartDesign/App/FeatureLinearPattern.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include "TaskMultiTransformParameters.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskLinearPatternParameters */

TaskLinearPatternParameters::TaskLinearPatternParameters(ViewProviderTransformed *TransformedView,QWidget *parent)
        : TaskTransformedParameters(TransformedView, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskLinearPatternParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    ui->buttonOK->hide();
    ui->checkBoxUpdateView->setEnabled(true);

    updateUIinProgress = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

TaskLinearPatternParameters::TaskLinearPatternParameters(TaskMultiTransformParameters *parentTask, QLayout *layout)
        : TaskTransformedParameters(parentTask)
{
    proxy = new QWidget(parentTask);
    ui = new Ui_TaskLinearPatternParameters();
    ui->setupUi(proxy);
    connect(ui->buttonOK, SIGNAL(pressed()),
            parentTask, SLOT(onSubTaskButtonOK()));
    QMetaObject::connectSlotsByName(this);

    layout->addWidget(proxy);

    ui->buttonOK->setEnabled(true);
    ui->listFeatures->hide();
    ui->checkBoxUpdateView->hide();

    updateUIinProgress = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

void TaskLinearPatternParameters::setupUI()
{
    connect(ui->buttonX, SIGNAL(pressed()),
            this, SLOT(onButtonX()));
    connect(ui->buttonY, SIGNAL(pressed()),
            this, SLOT(onButtonY()));
    connect(ui->buttonZ, SIGNAL(pressed()),
            this, SLOT(onButtonZ()));
    connect(ui->checkReverse, SIGNAL(toggled(bool)),
            this, SLOT(onCheckReverse(bool)));
    connect(ui->spinLength, SIGNAL(valueChanged(double)),
            this, SLOT(onLength(double)));
    connect(ui->spinOccurrences, SIGNAL(valueChanged(int)),
            this, SLOT(onOccurrences(int)));
    connect(ui->buttonReference, SIGNAL(pressed()),
            this, SLOT(onButtonReference()));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    // TODO: The following code could be generic in TaskTransformedParameters
    // if it were possible to make ui_TaskLinearPatternParameters a subclass of
    // ui_TaskTransformedParameters
    // ---------------------
    // Add a context menu to the listview of the originals to delete items
    QAction* action = new QAction(tr("Delete"), ui->listFeatures);
    action->connect(action, SIGNAL(triggered()),
                    this, SLOT(onOriginalDeleted()));
    ui->listFeatures->addAction(action);
    ui->listFeatures->setContextMenuPolicy(Qt::ActionsContextMenu);

    // Get the feature data
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    std::vector<App::DocumentObject*> originals = pcLinearPattern->Originals.getValues();

    // Fill data into dialog elements
    ui->listFeatures->setEnabled(true);
    ui->listFeatures->clear();
    for (std::vector<App::DocumentObject*>::const_iterator i = originals.begin(); i != originals.end(); i++)
    {
        if ((*i) != NULL)
            ui->listFeatures->addItem(QString::fromAscii((*i)->getNameInDocument()));
    }
    QMetaObject::invokeMethod(ui->listFeatures, "setFocus", Qt::QueuedConnection);
    // ---------------------

    ui->buttonX->setEnabled(true);
    ui->buttonY->setEnabled(true);
    ui->buttonZ->setEnabled(true);
    ui->checkReverse->setEnabled(true);
    ui->spinLength->setEnabled(true);
    ui->spinOccurrences->setEnabled(true);
    ui->buttonReference->setEnabled(true);
    ui->lineReference->setEnabled(false); // This is never enabled since it is for optical feed-back only
    updateUI();
}

void TaskLinearPatternParameters::updateUI()
{
    if (updateUIinProgress) return;
    updateUIinProgress = true;
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());

    App::DocumentObject* directionFeature = pcLinearPattern->Direction.getValue();
    std::vector<std::string> directions = pcLinearPattern->Direction.getSubValues();
    std::string stdDirection = pcLinearPattern->StdDirection.getValue();
    bool reverse = pcLinearPattern->Reversed.getValue();
    double length = pcLinearPattern->Length.getValue();
    unsigned occurrences = pcLinearPattern->Occurrences.getValue();

    if ((featureSelectionMode || insideMultiTransform) && !stdDirection.empty())
    {
        ui->buttonReference->setDown(false);
        ui->buttonX->setAutoExclusive(true);
        ui->buttonY->setAutoExclusive(true);
        ui->buttonZ->setAutoExclusive(true);
        ui->buttonX->setChecked(stdDirection == "X");
        ui->buttonY->setChecked(stdDirection == "Y");
        ui->buttonZ->setChecked(stdDirection == "Z");
        ui->lineReference->setText(tr(""));
    } else if ((directionFeature != NULL) && !directions.empty()) {
        ui->buttonX->setAutoExclusive(false);
        ui->buttonY->setAutoExclusive(false);
        ui->buttonZ->setAutoExclusive(false);
        ui->buttonX->setChecked(false);
        ui->buttonY->setChecked(false);
        ui->buttonZ->setChecked(false);
        ui->buttonReference->setDown(!featureSelectionMode);
        ui->lineReference->setText(QString::fromAscii(directions.front().c_str()));
    } else {
        // Error message?
        ui->lineReference->setText(tr(""));
    }

    // Note: These three lines would trigger onLength(), on Occurrences() and another updateUI() if we
    // didn't check for updateUIinProgress
    ui->checkReverse->setChecked(reverse);
    ui->spinLength->setValue(length);
    ui->spinOccurrences->setValue(occurrences);

    updateUIinProgress = false;
}

void TaskLinearPatternParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    App::DocumentObject* selectedObject = pcLinearPattern->getDocument()->getActiveObject();
    if ((selectedObject == NULL) || !selectedObject->isDerivedFrom(Part::Feature::getClassTypeId()))
        return;

    if (featureSelectionMode) {
        if (originalSelected(msg))
            ui->listFeatures->addItem(QString::fromAscii(selectedObject->getNameInDocument()));
    } else {
        if (!msg.pSubName || msg.pSubName[0] == '\0')
            return;

        std::string element(msg.pSubName);

        if (msg.Type == Gui::SelectionChanges::AddSelection) {
            // TODO
//            if (originalElementName == "") {
//                Base::Console().Error("Element created by this pattern cannot be used for direction\n");
//                return;
//            }

            std::vector<std::string> directions;
            directions.push_back(element.c_str());
            pcLinearPattern->Direction.setValue(getOriginalObject(), directions);

            if (insideMultiTransform) {
                if (parentTask->updateView())
                    recomputeFeature();
            } else
                if (ui->checkBoxUpdateView->isChecked())
                    recomputeFeature();

            if (!insideMultiTransform)
                featureSelectionMode = true; // Jump back to selection of originals

            showObject();
            hideOriginals();
            updateUI();
        }
    }
}

void TaskLinearPatternParameters::onOriginalDeleted()
{
    int row = ui->listFeatures->currentIndex().row();
    TaskTransformedParameters::onOriginalDeleted(row);
    ui->listFeatures->model()->removeRow(row);
}

void TaskLinearPatternParameters::onButtonX() {
    onStdDirection("X");
}

void TaskLinearPatternParameters::onButtonY() {
    onStdDirection("Y");
}

void TaskLinearPatternParameters::onButtonZ() {
    onStdDirection("Z");
}

void TaskLinearPatternParameters::onCheckReverse(const bool on) {
    if (updateUIinProgress) return;
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    pcLinearPattern->Reversed.setValue(on);
    updateUI();
    if (insideMultiTransform && !parentTask->updateView())
        return;
    recomputeFeature();
}

void TaskLinearPatternParameters::onLength(const double l) {
    if (updateUIinProgress) return;
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    pcLinearPattern->Length.setValue(l);
    updateUI();
    if (insideMultiTransform && !parentTask->updateView())
        return;
    recomputeFeature();
}

void TaskLinearPatternParameters::onOccurrences(const int n) {
    if (updateUIinProgress) return;
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    pcLinearPattern->Occurrences.setValue(n);
    updateUI();
    if (insideMultiTransform && !parentTask->updateView())
        return;
    recomputeFeature();
}

void TaskLinearPatternParameters::onStdDirection(const std::string& dir) {
    if (updateUIinProgress) return;
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    pcLinearPattern->StdDirection.setValue(dir.c_str());
    pcLinearPattern->Direction.setValue(NULL);
    if (!insideMultiTransform)
        featureSelectionMode = true;
    updateUI();
    if (insideMultiTransform && !parentTask->updateView())
        return;
    recomputeFeature();
}

void TaskLinearPatternParameters::onButtonReference()
{
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    pcLinearPattern->StdDirection.setValue("");
    featureSelectionMode = false;
    hideObject();
    showOriginals();
    updateUI();
}

void TaskLinearPatternParameters::onUpdateView(bool on)
{
    ui->buttonX->blockSignals(!on);
    ui->buttonY->blockSignals(!on);
    ui->buttonZ->blockSignals(!on);
    ui->listFeatures->blockSignals(!on);
    ui->checkReverse->blockSignals(!on);
    ui->spinLength->blockSignals(!on);
    ui->spinOccurrences->blockSignals(!on);
}

const std::string TaskLinearPatternParameters::getStdDirection(void) const
{
    std::string stdDirection;

    if (ui->buttonX->isChecked())
        stdDirection = "X";
    else if (ui->buttonY->isChecked())
        stdDirection = "Y";
    else if (ui->buttonZ->isChecked())
        stdDirection = "Z";

    if (!stdDirection.empty())
        return std::string("\"") + stdDirection + "\"";
    else
        return std::string("");
}

const QString TaskLinearPatternParameters::getDirection(void) const
{
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    App::DocumentObject* feature = pcLinearPattern->Direction.getValue();
    if (feature == NULL)
        return QString::fromUtf8("");
    std::vector<std::string> directions = pcLinearPattern->Direction.getSubValues();
    QString buf;

    if ((feature != NULL) && !directions.empty()) {
        buf = QString::fromUtf8("(App.ActiveDocument.%1,[\"%2\"])");
        buf = buf.arg(QString::fromUtf8(feature->getNameInDocument()));
        buf = buf.arg(QString::fromUtf8(directions.front().c_str()));
    }
    else
        buf = QString::fromUtf8("");

    return buf;
}

const bool TaskLinearPatternParameters::getReverse(void) const {
    return ui->checkReverse->isChecked();
}

const double TaskLinearPatternParameters::getLength(void) const {
    return ui->spinLength->value();
}

const unsigned TaskLinearPatternParameters::getOccurrences(void) const {
    return ui->spinOccurrences->value();
}

TaskLinearPatternParameters::~TaskLinearPatternParameters()
{
    delete ui;
    if (proxy)
        delete proxy;
}

void TaskLinearPatternParameters::changeEvent(QEvent *e)
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

TaskDlgLinearPatternParameters::TaskDlgLinearPatternParameters(ViewProviderLinearPattern *LinearPatternView)
    : TaskDlgTransformedParameters(LinearPatternView)
{
    parameter = new TaskLinearPatternParameters(LinearPatternView);

    Content.push_back(parameter);
}
//==== calls from the TaskView ===============================================================

bool TaskDlgLinearPatternParameters::accept()
{
    std::string name = TransformedView->getObject()->getNameInDocument();

    try {
        //Gui::Command::openCommand("LinearPattern changed");
        // Handle Originals
        if (!TaskDlgTransformedParameters::accept())
            return false;

        TaskLinearPatternParameters* linearpatternParameter = static_cast<TaskLinearPatternParameters*>(parameter);
        std::string direction = linearpatternParameter->getDirection().toStdString();
        if (!direction.empty())
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Direction = %s", name.c_str(), direction.c_str());
        std::string stdDirection = linearpatternParameter->getStdDirection();
        if (!stdDirection.empty())
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.StdDirection = %s",name.c_str(),stdDirection.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %u",name.c_str(),linearpatternParameter->getReverse());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Length = %f",name.c_str(),linearpatternParameter->getLength());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Occurrences = %u",name.c_str(),linearpatternParameter->getOccurrences());
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

#include "moc_TaskLinearPatternParameters.cpp"
