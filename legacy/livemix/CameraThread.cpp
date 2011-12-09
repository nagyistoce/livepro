
#include <QImage>
#include <QMutex>
#include <QMutexLocker>
#include <QStringList>
#include <QDebug>
#include <QApplication>

#include <assert.h>

#include <QImageWriter>
#include <QColor>

//#define DEBUG

//#undef ENABLE_DECKLINK_CAPTURE

#include "CameraThread.h"

extern "C" {
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
}

#include "SimpleV4L2.h"


#ifdef ENABLE_DECKLINK_CAPTURE

// BMD needs to be integrated in these areas:
// Enumeration (create custom string, something like /dev/blackmagic/card0)
// Read Frame
// Input enumration/setting

#include "DeckLinkAPI.h"

/// \class BMDCaptureDelegate 
///	Provides integration with Blackmagic DeckLink-compatible device (such as the Intensity Prso) for
///	raw video capturing.
class BMDCaptureDelegate : public IDeckLinkInputCallback
{
	BMDCaptureDelegate(CameraThread *, IDeckLink *);
	
public:
	virtual ~BMDCaptureDelegate();

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID /*iid*/, LPVOID */*ppv*/) { return E_NOINTERFACE; }
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE  Release(void);
	virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents, IDeckLinkDisplayMode*, BMDDetectedVideoInputFormatFlags);
	virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame*, IDeckLinkAudioInputPacket*);
	
	virtual void enableRawFrames(bool flag) { m_rawFrames = flag; }
	
	static QStringList enumDeviceNames(bool forceReload=false);
	static BMDCaptureDelegate *openDevice(QString deviceName, CameraThread *api);

private:
	static QStringList	s_knownDevices;
	
	ULONG			m_refCount;
	pthread_mutex_t		m_mutex;
	CameraThread *		m_api;
	int			m_frameCount;
	
	IDeckLink *		m_deckLink;
	IDeckLinkInput *	m_deckLinkInput;
	IDeckLinkDisplayModeIterator * m_displayModeIterator;
	
	IDeckLinkOutput *	   m_deckLinkOutput;
	IDeckLinkVideoConversion * m_deckLinkConverter;
	
	AVFrame *allocPicture(enum PixelFormat pix_fmt, int width, int height);
	struct SwsContext *	m_swsContext;
	QSize 			m_swsInitSize;
	AVFrame *		m_yuvPicture;
	AVFrame *		m_rgbPicture;
	
	bool			m_rawFrames;
};

QStringList BMDCaptureDelegate::s_knownDevices = QStringList();

#endif

//#include "ccvt/ccvt.h"

//#define DEINTERLACE 1

#if defined(Q_OS_WIN)
// 	#ifdef DEINTERLACE
		#define RAW_PIX_FMT PIX_FMT_BGR32
// 	#else
// 		#define RAW_PIX_FMT PIX_FMT_BGR565
// 	#endif
#else
// 	#ifdef DEINTERLACE
		#define RAW_PIX_FMT PIX_FMT_RGB32
// 	#else
// 		#define RAW_PIX_FMT PIX_FMT_RGB565
// 	#endif
#endif

namespace
{
	// NOTE: This code was copied from the iLab saliency project by USC
	// found at http://ilab.usc.edu/source/. The note following is from
	// the USC code base. I've just converted it to use uchar* instead of
	// the iLab (I assume) specific typedef 'byte'. - Josiah 20100730

void bobDeinterlace(const uchar* src, const uchar* const srcend,
		    uchar* dest, uchar* const destend,
		    const int height, const int stride,
		    const bool in_bottom_field)
{

	// NOTE: this deinterlacing code was derived from and/or inspired by
	// code from tvtime (http://tvtime.sourceforge.net/); original
	// copyright notice here:

	/**
	* Copyright (c) 2001, 2002, 2003 Billy Biggs <vektor@dumbterm.net>.
	*
	* This program is free software; you can redistribute it and/or modify
	* it under the terms of the GNU General Public License as published by
	* the Free Software Foundation; either version 2, or (at your option)
	* any later version.
	*
	* This program is distributed in the hope that it will be useful,
	* but WITHOUT ANY WARRANTY; without even the implied warranty of
	* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	* GNU General Public License for more details.
	*
	* You should have received a copy of the GNU General Public License
	* along with this program; if not, write to the Free Software Foundation,
	* Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
	*/

	assert(height > 0);
	assert(stride > 0);

	// NOTE: on x86 machines with glibc 2.3.6 and g++ 3.4.4, it looks
	// like std::copy is faster than memcpy, so we use that to do the
	// copying here:

#if 1
#  define DOCOPY(dst,src,n) std::copy((src),(src)+(n),(dst))
#else
#  define DOCOPY(dst,src,n) memcpy((dst),(src),(n))
#endif

	if (in_bottom_field)
	{
		src += stride;

		DOCOPY(dest, src, stride);

		dest += stride;
	}

	DOCOPY(dest, src, stride);

	dest += stride;

	const int N = (height / 2) - 1;
	for (int i = 0; i < N; ++i)
	{
		const uchar* const src2 = src + (stride*2);

		for (int k = 0; k < stride; ++k)
			dest[k] = (src[k] + src2[k]) / 2;

		dest += stride;

		DOCOPY(dest, src2, stride);

		src += stride*2;
		dest += stride;
	}

	if (!in_bottom_field)
	{
		DOCOPY(dest, src, stride);

		src += stride*2;
		dest += stride;
	}
	else
		src += stride;

	// consistency check: make sure we've done all our counting right:

	if (src != srcend)
		qFatal("deinterlacing consistency check failed: %d src %p-%p=%d",
			int(in_bottom_field), src, srcend, int(src-srcend));

	if (dest != destend)
		qFatal("deinterlacing consistency check failed: %d dst %p-%p=%d",
			int(in_bottom_field), dest, destend, int(dest-destend));
}

#undef DOCOPY

}


QMap<QString,CameraThread *> CameraThread::m_threadMap;
QStringList CameraThread::m_enumeratedDevices;
bool CameraThread::m_devicesEnumerated = false;
QMutex CameraThread::threadCacheMutex;

CameraThread::CameraThread(const QString& camera, QObject *parent)
	: VideoSource(parent)
	, m_fps(30)
	, m_inited(false)
	, m_cameraFile(camera)
	, m_frameCount(0)
	, m_deinterlace(false)
	, m_v4l2(0)
	, m_bmd(0)
	, m_error(false)
	, m_buffer(0)
	, m_av_rgb_frame(0)
{
	m_time_base_rational.num = 1;
	m_time_base_rational.den = AV_TIME_BASE;

	m_sws_context = NULL;
	m_frame = NULL;

	m_rawFrames = false;

	setIsBuffered(false);
}

void CameraThread::destroySource()
{
	#ifdef DEBUG
	qDebug() << "CameraThread::destroySource(): "<<this;
	#endif
	QMutexLocker lock(&threadCacheMutex);
	m_threadMap.remove(m_cameraFile);

	VideoSource::destroySource();
}

CameraThread * CameraThread::threadForCamera(const QString& camera)
{
	if(camera.isEmpty())
		return 0;

	QStringList devices = enumerateDevices();

	if(!devices.contains(camera))
		return 0;

	QMutexLocker lock(&threadCacheMutex);

	if(m_threadMap.contains(camera))
	{
		CameraThread *v = m_threadMap[camera];
		v->m_refCount++;
		#ifdef DEBUG
 		qDebug() << "CameraThread::threadForCamera(): "<<v<<": "<<camera<<": [CACHE HIT] +";
 		#endif
		return v;
	}
	else
	{
		CameraThread *v = new CameraThread(camera);
		m_threadMap[camera] = v;
		v->m_refCount=1;
 		#ifdef DEBUG
 		qDebug() << "CameraThread::threadForCamera(): "<<v<<": "<<camera<<": [CACHE MISS] -";
 		#endif
//  		v->initCamera();
		v->start(QThread::NormalPriority); //QThread::HighPriority);
		usleep(750 * 1000); // give it half a sec or so to init

		return v;
	}
}


QStringList CameraThread::enumerateDevices(bool forceReenum)
{
	VideoSource::initAV();

	if(!forceReenum && m_devicesEnumerated)
		return m_enumeratedDevices;

	m_enumeratedDevices.clear();
	m_devicesEnumerated = true;

	#ifdef Q_OS_WIN32
		QString deviceBase = "vfwcap://";
		QString formatName = "vfwcap";
	#else
		QString deviceBase = "/dev/video";
		QString formatName = "video4linux";
	#endif
	QStringList list;


	AVInputFormat *inFmt = NULL;
	AVFormatParameters formatParams;

	for(int i=0; i<10; i++)
	{
		memset(&formatParams, 0, sizeof(AVFormatParameters));

		#ifdef Q_OS_WIN32
			QString file = QString::number(i);
		#else
			QString file = QString("/dev/video%1").arg(i);
		#endif

		inFmt = av_find_input_format(qPrintable(formatName));
		if( !inFmt )
		{
			qDebug() << "[ERROR] CameraThread::enumerateDevices(): Unable to find input format:"<<formatName;
			break;
		}

		formatParams.time_base.num = 1;
		formatParams.time_base.den = 29; //25;
		formatParams.channel = 0;
		formatParams.standard = "ntsc";

		//formatParams.width = 352;
		//formatParams.height = 288;
		//formatParams.channel = 0;
		//formatParams.pix_fmt = PIX_FMT_RGB24 ;

		// Open video file
		//
		AVFormatContext * formatCtx;
		if(av_open_input_file(&formatCtx, qPrintable(file), inFmt, 0, &formatParams) != 0)
		//if(av_open_input_file(&m_av_format_context, "1", inFmt, 0, NULL) != 0)
		{
			//qDebug() << "[WARN] CameraThread::load(): av_open_input_file() failed, file:"<<file;
			break;
		}
		else
		{
			list << QString("%1%2").arg(deviceBase).arg(i);
			av_close_input_file(formatCtx);
		}
	}

	#ifdef ENABLE_DECKLINK_CAPTURE
	list << BMDCaptureDelegate::enumDeviceNames(forceReenum);
	#endif
	
	#ifdef DEBUG
	qDebug() << "CameraThread::enumerateDevices: Found: "<<list;
	#endif

	m_enumeratedDevices = list;
	
	return list;
}

QStringList CameraThread::inputs()
{
	#if !defined(Q_OS_LINUX)
		return QStringList() << "Default";
	#endif

	SimpleV4L2 * api = m_v4l2;
	bool deleteIt = false;
	if(!api)
	{
		api = new SimpleV4L2();
		deleteIt = true;
		if(!api->openDevice(qPrintable(m_cameraFile)))
		{
			delete api;
			return QStringList() << "Default";
		}
	}

	QStringList inputs = api->inputs();

	if(deleteIt)
		delete api;
	return inputs;
}

int CameraThread::input()
{
	#if !defined(Q_OS_LINUX)
		return 0;
	#endif

	SimpleV4L2 * api = m_v4l2;
	bool deleteIt = false;
	if(!api)
	{
		api = new SimpleV4L2();
		deleteIt = true;
		if(!api->openDevice(qPrintable(m_cameraFile)))
		{
			delete api;
			return 0;
		}
	}

	int idx = api->input();

	if(deleteIt)
		delete api;

	return idx;
}

void CameraThread::setInput(int idx)
{
	#if !defined(Q_OS_LINUX)
		return;
	#endif

	SimpleV4L2 * api = m_v4l2;
	bool deleteIt = false;
	if(!api)
	{
		api = new SimpleV4L2();
		deleteIt = true;
		if(!api->openDevice(qPrintable(m_cameraFile)))
		{
			delete api;
			return;
		}
	}

	api->setInput(idx);
	api->setStandard("NTSC");

	if(deleteIt)
		delete api;
}

bool CameraThread::setInput(const QString& name)
{
	#if !defined(Q_OS_LINUX)
		return false;
	#else
	
	m_inputName = name;


	SimpleV4L2 * api = m_v4l2;
	bool deleteIt = false;
	if(!api)
	{
		api = new SimpleV4L2();
		deleteIt = true;
		if(!api->openDevice(qPrintable(m_cameraFile)))
		{
			delete api;
			return false;
		}
	}

	bool flag = api->setInput(name);
	if(flag)
		api->setStandard("NTSC");

	if(deleteIt)
		delete api;

	return flag;

	#endif
}



int CameraThread::initCamera()
{
 	#ifdef DEBUG
 	qDebug() << "CameraThread::initCamera(): "<<this<<" start";
 	#endif
	QMutexLocker lock(&m_initMutex);
	
	m_inited = false;
	
	#ifdef ENABLE_DECKLINK_CAPTURE
	if(m_cameraFile.startsWith("bmd:"))
	{
		m_bmd = BMDCaptureDelegate::openDevice(m_cameraFile, this);
		if(!m_bmd)
		{
			qDebug() << "CameraThread::initCamera(): "<<this<<" Unable to open Blackmagic input:"<<m_cameraFile<<". No capturing will be done.";
			return 0;
		}
		else
		{
			qDebug() << "CameraThread::initCamera(): "<<this<<" Opened Blackmagic input:"<<m_cameraFile;
			return 1;
		}
	}
	#endif
	
	#if defined(Q_OS_LINUX)
	if(m_rawFrames)
	{
		m_v4l2 = new SimpleV4L2();
		if(m_v4l2->openDevice(qPrintable(m_cameraFile)))
		{
			// Do the code here so that we dont have to open the device twice
			if(!m_inputName.isEmpty())
			{
				setInput(m_inputName);
			}
			else
			{
				if(!setInput("Composite"))
					if(!setInput("Composite1"))
						setInput(0);
			}

			m_v4l2->initDevice();
			if(!m_v4l2->startCapturing())
			{
				m_error = true;
				return 0;
			}
			else
			{
				m_inited = true;
				
	 			#ifdef DEBUG
	 			qDebug() << "CameraThread::initCamera(): "<<this<<" finish2";
	 			#endif
	
				return 1;
			}
		}
		else
		{
			m_error = true;
			return 0;
		}
	}
	#endif
	
	if(m_inited)
	{
		#ifdef DEBUG
 		qDebug() << "CameraThread::initCamera(): "<<this<<" finish3";
 		#endif
		return 1;
	}

	// And do it here even if we're not using raw frames because setInput() will use SimpleV4L2 on linux,
	// and its a NOOP on windows.
	if(!setInput("Composite"))
		if(!setInput("Composite1"))
			setInput(0);


	AVInputFormat *inFmt = NULL;
	AVFormatParameters formatParams;
	memset(&formatParams, 0, sizeof(AVFormatParameters));

	QString fileTmp = m_cameraFile;

	#ifdef Q_OS_WIN32
	QString fmt = "vfwcap";
	if(fileTmp.startsWith("vfwcap://"))
		fileTmp = fileTmp.replace("vfwcap://","");
	#else
	QString fmt = "video4linux";
	#endif

	#ifdef DEBUG
	qDebug() << "[DEBUG] CameraThread::load(): "<<this<<" fmt:"<<fmt<<", filetmp:"<<fileTmp;
	#endif

	inFmt = av_find_input_format(qPrintable(fmt));
	if( !inFmt )
	{
		qDebug() << "[ERROR] CameraThread::load(): "<<this<<" Unable to find input format:"<<fmt;
		return -1;
	}

	formatParams.time_base.num = 1;
	formatParams.time_base.den = 35; //25;
// 	formatParams.width = 352;
// 	formatParams.height = 288;
	formatParams.width = 640;
	formatParams.height = 480;
// 	formatParams.channel = 1;
	//formatParams.pix_fmt = PIX_FMT_RGB24 ;


	// Open video file
	 //
	if(av_open_input_file(&m_av_format_context, qPrintable(fileTmp), inFmt, 0, &formatParams) != 0)
	//if(av_open_input_file(&m_av_format_context, "1", inFmt, 0, NULL) != 0)
	{
		qDebug() << "[WARN] CameraThread::load(): "<<this<<" av_open_input_file() failed, fileTmp:"<<fileTmp;
		return false;
	}

	//dump_format(m_av_format_context, 0, qPrintable(m_cameraFile), 0);
	#ifdef DEBUG
	qDebug() << "[DEBUG] dump_format(): "<<this<<" ";
	dump_format(m_av_format_context, 0, qPrintable(fileTmp), false);
	#endif

	uint i;

	// Find the first video stream
	m_video_stream = -1;
	m_audio_stream = -1;
	for(i = 0; i < m_av_format_context->nb_streams; i++)
	{
		if(m_av_format_context->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
		{
			m_video_stream = i;
		}
		if(m_av_format_context->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO)
		{
			m_audio_stream = i;
		}
	}
	if(m_video_stream == -1)
	{
		qDebug() << "[WARN] CameraThread::load(): "<<this<<" Cannot find video stream.";
		return false;
	}

	// Get a pointer to the codec context for the video and audio streams
	m_video_codec_context = m_av_format_context->streams[m_video_stream]->codec;
// 	m_video_codec_context->get_buffer = our_get_buffer;
// 	m_video_codec_context->release_buffer = our_release_buffer;

	// Find the decoder for the video stream
	m_video_codec = avcodec_find_decoder(m_video_codec_context->codec_id);
	if(m_video_codec == NULL)
	{
		qDebug() << "[WARN] CameraThread::load(): "<<this<<" avcodec_find_decoder() failed for codec_id:" << m_video_codec_context->codec_id;
		//return false;
	}

	// Open codec
	if(avcodec_open(m_video_codec_context, m_video_codec) < 0)
	{
		qDebug() << "[WARN] CameraThread::load(): "<<this<<" avcodec_open() failed.";
		//return false;
	}

	// Allocate video frame
	m_av_frame = avcodec_alloc_frame();

	// Allocate an AVFrame structure
	m_av_rgb_frame =avcodec_alloc_frame();
	if(m_av_rgb_frame == NULL)
	{
		qDebug() << "[WARN] CameraThread::load(): "<<this<<" avcodec_alloc_frame() failed.";
		return false;
	}

	#ifdef DEBUG
	qDebug() << "[DEBUG] "<<this<<" codec context size:"<<m_video_codec_context->width<<"x"<<m_video_codec_context->height;
	#endif

	// Determine required buffer size and allocate buffer
	int num_bytes = avpicture_get_size(RAW_PIX_FMT, m_video_codec_context->width, m_video_codec_context->height);

	m_buffer = (uint8_t *)av_malloc(num_bytes * sizeof(uint8_t));

	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset of AVPicture
	avpicture_fill((AVPicture *)m_av_rgb_frame, m_buffer, RAW_PIX_FMT,
					m_video_codec_context->width, m_video_codec_context->height);

	if(m_audio_stream != -1)
	{
		m_audio_codec_context = m_av_format_context->streams[m_audio_stream]->codec;

		m_audio_codec = avcodec_find_decoder(m_audio_codec_context->codec_id);
		if(!m_audio_codec)
		{
			//unsupported codec
			return false;
		}
		avcodec_open(m_audio_codec_context, m_audio_codec);
	}

	m_timebase = m_av_format_context->streams[m_video_stream]->time_base;

	m_inited = true;
	#ifdef DEBUG
 	qDebug() << "CameraThread::initCamera(): "<<this<<" finish";
 	#endif
	return true;
}

void CameraThread::start(QThread::Priority priority)
{
	QThread::start(priority);
	
	/*
	connect(&m_readTimer, SIGNAL(timeout()), this, SLOT(readFrame()));
	double finalFps = m_fps * 1.5 * (m_deinterlace ? 2 : 1);
	qDebug() << "CameraThread::start: m_fps:"<<m_fps<<", finalFps:"<<finalFps;
	m_readTimer.setInterval(1000 / finalFps);
	initCamera();
	m_readTimer.start();
	*/
}

void CameraThread::run()
{
 	initCamera();

	
	#ifdef DEBUG
	qDebug() << "CameraThread::run: "<<this<<" In Thread ID "<<QThread::currentThreadId();
	#endif
	
// 	int counter = 0;
	while(!m_killed)
	{
		#ifdef ENABLE_DECKLINK_CAPTURE
		if(!m_bmd)
		#endif
			readFrame();

// 		counter ++;
// 		if(m_singleFrame.holdTime>0)
// 		{
// 			QString file = QString("frame-%1.jpg").arg(counter %2 == 0?"even":"odd");
// 			qDebug() << "CameraThread::run(): frame:"<<counter<<", writing to file:"<<file;
// 			QImageWriter writer(file, "jpg");
// 			writer.write(m_singleFrame.image);
// 		}

		msleep(int(1000 / m_fps / 1.5 / (m_deinterlace ? 1 : 2)));
		//msleep(int(1000 / m_fps / 1.5));// (m_deinterlace ? 1 : 2)));
	};
}

void CameraThread::setDeinterlace(bool flag)
{
	m_deinterlace = flag;
}

void CameraThread::setFps(int fps)
{
	m_fps = fps;
}

CameraThread::~CameraThread()
{

	m_killed = true;
	quit();
	wait();

	#if defined(Q_OS_LINUX)
	if(m_v4l2)
	{
		delete m_v4l2;
		m_v4l2 = 0;
	}
	#endif
	
	#ifdef ENABLE_DECKLINK_CAPTURE
	if(m_bmd)
	{
		delete m_bmd;
		m_bmd = 0;
	}
	#endif

	freeResources();

	if(m_sws_context != NULL)
	{
		sws_freeContext(m_sws_context);
		m_sws_context = NULL;
	}

	if(m_frame != NULL)
	{
		delete m_frame;
		m_frame = 0;
	}
}

void CameraThread::freeResources()
{
	if(!m_inited)
		return;

	// Free the RGB image
	if(m_buffer != NULL)
	{
		av_free(m_buffer);
		m_buffer = 0;
	}
	if(m_av_rgb_frame != NULL)
	{
		av_free(m_av_rgb_frame);
		m_av_rgb_frame = 0;
	}

	// Free the YUV frame
	//av_free(m_av_frame);
	//mutex.unlock();

	// Close the codec
	if(m_video_codec_context != NULL)
	{
		avcodec_close(m_video_codec_context);
		m_video_codec_context = 0;
	}

	// Close the video file
	if(m_av_format_context != NULL)
	{
		av_close_input_file(m_av_format_context);
		m_av_format_context = 0;
	}
}

void CameraThread::enableRawFrames(bool enable)
{
	QMutexLocker lock(&m_readMutex);
	
	bool old = m_rawFrames;
	m_rawFrames = enable;

	
	if(old != enable)
	{
		#ifdef ENABLE_DECKLINK_CAPTURE
		if(m_bmd)
		{
			#ifdef DEBUG
			qDebug() << "CameraThread::enableRawFrames(): "<<this<<" flag:"<<enable<<", telling BMDCaptureDelegate "<<m_bmd<<" about the new flag";
			#endif
			
			m_bmd->enableRawFrames(enable);
		}
		else
		#endif
		{
			//qDebug() << "CameraThread::enableRawFrames(): "<<this<<" start, flag:"<<enable;
			m_initMutex.lock(); // make sure init isnt running, block while it is
			// switch from raw V4L2 to LibAV* (or visa versa based on m_rawFrames, since SimpleV4L2 only outputs raw ARGB32)
			//qDebug() << "CameraThread::enableRawFrames(): "<<this<<" flag changed, status: "<<m_rawFrames;
			
			freeResources();
			
			m_initMutex.unlock(); // dont block init now
			
			//qDebug() << "CameraThread::enableRawFrames(): "<<this<<" mark1";
			initCamera();
			
			//qDebug() << "CameraThread::enableRawFrames(): "<<this<<" finish";
		}
	}

}

VideoFormat CameraThread::videoFormat()
{

	return VideoFormat(
		m_rawFrames ?
				 VideoFrame::BUFFER_POINTER :
				 VideoFrame::BUFFER_IMAGE,
		m_rawFrames ?
			#if defined(Q_OS_LINUX)
				 QVideoFrame::Format_RGB32  :
			#else
				 QVideoFrame::Format_YUV420P :
			#endif
				 QVideoFrame::Format_ARGB32 //_Premultiplied
	);

	// Size defaults to 640,480
}

void CameraThread::imageDataAvailable(QImage img, QTime capTime)
{
	QMutexLocker lock(&m_readMutex);

	VideoFramePtr frame = VideoFramePtr(new VideoFrame(img.copy(), 1000/30));
	frame->setCaptureTime(capTime);
	
	//qDebug() << "CameraThread::rawDataAvailable: QImage BMD frame, KB:"<<img.byteCount()/1024<<", pixels:"<<img.size();
	
	enqueue(frame);
}

void CameraThread::rawDataAvailable(uchar *bytes, int size, QSize pxSize, QTime capTime)
{
	QMutexLocker lock(&m_readMutex);

	VideoFrame *frame = new VideoFrame();
	//frame->setPixelFormat(QVideoFrame::Format_RGB32);
	frame->setPixelFormat(QVideoFrame::Format_UYVY);
	frame->setCaptureTime(capTime);
	frame->setBufferType(VideoFrame::BUFFER_POINTER);
	frame->setHoldTime(1000/30);
	frame->setSize(pxSize);
	
	memcpy(frame->allocPointer(size), bytes, size);
	
	//qDebug() << "CameraThread::rawDataAvailable: raw BMD frame:"<<frame<<", KB:"<<size/1024<<", pixels:"<<pxSize;
	
	enqueue(frame);

			
			
// 	// We can do deinterlacing on these frames because SimpleV4L2 provides raw ARGB32 frames
// 	//m_deinterlace = false;
// 	if(m_deinterlace && frame->pointerLength() > 0)
// 	{
// 		VideoFrame *deinterlacedFrame = new VideoFrame();
// 		deinterlacedFrame->setCaptureTime ( frame->captureTime() );
// 		deinterlacedFrame->setHoldTime    ( frame->holdTime()    );
// 		deinterlacedFrame->setSize	  ( frame->size()    );
// 		deinterlacedFrame->setBufferType(VideoFrame::BUFFER_POINTER);
// 		
// 			// give us a new array, dont mudge the original image
// 		uchar * dest      = deinterlacedFrame->allocPointer(frame->pointerLength());
// 		uchar * src       = frame->pointer();
// 		const int h       = frame->size().height();
// 		const int stride  = frame->size().width()*4; // I can  cheat because I know SimpleV4L2 sends ARGB32 frames, with 4 bytes per pixel
// 		bool bottomFrame  = m_frameCount % 2 == 1;
// 		
// 		bobDeinterlace( (uchar*)src,  (uchar*)src +h*stride,
// 				(uchar*)dest, (uchar*)dest+h*stride,
// 				h, stride, bottomFrame);
// 				
// 		//qDebug() << "CameraThread::enqueue call: deinterlaced raw V4L2 frame:"<<deinterlacedFrame;
// 		enqueue(deinterlacedFrame);
// 		
// 		delete frame;
// 	}
// 	else
// 	{
// 		//qDebug() << "CameraThread::enqueue call: raw V4L2 frame:"<<frame<<", reading at fps:"<<m_fps;
// 		enqueue(frame);
// 	}
// 	
}

void CameraThread::readFrame()
{
	QMutexLocker lock(&m_readMutex);

	QTime capTime = QTime::currentTime();
	//qDebug() << "CameraThread::readFrame(): My Frame Count # "<<m_frameCount ++;
	m_frameCount ++;

	#if defined(Q_OS_LINUX)
	if(m_v4l2 && m_rawFrames)
	{
		if(!m_inited)
		{
			//qDebug() << "CameraThread::readFrame(): Unable to read raw from from V4L interface because not inited, error? "<<m_error;
			return;
		}
			
		VideoFrame *frame = m_v4l2->readFrame();
		if(frame->isValid())
		{
			frame->setCaptureTime(capTime);
			frame->setHoldTime(1000/m_fps);

			// We can do deinterlacing on these frames because SimpleV4L2 provides raw ARGB32 frames
			//m_deinterlace = false;
			if(m_deinterlace && frame->pointerLength() > 0)
			{
				VideoFrame *deinterlacedFrame = new VideoFrame();
				deinterlacedFrame->setCaptureTime ( frame->captureTime() );
 				deinterlacedFrame->setHoldTime    ( frame->holdTime()    );
 				deinterlacedFrame->setSize	  ( frame->size()    );
 				deinterlacedFrame->setBufferType(VideoFrame::BUFFER_POINTER);
				
				 // give us a new array, dont mudge the original image
				uchar * dest      = deinterlacedFrame->allocPointer(frame->pointerLength());
				uchar * src       = frame->pointer();
				const int h       = frame->size().height();
				const int stride  = frame->size().width()*4; // I can  cheat because I know SimpleV4L2 sends ARGB32 frames, with 4 bytes per pixel
				bool bottomFrame  = m_frameCount % 2 == 1;
				
				bobDeinterlace( (uchar*)src,  (uchar*)src +h*stride,
						(uchar*)dest, (uchar*)dest+h*stride,
						h, stride, bottomFrame);
						
				//qDebug() << "CameraThread::enqueue call: deinterlaced raw V4L2 frame:"<<deinterlacedFrame;
				enqueue(deinterlacedFrame);
				
				delete frame;
			}
			else
			{
				//qDebug() << "CameraThread::enqueue call: raw V4L2 frame:"<<frame<<", reading at fps:"<<m_fps;
				enqueue(frame);
			}
		}
		return;
	}
	#endif
	
	return;

	if(!m_inited)
	{
		//emit newImage(QImage());
		//emit frameReady(1000/m_fps);
		return;
	}
	AVPacket pkt1, *packet = &pkt1;
	double pts;



	int frame_finished = 0;
	while(!frame_finished && !m_killed)
	{
		if(av_read_frame(m_av_format_context, packet) >= 0)
		{
			// Is this a packet from the video stream?
			if(packet->stream_index == m_video_stream)
			{
				//global_video_pkt_pts = packet->pts;

//				mutex.lock();
				avcodec_decode_video(m_video_codec_context, m_av_frame, &frame_finished, packet->data, packet->size);
// 				mutex.unlock();

				if(packet->dts == (uint)AV_NOPTS_VALUE &&
						  m_av_frame->opaque &&
				  *(uint64_t*)m_av_frame->opaque != (uint)AV_NOPTS_VALUE)
				{
					pts = *(uint64_t *)m_av_frame->opaque;
				}
				else if(packet->dts != (uint)AV_NOPTS_VALUE)
				{
					pts = packet->dts;
				}
				else
				{
					pts = 0;
				}

				pts *= av_q2d(m_timebase);

				// Did we get a video frame?
				//frame_finished = 1;
				if(frame_finished)
				{

// 					if(m_rawFrames)
// 					{
//  						//qDebug() << "Decode Time: "<<capTime.msecsTo(QTime::currentTime())<<" ms";
// 						VideoFrame *frame = new VideoFrame(1000/m_fps,capTime);
// 						frame->setPointerData(m_av_frame->data, m_av_frame->linesize);
// 						frame->setPixelFormat(QVideoFrame::Format_YUV420P);
// 						frame->setSize(QSize(m_video_codec_context->width, m_video_codec_context->height));
// 						//qDebug() << "CameraThread::enqueue call: raw LibAV YUV420P frame";
// 						enqueue(frame);
// 					}
// 					else
					{
						// Convert the image from its native format to RGB, then copy the image data to a QImage
						if(m_sws_context == NULL)
						{
							//mutex.lock();
							//qDebug() << "Creating software scaler for pix_fmt: "<<m_video_codec_context->pix_fmt;
							m_sws_context = sws_getContext(
								m_video_codec_context->width, m_video_codec_context->height,
								m_video_codec_context->pix_fmt,
								m_video_codec_context->width, m_video_codec_context->height,
								//PIX_FMT_RGB32,SWS_BICUBIC,
								RAW_PIX_FMT, SWS_FAST_BILINEAR,
								NULL, NULL, NULL); //SWS_PRINT_INFO
							//mutex.unlock();
							//printf("decode(): created m_sws_context\n");
						}

						sws_scale(m_sws_context,
							m_av_frame->data,
							m_av_frame->linesize, 0,
							m_video_codec_context->height,
							m_av_rgb_frame->data,
							m_av_rgb_frame->linesize);

						if(m_deinterlace)
						{
							QImage frame(m_video_codec_context->width,
								m_video_codec_context->height,
								QImage::Format_ARGB32);//_Premultiplied);
							// I can cheat and claim premul because I know the video (should) never have alpha

							bool bottomFrame = m_frameCount % 2 == 1;

							uchar * dest = frame.scanLine(0); // use scanLine() instead of bits() to prevent deep copy
							uchar * src  = (uchar*)m_av_rgb_frame->data[0];
							const int h  = m_video_codec_context->height;
							const int stride = frame.bytesPerLine();

							bobDeinterlace( src,  src +h*stride,
									dest, dest+h*stride,
									h, stride, bottomFrame);

							//qDebug() << "CameraThread::enqueue call: deinterlaced QImage ARGB32 frame";
							enqueue(new VideoFrame(frame.copy(),1000/m_fps,capTime));
						}
						else
						{
							QImage frame(m_av_rgb_frame->data[0],
								m_video_codec_context->width,
								m_video_codec_context->height,
								//QImage::Format_RGB16);
								QImage::Format_ARGB32); //_Premultiplied);

							//qDebug() << "CameraThread::enqueue call: QImage ARGB32 frame";
							enqueue(new VideoFrame(frame.copy(),1000/m_fps,capTime));
						}
					}

					av_free_packet(packet);


				}

			}
			else if(packet->stream_index == m_audio_stream)
			{
				//decode audio packet, store in queue
				av_free_packet(packet);
			}
			else
			{
				av_free_packet(packet);
			}
		}
		else
		{
			//emit reachedEnd();
// 			qDebug() << "reachedEnd()";
		}
	}
}


#ifdef ENABLE_DECKLINK_CAPTURE

BMDCaptureDelegate::BMDCaptureDelegate(CameraThread *api, IDeckLink *deck) 
	: m_refCount(0)
	, m_api(api)
	, m_frameCount(0)
	, m_deckLink(deck)
	, m_deckLinkOutput(0)
	, m_deckLinkConverter(0)
	, m_swsContext(0)
	, m_yuvPicture(0)
	, m_rgbPicture(0)
	, m_rawFrames(false)
{
	pthread_mutex_init(&m_mutex, NULL);
	
	
	QString bmdInput = "Composite";
	if(!bmdInput.isEmpty())
	{
		IDeckLinkConfiguration *deckLinkConfiguration = NULL;
		BMDVideoConnection bmdVideoConnection = bmdVideoConnectionComposite;
	
		if (m_deckLink->QueryInterface(IID_IDeckLinkConfiguration, (void**)&deckLinkConfiguration) != S_OK) 
		{
			qDebug() << "BMDCaptureDelegate: "<<api<<" Could not obtain the IDeckLinkConfiguration interface";
		}
		else
		{
			if (bmdInput == "SDI") {
				bmdVideoConnection = bmdVideoConnectionSDI;
		
			} else if(bmdInput == "HDMI") {
				bmdVideoConnection = bmdVideoConnectionHDMI;
		
			} else if(bmdInput == "Component") {
				bmdVideoConnection = bmdVideoConnectionComponent;
		
			} else if(bmdInput == "Composite") {
				bmdVideoConnection = bmdVideoConnectionComposite;
		
			} else if(bmdInput == "S-Video") {
				bmdVideoConnection = bmdVideoConnectionSVideo;
		
			} else if(bmdInput == "Optical-SDI") {
				bmdVideoConnection = bmdVideoConnectionOpticalSDI;
			}
			
			//if (deckLinkConfiguration->SetVideoInputFormat(bmdVideoConnection) != S_OK)
			if (deckLinkConfiguration->SetInt(bmdDeckLinkConfigVideoInputConnection, bmdVideoConnection) != S_OK)
			
				qDebug() << "BMDCaptureDelegate: "<<api<<" Could not set input video connection to "<<bmdInput;
			
			if (deckLinkConfiguration != NULL) 
				deckLinkConfiguration->Release();
		}
	}
	
	if(m_deckLink && 
	   m_deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&m_deckLinkInput) == S_OK)
	{
		m_deckLinkInput->SetCallback(this);
	}
	else
	{
		qDebug() << "BMDCaptureDelegate: "<<api<<" IDeckLink given does not support video input. Video will not be captured.";
	}
	
 	if (m_deckLink && 
 	    m_deckLink->QueryInterface(IID_IDeckLinkOutput, (void**)&m_deckLinkOutput) != S_OK)
 	{
 		qDebug() << "BMDCaptureDelegate: "<<api<<" IDeckLink can not create new frames, therefore cannot convert YUV->RGB, therefore we will not capture video now.";
 		m_deckLinkOutput = NULL;
 		return;
 	}

	#ifdef Q_OS_WIN
	if(CoCreateInstance(CLSID_CDeckLinkVideoConversion, NULL, CLSCTX_ALL,IID_IDeckLinkVideoConversion, (void**)&m_deckLinkConverter) != S_OK)
	#else
	if(!(m_deckLinkConverter = CreateVideoConversionInstance()))
	#endif
 	{
 		qDebug() << "BMDCaptureDelegate: "<<api<<" Cannot create an instance of IID_IDeckLinkVideoConversion, therefore cannot convert YUV->RGB, therefore we will not emit proper RGB frames now.";
 		m_deckLinkConverter = NULL;
 		//return;
 	}

	if(m_deckLinkInput)
	{
		HRESULT result;
		
		// Obtain an IDeckLinkDisplayModeIterator to enumerate the display modes supported on output
		result = m_deckLinkInput->GetDisplayModeIterator(&m_displayModeIterator);
		if (result != S_OK)
		{
			fprintf(stderr, "Could not obtain the video output display mode iterator - result = %08x\n", result);
			qDebug() << "BMDCaptureDelegate: "<<api<<" DeckLink setup failed. Video will not be captured";
			return;
		}
		
		IDeckLinkDisplayMode	*displayMode;
		BMDPixelFormat		pixelFormat = bmdFormat8BitYUV;
		BMDVideoInputFlags	inputFlags = 0;
		BMDDisplayMode		selectedDisplayMode = bmdModeNTSC;
		int			displayModeCount = 0;
		int			videoModeIndex = 0;
		bool			foundDisplayMode = false;
		
		while (m_displayModeIterator->Next(&displayMode) == S_OK)
		{
			if (videoModeIndex == displayModeCount)
			{
				BMDDisplayModeSupport result;
				const char *displayModeName;
				
				foundDisplayMode = true;
				displayMode->GetName(&displayModeName);
				selectedDisplayMode = displayMode->GetDisplayMode();
				
				m_deckLinkInput->DoesSupportVideoMode(selectedDisplayMode, pixelFormat, bmdVideoInputFlagDefault, &result, NULL);
	
				if (result == bmdDisplayModeNotSupported)
				{
					fprintf(stderr, "The display mode %s is not supported with the selected pixel format\n", displayModeName);
					qDebug() << "BMDCaptureDelegate: "<<api<<" DeckLink setup failed. Video will not be captured";
					return;
				}
	
				if (inputFlags & bmdVideoInputDualStream3D)
				{
					if (!(displayMode->GetFlags() & bmdDisplayModeSupports3D))
					{
						fprintf(stderr, "The display mode %s is not supported with 3D\n", displayModeName);
						qDebug() << "BMDCaptureDelegate: "<<api<<" DeckLink setup failed. Video will not be captured";
						return;
					}
				}
				
				break;
			}
			displayModeCount++;
			displayMode->Release();
		}
	
		if (!foundDisplayMode)
		{
			fprintf(stderr, "Invalid mode %d specified\n", videoModeIndex);
			qDebug() << "BMDCaptureDelegate: "<<api<<" DeckLink setup failed. Video will not be captured";
			return;
		}
		
		result = m_deckLinkInput->EnableVideoInput(selectedDisplayMode, pixelFormat, inputFlags);
		if(result != S_OK)
		{
			fprintf(stderr, "Failed to enable video input. Is another application using the card? Does the card need a firmware update?\n");
			qDebug() << "BMDCaptureDelegate: "<<api<<" DeckLink setup failed. Video will not be captured";
			return;
		}
		
		result = m_deckLinkInput->StartStreams();
		if(result != S_OK)
		{
			qDebug() << "BMDCaptureDelegate: "<<api<<" DeckLink StartStreams() failed. Video will not be captured";
			return;
		}
	}
}

BMDCaptureDelegate::~BMDCaptureDelegate()
{
	pthread_mutex_destroy(&m_mutex);
	
	if(m_displayModeIterator != NULL)
	{
		m_displayModeIterator->Release();
		m_displayModeIterator = NULL;
	}
	
	if(m_deckLinkInput)
	{
		m_deckLinkInput->Release();
		m_deckLinkInput = NULL;
	}
	
	if(m_deckLink)
	{
		m_deckLink->Release();
		m_deckLink = NULL;
	}
	
	if(m_yuvPicture)
	{
		av_free(m_yuvPicture->data[0]);
		av_free(m_yuvPicture);
	}
	
	if(m_rgbPicture)
	{
		av_free(m_rgbPicture->data[0]);
		av_free(m_rgbPicture);
	}
	
}

ULONG BMDCaptureDelegate::AddRef(void)
{
	pthread_mutex_lock(&m_mutex);
		m_refCount++;
	pthread_mutex_unlock(&m_mutex);

	return (ULONG)m_refCount;
}

ULONG BMDCaptureDelegate::Release(void)
{
	pthread_mutex_lock(&m_mutex);
		m_refCount--;
	pthread_mutex_unlock(&m_mutex);

	if (m_refCount == 0)
	{
		delete this;
		return 0;
	}

	return (ULONG)m_refCount;
}

AVFrame *BMDCaptureDelegate::allocPicture(enum PixelFormat pix_fmt, int width, int height)
{
	AVFrame *picture;
	uint8_t *picture_buf;
	int size;

	picture = avcodec_alloc_frame();
	if (!picture)
		return NULL;
	size = avpicture_get_size(pix_fmt, width, height);
	picture_buf = (uint8_t*)av_malloc(size *  sizeof(uint8_t));
	if (!picture_buf)
	{
		av_free(picture);
		qDebug() << "BMDCaptureDelegate::allocPicture: "<<m_api<<" Error allocating a new avpicture, size:"<<width<<"x"<<height<<", pix_fmt:"<<pix_fmt<<" - probably out of memory.";
		return NULL;
	}
	avpicture_fill((AVPicture *)picture, picture_buf, pix_fmt, width, height);
	return picture;
}


HRESULT BMDCaptureDelegate::VideoInputFrameArrived(IDeckLinkVideoInputFrame* videoFrame, IDeckLinkAudioInputPacket* /*audioFrame*/)
{
	// These two objects are necessary to create frames and convert from YUV -> RGB
// 	if(!m_deckLinkOutput)
// 		return 0;
		
// 	if(!m_deckLinkConverter)
// 		return 0;
		
	void*	frameBytes;
// 	void*	audioFrameBytes;
	QTime   capTime = QTime::currentTime();
	
	// Handle Video Frame
	if(videoFrame)
	{
		if (videoFrame->GetFlags() & bmdFrameHasNoInputSource)
		{
			//fprintf(stderr, "Frame received (#%d) - No input signal detected\n", m_frameCount);
			//qDebug() << "BMDCaptureDelegate::VideoInputFrameArrived: "<<m_api<<" Frame "<<m_frameCount << " - No input signal detected";
			
			// Nice blue 'no signal' frame
			QImage frame(320,240, QImage::Format_ARGB32);
			frame.fill(QColor(0,0,198).rgba());
			m_api->imageDataAvailable(frame, capTime);
			
		}
		else
		{
// 			fprintf(stderr, "BMDCaptureDelegate::VideoInputFrameArrived: Frame received (#%lu) - Valid Frame - Size: %d bytes\n", 
// 				m_frameCount,
// 				videoFrame->GetRowBytes() * videoFrame->GetHeight());

			//qDebug() << "BMDCaptureDelegate::VideoInputFrameArrived: "<<m_api<<" Frame received: "<<m_frameCount << ", Bytes: "<< videoFrame->GetRowBytes() * videoFrame->GetHeight();
			
			int vWidth = videoFrame->GetWidth();
			int vHeight = videoFrame->GetHeight();

			if(m_swsContext == NULL ||
			   m_swsInitSize.width()  != vWidth ||
			   m_swsInitSize.height() != vHeight)
			{
				if(m_swsContext != NULL)
				{
					sws_freeContext(m_swsContext);
					m_swsContext = NULL;
				}
				
				if(m_yuvPicture)
				{
					av_free(m_yuvPicture->data[0]);
					av_free(m_yuvPicture);
				}
				
				if(m_rgbPicture)
				{
					av_free(m_rgbPicture->data[0]);
					av_free(m_rgbPicture);
				}
				
				
				m_yuvPicture = allocPicture(PIX_FMT_UYVY422, vWidth, vHeight);
				if(!m_yuvPicture)
				{
					qDebug() << "BMDCaptureDelegate::VideoInputFrameArrived: "<<m_api<<" Error allocating m_yuvPicture for sws format conversion.";
					return 0; 
				}
				
				m_rgbPicture = allocPicture(PIX_FMT_RGB32, vWidth, vHeight);
				if(!m_rgbPicture)
				{
					qDebug() << "BMDCaptureDelegate::VideoInputFrameArrived: "<<m_api<<" Error allocating m_yuvPicture for sws format conversion.";
					return 0; 
				}
		
				m_swsContext   = sws_getContext(vWidth, vHeight,
								PIX_FMT_UYVY422,
								vWidth, vHeight,
								PIX_FMT_RGB32, 
								SWS_FAST_BILINEAR,
								//SWS_BICUBIC, 
								NULL, NULL, NULL);
								
				if (m_swsContext == NULL)
				{
					fprintf(stderr, "BMDCaptureDelegate::VideoInputFrameArrived: Cannot initialize the libsws conversion context\n");
					return 0;
				}
				
				m_swsInitSize = QSize(vWidth, vHeight);
				
				qDebug() << "BMDCaptureDelegate::VideoInputFrameArrived: "<<m_api<<" Created new SWS conversion context for size: "<<m_swsInitSize; 
			}
			
			videoFrame->GetBytes(&frameBytes);
// 			if(m_rawFrames)
// 			{
//  				m_api->rawDataAvailable((uchar*)frameBytes, videoFrame->GetRowBytes() * vHeight, QSize(vWidth, vHeight), capTime);
//  			}
// 			else
			if(1)
			{
				m_yuvPicture->data[0] = (uchar*)frameBytes; 
				
				sws_scale(m_swsContext,
					m_yuvPicture->data,
					m_yuvPicture->linesize, 
					0, vHeight,
					m_rgbPicture->data,
					m_rgbPicture->linesize);
				
				//m_api->rawDataAvailable(m_rgbPicture->data[0], m_rgbPicture->linesize[0], m_swsInitSize, capTime);
				
				QImage frame = QImage(m_rgbPicture->data[0],
							vWidth,
							vHeight,
							//QImage::Format_RGB16);
							QImage::Format_ARGB32);
				//m_bufferMutex.unlock();
				m_api->imageDataAvailable(frame, capTime);
			}
			
			if(0)
			{
				IDeckLinkMutableVideoFrame *rgbFrame = NULL;
				HRESULT res = m_deckLinkOutput->CreateVideoFrame(
					videoFrame->GetWidth(),
					videoFrame->GetHeight(),
					videoFrame->GetWidth() * 4,
					bmdFormat8BitBGRA, // TBD: BGRA or RGBA ...or ARGB ?? (ref 8_8_8_8_REV..?)
					bmdFrameFlagDefault,
					&rgbFrame);
				if(res != S_OK)
				{
					qDebug() << "BMDCaptureDelegate::VideoInputFrameArrived: "<<m_api<<" Error creating RGB frame, nothing captured, res:"<<res;
	// 				return 0;
				}
				else
				{
					//qDebug() << "Created frame of size:"<<videoFrame->GetWidth() << "x"<<videoFrame->GetHeight()<<", frame:"<<rgbFrame;
				}
				
				// 	Pixel conversions
				//  Source frame      Target frame
				//  bmdFormat8BitRGBA bmdFormat8BitYUV
				//                    bmdFormat8BitARGB
				//  bmdFormat8BitBGRA bmdFormat8BitYUV
				//  bmdFormat8BitARGB bmdFormat8BitYUV
				if(m_deckLinkConverter)
				{
					m_deckLinkConverter->ConvertFrame(videoFrame, rgbFrame);
					
					rgbFrame->GetBytes(&frameBytes);
					m_api->rawDataAvailable((uchar*)frameBytes, rgbFrame->GetRowBytes() * rgbFrame->GetHeight(), QSize(rgbFrame->GetWidth(), rgbFrame->GetHeight()), capTime);
					
					rgbFrame->Release();
					rgbFrame = NULL;
				}
				else
				{
					qDebug() << "BMDCaptureDelegate::VideoInputFrameArrived: "<<m_api<<" No m_deckLinkConverter available, unable to convert frame.";
				}
			}
			

			
// 			if (videoOutputFile != -1)
// 			{
// 			
// 				videoFrame->GetBytes(&frameBytes);
// 				write(videoOutputFile, frameBytes, videoFrame->GetRowBytes() * videoFrame->GetHeight());
// 				
// 				
// 				if (rightEyeFrame)
// 				{
// 					rightEyeFrame->GetBytes(&frameBytes);
// 					write(videoOutputFile, frameBytes, videoFrame->GetRowBytes() * videoFrame->GetHeight());
// 				}
// 			}
		}
 		
 		m_frameCount++;
	}

	// Handle Audio Frame
// 	if (audioFrame)
// 	{
// 		if (audioOutputFile != -1)
// 		{
// 			audioFrame->GetBytes(&audioFrameBytes);
// 			write(audioOutputFile, audioFrameBytes, audioFrame->GetSampleFrameCount() * g_audioChannels * (g_audioSampleDepth / 8));
// 		}
// 	}
	return S_OK;
}

HRESULT BMDCaptureDelegate::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents /*events*/, IDeckLinkDisplayMode */*mode*/, BMDDetectedVideoInputFormatFlags)
{
	return S_OK;
}

QStringList BMDCaptureDelegate::enumDeviceNames(bool forceReload)
{
	if(!s_knownDevices.isEmpty())
	{
		if(!forceReload)
			return s_knownDevices;
		else	
			s_knownDevices.clear();
	}
		
	IDeckLinkIterator *deckLinkIterator = CreateDeckLinkIteratorInstance();
	if (deckLinkIterator == NULL)
	{
		fprintf(stderr, "BMDCaptureDelegate::enumDeviceNames: A DeckLink iterator could not be created.  The DeckLink drivers may not be installed.\n");
		return QStringList();
	}
	
	IDeckLink	*deckLink;
	IDeckLinkInput	*deckLinkInput;
	
	int index = 0;
	
	// Enumerate all cards in this system
	while (deckLinkIterator->Next(&deckLink) == S_OK)
	{
		if (deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&deckLinkInput) == S_OK)
		{
			s_knownDevices << QString("bmd:%1").arg(index);
			
			deckLinkInput->Release();
			deckLinkInput = NULL;
		}
		
		
		index ++;
		
		// Release the IDeckLink instance when we've finished with it to prevent leaks
		deckLink->Release();
	}
	
	deckLinkIterator->Release();
	qDebug() << "BMDCaptureDelegate::enumDeviceNames(): s_knownDevices:"<<s_knownDevices;
	
	return s_knownDevices;

}

BMDCaptureDelegate *BMDCaptureDelegate::openDevice(QString deviceName, CameraThread *api)
{
	if(enumDeviceNames().indexOf(deviceName) < 0)
		return 0;
		
	QString name = deviceName.replace("bmd:","");
	int cardNum = name.toInt();
	
	IDeckLinkIterator *deckLinkIterator = CreateDeckLinkIteratorInstance();
	if (deckLinkIterator == NULL)
	{
		qDebug() << "BMDCaptureDelegate::openDevice: "<<api<<" A DeckLink iterator could not be created.  The DeckLink drivers may not be installed.";
		return 0;
	}
		
	
	int index = 0;
	
	IDeckLink* deckLink;
	
	// Find the card requested
	while (deckLinkIterator->Next(&deckLink) == S_OK)
	{
		if(index == cardNum)
			return new BMDCaptureDelegate(api, deckLink);
		
		index ++;
		
		// Release the IDeckLink instance when we've finished with it to prevent leaks
		deckLink->Release();
	}
	
	deckLinkIterator->Release();
		
	return 0;
}


#endif
