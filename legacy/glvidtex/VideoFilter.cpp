#include "VideoFilter.h"

VideoFilter::VideoFilter(QObject *parent)
	: VideoSource(parent)
	, VideoConsumer()
	, m_frameDirty(false)
	, m_threadFps(30)
	, m_fpsLimit(-1)
	, m_isThreaded(false)
	
{
	connect(&m_processTimer, SIGNAL(timeout()), this, SLOT(processFrame()));
	m_processTimer.setSingleShot(true);
	m_processTimer.setInterval(0);
}

VideoFilter::~VideoFilter()
{
	setVideoSource(0);
}

void VideoFilter::setVideoSource(VideoSource* source)
{
	if(source == m_source)
		return;
	
	if(m_source)
	{
		disconnectVideoSource();
	}
	
	m_source = source;
	
	if(m_source)
	{
		connect(m_source, SIGNAL(frameReady()), this, SLOT(frameAvailable()));
		connect(m_source, SIGNAL(destroyed()),  this, SLOT(disconnectVideoSource()));
		m_source->registerConsumer(this);
		
		// pull in the first frame
		frameAvailable();
	}
	else
	{
		//qDebug() << "VideoFilter::setVideoSource(): "<<(QObject*)this<<" Source is NULL";
	}
}

void VideoFilter::disconnectVideoSource()
{
	if(!m_source)
		return;
	disconnect(m_source, 0, this, 0);
	m_source->release(this);
	m_source = 0;
}

void VideoFilter::frameAvailable()
{
// 	qDebug() << "GLVideoDrawable::frameReady(): "<<objectName()<<" m_source:"<<m_source;
	if(m_source)
	{
		VideoFramePtr f = m_source->frame();
// 		enqueue(f);
// 		return;
		
 		//qDebug() << "GLVideoDrawable::frameReady(): "<<objectName()<<" f:"<<f;
		if(!f)
			return;
		if(f->isValid())
		{
			QMutexLocker lock(&m_frameAccessMutex);
			m_frame = f;
			
			if(m_isThreaded)
			{
				m_frameDirty = true;
			}
			else
			{
				if(m_processTimer.isActive())
					m_processTimer.stop();
				m_processTimer.start();
			}
		}
	}
}

void VideoFilter::processFrame()
{
	enqueue(m_frame);
}

void VideoFilter::setIsThreaded(bool flag)
{
	m_isThreaded = flag;
	if(flag)
	{
		start();
	}
	else
	{
		m_killed = true;
		quit();
		wait();
	}
}

void VideoFilter::run()
{
	while(!m_killed)
	{
 		if(m_frameDirty)
 		{
 			m_frameAccessMutex.lock();
			processFrame();
			m_frameDirty = false;
			m_frameAccessMutex.unlock();
		}
		msleep(1000/m_threadFps);
	}
}

QImage VideoFilter::frameImage()
{
	if(!m_frame)
		return QImage();
	return m_frame->toImage();
// 	//qDebug() << "VideoSender::frameReady: Downscaling video for transmission to "<<m_transmitSize;
// 	// To scale the video frame, first we must convert it to a QImage if its not already an image.
// 	// If we're lucky, it already is. Otherwise, we have to jump thru hoops to convert the byte 
// 	// array to a QImage then scale it.
// 	QImage image;
// 	if(!m_frame->image().isNull())
// 	{
// 		image = m_frame->image();
// 	}
// 	else
// 	{
// 		const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(m_frame->pixelFormat());
// 		if(imageFormat != QImage::Format_Invalid)
// 		{
// 			image = QImage(m_frame->pointer(),
// 				m_frame->size().width(),
// 				m_frame->size().height(),
// 				m_frame->size().width() *
// 					(imageFormat == QImage::Format_RGB16  ||
// 					imageFormat == QImage::Format_RGB555 ||
// 					imageFormat == QImage::Format_RGB444 ||
// 					imageFormat == QImage::Format_ARGB4444_Premultiplied ? 2 :
// 					imageFormat == QImage::Format_RGB888 ||
// 					imageFormat == QImage::Format_RGB666 ||
// 					imageFormat == QImage::Format_ARGB6666_Premultiplied ? 3 :
// 					4),
// 				imageFormat);
// 				
// 			image = image.copy();
// 			//qDebug() << "Downscaled image from "<<image.byteCount()<<"bytes to "<<scaledImage.byteCount()<<"bytes, orig ptr len:"<<m_frame->pointerLength()<<", orig ptr:"<<m_frame->pointer();
// 		}
// 		else
// 		{
// 			qDebug() << "VideoFilter::frameImage: Unable to convert pixel format to image format, cannot scale frame. Pixel Format:"<<m_frame->pixelFormat();
// 		}
// 	}
// 	
// 	return image;
	
}

void VideoFilter::setFpsLimit(int limit)
{
	m_fpsLimit = limit;
	//m_processTimer.setInterval(m_fpsLimit > 0 ? 1000 / m_fpsLimit : 50);
}
