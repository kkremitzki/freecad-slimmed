/***************************************************************************
 *   Copyright (c) 2017 WandererFan <wandererfan@gmail.com>                *
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

#ifndef DRAWINGGUI_QGDISPLAYAREA_H
#define DRAWINGGUI_QGDISPLAYAREA_H

#include <QGraphicsItem>
#include <QPointF>
#include <QRectF>

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

namespace TechDrawGui
{

class TechDrawGuiExport QGDisplayArea : public QGraphicsItemGroup
{
public:
    explicit QGDisplayArea(void);
    ~QGDisplayArea() {}

    enum {Type = QGraphicsItem::UserType + 137};
    int type() const { return Type;}
    virtual QRectF boundingRect() const;

    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = nullptr );
    virtual void centerAt(QPointF centerPos);
    virtual void centerAt(double cX, double cY);

protected:

private:

};

}

#endif // DRAWINGGUI_QGDISPLAYAREA_H

