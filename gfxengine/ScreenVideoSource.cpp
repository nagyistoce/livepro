#include "ScreenVideoSource.h"

#include <QApplication>

ScreenVideoSource::ScreenVideoSource(QObject *parent)
	: VideoSource(parent)
	, m_fps(0)
{
 	setIsBuffered(false);
	setFps(15);
	
	connect(&m_captureTimer, SIGNAL(timeout()), this, SLOT(captureScreen()));
	
	setCaptureRect(QRect(0,0,320,240));
	
	captureScreen();
	m_captureTimer.start();
}
void ScreenVideoSource::setFps(int fps)
{
	m_fps = fps;
	m_captureTimer.setInterval(1000/fps);
}

void ScreenVideoSource::captureScreen()
{
	QPixmap pixmap;
	if(m_captureRect.isValid())
		pixmap = QPixmap::grabWindow(QApplication::desktop()->winId(), m_captureRect.x(), m_captureRect.y(), m_captureRect.width(), m_captureRect.height());
	else
		pixmap = QPixmap::grabWindow(QApplication::desktop()->winId());
	
	static int counter = 0;
	counter ++;
	
	QImage image = pixmap.toImage();
	
// 	QImage image2 = pixmap.toImage();
// 	//if(image.format() != QImage::Format_ARGB32)
// 	//image = image.convertToFormat(QImage::Format_RGB32).scaled(240,180);
// 	QImage image(image2.size(), QImage::Format_RGB32);
// 	image.fill(QColor(0,178,0).rgba());
//  	QPainter p(&image);
//  	p.setPen(Qt::black);
//  	p.drawText(150,150,tr("%1").arg(counter));
//  	QRect rect = QRect(image.rect().bottomRight() - QPoint(5,5),QSize(5,5));
//  	//rect.moveCenter(QPoint(-5,0));
//  	p.fillRect(rect,QColor(178,0,0).rgba());
//  	p.end();
// 	
// 	if(counter % 2 == 0)
// 		image.fill(QColor(178,0,0).rgba());
	
	VideoFrame *frame = new VideoFrame(image,1000/m_fps);
	frame->setCaptureTime(QTime::currentTime());
	enqueue(frame);
	
	emit frameReady();
}

void ScreenVideoSource::run()
{
	// Can't thread this video source - need to capture from the screen!
}


