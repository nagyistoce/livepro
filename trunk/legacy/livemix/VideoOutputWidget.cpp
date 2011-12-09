#include "VideoOutputWidget.h"
#include <QResizeEvent>

#include "VideoSource.h"
VideoOutputWidget::VideoOutputWidget(QWidget *parent) 
	: QGLWidget(parent)
	, m_thread(this)
{ 
	setAutoBufferSwap(false);
	resize(320, 240);
}

void VideoOutputWidget::startRendering()
{
	m_thread.start();
}
	
void VideoOutputWidget::stopRendering()
{
	m_thread.stop();
	m_thread.wait();
}

void VideoOutputWidget::resizeEvent(QResizeEvent *evt)
{
	m_thread.resizeViewport(evt->size());
}

void VideoOutputWidget::paintEvent(QPaintEvent *)
{
	// Handled by the m_thread
}

void VideoOutputWidget::closeEvent(QCloseEvent *evt)
{
	stopRendering();
	QGLWidget::closeEvent(evt);
	deleteLater();
}

void VideoOutputWidget::setVideoSource(VideoSource *source)
{
	m_thread.setVideoSource(source);
}

void VideoOutputWidget::applyGeometry(const QRect& rect, bool isFullScreen)
{
	qDebug() << "VideoOutputWidget::applyGeometry(): rect: "<<rect;
// 	if(isFullScreen)
// 		setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip);
// 	else
// 		setWindowFlags(Qt::FramelessWindowHint);
				
				
	resize(rect.width(),rect.height());
	move(rect.left(),rect.top());
		
	show();
	startRendering();
	
	setWindowTitle(QString("Video Output - LiveMix")/*.arg(m_output->name())*/);
	setWindowIcon(QIcon(":/data/icon-d.png"));
}
