/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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
#endif

#include "ui_TaskSketcherGeneral.h"
#include "TaskSketcherGeneral.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/UnitsApi.h>

#include "ViewProviderSketch.h"

using namespace SketcherGui;
using namespace Gui::TaskView;

SketcherGeneralWidget::SketcherGeneralWidget(QWidget *parent)
  : QWidget(parent), ui(new Ui_TaskSketcherGeneral)
{
    ui->setupUi(this);

    // connecting the needed signals
    connect(ui->checkBoxShowGrid, SIGNAL(toggled(bool)),
            this, SLOT(toggleGridView(bool)));
    connect(ui->checkBoxGridSnap, SIGNAL(stateChanged(int)),
            this, SLOT(toggleGridSnap(int)));
    connect(ui->gridSize, SIGNAL(valueChanged(double)),
            this, SLOT(setGridSize(double)));
    connect(ui->checkBoxAutoconstraints, SIGNAL(stateChanged(int)),
            this, SIGNAL(emitToggleAutoconstraints(int)));
    connect(ui->renderingOrder->model(), SIGNAL(layoutChanged()),
            this, SLOT(renderOrderChanged()));
}

SketcherGeneralWidget::~SketcherGeneralWidget()
{
    delete ui;
}

void SketcherGeneralWidget::saveSettings()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Sketcher/General");
    hGrp->SetBool("ShowGrid", ui->checkBoxShowGrid->isChecked());

    ui->gridSize->pushToHistory();

    hGrp->SetBool("GridSnap", ui->checkBoxGridSnap->isChecked());
    hGrp->SetBool("AutoConstraints", ui->checkBoxAutoconstraints->isChecked());
    
    //not necessary to save renderOrder, as it is already stored in renderOrderChanged on every change.
}

void SketcherGeneralWidget::loadSettings()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Sketcher/General");
    ui->checkBoxShowGrid->setChecked(hGrp->GetBool("ShowGrid", true));
    ui->gridSize->setParamGrpPath(QByteArray("User parameter:BaseApp/History/SketchGridSize"));
    ui->gridSize->setToLastUsedValue();
    ui->checkBoxGridSnap->setChecked(hGrp->GetBool("GridSnap", ui->checkBoxGridSnap->isChecked()));
    ui->checkBoxAutoconstraints->setChecked(hGrp->GetBool("AutoConstraints", ui->checkBoxAutoconstraints->isChecked()));
    
    ParameterGrp::handle hGrpp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
    
    // 1->Normal Geometry, 2->Construction, 3->External
    int topid = hGrpp->GetInt("TopRenderGeometryId",1);
    int midid = hGrpp->GetInt("MidRenderGeometryId",2);
    int lowid = hGrpp->GetInt("LowRenderGeometryId",3);
    
    QListWidgetItem *newItem = new QListWidgetItem;
    newItem->setData(Qt::UserRole, QVariant(topid));
    newItem->setText( topid==1?tr("Normal Geometry"):topid==2?tr("Construction Geometry"):tr("External Geometry"));
    ui->renderingOrder->insertItem(0,newItem);
    
    newItem = new QListWidgetItem;
    newItem->setData(Qt::UserRole, QVariant(midid));
    newItem->setText(midid==1?tr("Normal Geometry"):midid==2?tr("Construction Geometry"):tr("External Geometry"));
    ui->renderingOrder->insertItem(1,newItem);
    
    newItem = new QListWidgetItem;
    newItem->setData(Qt::UserRole, QVariant(lowid));
    newItem->setText(lowid==1?tr("Normal Geometry"):lowid==2?tr("Construction Geometry"):tr("External Geometry"));
    ui->renderingOrder->insertItem(2,newItem);
}

void SketcherGeneralWidget::toggleGridView(bool on)
{
    ui->label->setEnabled(on);
    ui->gridSize->setEnabled(on);
    ui->checkBoxGridSnap->setEnabled(on);
    emitToggleGridView(on);
}

void SketcherGeneralWidget::setGridSize(double val)
{
    emitSetGridSize(val);
}

void SketcherGeneralWidget::setInitGridSize(double val)
{
    ui->gridSize->setValue(Base::Quantity(val,Base::Unit::Length));
}

void SketcherGeneralWidget::toggleGridSnap(int state)
{
    emitToggleGridSnap(state);
}

void SketcherGeneralWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void SketcherGeneralWidget::renderOrderChanged()
{
    int topid = ui->renderingOrder->item(0)->data(Qt::UserRole).toInt();
    int midid = ui->renderingOrder->item(1)->data(Qt::UserRole).toInt();
    int lowid = ui->renderingOrder->item(2)->data(Qt::UserRole).toInt();
    
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
    hGrp->SetInt("TopRenderGeometryId",topid);
    hGrp->SetInt("MidRenderGeometryId",midid);
    hGrp->SetInt("LowRenderGeometryId",lowid);
    
    emitrenderOrderChanged();
}

// ----------------------------------------------------------------------------

TaskSketcherGeneral::TaskSketcherGeneral(ViewProviderSketch *sketchView)
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"),tr("Edit controls"),true, 0)
    , sketchView(sketchView)
{
    // we need a separate container widget to add all controls to
    widget = new SketcherGeneralWidget(this);
    this->groupLayout()->addWidget(widget);

    // connecting the needed signals
    QObject::connect(
        widget, SIGNAL(emitToggleGridView(bool)),
        this  , SLOT  (toggleGridView(bool))
        );
    QObject::connect(
        widget, SIGNAL(emitToggleGridSnap(int)),
        this  , SLOT  (toggleGridSnap(int))
       );

    QObject::connect(
        widget, SIGNAL(emitSetGridSize(double)),
        this  , SLOT  (setGridSize(double))
       );

    QObject::connect(
        widget, SIGNAL(emitToggleAutoconstraints(int)),
        this  , SLOT  (toggleAutoconstraints(int))
       );
    
    QObject::connect(
        widget, SIGNAL(emitrenderOrderChanged()),
                     this  , SLOT  (renderOrderChanged())
    );
    

    Gui::Selection().Attach(this);
    widget->loadSettings();
    widget->setInitGridSize(sketchView->GridSize.getValue() );
}

TaskSketcherGeneral::~TaskSketcherGeneral()
{
    widget->saveSettings();
    Gui::Selection().Detach(this);
}

void TaskSketcherGeneral::toggleGridView(bool on)
{
    sketchView->ShowGrid.setValue(on);
}

void TaskSketcherGeneral::setGridSize(double val)
{
    if (val > 0)
        sketchView->GridSize.setValue(val);
}

void TaskSketcherGeneral::toggleGridSnap(int state)
{
    sketchView->GridSnap.setValue(state == Qt::Checked);
}

void TaskSketcherGeneral::toggleAutoconstraints(int state)
{
    sketchView->Autoconstraints.setValue(state == Qt::Checked);
}

/// @cond DOXERR
void TaskSketcherGeneral::OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                                   Gui::SelectionSingleton::MessageType Reason)
{
    Q_UNUSED(rCaller);
    Q_UNUSED(Reason);
    //if (Reason.Type == SelectionChanges::AddSelection ||
    //    Reason.Type == SelectionChanges::RmvSelection ||
    //    Reason.Type == SelectionChanges::SetSelection ||
    //    Reason.Type == SelectionChanges::ClrSelection) {
    //}
}
/// @endcond DOXERR

void TaskSketcherGeneral::renderOrderChanged()
{
    sketchView->updateColor();
}

#include "moc_TaskSketcherGeneral.cpp"
