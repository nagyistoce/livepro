#include "VideoSource.h"

#include "VideoWidget.h"

//////////////

//#ifndef UINT64_C
//#define UINT64_C(val) val##ui64

// 7.18.4.2 Macros for greatest-width integer constants
//#define INTMAX_C   INT64_C
//#define UINTMAX_C  UINT64_C

#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif



extern "C" {
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

bool VideoSource::isLibAVInit = false;

void VideoSource::initAV()
{
	if(isLibAVInit)
		return;
	isLibAVInit = true;
	
	//qDebug() << "VideoSource::initAV()";
	
	avcodec_init();
	avcodec_register_all();
	avdevice_register_all();
	av_register_all();
}

//////////////

VideoSource::VideoSource(QObject *parent)
	: QThread(parent)
	, m_killed(false)
	, m_isBuffered(true)
	, m_singleFrame(0)
	, m_autoDestroy(true)
{
	m_frameQueue.setMaxByteSize(1024 * 1024 //  1 MB
				         *   64 // 64 MB
				   );

	initAV();
}

void VideoSource::registerConsumer(QObject *consumer)
{
	m_consumerList.append(consumer);
	connect(consumer, SIGNAL(destroyed()), this, SLOT(consumerDestroyed()));
	consumerRegistered(consumer);
	//qDebug() << "VideoSource::registerConsumer(): "<<this<<": consumer list:"<<m_consumerList.size(); //m_refCount:"<<m_refCount;
}

VideoSource::~VideoSource() 
{
	if(!m_killed)
		destroySource();
}

void VideoSource::run()
{
	exec();
}

VideoFramePtr VideoSource::frame()
{
	QMutexLocker lock(&m_queueMutex);
	if(!m_isBuffered ||
	    m_frameQueue.isEmpty())
	{
		#ifdef DEBUG_VIDEOFRAME_POINTERS
		qDebug() << "VideoSource::frame(): Testing validity of m_singleFrame:"<<m_singleFrame;
		#endif
		
		#ifdef DEBUG_VIDEOFRAME_POINTERS
		qDebug() << "VideoSource::frame(): Returning m_singleFrame:"<<m_singleFrame;
		#endif
		
		return m_singleFrame;
	}
	//qDebug() << "VideoSource::frame(): Queue size: "<<m_frameQueue.size();
	VideoFramePtr frame = m_frameQueue.dequeue();
	#ifdef DEBUG_VIDEOFRAME_POINTERS
	qDebug() << "VideoSource::frame(): Returning frame from queue:"<<frame;
	#endif
	
	return frame;
}

void VideoSource::enqueue(VideoFrame *frame)
{
	//qDebug() << "VideoSource::enqueue(1): "<<frame; 
	
	//m_queueMutex.lock();
	
	VideoFramePtr ptr(frame);
	enqueue(ptr);
	
// 	if(m_isBuffered)
// 		m_frameQueue.enqueue(ptr);
// 	m_singleFrame = ptr;
// 	
// 	//qDebug() << "VideoSource::enqueue(): "<<this<<" m_isBuffered:"<<m_isBuffered<<", Queue size: "<<m_frameQueue.size();
// 	m_queueMutex.unlock();
// 	
// 	emit frameReady();
}


void VideoSource::enqueue(VideoFramePtr ptr)
{
	//qDebug() << "VideoSource::enqueue(2): "<<ptr;
	//QMutexLocker lock(&m_queueMutex);
	m_queueMutex.lock();
	if(m_isBuffered)
		m_frameQueue.enqueue(ptr);
	m_singleFrame = ptr;
	
	//qDebug() << "VideoSource::enqueue(): "<<this<<" m_isBuffered:"<<m_isBuffered<<", Queue size: "<<m_frameQueue.size();
	m_queueMutex.unlock();
	
	emit frameReady();
}

void VideoSource::setIsBuffered(bool flag)
{
	m_isBuffered = flag;
	if(!flag)
		m_frameQueue.clear();
}

void VideoSource::consumerDestroyed()
{
	release(sender());
}

void VideoSource::release(QObject *consumer)
{
	if(!consumer)
		return;
		
	if(!m_consumerList.contains(consumer))
		return;
		
	m_consumerList.removeAll(consumer);
	
	consumerReleased(consumer);
	//m_refCount --;
	//qDebug() << "VideoSource::release(): "<<this<<": consumer list:"<<m_consumerList.size(); //m_refCount:"<<m_refCount;
	//if(m_refCount <= 0)
	if(m_consumerList.isEmpty() &&
	   m_autoDestroy)
		destroySource();
}

void VideoSource::setAutoDestroy(bool flag)
{
	m_autoDestroy = flag;
}

void VideoSource::destroySource()
{
	//qDebug() << "VideoSource::destroySource(): "<<this;
	m_killed = true;
	quit();
	wait();
	deleteLater();
}
