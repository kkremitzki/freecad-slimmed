/***************************************************************************
 *   Copyright (c) 2009 J�rgen Riegel <juergen.riegel@web.de>              *
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


#ifndef ROBOTGUI_TASKWATCHER_H
#define ROBOTGUI_TASKWATCHER_H



#include <Gui/TaskView/TaskWatcher.h>
#include "TaskRobot6Axis.h"
#include "TaskRobotControl.h"

#include <QObject>

namespace RobotGui {

class TaskContent;

/// Father class of watcher classes
class RobotGuiExport TaskWatcherRobot : public Gui::TaskView::TaskWatcher 
{
    Q_OBJECT

public:
    TaskWatcherRobot();
    ~TaskWatcherRobot();

    /// is called when the document or the selection changes. 
    virtual bool shouldShow(void);

protected:
    TaskRobot6Axis    *rob; 
    TaskRobotControl  *ctr ;
; 

};


} //namespace RobotGui

#endif // ROBOTGUI_TASKWATCHER_H
