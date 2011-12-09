
#include <QImage>
#include <QMutex>
#include <QMutexLocker>
#include <QStringList>
#include <QDebug>
#include <QApplication>

#include <assert.h>

#include "DVizSharedMemoryThread.h"

// For defenition of FRAME_*
#include "../glvidtex/SharedMemorySender.h"

#ifndef Q_OS_WIN
#define V4L_OUTPUT "/dev/video2"
#endif

#ifdef V4L_OUTPUT
	//#define V4L_QIMAGE_FORMAT QImage::Format_RGB555

// 	#define V4L_NATIVE_FORMAT VIDEO_PALETTE_RGB32
// 	#define V4L_QIMAGE_FORMAT QImage::Format_RGB32

	#define V4L_NATIVE_FORMAT VIDEO_PALETTE_RGB24
	#define V4L_QIMAGE_FORMAT QImage::Format_RGB888

// 	#define V4L_NATIVE_FORMAT VIDEO_PALETTE_RGB565
// 	#define V4L_QIMAGE_FORMAT QImage::Format_RGB16

// 	#define V4L_NATIVE_FORMAT VIDEO_PALETTE_RGB555
// 	#define V4L_QIMAGE_FORMAT QImage::Format_RGB555


	extern "C" {
		#include <unistd.h>
		#include <stdlib.h>
		#include <stdio.h>
		#include <fcntl.h>
		#include <string.h>
		#include <errno.h>
		#include <sys/ioctl.h>
		#include <sys/mman.h>
		#include <signal.h>
		#include <sys/wait.h>
		#include <linux/videodev.h>
	}

	int start_pipe (int dev, int width, int height)
	{
		struct video_capability vid_caps;
		struct video_window vid_win;
		struct video_picture vid_pic;

		if (ioctl(dev, VIDIOCGCAP, &vid_caps) == -1) {
			printf ("ioctl (VIDIOCGCAP)\nError[%s]\n", strerror(errno));
			return (1);
		}

		if (ioctl(dev, VIDIOCGPICT, &vid_pic)== -1) {
			printf ("ioctl VIDIOCGPICT\nError[%s]\n", strerror(errno));
			return (1);
		}

		//vid_pic.palette = VIDEO_PALETTE_RGB555;
		//vid_pic.palette = VIDEO_PALETTE_RGB24;
		vid_pic.palette = V4L_NATIVE_FORMAT;


		if (ioctl(dev, VIDIOCSPICT, &vid_pic) == -1) {
			printf ("ioctl VIDIOCSPICT\nError[%s]\n", strerror(errno));
			return (1);
		}

		if (ioctl (dev, VIDIOCGWIN, &vid_win) == -1) {
			printf ("ioctl VIDIOCGWIN\nError[%s]\n", strerror(errno));
			return (1);
		}

		vid_win.width  = width;
		vid_win.height = height;

		if (ioctl(dev, VIDIOCSWIN, &vid_win)== -1) {
			printf ("ioctl VIDIOCSWIN\nError[%s]\n", strerror(errno));
			return (1);
		}
		return 0;
	}

#endif

QMap<QString,DVizSharedMemoryThread *> DVizSharedMemoryThread::m_threadMap;
QMutex DVizSharedMemoryThread::threadCacheMutex;

DVizSharedMemoryThread::DVizSharedMemoryThread(const QString& key, QObject *parent)
	: VideoSource(parent)
	, m_fps(30)
	, m_key(key)
	, m_timeAccum(0)
	, m_frameCount(0)
	, m_v4lOutputDev(-1)
{
	connect(&m_readTimer, SIGNAL(timeout()), this, SLOT(readFrame()));
	m_readTimer.setInterval(1000/m_fps);

	setIsBuffered(true);

	#ifdef V4L_OUTPUT
	{
		m_v4lOutputDev = open (V4L_OUTPUT, O_RDWR);
		if (m_v4lOutputDev < 0)
		{
			qDebug() << "DVizSharedMemoryThread: Failed to open output video device '"<<V4L_OUTPUT<<"', error: "<<strerror(errno);
		}
		else
		{
			start_pipe(m_v4lOutputDev, FRAME_WIDTH, FRAME_HEIGHT);
			qDebug() << "DVizSharedMemoryThread: Opened V4L output to filenum "<<m_v4lOutputDev<<", size:"<<FRAME_WIDTH<<"x"<<FRAME_HEIGHT;
		}
	}
	//#else
	//	qDebug() << "DVizSharedMemoryThread: V4L output not enabled, defined value: '"<<V4L_OUTPUT<<"'";
	#endif
}

void DVizSharedMemoryThread::destroySource()
{
	qDebug() << "DVizSharedMemoryThread::destroySource(): "<<this;
	QMutexLocker lock(&threadCacheMutex);
	m_threadMap.remove(m_key);
	m_sharedMemory.detach();

	VideoSource::destroySource();
}

DVizSharedMemoryThread * DVizSharedMemoryThread::threadForKey(const QString& key)
{
	if(key.isEmpty())
		return 0;

	QMutexLocker lock(&threadCacheMutex);

	if(m_threadMap.contains(key))
	{
		DVizSharedMemoryThread *v = m_threadMap[key];
		qDebug() << "DVizSharedMemoryThread::threadForKey(): "<<v<<": "<<key<<": [CACHE HIT] +";
		return v;
	}
	else
	{
		DVizSharedMemoryThread *v = new DVizSharedMemoryThread(key);
		m_threadMap[key] = v;
		qDebug() << "DVizSharedMemoryThread::threadForKey(): "<<v<<": "<<key<<": [CACHE MISS] -";
		v->start();
		v->moveToThread(v);
//		v->m_readTimer.start();

		return v;
	}
}


void DVizSharedMemoryThread::run()
{
	QTime t;

	while(!m_killed)
	{
		t.start();

		readFrame();

		int time = (1000 / m_fps) -  t.elapsed();
		if(time < 20)
			time = 20;
		if(time > 50)
			time = 50;

		msleep(time);
		//qDebug() << "DVizSharedMemoryThread::run(): sleep time:"<<time;
		//yieldCurrentThread();
	}

	m_sharedMemory.detach();
}

void DVizSharedMemoryThread::readFrame()
{
	if(!m_sharedMemory.isAttached())
	{
		m_sharedMemory.setKey(m_key);

		if(m_sharedMemory.isAttached())
			m_sharedMemory.detach();

		m_sharedMemory.attach(QSharedMemory::ReadWrite);
	}


	m_readTime.restart();

	if(!m_sharedMemory.isAttached())
		m_sharedMemory.attach(QSharedMemory::ReadWrite);
	else
	{
		QImage image(FRAME_WIDTH,
				FRAME_HEIGHT,
				FRAME_FORMAT);

		uchar *to = image.scanLine(0);

		m_sharedMemory.lock();
		const uchar *from = (uchar*)m_sharedMemory.data();
		memcpy(to, from, qMin(m_sharedMemory.size(), image.byteCount()));
		QTime time = QTime::currentTime();
		//qDebug() << "DVizSharedMemoryThread::run: capture time:"<<time.second() * 1000 + time.msec(); //ShMem size: "<<m_sharedMemory.size()<<", image size:"<<image.byteCount();

		m_sharedMemory.unlock();

		//image.save("/mnt/phc/Video/tests/frame.jpg");

		#ifdef V4L_OUTPUT
		{
			QImage outImg = image;
			if(outImg.format() != V4L_QIMAGE_FORMAT)
				outImg = outImg.convertToFormat(V4L_QIMAGE_FORMAT);

			if(V4L_NATIVE_FORMAT == VIDEO_PALETTE_RGB24 &&
			   V4L_QIMAGE_FORMAT == QImage::Format_RGB888)
			{
				uchar *bits = outImg.scanLine(0);
				uchar  tmp;
				for(int i=0; i<outImg.byteCount(); i+=3)
				{
					tmp       = bits[i];
					bits[i]   = bits[i+2];
					bits[i+2] = tmp;
				}
			}

			if(write(m_v4lOutputDev, (const uchar*)outImg.bits(), outImg.byteCount()) != outImg.byteCount())
			{
				qDebug() << "DVizSharedMemoryThread::readFrame(): Error writing to "<<V4L_OUTPUT<<" (bytes written does not match bytes requested), V4L error: " << strerror(errno);
			}
			else
			{
				//qDebug() << "DVizSharedMemoryThread::readFrame(): Wrote "<<outImg.byteCount()<<" bytes to "<<V4L_OUTPUT;
			}
		}
		#endif

		//enqueue(new VideoFrame(image.convertToFormat(QImage::Format_RGB555),1000/m_fps));
		VideoFrame *frame = new VideoFrame(image,1000/m_fps);
		frame->setCaptureTime(time);
		enqueue(frame);

		emit frameReady();
	}


	m_frameCount ++;
	m_timeAccum  += m_readTime.elapsed();

	if(m_frameCount % m_fps == 0)
	{
		QString msPerFrame;
		msPerFrame.setNum(((double)m_timeAccum) / ((double)m_frameCount), 'f', 2);

		qDebug() << "DVizSharedMemoryThread::readFrame(): Avg MS per Frame:"<<msPerFrame;
	}

	if(m_frameCount % (m_fps * 10) == 0)
	{
		m_timeAccum  = 0;
		m_frameCount = 0;
	}
}


void DVizSharedMemoryThread::setFps(int fps)
{
	m_fps = fps;
	m_readTimer.setInterval(1000/m_fps);
}

DVizSharedMemoryThread::~DVizSharedMemoryThread()
{
	m_killed = true;
	quit();
	wait();
}

