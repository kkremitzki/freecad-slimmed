/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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


#ifndef GUI_TASKVIEW_TaskPostDisplay_H
#define GUI_TASKVIEW_TaskPostDisplay_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Base/Parameter.h>
#include <App/PropertyLinks.h>
#include "ViewProviderFemPostFunction.h"

class QComboBox;
class Ui_TaskPostDisplay;
class Ui_TaskPostClip;
class Ui_TaskPostScalarClip;
class Ui_TaskPostWarpVector;


namespace FemGui { 

class TaskPostBox : public Gui::TaskView::TaskBox {
    
    Q_OBJECT
    
public:
    TaskPostBox(Gui::ViewProviderDocumentObject* view, const QPixmap &icon, const QString &title, QWidget *parent = 0);
    ~TaskPostBox();
    
    virtual void applyPythonCode() = 0; 
    virtual bool isGuiTaskOnly() {return false;}; //return true if only gui properties are manipulated
        
protected:
    App::DocumentObject*                getObject() {return m_object;};
    template<typename T>
    T* getTypedObject() {return static_cast<T*>(m_object);};
    Gui::ViewProviderDocumentObject*    getView() {return m_view;};
    template<typename T>
    T* getTypedView() {return static_cast<T*>(m_view);};
    
    bool autoApply();
    void recompute();
    
    static void updateEnumerationList(App::PropertyEnumeration&, QComboBox* box);

private:
    App::DocumentObject*              m_object;
    Gui::ViewProviderDocumentObject*  m_view;
};

/// simulation dialog for the TaskView
class TaskDlgPost : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgPost(Gui::ViewProviderDocumentObject *view, bool newObj=false);
    ~TaskDlgPost();

    void appendBox(TaskPostBox* box);
    Gui::ViewProviderDocumentObject* getView() const
    { return m_view; }

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
    virtual bool isAllowedAlterDocument(void) const
    { return false; }
    virtual void modifyStandardButtons(QDialogButtonBox*);

    /// returns for Close and Help button 
    virtual QDialogButtonBox::StandardButtons getStandardButtons(void) const;

protected:
    Gui::ViewProviderDocumentObject*  m_view;
    std::vector<TaskPostBox*>   m_boxes;
};


class TaskPostDisplay : public TaskPostBox
{
    Q_OBJECT

public:
    TaskPostDisplay(Gui::ViewProviderDocumentObject* view, QWidget *parent = 0);
    ~TaskPostDisplay();

    virtual void applyPythonCode();
    virtual bool isGuiTaskOnly() {return true;};
    
private Q_SLOTS:
    void on_Representation_activated(int i);
    void on_Field_activated(int i);
    void on_VectorMode_activated(int i);
    void on_Transparency_valueChanged(int i);
    
private:  
    QWidget* proxy;
    Ui_TaskPostDisplay* ui;
};

class TaskPostFunction : public TaskPostBox {
  
    Q_OBJECT
    
public:
    TaskPostFunction(Gui::ViewProviderDocumentObject* view, QWidget* parent = 0);
    virtual ~TaskPostFunction();
    
    virtual void applyPythonCode();
};

class TaskPostClip : public TaskPostBox {
    
    Q_OBJECT
    
public:
    TaskPostClip(Gui::ViewProviderDocumentObject* view, App::PropertyLink* function, QWidget* parent = 0);
    virtual ~TaskPostClip();
    
    virtual void applyPythonCode();
    
private Q_SLOTS:
    void on_CreateButton_triggered(QAction* a);
    void on_FunctionBox_currentIndexChanged(int idx);
    void on_InsideOut_toggled(bool val);
    void on_CutCells_toggled(bool val);
    
private:
    void collectImplicitFunctions();
    
    App::PropertyLink* m_functionProperty;
    QWidget* proxy;
    Ui_TaskPostClip* ui;
    FunctionWidget* fwidget;
};

class TaskPostScalarClip : public TaskPostBox {
    
    Q_OBJECT
    
public:
    TaskPostScalarClip(Gui::ViewProviderDocumentObject* view, QWidget* parent = 0);
    virtual ~TaskPostScalarClip();
    
    virtual void applyPythonCode();
    
private Q_SLOTS:
    void on_Slider_valueChanged(int v);
    void on_Value_valueChanged(double v);
    void on_Scalar_currentIndexChanged(int idx);
    void on_InsideOut_toggled(bool val);
    
private:
    QWidget* proxy;
    Ui_TaskPostScalarClip* ui;
};

class TaskPostWarpVector : public TaskPostBox {
    
    Q_OBJECT
    
public:
    TaskPostWarpVector(Gui::ViewProviderDocumentObject* view, QWidget* parent = 0);
    virtual ~TaskPostWarpVector();
    
    virtual void applyPythonCode();
    
private Q_SLOTS:
    void on_Slider_valueChanged(int v);
    void on_Value_valueChanged(double v);
    void on_Max_valueChanged(double v);
    void on_Min_valueChanged(double v);
    void on_Vector_currentIndexChanged(int idx);
    
private:
    QWidget* proxy;
    Ui_TaskPostWarpVector* ui;
};

} //namespace FemGui

#endif // GUI_TASKVIEW_TaskPostDisplay_H
