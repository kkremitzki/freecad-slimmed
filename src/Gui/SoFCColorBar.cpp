/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Inventor/actions/SoGLRenderAction.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoEventCallback.h>
# include <Inventor/nodes/SoSwitch.h>
# include <QApplication>
# include <QMenu>
#endif

#include "SoFCColorBar.h"
#include "SoFCColorGradient.h"
#include "SoFCColorLegend.h"


using namespace Gui;

SO_NODE_ABSTRACT_SOURCE(SoFCColorBarBase)

/*!
  Constructor.
*/
SoFCColorBarBase::SoFCColorBarBase(): _windowSize(0,0)
{
    SO_NODE_CONSTRUCTOR(SoFCColorBarBase);
}

/*!
  Destructor.
*/
SoFCColorBarBase::~SoFCColorBarBase()
{
    //delete THIS;
}

// doc from parent
void SoFCColorBarBase::initClass()
{
    SO_NODE_INIT_ABSTRACT_CLASS(SoFCColorBarBase,SoSeparator,"Separator");
}

void SoFCColorBarBase::finish()
{
    atexit_cleanup();
}

void SoFCColorBarBase::GLRenderBelowPath(SoGLRenderAction *  action)
{
    const SbViewportRegion& vp = action->getViewportRegion();
    const SbVec2s&  size = vp.getWindowSize();
    if (_windowSize != size) {
        _windowSize = size;
        setViewportSize(size);
    }
    SoSeparator::GLRenderBelowPath(action);
}

// --------------------------------------------------------------------------

namespace Gui {
// Proxy class that receives an asynchronous custom event
class SoFCColorBarProxyObject : public QObject
{
public:
    SoFCColorBarProxyObject(SoFCColorBar* b)
        : QObject(nullptr), bar(b) {}
    ~SoFCColorBarProxyObject() {}
    void customEvent(QEvent *)
    {
        bar->customize(bar->getActiveBar());
        this->deleteLater();
    }

private:
    SoFCColorBar* bar;
};
}

SO_NODE_SOURCE(SoFCColorBar)

/*!
  Constructor.
*/
SoFCColorBar::SoFCColorBar()
{
    SO_NODE_CONSTRUCTOR(SoFCColorBar);

//  SoEventCallback * cb = new SoEventCallback;
//  cb->addEventCallback(SoMouseButtonEvent::getClassTypeId(), eventCallback, this);
//  insertChild(cb, 0);

    pColorMode = new SoSwitch;
    addChild(pColorMode);

    _colorBars.push_back( new SoFCColorGradient );
    _colorBars.push_back( new SoFCColorLegend );

    for (std::vector<SoFCColorBarBase*>::const_iterator it = _colorBars.begin(); it != _colorBars.end(); ++it)
        pColorMode->addChild( *it );
    pColorMode->whichChild = 0;
}

/*!
  Destructor.
*/
SoFCColorBar::~SoFCColorBar()
{
    //delete THIS;
}

// doc from parent
void SoFCColorBar::initClass()
{
    SO_NODE_INIT_CLASS(SoFCColorBar,SoFCColorBarBase,"Separator");
}

void SoFCColorBar::finish()
{
    atexit_cleanup();
}

SoFCColorBarBase* SoFCColorBar::getActiveBar() const
{
    int child = pColorMode->whichChild.getValue();
    return _colorBars[child];
}

void SoFCColorBar::setViewportSize( const SbVec2s& size )
{
    std::ignore = size;
}

void SoFCColorBar::setRange( float fMin, float fMax, int prec )
{
    for (std::vector<SoFCColorBarBase*>::const_iterator it = _colorBars.begin(); it != _colorBars.end(); ++it)
        (*it)->setRange(fMin, fMax, prec);
}

void SoFCColorBar::setOutsideGrayed (bool bVal)
{
    for (std::vector<SoFCColorBarBase*>::const_iterator it = _colorBars.begin(); it != _colorBars.end(); ++it)
        (*it)->setOutsideGrayed(bVal);
}

bool SoFCColorBar::isVisible (float fVal) const
{
    return this->getActiveBar()->isVisible(fVal);
}

float SoFCColorBar::getMinValue () const
{
    return this->getActiveBar()->getMinValue();
}

float SoFCColorBar::getMaxValue () const
{
    return this->getActiveBar()->getMaxValue();
}

void SoFCColorBar::triggerChange(SoFCColorBarBase*)
{
    Notify(0);
}

void SoFCColorBar::customize(SoFCColorBarBase* child)
{
    try {
        return child->customize(this);
    }
    catch (const Base::ValueError& e) {
        e.ReportException();
    }
}

App::Color SoFCColorBar::getColor( float fVal ) const
{
    return this->getActiveBar()->getColor( fVal );
}

void SoFCColorBar::eventCallback(void * /*userdata*/, SoEventCallback * node)
{
    const SoEvent * event = node->getEvent();
    if (event->getTypeId().isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
        const SoMouseButtonEvent*  e = static_cast<const SoMouseButtonEvent*>(event);
        if ((e->getButton() == SoMouseButtonEvent::BUTTON2)) {
            if (e->getState() == SoButtonEvent::UP) {
                // do nothing here
            }
        }
    }
}

void SoFCColorBar::handleEvent (SoHandleEventAction *action)
{
    const SoEvent * event = action->getEvent();

    // check for mouse button events
    if (event->getTypeId().isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
        const SoMouseButtonEvent*  e = static_cast<const SoMouseButtonEvent*>(event);

        // check if the cursor is near to the color bar
        if (!action->getPickedPoint())
            return; // not inside the rectangle

        // left mouse pressed
        action->setHandled();
        if ((e->getButton() == SoMouseButtonEvent::BUTTON1)) {
            if (e->getState() == SoButtonEvent::DOWN) {
                // double click event
                if (!_timer.isValid()) {
                    _timer.start();
                }
                else if (_timer.restart() < QApplication::doubleClickInterval()) {
                    QApplication::postEvent(
                        new SoFCColorBarProxyObject(this),
                        new QEvent(QEvent::User));
                }
            }
        }
        // right mouse pressed
        else if ((e->getButton() == SoMouseButtonEvent::BUTTON2)) {
            if (e->getState() == SoButtonEvent::UP) {
                SoFCColorBarBase* current = getActiveBar();
                QMenu menu;
                int i=0;
                for (std::vector<SoFCColorBarBase*>::const_iterator it = _colorBars.begin(); it != _colorBars.end(); ++it) {
                    QAction* item = menu.addAction(QLatin1String((*it)->getColorBarName()));
                    item->setCheckable(true);
                    item->setChecked((*it) == current);
                    item->setData(QVariant(i++));
                }

                menu.addSeparator();
                QAction* option = menu.addAction(QObject::tr("Options..."));
                QAction* select = menu.exec(QCursor::pos());

                if (select == option) {
                    QApplication::postEvent(
                        new SoFCColorBarProxyObject(this),
                        new QEvent(QEvent::User));
                }
                else if (select) {
                    int id = select->data().toInt();
                    pColorMode->whichChild = id;
                }
            }
        }
    }
}
