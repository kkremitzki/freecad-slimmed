/***************************************************************************
 *   Copyright (c) 2021 edi                                                *
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

#ifndef TECHDRAWGUI_TASKSELECTLINEATTRIBUTES_H
#define TECHDRAWGUI_TASKSELECTLINEATTRIBUTES_H

#include <App/DocumentObject.h>
#include <Base/Vector3D.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>

#include <Mod/TechDraw/App/Cosmetic.h>

#include <Mod/TechDraw/Gui/ui_TaskSelectLineAttributes.h>

class Ui_TaskSelectLineAttributes;

namespace App {
class DocumentObject;
}

namespace TechDraw
{
class DrawPage;
class DrawView;
class DrawViewPart;
class CosmeticEdge;
class LineFormat;
}

namespace TechDraw
{
class Face;
}

namespace TechDrawGui
{
class QGVPage;
class QGIView;
class QGIPrimPath;
class MDIViewPage;
class ViewProviderViewPart;

class lineAttributes {
        int style;
        int width;
        int color;

    public:

    lineAttributes(void);
    void setStyle(int);
    int getStyle(void);
    void setWidth(float);
    int getWidth(void);
    float getWidthValue(void);
    void setColor(int);
    int getColor(void);
    App::Color getColorValue(void);

}; // class lineAttributes

class TaskSelectLineAttributes : public QWidget
{
    Q_OBJECT

public:
    TaskSelectLineAttributes(lineAttributes * ptActiveAttributes);
    ~TaskSelectLineAttributes();

public Q_SLOTS:

public:
    virtual bool accept();
    virtual bool reject();
    void updateTask();
    lineAttributes* activeAttributes;

protected Q_SLOTS:

protected:
    void changeEvent(QEvent *e);

    void setUiEdit(void);

private:
    std::unique_ptr<Ui_TaskSelectLineAttributes> ui;

}; // class TaskSelectLineAttributes

class TaskDlgSelectLineAttributes : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgSelectLineAttributes(lineAttributes * ptActiveAttributes);
    ~TaskDlgSelectLineAttributes();

public:
    /// is called the TaskView when the dialog is opened
    virtual void open();
    /// is called by the framework if an button is clicked which has no accept or reject role
    virtual void clicked(int);
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();
    /// is called by the framework if the user presses the help button
    virtual void helpRequested() { return;}
    virtual bool isAllowedAlterDocument(void) const
                        { return false; }
    void update();

protected:

private:
    TaskSelectLineAttributes* widget;

    Gui::TaskView::TaskBox* taskbox;
}; // class TaskDlgSelectLineAttributes

} // namespace TechDrawGui

#endif // #ifndef TECHDRAWGUI_TASKSELECTLINEATTRIBUTES_H
