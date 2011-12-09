/***************************************************************************
 *                                                                         *
 *   This file is part of the Fotowall project,                            *
 *       http://code.google.com/p/fotowall                                 *
 *                                                                         *
 *   Copyright (C) 2009 by Enrico Ros <enrico.ros@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __CornerItem_h__
#define __CornerItem_h__

#include <QGraphicsItem>
#include <QPointF>
class GLDrawable;

#define GRID_SIZE 10

class CornerItem : public QGraphicsItem
{
    public:
    	enum CornerPosition { TopLeftCorner, TopRightCorner, BottomLeftCorner, BottomRightCorner, MidTop, MidLeft, MidBottom, MidRight };
        CornerItem(CornerPosition corner, bool rotateOnly, GLDrawable * parent);

        void relayout(const QRect & rect);

        // ::QGraphicsItem
        QRectF boundingRect() const;
        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event);
        void mousePressEvent(QGraphicsSceneMouseEvent * event);
        void mouseMoveEvent(QGraphicsSceneMouseEvent * event);
        void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);
        void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
        
        enum Operation {
            Off         = 0x0000,
            Rotate      = 0x0001,
            FixRotate   = 0x0002,
            Scale       = 0x0010,
            FixScale    = 0x0020,
            Crop	= 0x0100,
            AllowAll    = 0xFFFF,
        };
        
        void setDefaultLeftOp(int);
        void setDefaultRightOp(int);
        void setDefaultMidOp(int);

    private:
        
        GLDrawable * m_content;
        CornerPosition m_corner;
        int m_opMask;
        int m_side;
        int m_operation;
        int m_defaultLeftOp;
        int m_defaultMidOp;
        int m_defaultRightOp;
        double m_startRatio;
        QPointF m_startPos;
        QPointF m_startScenePos;
        QRectF m_startRect;
};

#endif
