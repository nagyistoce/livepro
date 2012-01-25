#include "RectSelectVideoWidget.h"

RectSelectVideoWidget::RectSelectVideoWidget(QWidget *parent)
	: VideoWidget(parent)
	, m_rectSelectEnabled(true)
{
	setAutoFillBackground(false);
}

void RectSelectVideoWidget::setRectSelectEnabled(bool flag)
{
	m_rectSelectEnabled = flag;
}
	
void RectSelectVideoWidget::paintEvent(QPaintEvent *event)
{
	VideoWidget::paintEvent(event);
	
	QPainter p(this);
	
 	if(m_mouseIsDown)
 	{
		int penWidth = 3;
		p.setPen(QPen(Qt::green,(double)penWidth));
		//p.setBrush(QBrush());
		p.drawRect(m_rect.adjusted(penWidth/2,penWidth/2,-penWidth/2,-penWidth/2));
	}
	
}
	
void RectSelectVideoWidget::mousePressEvent(QMouseEvent *event)
{
	m_rect = QRect(event->pos(),QSize(0,0));
	m_mouseIsDown = true;

}

void RectSelectVideoWidget::mouseMoveEvent(QMouseEvent *event)
{
	QPoint mousePos = event->pos();
	QPoint curPos = m_rect.topLeft();
	
	QRect newRect = QRect(curPos, QSize(mousePos.x() - curPos.x(),mousePos.y() - curPos.y())).normalized();
	
	m_rect = newRect;
	update();
}

void RectSelectVideoWidget::mouseReleaseEvent(QMouseEvent *event)
{
	m_mouseIsDown = false;
	
	/// TODO: Scale m_rect to m_targetRect coordinates
	
	emit rectSelected(m_rect);
}
