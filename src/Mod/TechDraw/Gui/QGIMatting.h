/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#ifndef DRAWINGGUI_QGIMATTING_H
#define DRAWINGGUI_QGIMATTING_H

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QGraphicsItem>
#include <QGraphicsItemGroup>
#include <QGraphicsPathItem>
#endif

#include <Mod/TechDraw/TechDrawGlobal.h>


QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

namespace TechDrawGui
{
class QGCustomRect;

class TechDrawGuiExport QGIMatting : public QGraphicsItemGroup
{
public:
    explicit QGIMatting();
    ~QGIMatting() override {}

    enum {Type = QGraphicsItem::UserType + 205};
    int type() const override { return Type;}

    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = nullptr ) override;
    QRectF boundingRect() const override;

    virtual void setSize(double w, double h) {m_height = h; m_width = w;}
    //virtual void setHoleStyle(int hs) {m_holeStyle = hs;}
    virtual void setRadius(double r)  {m_radius = r;}
    virtual void draw();

protected:
    double m_height;
    double m_width;
    //int    m_holeStyle;    //round or rect
    double m_radius;
    int getHoleStyle();

    QGraphicsPathItem* m_mat;
    QGraphicsPathItem* m_border;

//    QPainterPath m_perimeter;
//    QPainterPath m_cutout;

private:
    QPen m_pen;
    QBrush m_brush;
    QPen m_penB;
    QBrush m_brushB;

};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_QGIMATTING_H
