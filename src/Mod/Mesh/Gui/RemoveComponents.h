/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef MESHGUI_REMOVECOMPONENTS_H
#define MESHGUI_REMOVECOMPONENTS_H

#include <QDialog>
#include <Inventor/nodes/SoEventCallback.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

// forward declarations
class SoEventCallback;
namespace App { class DocumentObject; }
namespace Gui { class View3DInventorViewer; }
namespace Gui { class Document; }
namespace Mesh { class Feature; }

namespace MeshGui {
class ViewProviderMesh;
class Ui_RemoveComponents;

/**
 * Non-modal dialog to de/select components, regions, the complete or single faces
 * of a mesh and delete them.
 * @author Werner Mayer
 */
class MeshGuiExport RemoveComponents : public QWidget
{
    Q_OBJECT

public:
    RemoveComponents(QWidget* parent = 0, Qt::WFlags fl = 0);
    ~RemoveComponents();
    void reject();
    void deleteSelection();
    void invertSelection();

public Q_SLOTS:
    void on_selectRegion_clicked();
    void on_selectAll_clicked();
    void on_selectComponents_clicked();
    void on_selectTriangle_clicked();
    void on_deselectRegion_clicked();
    void on_deselectAll_clicked();
    void on_deselectComponents_clicked();
    void on_deselectTriangle_clicked();

protected:
    void changeEvent(QEvent *e);

private:
    std::list<ViewProviderMesh*> getViewProviders(const Gui::Document*) const;
    Gui::View3DInventorViewer* getViewer() const;
    void startInteractiveCallback(Gui::View3DInventorViewer* ,SoEventCallbackCB *);
    void stopInteractiveCallback(Gui::View3DInventorViewer*);

    static void selectGLCallback(void * ud, SoEventCallback * n);
    static void pickFaceCallback(void * ud, SoEventCallback * n);

private:
    Ui_RemoveComponents* ui;
    SoEventCallbackCB *_interactiveMode;
    bool selectRegion;
};

/**
 * Embed the panel into a dialog.
 */
class MeshGuiExport RemoveComponentsDialog : public QDialog
{
    Q_OBJECT

public:
    RemoveComponentsDialog(QWidget* parent = 0, Qt::WFlags fl = 0);
    ~RemoveComponentsDialog();
    void reject();

private Q_SLOTS:
    void clicked(QAbstractButton* btn);

private:
    RemoveComponents* widget;
};

/**
 * Embed the panel into a task dialog.
 */
class TaskRemoveComponents : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskRemoveComponents();
    ~TaskRemoveComponents();

public:
    bool accept();
    void clicked(int);

    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Ok | QDialogButtonBox::Close; }
    virtual bool isAllowedAlterDocument(void) const
    { return true; }
    virtual void modifyStandardButtons(QDialogButtonBox*);

private:
    RemoveComponents* widget;
    Gui::TaskView::TaskBox* taskbox;
};

}

#endif // MESHGUI_REMOVECOMPONENTS_H
