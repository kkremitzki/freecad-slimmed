/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef REENGUI_POISSON_H
#define REENGUI_POISSON_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>
#include <App/DocumentObserver.h>

namespace ReenGui {

class PoissonWidget : public QWidget
{
    Q_OBJECT

public:
    PoissonWidget(const App::DocumentObjectT&, QWidget* parent = nullptr);
    ~PoissonWidget();

    bool accept();

private:
    void changeEvent(QEvent *e);

private:
    class Private;
    Private* d;
};

class TaskPoisson : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskPoisson(const App::DocumentObjectT&);
    ~TaskPoisson();

public:
    void open();
    bool accept();

    QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Ok|QDialogButtonBox::Cancel; }

private:
    PoissonWidget* widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace ReenGui

#endif // REENGUI_POISSON_H
