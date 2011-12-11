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

#include "CornerItem.h"
#include "GLDrawable.h"
//#include "RenderOpts.h"
//#include "AppSettings.h"
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <math.h>

CornerItem::CornerItem(CornerPosition corner, bool rotateOnly, GLDrawable * parent)
	: QGraphicsItem(parent)
	, m_content(parent)
	, m_corner(corner)
	, m_opMask(rotateOnly ? Rotate | FixRotate : AllowAll)
	, m_side(8)
	, m_operation(Off)
	, m_defaultLeftOp(Scale | FixScale)
        , m_defaultMidOp(Scale)
        , m_defaultRightOp( Scale | FixScale)
        //Rotate
{
	setAcceptsHoverEvents(true);
}

void CornerItem::setDefaultLeftOp(int mask)
{
	m_defaultLeftOp = mask;
}

void CornerItem::setDefaultMidOp(int mask)
{
	m_defaultMidOp = mask;
}

void CornerItem::setDefaultRightOp(int mask)
{
	m_defaultRightOp = mask;
}

void CornerItem::relayout(const QRect & rect)
{
//     // change side, if needed
//     int side = 1 + (int)sqrt((float)qMin(rect.width(), rect.height()));
//     if (side != m_side) {
//         prepareGeometryChange();
//         //m_side = side;
//         
//         // TODO does this break anything?
//     	m_side = 8;
//     }
    
    

    // place at the right corner
    switch (m_corner) {
        case TopLeftCorner: setPos(rect.topLeft() + QPoint(m_side, m_side)); setCursor(Qt::SizeFDiagCursor); break;
        case TopRightCorner: setPos(rect.topRight() + QPoint(-m_side + 1, m_side)); setCursor(Qt::SizeBDiagCursor); break;
        case BottomLeftCorner: setPos(rect.bottomLeft() + QPoint(m_side, -m_side + 1)); setCursor(Qt::SizeBDiagCursor); break;
        case BottomRightCorner: setPos(rect.bottomRight() + QPoint(-m_side + 1, -m_side + 1)); setCursor(Qt::SizeFDiagCursor); break;
        
        case MidTop: setPos(QPoint(rect.left() + rect.width()/2, rect.top()) + QPoint(m_side, m_side)); setCursor(Qt::SizeVerCursor); break;
        case MidLeft: setPos(QPoint(rect.left(), rect.top() + rect.height()/2) + QPoint(-m_side + 1, m_side)); setCursor(Qt::SizeHorCursor); break;
        case MidRight: setPos(QPoint(rect.right(), rect.top() + rect.height()/2) + QPoint(m_side, -m_side + 1)); setCursor(Qt::SizeHorCursor); break;
        case MidBottom: setPos(QPoint(rect.left() + rect.width()/2, rect.bottom()) + QPoint(-m_side + 1, -m_side + 1)); setCursor(Qt::SizeVerCursor); break;
    }

}

QRectF CornerItem::boundingRect() const
{
	QRectF rect(-m_side, -m_side, m_side, m_side);
	if(m_corner == TopLeftCorner)
	{
		rect.translate(-m_side/4,-m_side/4); //,-m_side/2,-m_side/2);
	}
	else
	if(m_corner == BottomLeftCorner)
	{
		//rect.adjust(-m_side/2,m_side/2,-m_side/2,-m_side/2);
		rect.translate(-m_side/4,m_side*1.25); //,-m_side/2,-m_side/2);
	}
	else
	if(m_corner == BottomRightCorner)
	{
		//rect.adjust(m_side/2,m_side/2,-m_side/2,-m_side/2);
		rect.translate(m_side*1.25,m_side*1.25); //,-m_side/2,-m_side/2);
	}
	else
	if(m_corner == TopRightCorner)
	{
		//rect.adjust(m_side/2,m_side/2,-m_side/2,-m_side/2);
		rect.translate(m_side*1.25,-m_side/4); //,-m_side/2,-m_side/2);
	}
	else
	if(m_corner == MidTop)
	{
		//rect.adjust(m_side/2,m_side/2,-m_side/2,-m_side/2);
		rect.translate(-m_side/2,-m_side/4); //,-m_side/2,-m_side/2);
	}
	if(m_corner == MidLeft)
	{
		rect.translate(m_side + m_side/2,-m_side/2); //,-m_side/2,-m_side/2);
	}
	else
	if(m_corner == MidBottom)
	{
		//rect.adjust(-m_side/2,m_side/2,-m_side/2,-m_side/2);
		rect.translate(m_side*1.25,m_side*1.25); //,-m_side/2,-m_side/2);
	}
	else
	if(m_corner == MidRight)
	{
		//rect.adjust(m_side/2,m_side/2,-m_side/2,-m_side/2);
		rect.translate(-m_side/2,m_side + m_side/2); //,-m_side/2,-m_side/2);
	}

	return rect;

}

void CornerItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)
{
    event->accept();

    // do the right op
    switch (event->button()) {
        //case Qt::LeftButton:    if (m_opMask & Scale)  m_content->resetContentsRatio(); break;
        //case Qt::RightButton:   if (m_opMask & Rotate) m_content->setRotation(0.0, Qt::ZAxis); break;
        default:                break;
    }
}

void CornerItem::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    event->accept();

    // do the right op
    switch (event->button()) {
        case Qt::LeftButton:    m_operation = m_defaultLeftOp; break; //Scale | FixScale; break;
        case Qt::RightButton:   m_operation =  Rotate; break;
        //Scale | FixScale |
        //
        //m_defaultRightOp; break; //
        case Qt::MidButton:     m_operation = Scale; break;
        //m_defaultMidOp; //
        default:                m_operation = Off; return;
    }

    // filter out unwanted operations
    m_operation &= m_opMask;
    
    //m_operation = Crop | Scale;

    // intial parameters
    QRect contentsRect = m_content->boundingRect().toRect(); //contentsRect();
    m_startRatio = (double)contentsRect.width() / (double)contentsRect.height();
    m_startPos = event->pos(); //m_content->pos();

    update();
}

void CornerItem::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
	if (m_operation == Off)
		return;
	event->accept();


	// modify the operation using the shortcuts
	int op = m_operation;
	if (event->modifiers() & Qt::ShiftModifier)
		op &= ~FixScale & ~Rotate;
	if (event->modifiers() & Qt::ControlModifier)
		//op |= FixRotate;
		op |= Rotate;
	if (event->modifiers() & Qt::AltModifier)
		//op &= ~(Scale | FixScale);
		op |= FixRotate;
	op &= m_opMask;
	if ((op & (Rotate | Scale)) == (Rotate | Scale))
		op |= FixScale;
	if ((op & (Rotate | Scale)) == Off)
		return;

	QPointF eventPos = event->pos();//AppSettings::snapToGrid(event->pos());
	// vector relative to the centre
	//QPointF v = pos() + event->pos();
	QPointF v = pos() + eventPos;
	if (v.isNull())
		return;

	// do scaling
	//m_content->delayedDirty();
	m_content->update();
	if (op & Scale)
	{
// 		if (op & FixScale)
// 		{
// 			const double r = m_startRatio;
// 			const double D = sqrt(v.x()*v.x() + v.y()*v.y());
// 			const double K = sqrt(1 + 1/(r * r));
// 			int W = qMax((int)((2*D)/(K)), 50);
// 			int H = qMax((int)((2*D)/(r*K)), 40);
// 			m_content->resizeContents(QRect(-W / 2, -H / 2, W, H));
// 		} else
// 		{
			static int counter = 0;

			QRectF cr = m_content->rect(); //boundingRect().toRect(); //contentsRect();
			//QPointF d = AppSettings::snapToGrid( event->pos() - m_startPos );
			QPointF d = ( event->pos() - m_startPos );

			int W = (int)(cr.width()  + d.x());
			int H = (int)(cr.height() + d.y());

			counter++;

			QPointF v2 = event->pos() + m_startPos;
// 			qreal W2 = 2 * v2.x();
//             		qreal H2 = 2 * v2.y();

			//qDebug() << "#"<<counter<<" v:"<<v<<", W:"<<W<<", H:"<<H<<", d:"<<d<<", m_corner:"<<m_corner<<", pos:"<<pos()<<", event->pos:"<<event->pos()<<", startPos:"<<m_startPos<<", W2:"<<W2<<", H2:"<<H2;

			QRectF br = boundingRect();
			QPointF cpos = cr.topLeft(); //m_content->pos();
// 			if(cpos.x() <=0)
// 				d.setX(0);
// 			if(cpos.y() <=0)
// 				d.setY(0);

			//QPointF srcTL  = m_content->sourceOffsetTL();
			//QPointF srcBR  = m_content->sourceOffsetBR();

			if(m_corner == TopLeftCorner)
			{
				//m_content->setPos(m_startPos + (event->pos() - pos()));

				if(op & FixScale)
				{


					if(d.x() > d.y())
					{
						int newWidth = (int)(cr.width()  -d.x());
						int newHeight = (int)(newWidth / m_startRatio);
						int deltaY = (int)(cr.height() - newHeight);

						//m_content->moveBy(d.x(),deltaY);
						cr.translate(d.x(),deltaY);
						cr.setWidth(newWidth); // adjust width (then height..) to compensate for moveBy()
						cr.setHeight(newHeight); // adjust width (then height..) to compensate for moveBy()
					}
					else
					{
						int newHeight = (int)(cr.height()  -d.y());
						int newWidth = (int)(newHeight * m_startRatio);
						int deltaX = (int)(cr.width() - newWidth);

						//m_content->moveBy(deltaX,d.y());
						cr.translate(deltaX,d.y());
						cr.setWidth(newWidth); // adjust width (then height..) to compensate for moveBy()
						cr.setHeight(newHeight); // adjust width (then height..) to compensate for moveBy()

					}
				}
				else
				{
					//m_content->moveBy(d.x(),d.y());
					cr.translate(d.x(),d.y());
					cr.setWidth((int)(cr.width()   - d.x())); // adjust width (then height..) to compensate for moveBy()
					cr.setHeight((int)(cr.height() - d.y()));
					
// 					if(op & Crop)
// 					{
// 						QPointF pre = srcTL;
// 						
// 						qreal perc = d.x() / cr.width();
// 						srcTL.setX( srcTL.x() + perc);
// 						
// 						perc = d.y() / cr.height();
// 						srcTL.setY( srcTL.y() + perc);
// 					}
 				}


				//qDebug("- TopLeft (%.02f,%.02f)",d.x(),y());

			}
			else
			if(m_corner == MidTop)
			{
				//m_content->setPos(m_startPos + (event->pos() - pos()));

				if(op & FixScale)
				{
					int newHeight = (int)(cr.height()  -d.y());
					int newWidth = (int)(newHeight * m_startRatio);
					int deltaX = (int)(cr.width() - newWidth);

					//m_content->moveBy(deltaX,d.y());
					cr.translate(deltaX,d.y());
					cr.setWidth(newWidth); // adjust width (then height..) to compensate for moveBy()
					cr.setHeight(newHeight); // adjust width (then height..) to compensate for moveBy()
				}
				else
				{
					//m_content->moveBy(0,d.y());
					cr.translate(0,d.y());
					//cr.setWidth((int)(cr.width()  -d.x())); // adjust width (then height..) to compensate for moveBy()
					cr.setHeight((int)(cr.height()-d.y()));
					
// 					if(op & Crop)
// 					{
// 					
// 						QPointF pre = srcTL;
// 						qreal perc = d.y() / cr.height();
// 						srcTL.setY( srcTL.y() + perc);
// 						
// 						//qDebug() << "CornerItem::mouseMoveEvent(): /Scale/MidTop/Crop: pre:"<<pre<<", perc:"<<perc<<", post:"<<srcTL;
// 					}
 				}
			}
			else
			if(m_corner == MidLeft)
			{
				//m_content->setPos(m_startPos + (event->pos() - pos()));

				if(op & FixScale)
				{
					int newWidth = (int)(cr.width()  -d.x());
					int newHeight = (int)(newWidth / m_startRatio);
					int deltaY = (int)(cr.height() - newHeight);

					//m_content->moveBy(d.x(),deltaY);
					cr.translate(d.x(),deltaY);
					cr.setWidth(newWidth); // adjust width (then height..) to compensate for moveBy()
					cr.setHeight(newHeight); // adjust width (then height..) to compensate for moveBy()
				}
				else
				{
					//m_content->moveBy(d.x(),0);
					cr.translate(d.x(),0);
					cr.setWidth((int)(cr.width()  -d.x())); // adjust width (then height..) to compensate for moveBy()
					//cr.setHeight((int)(cr.height()-d.y()));
					
// 					if(op & Crop)
// 					{
// 						sr.translate((int)-d.x(),0);
// 						sr.setWidth((int)(sr.width()   + d.x()));
// 					}

// 					if(op & Crop)
// 					{
// 					
// 						QPointF pre = srcTL;
// 						qreal perc = d.x() / cr.width();
// 						srcTL.setX( srcTL.x() + perc);
// 						
// 						//qDebug() << "CornerItem::mouseMoveEvent(): /Scale/MidLeft/Crop: pre:"<<pre<<", perc:"<<perc<<", post:"<<srcTL;
// 					}
 				}


				//qDebug("- TopLeft (%.02f,%.02f)",d.x(),y());

			}
			else
			if(m_corner == BottomLeftCorner)
			{
				//m_content->moveBy(d.x(),0);
				cr.translate(d.x(),0);
				cr.setWidth((int)(cr.width()-d.x()));

				if(op & FixScale)
				{
					cr.setHeight((int)(cr.width() / m_startRatio));
				}
				else
				{
					cr.setHeight(H);
				}
				
// 				if(op & Crop)
// 				{
// 					QPointF pre = srcTL;
// 					qreal perc = d.x() / cr.width();
// 					srcTL.setX( srcTL.x() + perc);
// 					
// 					pre = srcBR;
// 					perc = d.y() / cr.height();
// 					srcBR.setY( srcBR.y() + perc);
// 				}


				//qDebug("- BottomLeft (%.02f,%d)",d.x(),H);
			}
			else
			if(m_corner == TopRightCorner)
			{


// 				int oldWidth = cr.width();
				if(op & FixScale)
				{
					int newHeight = (int)(cr.height()  -d.y());
					int newWidth = (int)(newHeight * m_startRatio);

					//m_content->moveBy(0,d.y());
					cr.translate(0,d.y());
					cr.setWidth(newWidth); // adjust width (then height..) to compensate for moveBy()
					cr.setHeight(newHeight); // adjust width (then height..) to compensate for moveBy()

				}
				else
				{
					//m_content->moveBy(0,d.y());
					cr.translate(0,d.y());
					cr.setWidth(W);
					cr.setHeight((int)(cr.height()-d.y()));
				}

// 				if(op & Crop)
// 				{
// 					QPointF pre = srcTL;
// 					qreal perc = d.x() / cr.width();
// 					srcTL.setX( srcTL.x() + perc);
// 					
// 					pre = srcBR;
// 					perc = d.y() / cr.height();
// 					srcBR.setY( srcBR.y() + perc);
// 				}
				//qDebug("- TopRight (%.02f,%d)",d.y(),W);
			}
			else
			if(m_corner == BottomRightCorner)
			{
// 				int oldWidth = cr.width();
// 				int oldHeight= cr.height();
				
				cr.setWidth(W);
// 				int hfw = m_content->contentHeightForWidth(W);
// 				if (hfw > 1)
// 					H = hfw;
				if(op & FixScale)
				{
					cr.setHeight((int)(cr.width() / m_startRatio));
				}
				else
				{
					cr.setHeight(H);
				}
				
// 				if(op & Crop)
// 				{
// 					QPointF pre = srcBR;
// 					
// 					qreal perc = d.x() / cr.width();
// 					srcBR.setX( srcBR.x() + perc);
// 					
// 					perc = d.y() / cr.height();
// 					srcBR.setY( srcBR.y() + perc);
// 				}
				//qDebug("- BottomRight (%d,%d)",W,H);
			}
			else
			if(m_corner == MidRight)
			{
// 				int oldWidth = cr.width();
				cr.setWidth(W);
// 				int hfw = m_content->contentHeightForWidth(W);
// 				if (hfw > 1)
// 					H = hfw;
				if(op & FixScale)
				{
					cr.setHeight((int)(cr.width() / m_startRatio));
				}
				
// 				if(op & Crop)
// 				{
// 					QPointF pre = srcBR;
// 					
// 					qreal perc = d.x() / cr.width();
// 					srcBR.setX( srcBR.x() + perc);
// 				}
			}
			else
			if(m_corner == MidBottom)
			{
// 				int oldHeight= cr.height();
				
				if(op & FixScale)
				{
					int newWidth = (int)(H * m_startRatio);
					cr.setWidth(newWidth); // adjust width (then height..) to compensate for moveBy()
				}
				
				cr.setHeight(H);
				
// 				if(op & Crop)
// 				{
// 					QPointF pre = srcBR;
// 					qreal perc = d.y() / cr.height();
// 					srcBR.setY( srcBR.y() + perc);
// 				}
			}

			//m_content->dirtyCache();
			m_content->setRect(cr);//resizeContents(cr);
			
// 			if(op & Crop)
// 			{
// 				m_content->setSourceOffsetTL(srcTL);
// 				m_content->setSourceOffsetBR(srcBR);
// 			}
			//m_content->update();
		//}
	}

	// do rotation
// 	if (op & Rotate)
// 	{
// 		QPointF refPos = pos();
// 
// 		// set item rotation (set rotation relative to current)
// 		qreal refAngle = atan2(refPos.y(), refPos.x());
// 		qreal newAngle = atan2(v.y(), v.x());
// 		double dZr = 57.29577951308232 * (newAngle - refAngle); // 180 * a / M_PI
// 		double zr = m_content->rotation(Qt::ZAxis) + dZr;
// 
// 		// snap to M_PI/4
// 		if (op & FixRotate)
// 		{
// 			int fracts = (int)((zr - 7.5) / 15.0);
// 			zr = (qreal)fracts * 15.0;
// 		}
// 
// 		// apply rotation
// 		m_content->setRotation(zr, Qt::ZAxis);
// 	}
}

void CornerItem::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
	event->accept();
// 	bool accepted = m_operation != Off;
	m_operation = Off;
	update();

	// clicked
	//if (accepted) {
		//m_content->syncToModelItem(0);
		//qDebug() << "CornerItem::mouseReleaseEvent(): synced!";
	//}
}

void CornerItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * /*widget*/)
{
// 	if (RenderOpts::HQRendering)
// 		return;

	QColor color = Qt::black; //RenderOpts::hiColor;
	QColor fill = Qt::white;
	if (option->state & QStyle::State_MouseOver)
	{
		if (m_operation != Off)
			//color.setAlpha(250);
			fill = Qt::yellow;
		else
			//color.setAlpha(196);
			fill = Qt::blue;
	}
	// else
		//color.setAlpha(128);
	painter->setPen(color);
	painter->setBrush(fill);
	painter->drawRect(boundingRect());
}
