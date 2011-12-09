#include "GLSpinnerDrawable.h"

GLSpinnerDrawable::GLSpinnerDrawable(QObject *parent)
	: GLImageDrawable("",parent)
	, m_fillColor(Qt::white)
	, m_borderColor(Qt::black)
	, m_borderWidth(0.0)
	, m_cycleDuration(15.)
	, m_loopAtEnd(true)
	, m_animClockStarted(false)
	, m_sceneTickLock(true)
{
/*	foreach(CornerItem *corner, m_cornerItems)
		corner->setDefaultLeftOp(CornerItem::Scale);*/
	
	connect(&m_animTimer, SIGNAL(timeout()), this, SLOT(tick()));
	m_animTimer.setInterval(1000/15);
	
	setXFadeEnabled(false);
	renderImage(0);
	
	start();
}

void GLSpinnerDrawable::setFillColor(QColor c)
{
	m_fillColor = c;
	tick();
}

void GLSpinnerDrawable::setBorderColor(QColor c)
{
	m_borderColor = c;
	tick();
}

void GLSpinnerDrawable::setBorderWidth(double d)
{
	m_borderWidth = d;
	tick();
}

void GLSpinnerDrawable::setCycleDuration(double d)
{
	m_cycleDuration = d;
}

void GLSpinnerDrawable::setLoopAtEnd(bool flag)
{
	m_loopAtEnd = flag;
}

void GLSpinnerDrawable::start()
{
	m_animClockStarted = false;
	m_animTimer.start();
}

void GLSpinnerDrawable::tick()
{
	//if(!glWidget() && (!scene() || m_sceneTickLock))
	//   return;
	
	if(!liveStatus())
		return;
		
	if(!m_animClockStarted)
	{
		m_animClockStarted = true;
		m_animClock.start();
	}
	
	double progress = ((double)m_animClock.elapsed()) / (m_cycleDuration * 1000.);
	if(progress > 1.0)
	{
		if(m_loopAtEnd)
		{
			progress = 0.0;
			m_animClock.start(); // restart elapsed time
		}
		else
		{
			m_animTimer.stop();
			progress = 1.0;
		}
		emit cycleFinished();
	}
	renderImage(progress);
}

void GLSpinnerDrawable::renderImage(double progress)
{
	QImage image(rect().size().toSize(),QImage::Format_ARGB32);
	memset(image.scanLine(0),0,image.byteCount());
	QPainter painter(&image);
	if(m_borderWidth > 0.0)
		painter.setPen(QPen(m_borderColor,m_borderWidth));
	else
		painter.setPen(QPen());
	painter.setBrush(m_fillColor);
	
	QRect target = image.rect();
	if(m_borderWidth > 0.0)
	{
		int v = (int)(m_borderWidth / 2);
		target = target.adjusted(v,v,-v,-v);
	}
	
	// drawPie() docs report 0 angle at 3 o'clock - so move zero to top (positive numbers = counter clockwise)
	int startAngle = 0; //90
	// drawPie() docs report negative numbers go clockwise
	//int spanAngle  = (int)(90 - (360 * progress));
	int spanAngle  = (int)(360 * progress) * -1;
	
	//qDebug() << "GLSpinnerDrawable::renderImage: "<<(QObject*)this<<scene()<<glWidget()<<" progress:"<<progress<<", angle:"<<spanAngle<<", target:"<<target;
	
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.drawPie(target, startAngle * 16, spanAngle * 16);
	//painter.drawrect(target);
	painter.end();
	
	image = image.transformed(QTransform().rotate(-90));
	
	setXFadeEnabled(false);
	setImage(image);
}

void GLSpinnerDrawable::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
	if(m_sceneTickLock)
	{
		m_sceneTickLock = false;
		tick();
	}
	
	GLImageDrawable::paint(painter,option,widget);
}
