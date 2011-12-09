#include "SharedMemorySender.h"

#include "../livemix/VideoSource.h"

SharedMemorySender::SharedMemorySender(QString key, QObject *parent)
	: QObject(parent)
{
	m_source = 0;
// 	connect(&m_frameTimer, SIGNAL(timeout()), this, SLOT(generateFrame()));
// 	m_frameTimer.setInterval(1000/WRITER_FPS);
	
	QImage testImage(FRAME_WIDTH,
			 FRAME_HEIGHT,
			 FRAME_FORMAT);
	//testImage.fill(Qt::black);
	m_memorySize = testImage.byteCount();
	//qDebug() << "SharedMemorySender::ctor: size:"<<testImage.size()<<", bytes:"<<m_memorySize<<", format:"<<testImage.format();
	
// 	m_timeAccum = 0;
// 	m_frameCount = 0;

	m_sharedMemory.setKey(key);
	if(m_sharedMemory.isAttached())
		m_sharedMemory.detach();
		
	if (!m_sharedMemory.attach(QSharedMemory::ReadWrite)) 
	{
		// Shared memory does not exist, create it
		if (!m_sharedMemory.create(m_memorySize, QSharedMemory::ReadWrite)) 
		{
			qDebug() << "SharedMemorySender::enable("<<key<<"): Error:"<<m_sharedMemory.errorString();
			return;
		}
		else 
		{
			m_sharedMemory.lock();
			memset(m_sharedMemory.data(), 0, m_sharedMemory.size());
			m_sharedMemory.unlock();
		}
	}
}

SharedMemorySender::~SharedMemorySender()
{
	if(m_sharedMemory.isAttached())
		m_sharedMemory.detach();
	setVideoSource(0);
}
	
void SharedMemorySender::setVideoSource(VideoSource *source)
{
	if(m_source == source)
		return;
		
	if(m_source)
		disconnectVideoSource();
	
	m_source = source;
	if(m_source)
	{	
		connect(m_source, SIGNAL(frameReady()), this, SLOT(frameReady()));
		connect(m_source, SIGNAL(destroyed()), this, SLOT(disconnectVideoSource()));
		
		//qDebug() << "GLVideoDrawable::setVideoSource(): "<<objectName()<<" m_source:"<<m_source;
		//setVideoFormat(m_source->videoFormat());
		
		frameReady();
	}
	else
	{
		qDebug() << "SharedMemorySender::setVideoSource(): "<<this<<" Source is NULL";
	}

}

void SharedMemorySender::disconnectVideoSource()
{
	if(!m_source)
		return;
	disconnect(m_source, 0, this, 0);
	m_source = 0;
}


void SharedMemorySender::frameReady()
{
	if(!m_source)
		return;
	
	VideoFramePtr frame = m_source->frame();
	if(!frame || !frame->isValid())
	{
		//qDebug() << "SharedMemorySender::frameReady(): Invalid frame or no frame";
		return;
	}
	
	m_frame = frame;
	
	//if(m_transmitFps <= 0)
		processFrame();
}


void SharedMemorySender::processFrame()
{
	//qDebug() << "SharedMemorySender::frameReady: Downscaling video for transmission to "<<m_transmitSize;
	// To scale the video frame, first we must convert it to a QImage if its not already an image.
	// If we're lucky, it already is. Otherwise, we have to jump thru hoops to convert the byte 
	// array to a QImage then scale it.
	QImage image;
	if(!m_frame->image().isNull())
	{
		image = m_frame->image();
	}
	else
	{
		const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(m_frame->pixelFormat());
		if(imageFormat != QImage::Format_Invalid)
		{
			image = QImage(m_frame->pointer(),
				m_frame->size().width(),
				m_frame->size().height(),
				m_frame->size().width() *
					(imageFormat == QImage::Format_RGB16  ||
					imageFormat == QImage::Format_RGB555 ||
					imageFormat == QImage::Format_RGB444 ||
					imageFormat == QImage::Format_ARGB4444_Premultiplied ? 2 :
					imageFormat == QImage::Format_RGB888 ||
					imageFormat == QImage::Format_RGB666 ||
					imageFormat == QImage::Format_ARGB6666_Premultiplied ? 3 :
					4),
				imageFormat);
				
			image = image.copy();
			//qDebug() << "Downscaled image from "<<image.byteCount()<<"bytes to "<<scaledImage.byteCount()<<"bytes, orig ptr len:"<<m_frame->pointerLength()<<", orig ptr:"<<m_frame->pointer();
		}
		else
		{
			qDebug() << "VideoFilter::frameImage: Unable to convert pixel format to image format, cannot scale frame. Pixel Format:"<<m_frame->pixelFormat();
		}
	}
	
	if(image.width()  != FRAME_WIDTH ||
	   image.height() != FRAME_HEIGHT)
	{
		image = image.scaled(FRAME_WIDTH,FRAME_HEIGHT,Qt::IgnoreAspectRatio);
	}
	
	if(image.format() != FRAME_FORMAT)
		image = image.convertToFormat(FRAME_FORMAT);
		
	//image.save("/mnt/phc/Video/tests/sent-frame.jpg");
		
	int size = image.byteCount();
	const uchar *from = (const uchar*)image.bits();
	
	m_sharedMemory.lock();
	uchar *to = (uchar*)m_sharedMemory.data();
	memset(to, 0, m_sharedMemory.size());
	
	//qDebug() << "SharedMemorySender::processFrame: ShMem size: "<<m_sharedMemory.size()<<", image bytes:"<<size<<", size:"<<image.size()<<", format:"<<image.format();
	memcpy(to, from, qMin(m_sharedMemory.size(), size));
	m_sharedMemory.unlock();
}
