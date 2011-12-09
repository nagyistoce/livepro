#include "GLVideoDrawable.h"
#include "GLSceneGroup.h"

#include "GLWidget.h"

#include <QVideoFrame>
#include <QAbstractVideoSurface>
#include "../livemix/VideoSource.h"
#include "../livemix/CameraThread.h"

#include "VideoSender.h"

#include <QImageWriter>
#include <QGLFramebufferObject>
#include <string.h>

#include "GLCommonShaders.h"

#ifndef GL_UNSIGNED_INT_8_8_8_8_REV
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#endif

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

#define CAMERA_OVERSCAN .02

/// If the #define \em RECOMPILE_SHADERS_TO_INCLUDE_LEVELS is defined,
/// then the shaders will be recompiled when the black/white levels
/// change so as to enable/disable the levels code. (E.g. if black falls <=
/// 0 or white >= 255, then level coding in shader is not needed.) Note that
/// the shader recompile can cause a black frame to 'flash' on screen during
/// live video - so by default, \em RECOMPILE_SHADERS_TO_INCLUDE_LEVELS is not defined.
/// However, if the #define RECOMPILE_SHADERS_TO_INCLUDE_LEVELS is NOT defined,
/// a conditional GLSL expression in the shader itself will conditionally
/// run the leveling GLSL code depending if white<1.0 or black>0.0. (The white/black
/// levels for GLSL are normalized from 0-255 to 0-1.) 
/// By default, the #define \em RECOMPILE_SHADERS_TO_INCLUDE_LEVELS is NOT defined,
/// which means the conditional GLSL code is active. The only reason to define this
/// value is if you see a noticable GLSL slowdown due to the conditional GLSL statement.
//#define RECOMPILE_SHADERS_TO_INCLUDE_LEVELS

QByteArray VideoDisplayOptions::toByteArray()
{
	QByteArray array;
	QDataStream b(&array, QIODevice::WriteOnly);

	b << QVariant(flipHorizontal);
	b << QVariant(flipVertical);
	b << QVariant(cropTopLeft);
	b << QVariant(cropBottomRight);
	b << QVariant(brightness);
	b << QVariant(contrast);
	b << QVariant(hue);
	b << QVariant(saturation);

	return array;
}


void VideoDisplayOptions::fromByteArray(QByteArray array)
{
	QDataStream b(&array, QIODevice::ReadOnly);
	QVariant x;

	b >> x; flipHorizontal = x.toBool();
	b >> x; flipVertical = x.toBool();
	b >> x; cropTopLeft = x.toPointF();
	b >> x; cropBottomRight = x.toPointF();
	b >> x; brightness = x.toInt();
	b >> x; contrast = x.toInt();
	b >> x; hue = x.toInt();
	b >> x; saturation = x.toInt();
}

QDebug operator<<(QDebug dbg, const VideoDisplayOptions &opts)
{
	dbg.nospace() << "VideoDisplayOptions("<<opts.flipHorizontal<<","<<opts.flipVertical<<","<<opts.cropTopLeft<<","<<opts.cropBottomRight<<","<<opts.brightness<<","<<opts.contrast<<","<<opts.hue<<","<<opts.saturation<<")";

	return dbg.space();
}


int GLVideoDrawable::m_videoSenderPortAllocator = 7755;

GLVideoDrawable::GLVideoDrawable(QObject *parent)
	: GLDrawable(parent)
	, m_frame(0)
	, m_frame2(0)
	, m_visiblePendingFrame(false)
	, m_aspectRatioMode(Qt::KeepAspectRatio)
	, m_glInited(false)
	, m_program(0)
	, m_program2(0)
	, m_textureCount(1)
	, m_textureCount2(1)
	, m_colorsDirty(true)
	, m_source(0)
	, m_source2(0)
	, m_frameCount(0)
	, m_latencyAccum(0)
	, m_debugFps(false)
	, m_validShader(false)
	, m_validShader2(false)
	, m_uploadedCacheKey(0)
	, m_textureOffset(0,0)
	, m_texturesInited(false)
	, m_useShaders(true)
	, m_rateLimitFps(0.0)
	, m_xfadeEnabled(true)
	, m_xfadeLength(300)
	, m_fadeValue(0.)
	, m_fadeActive(false)
	, m_fadeTimeStarted(false)
	, m_videoSender(0)
	, m_videoSenderEnabled(false)
	, m_videoSenderPort(-1)
	, m_ignoreAspectRatio(false)
	, m_crossFadeMode(JustFront)
	, m_isCameraThread(false)
	, m_updateLeader(0)
	, m_electionNeeded(false)
	, m_avgFps(30)
	, m_unrenderedFrames(0)
	, m_liveStatus(false)
	, m_textureUpdateNeeded(false)
	, m_levelsEnabled(true)
	, m_blackLevel(0)
	, m_whiteLevel(255)
	, m_midLevel(128)
	, m_gamma(1.0)
	, m_kernelSize(9)
	, m_filterType(Filter_None)
	, m_sharpAmount(0.5)
	, m_blurAmount(1.0)
{

	m_imagePixelFormats
		<< QVideoFrame::Format_RGB32
		<< QVideoFrame::Format_ARGB32
		<< QVideoFrame::Format_RGB24
		<< QVideoFrame::Format_RGB565
		<< QVideoFrame::Format_YV12
		<< QVideoFrame::Format_YUV420P
		<< QVideoFrame::Format_YUV444
		<< QVideoFrame::Format_AYUV444
		<< QVideoFrame::Format_UYVY
		<< QVideoFrame::Format_YUYV;

	connect(&m_fpsRateLimiter, SIGNAL(timeout()), this, SLOT(updateGL()));
	m_fpsRateLimiter.setInterval(1000/30);
	// only start if setFpsLimit(float) is called

	connect(&m_fadeTick, SIGNAL(timeout()), this, SLOT(xfadeTick()));
	m_fadeTick.setInterval(1000/25);
}

GLVideoDrawable::~GLVideoDrawable()
{
	setVideoSource(0);
	if(m_isCameraThread) //m_updateLeader) // == this)
	{
		//qDebug() << "GLVideoDrawable::~GLVideoDrawable(): "<<(QObject*)this<<" In destructor, calling electUpdateLeader() to elect new leader";
		electUpdateLeader(this); // reelect an update leader, ignore this drawable
	} 
	
	m_alphaMask = QImage(); // see if this frees the mask, because reported by valgrind...
	//
	// The mask was reported still in memory at end of process by valgrind:
	//
// 	==22564== 307,200 bytes in 1 blocks are possibly lost in loss record 5,901 of 5,912
// 	==22564==    at 0x4005903: malloc (vg_replace_malloc.c:195)
// 	==22564==    by 0x560C669: QImageData::create(QSize const&, QImage::Format, int) (qimage.cpp:242)
// 	==22564==    by 0x560D9C5: QImage::QImage(int, int, QImage::Format) (qimage.cpp:837)
// 	==22564==    by 0x560EDF4: QImage::convertToFormat(QImage::Format, QFlags<Qt::ImageConversionFlag>) const (qimage.cpp:3428)
// 	==22564==    by 0x80980D9: GLVideoDrawable::setAlphaMask(QImage const&) (GLVideoDrawable.cpp:603)
// 	==22564==    by 0x8098A8F: GLVideoDrawable::updateRects(bool) (GLVideoDrawable.cpp:1880)
// 	==22564==    by 0x809682D: GLVideoDrawable::updateTexture(bool) (GLVideoDrawable.cpp:2197)
// 	==22564==    by 0x809E876: GLVideoDrawable::frameReady() (GLVideoDrawable.cpp:450)
// 	==22564==    by 0x8264201: GLVideoDrawable::qt_metacall(QMetaObject::Call, int, void**) (moc_GLVideoDrawable.cpp:381)
// 	==22564==    by 0x8267792: GLVideoFileDrawable::qt_metacall(QMetaObject::Call, int, void**) (moc_GLVideoFileDrawable.cpp:101)
// 	==22564==    by 0x604CADA: QMetaObject::metacall(QObject*, QMetaObject::Call, int, void**) (qmetaobject.cpp:237)
// 	==22564==    by 0x605A5A6: QMetaObject::activate(QObject*, QMetaObject const*, int, void**) (qobject.cpp:3285)

}

void GLVideoDrawable::setFpsLimit(float fps)
{
	m_rateLimitFps = fps;
	if(fps > 0.0)
	{
		m_fpsRateLimiter.setInterval((int)(1000./fps));
		m_fpsRateLimiter.start();
	}
	else
	{
		m_fpsRateLimiter.stop();
	}
}

void GLVideoDrawable::setVideoSenderEnabled(bool flag)
{
	m_videoSenderEnabled = flag;

	if(m_videoSenderEnabled)
	{

		if(!m_videoSender)
			m_videoSender = new VideoSender();


		if(m_videoSenderPort == -1)
		{
			bool done = false;
			while(!done)
			{
				m_videoSenderPort = m_videoSenderPortAllocator ++;
				if(m_videoSender->listen(QHostAddress::Any,m_videoSenderPort))
				{
					done = true;
				}
			}
		}
		else
		{
			if(!m_videoSender->listen(QHostAddress::Any,m_videoSenderPort))
			{
				qDebug() << "VideoServer could not start on port"<<m_videoSenderPort<<": "<<m_videoSender->errorString();
				//return -1;
			}
		}

		if(m_source)
		{
			m_videoSender->setVideoSource(m_source);
		}
	}
	else
	if(m_videoSender)
	{
		m_videoSender->close();
		delete m_videoSender;
		m_videoSender = 0;
	}
}

void GLVideoDrawable::setVideoSenderPort(int port)
{
	m_videoSenderPort = port;

	if(!m_videoSender)
		// Port will be applied when video sender is enabled (above)
		return;

	if(!m_videoSender->listen(QHostAddress::Any,m_videoSenderPort))
	{
		qDebug() << "VideoServer could not start on port"<<m_videoSenderPort<<": "<<m_videoSender->errorString();
		//return -1;
	}
}

void GLVideoDrawable::setVideoSource(VideoSource *source)
{
	if(m_source == source)
		return;

	if(m_source)
	{
		if(!m_xfadeEnabled)
		{
			disconnectVideoSource();
		}
		else
		{
			bool invertFadeStart = source == m_source2;

			disconnect(m_source, 0, this, 0);

			m_source2 = m_source;
			connect(m_source2, SIGNAL(frameReady()), this, SLOT(frameReady2()));
			connect(m_source2, SIGNAL(destroyed()),  this, SLOT(disconnectVideoSource2()));

// 			if(m_frame2 &&
// 			   m_frame2->release())
// 			{
// 				#ifdef DEBUG_VIDEOFRAME_POINTERS
// 				qDebug() << "GLVideoDrawable::setVideoSource(): Deleting old m_frame2:"<<m_frame2;
// 				#endif
// 				delete m_frame2;
// 				m_frame2 = 0;
// 			}
			m_frame2 = m_frame;
			#ifdef DEBUG_VIDEOFRAME_POINTERS
			qDebug() << "GLVideoDrawable::setVideoSource(): Copied m_frame:"<<m_frame<<"to m_frame2:"<<m_frame2<<", calling incRef() on m_frame2";
			#endif
			updateTexture(true);

			xfadeStart(invertFadeStart);
		}
	}

	m_source = source;
	if(m_source)
	{
		m_source->registerConsumer(this);
		
		// If m_isCameraThread, then we tell GLWidget to updateGL *now* instead of using a 0-length timer to batch updateGL() calls into a single call
		m_isCameraThread = (NULL != dynamic_cast<CameraThread*>(source));
		
		if(m_isCameraThread)
			electUpdateLeader();

		connect(m_source, SIGNAL(frameReady()), this, SLOT(frameReady()));
		connect(m_source, SIGNAL(destroyed()),  this, SLOT(disconnectVideoSource()));

		//qDebug() << "GLVideoDrawable::setVideoSource(): "<<(QObject*)this()<<" m_source:"<<m_source;
		setVideoFormat(m_source->videoFormat());

		frameReady();

		if(m_videoSender)
			m_videoSender->setVideoSource(m_source);
		
		
		// Automatically add "overscan" using 3.5% (approx) based on 'Overscan amounts' recommended at http://en.wikipedia.org/wiki/Overscan 
// 		if(m_isCameraThread)
// 		{
// 			double assumedFrameWidth  = 680;
// 			double assumedFrameHeight = 480;
// 			double overscanAmount = .035 * .5; // 3.5% * .5 = .0175
// 			double cropX = assumedFrameWidth  * overscanAmount; // 11.2
// 			double cropY = assumedFrameHeight * overscanAmount; // 8.4
// 			
// 			m_displayOpts.cropTopLeft = QPointF(cropX,cropY);
// 			m_displayOpts.cropBottomRight = QPointF(-cropX,-cropY);
// 		}

		updateRects();
	}
	else
	{
		qDebug() << "GLVideoDrawable::setVideoSource(): "<<(QObject*)this<<" Source is NULL";
	}

}

void GLVideoDrawable::xfadeStart(bool invertStart)
{
	if(m_xfadeLength <= 50/1000)
	{
		xfadeStop();
		//qDebug() << "GLVideoDrawable::xfadeStart(): "<<(QObject*)this<<" m_xfadeLength is "<<m_xfadeLength<<", less than "<<(50/1000)<<", not running fade";
		return;
	}

	m_fadeTick.start();
	//m_fadeTime.start();
	m_fadeActive = true;
	m_fadeTimeStarted = false;
	m_startOpacity = invertStart ? 1.0 - m_fadeValue : m_fadeValue;
//	m_fadeValue = 0.0;
	//qDebug() << "GLVideoDrawable::xfadeStart(): "<<(QObject*)this<<" start opacity: "<<m_startOpacity;
	
	m_fadeCurve.setType(QEasingCurve::InOutQuad);
//	m_fadeCurve.setType(QEasingCurve::OutCubic);
}

void GLVideoDrawable::xfadeTick(bool callUpdate)
{
	// Only start the counter once we actually get the first 'tick' of the timer
	// because there may be a significant delay between the time the timer is started
	// and the first 'tick' is received, which (if we started the counter above), would
	// cause a noticable and undesirable jump in the opacity
	if(!m_fadeTimeStarted)
	{
		m_fadeTime.start();
		m_fadeTimeStarted = true;
	}

	double elapsed = m_fadeTime.elapsed() + (m_startOpacity * m_xfadeLength);
	double progress = ((double)elapsed) / ((double)m_xfadeLength);
	m_fadeValue = m_fadeCurve.valueForProgress(progress);
	//qDebug() << "GLVideoDrawable::xfadeTick(): elapsed:"<<elapsed<<", start:"<<m_startOpacity<<", progress:"<<progress<<", fadeValue:"<<m_fadeValue;

	if(elapsed >= m_xfadeLength)
		xfadeStop();

	if(callUpdate)
		updateGL();
}

void GLVideoDrawable::xfadeStop()
{
	//qDebug() << "GLVideoDrawable::xfadeStop()";
	m_fadeActive = false;
	m_fadeTick.stop();
	m_fadeValue = 0.0;

	disconnectVideoSource2();
	updateGL();
}

void GLVideoDrawable::setXFadeEnabled(bool flag)
{
	m_xfadeEnabled = flag;
}

void GLVideoDrawable::setXFadeLength(int length)
{
	m_xfadeLength = length;
}

void GLVideoDrawable::disconnectVideoSource()
{
	if(!m_source)
		return;
	m_source->release(this);
	
	/// NOTE: This PROBABLY is not a good idea - to NOT disconnect...
	//disconnect(m_source, 0, this, 0);
	emit sourceDiscarded(m_source);
	
	m_source = 0;
}

void GLVideoDrawable::disconnectVideoSource2()
{
	if(!m_source2)
		return;
	m_source2->release(this);
	
	/// NOTE: This PROBABLY is not a good idea - to NOT disconnect...
	//disconnect(m_source2, 0, this, 0);
	emit sourceDiscarded(m_source2);
	
	m_source2 = 0;
}

void GLVideoDrawable::setVisible(bool flag, bool pendingFrame)
{
	//qDebug() << "GLVideoDrawable::setVisible: "<<this<<", flag:"<<flag<<", pendingFrame:"<<pendingFrame;
	m_tempVisibleValue = flag;
	if(flag && pendingFrame)
	{
		m_visiblePendingFrame = true;
	}
	else
	{
		if(m_visiblePendingFrame)
			m_visiblePendingFrame = false;

		GLDrawable::setVisible(flag);
	}
}

void GLVideoDrawable::frameReady()
{
	// This seems to prevent crashes during startup of an application when a thread is pumping out frames
	// before the app has finished initalizing.
	//QMutexLocker lock(&m_frameReadyLock);

 	//qDebug() << "GLVideoDrawable::frameReady(): "<<(QObject*)this<<" m_source:"<<m_source;
	if(m_source)
	{
		VideoFramePtr f = m_source->frame();
// 		qDebug() << "GLVideoDrawable::frameReady(): "<<objectName()<<" f:"<<f;
		if(!f)
			return;
		if(f->isValid())
			m_frame = f;
	}

// 	qDebug() << "GLVideoDrawable::frameReady(): "<<objectName()<<" going to updateTexture";
	updateTexture();

	if(m_rateLimitFps <= 0.0)
	{
		if(m_isCameraThread)
		{
			if(m_updateLeader == this)
				updateGL(true);
			else
			{
				m_unrenderedFrames ++;
				
				if(m_unrenderedFrames > 1)
				{
					qDebug() << "GLVideoDrawable::frameReady(): "<<(QObject*)this<<" m_unrenderedFrames greater than threshold, electing new update leader, ignoring "<<(QObject*)m_updateLeader;
					electUpdateLeader(m_updateLeader); // ignore current update leader
					m_unrenderedFrames = 0;
				}
			}
		}
		else
		{
			updateGL();
		}
		//updateGL(m_isCameraThread);
	}
	//else
	//	qDebug() << "GLVideoDrawable::frameReady(): "<<(QObject*)this<<" rate limited, waiting on timer";

	if(m_visiblePendingFrame)
	{
		//qDebug() << "GLVideoDrawable::frameReady: "<<this<<", pending visible set, calling setVisible("<<m_tempVisibleValue<<")";
		m_visiblePendingFrame = false;
		GLDrawable::setVisible(m_tempVisibleValue);
	}
}

void GLVideoDrawable::electUpdateLeader(GLVideoDrawable *ignore)
{
	if(!m_isCameraThread)
	{
		//qDebug() << "GLVideoDrawable::electUpdateLeader(): "<<(QObject*)this<<" No camera thread, no update leader needed.";
		m_updateLeader = NULL;
		return;
	} 
	
	if(!m_glw)
	{
		//qDebug() << "GLVideoDrawable::electUpdateLeader(): "<<(QObject*)this<<" No GLWidget set yet, temporarily setting self as update leader and requesting election.";
		m_electionNeeded = true;
		m_updateLeader = this;
		return;
	}
	
	QList<GLDrawable*> list = m_glw->drawables();
	GLVideoDrawable *newLead = 0;
	GLVideoDrawable *firstCameraFound = 0;	
	int maxFps = 0;
	foreach(GLDrawable *gld, list)
	{
		if(GLVideoDrawable *vid = dynamic_cast<GLVideoDrawable*>(gld))
		{
			if(vid != ignore && 
			   vid->m_isCameraThread)
			{
				firstCameraFound = vid;
				if(((int)vid->m_avgFps) > maxFps)
				{
					newLead = vid;
					maxFps = (int)vid->m_avgFps;
				}
			}
		}
	}
	
	if(!newLead)
		newLead = firstCameraFound;
	
	if(newLead == m_updateLeader)
	{
		//qDebug() << "GLVideoDrawable::electUpdateLeader(): "<<(QObject*)this<<" Update leader not changed.";
	}
	else
	{
		//qDebug() << "GLVideoDrawable::electUpdateLeader(): "<<(QObject*)this<<" New update leader:"<<(QObject*)newLead;
		//if(m_updateLeader)
		//	qDebug() << "GLVideoDrawable::electUpdateLeader(): "<<(QObject*)this<<" Old leader:"<<(QObject*)m_updateLeader;
	}
	
	foreach(GLDrawable *gld, list)
	{
		if(GLVideoDrawable *vid = dynamic_cast<GLVideoDrawable*>(gld))
		{
			if(vid != ignore &&
			   vid->m_isCameraThread)
				vid->m_updateLeader = newLead;
			else
				vid->m_updateLeader = NULL;
		}
	}
	
}

void GLVideoDrawable::frameReady2()
{
	if(m_source2)
	{
		VideoFramePtr f = m_source2->frame();
		if(!f)
			return;
		if(f->isValid())
			m_frame2 = f;
	}

	updateTexture(true);
	
	updateGL();
}

void GLVideoDrawable::setAlphaMask(const QImage &mask)
{
	m_alphaMask_preScaled = mask;
	m_alphaMask = mask;

	if(mask.isNull())
	{
		//qDebug() << "GLVideoDrawable::setAlphaMask: "<<this<<",  Got null mask, size:"<<mask.size();
		//return;
		m_alphaMask = QImage(16,16,QImage::Format_RGB32);
		m_alphaMask.fill(Qt::black);
 		//qDebug() << "GLVideoDrawable::initGL: BLACK m_alphaMask.size:"<<m_alphaMask.size();
 		setAlphaMask(m_alphaMask);
 		return;
	}

	if(m_glInited && glWidget())
	{
		if(m_sourceRect.size().toSize() == QSize(0,0))
		{
			//qDebug() << "GLVideoDrawable::setAlphaMask: "<<this<<", Not scaling or setting mask, video size is 0x0";
			return;
		}

		glWidget()->makeRenderContextCurrent();
		if(m_alphaMask.size() != m_sourceRect.size().toSize())
		{
			//qDebug() << "GLVideoDrawable::setAlphaMask: "<<this<<",  Mask size and source size different, scaling";
			m_alphaMask = m_alphaMask.scaled(m_sourceRect.size().toSize());
		}

		if(m_alphaMask.format() != QImage::Format_ARGB32)
		{
			//qDebug() << "GLVideoDrawable::setAlphaMask: "<<this<<",  Mask format not ARGB32, reformatting";
			m_alphaMask = m_alphaMask.convertToFormat(QImage::Format_ARGB32);
		}

		if(m_alphaMask.cacheKey() == m_uploadedCacheKey)
		{
			//qDebug() << "GLVideoDrawable::setAlphaMask: "<<this<<",  Current mask already uploaded to GPU, not re-uploading.";
			return;
		}


		m_uploadedCacheKey = m_alphaMask.cacheKey();

		//qDebug() << "GLVideoDrawable::setAlphaMask: "<<this<<",  Valid mask, size:"<<m_alphaMask.size()<<", null?"<<m_alphaMask.isNull();

		glBindTexture(GL_TEXTURE_2D, m_alphaTextureId);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA,
			mask.width(),
			mask.height(),
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			mask.scanLine(0)
			);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

// 	qDebug() << "GLVideoDrawable::setAlphaMask: "<<this<<",  AT END :"<<m_alphaMask.size()<<", null?"<<m_alphaMask.isNull();

}

void GLVideoDrawable::setFlipHorizontal(bool value)
{
	m_displayOpts.flipHorizontal = value;
	//qDebug() << "GLVideoDrawable::setFlipHorizontal(): "<<value;
	emit displayOptionsChanged(m_displayOpts);
	updateGL();
}

void GLVideoDrawable::setFlipVertical(bool value)
{
	m_displayOpts.flipVertical = value;
	//qDebug() << "GLVideoDrawable::setFlipVertical(): "<<value;
	emit displayOptionsChanged(m_displayOpts);
	updateGL();
}

void GLVideoDrawable::setCropTopLeft(QPointF value)
{
	if(m_isCameraThread)
	{
		//double overscanAmount = .02; //035 * .5; // 3.5% * .5 = .0175
		if(value.x() < CAMERA_OVERSCAN)
			value.setX(0);
		if(value.y() < CAMERA_OVERSCAN)
			value.setY(0);
	}
	//qDebug() << "GLVideoDrawable::setCropTopLeft(): "<<value;
		
	m_displayOpts.cropTopLeft = value;
	updateRects();
	emit displayOptionsChanged(m_displayOpts);
	updateGL();
}

void GLVideoDrawable::setCropBottomRight(QPointF value)
{
	if(m_isCameraThread)
	{
		if(value.x() > -CAMERA_OVERSCAN)
			value.setX(0);
		if(value.y() > -CAMERA_OVERSCAN)
			value.setY(0);
	}
	//qDebug() << "GLVideoDrawable::setCropBottomRight(): "<<value;
		
	m_displayOpts.cropBottomRight = value;
	emit displayOptionsChanged(m_displayOpts);
	updateRects();
	updateGL();
}

void GLVideoDrawable::setAspectRatioMode(Qt::AspectRatioMode mode)
{
	//qDebug() << "GLVideoDrawable::setAspectRatioMode: "<<this<<", mode:"<<mode<<", int:"<<(int)mode;
	m_aspectRatioMode = mode;
	updateRects();
	updateAlignment();
	updateGL();
}

void GLVideoDrawable::setIgnoreAspectRatio(bool flag)
{
	m_ignoreAspectRatio = flag;
	setAspectRatioMode(flag ? Qt::IgnoreAspectRatio : Qt::KeepAspectRatio);
}

/*!
*/
int GLVideoDrawable::brightness() const
{
	return m_displayOpts.brightness;
}

/*!
*/
void GLVideoDrawable::setBrightness(int brightness)
{
	m_displayOpts.brightness = brightness;
	emit displayOptionsChanged(m_displayOpts);
	m_colorsDirty = true;
	updateGL();
}

/*!
*/
int GLVideoDrawable::contrast() const
{
	return m_displayOpts.contrast;
}

/*!
*/
void GLVideoDrawable::setContrast(int contrast)
{
	m_displayOpts.contrast = contrast;
	emit displayOptionsChanged(m_displayOpts);
	m_colorsDirty = true;
	updateGL();
}

/*!
*/
int GLVideoDrawable::hue() const
{
	return m_displayOpts.hue;
}

/*!
*/
void GLVideoDrawable::setHue(int hue)
{
	m_displayOpts.hue = hue;
	emit displayOptionsChanged(m_displayOpts);
	m_colorsDirty = true;
	updateGL();
}

/*!
*/
int GLVideoDrawable::saturation() const
{
	return m_displayOpts.saturation;
}

/*!
*/
void GLVideoDrawable::setSaturation(int saturation)
{
	m_displayOpts.saturation = saturation;
	emit displayOptionsChanged(m_displayOpts);
	m_colorsDirty = true;
	updateGL();
}

void GLVideoDrawable::updateColors(int brightness, int contrast, int hue, int saturation)
{
	const qreal b = brightness / 200.0;
	const qreal c = contrast / 200.0 + 1.0;
	const qreal h = hue / 200.0;
	const qreal s = saturation / 200.0 + 1.0;

	const qreal cosH = qCos(M_PI * h);
	const qreal sinH = qSin(M_PI * h);

	const qreal h11 = -0.4728 * cosH + 0.7954 * sinH + 1.4728;
	const qreal h21 = -0.9253 * cosH - 0.0118 * sinH + 0.9523;
	const qreal h31 =  0.4525 * cosH + 0.8072 * sinH - 0.4524;

	const qreal h12 =  1.4728 * cosH - 1.3728 * sinH - 1.4728;
	const qreal h22 =  1.9253 * cosH + 0.5891 * sinH - 0.9253;
	const qreal h32 = -0.4525 * cosH - 1.9619 * sinH + 0.4525;

	const qreal h13 =  1.4728 * cosH - 0.2181 * sinH - 1.4728;
	const qreal h23 =  0.9253 * cosH + 1.1665 * sinH - 0.9253;
	const qreal h33 =  0.5475 * cosH - 1.3846 * sinH + 0.4525;

	const qreal sr = (1.0 - s) * 0.3086;
	const qreal sg = (1.0 - s) * 0.6094;
	const qreal sb = (1.0 - s) * 0.0820;

	const qreal sr_s = sr + s;
	const qreal sg_s = sg + s;
	const qreal sb_s = sr + s;

	const float m4 = (s + sr + sg + sb) * (0.5 - 0.5 * c + b);

	m_colorMatrix(0, 0) = c * (sr_s * h11 + sg * h21 + sb * h31);
	m_colorMatrix(0, 1) = c * (sr_s * h12 + sg * h22 + sb * h32);
	m_colorMatrix(0, 2) = c * (sr_s * h13 + sg * h23 + sb * h33);
	m_colorMatrix(0, 3) = m4;

	m_colorMatrix(1, 0) = c * (sr * h11 + sg_s * h21 + sb * h31);
	m_colorMatrix(1, 1) = c * (sr * h12 + sg_s * h22 + sb * h32);
	m_colorMatrix(1, 2) = c * (sr * h13 + sg_s * h23 + sb * h33);
	m_colorMatrix(1, 3) = m4;

	m_colorMatrix(2, 0) = c * (sr * h11 + sg * h21 + sb_s * h31);
	m_colorMatrix(2, 1) = c * (sr * h12 + sg * h22 + sb_s * h32);
	m_colorMatrix(2, 2) = c * (sr * h13 + sg * h23 + sb_s * h33);
	m_colorMatrix(2, 3) = m4;

	m_colorMatrix(3, 0) = 0.0;
	m_colorMatrix(3, 1) = 0.0;
	m_colorMatrix(3, 2) = 0.0;
	m_colorMatrix(3, 3) = 1.0;

	if (m_yuv)
	{
		m_colorMatrix = m_colorMatrix * QMatrix4x4(
			1.0,  0.000,  1.140, -0.5700,
			1.0, -0.394, -0.581,  0.4875,
			1.0,  2.028,  0.000, -1.0140,
			0.0,  0.000,  0.000,  1.0000);
	}
}

void GLVideoDrawable::initRgbTextureInfo(GLenum internalFormat, GLuint format, GLenum type, const QSize &size, bool secondSource)
{
	if(!secondSource)
		m_yuv = false;
	else
		m_yuv2 = false;

	if(!secondSource)
	{
		m_textureInternalFormat = internalFormat;
		m_textureFormat = format;
		m_textureType   = type;

		m_textureCount  = 1;
	}
	else
	{
		m_textureInternalFormat2 = internalFormat;
		m_textureFormat2 = format;
		m_textureType2   = type;

		m_textureCount2  = 1;
	}

	int idx = secondSource ? 3:0;

	m_textureWidths[0+idx]  = size.width();
	m_textureHeights[0+idx] = size.height();
	m_textureOffsets[0+idx] = 0;
}

void GLVideoDrawable::initYuv420PTextureInfo(const QSize &size, bool secondSource)
{
	if(!secondSource)
		m_yuv = true;
	else
		m_yuv2 = true;

	if(!secondSource)
	{
		m_textureInternalFormat = GL_LUMINANCE;
		m_textureFormat = GL_LUMINANCE;
		m_textureType   = GL_UNSIGNED_BYTE;
		m_textureCount  = 3;
	}
	else
	{
		m_textureInternalFormat2 = GL_LUMINANCE;
		m_textureFormat2 = GL_LUMINANCE;
		m_textureType2   = GL_UNSIGNED_BYTE;
		m_textureCount2  = 3;
	}

	int idx = secondSource ? 3:0;

	m_textureWidths[0+idx] = size.width();
	m_textureHeights[0+idx] = size.height();
	m_textureOffsets[0+idx] = 0;

	m_textureWidths[1+idx] = size.width() / 2;
	m_textureHeights[1+idx] = size.height() / 2;
	m_textureOffsets[1+idx] = size.width() * size.height();
	//qDebug() << "GLVideoDrawable::initYuv420PTextureInfo: size:"<<size;

	m_textureWidths[2+idx] = size.width() / 2;
	m_textureHeights[2+idx] = size.height() / 2;
	m_textureOffsets[2+idx] = size.width() * size.height() * 5 / 4;

	//qDebug() << "yuv420p tex: off2:"<<m_textureOffsets[2];
}

void GLVideoDrawable::initYv12TextureInfo(const QSize &size, bool secondSource)
{
	if(!secondSource)
		m_yuv = true;
	else
		m_yuv2 = true;

	if(!secondSource)
	{
		m_textureInternalFormat = GL_LUMINANCE;
		m_textureFormat = GL_LUMINANCE;
		m_textureType   = GL_UNSIGNED_BYTE;
		m_textureCount  = 3;
	}
	else
	{
		m_textureInternalFormat2 = GL_LUMINANCE;
		m_textureFormat2 = GL_LUMINANCE;
		m_textureType2   = GL_UNSIGNED_BYTE;
		m_textureCount2  = 3;
	}

	int idx = secondSource ? 3:0;

	m_textureWidths[0+idx] = size.width();
	m_textureHeights[0+idx] = size.height();
	m_textureOffsets[0+idx] = 0;

	m_textureWidths[1+idx] = size.width() / 2;
	m_textureHeights[1+idx] = size.height() / 2;
	m_textureOffsets[1+idx] = size.width() * size.height() * 5 / 4;

	m_textureWidths[2+idx] = size.width() / 2;
	m_textureHeights[2+idx] = size.height() / 2;
	m_textureOffsets[2+idx] = size.width() * size.height();
}


void GLVideoDrawable::initYuv442TextureInfo( const QSize &size, bool secondSource )
{
	// YUV 4:2:2 pixel format is a packed YUV format.
	// One pixel(4 bytes) contains chrome and luminence values
	// for two pixels(horizontally).
	//
	//
	// A packed UYVY pixel consist of four components
	//
	//      U Y1 V Y2
	//
	// A packed YUYV pixel consist of four components
	//
	//      Y1 U Y2 V
	//
	// When a packed pixel is unpacked we get two pixels:
	//      Y1 U V
	//      Y2 U V
	//
	//
	// Two textures are created. One for luminance values and the other
	// one for chrominance values. Width of the chrominance texture is
	// half of the luminance texture's width because the same chrominance
	// values are used for two sequential pixels(pixels packed togerther).
	// Fragment shader takes care of reading correct components from
	// textures for a YUV color and converting the YUV color to RGB.
	
	// Make sure that widht is even value
	QSize unpackedSize = size;
	if( unpackedSize.width() % 2 != 0 )
	{
		unpackedSize.setWidth( unpackedSize.width() - 1 );
	}
	
	if(!secondSource)
	{
		m_yuv = true;
		m_packed_yuv = true;
		m_textureType = GL_UNSIGNED_BYTE;
		m_textureCount = 2;
	}
	else
	{
		m_yuv2 = true;
		m_packed_yuv2 = true;
		m_textureType2 = GL_UNSIGNED_BYTE;
		m_textureCount2 = 2;

	}
	
	int idx = secondSource ? 3:0;
	
	// Luminance(Y) texture. Luminance value is stored to alpha component.
	m_textureWidths [0 + idx] = unpackedSize.width();
	m_textureHeights[0 + idx] = unpackedSize.height();
	m_textureOffsets[0 + idx] = 0;
	m_textureFormats[0 + idx] = GL_LUMINANCE_ALPHA;
	m_textureInternalFormats[0 + idx] = GL_LUMINANCE_ALPHA;
	
	// Chrome texture. Chrome values are stored to red and blue components.
	// U = red
	// V = blue
	m_textureWidths [1 + idx] = unpackedSize.width() / 2;
	m_textureHeights[1 + idx] = unpackedSize.height();;
	m_textureOffsets[1 + idx] = 0;
	m_textureFormats[1 + idx] = GL_RGBA;
	m_textureInternalFormats[1 + idx] = GL_RGBA;
}



void GLVideoDrawable::setGLWidget(GLWidget* widget)
{
	if(widget)
	{
		if(!liveStatus())
			setLiveStatus(true);

		GLDrawable::setGLWidget(widget);

		if(m_textureUpdateNeeded)
		{
			updateTexture();
			m_textureUpdateNeeded = false;
		}
		
// 		if(m_electionNeeded)
// 		{
// 			qDebug() << "GLVideoDrawable::setGLWidget(): "<<(QObject*)this<<" m_electionNeeded, calling elect"; 
// 			electUpdateLeader();
// 			m_electionNeeded = false;
// 		}

		//qDebug() << "GLVideoDrawable::setGLWidget(): "<<(QObject*)this<<" live status going UP, calling elect";
		if(m_isCameraThread) 
			electUpdateLeader(); // reelect an update leader
	}
	else
	{
		//qDebug() << "GLVideoDrawable::setGLWidget(): "<<(QObject*)this<<" live status going down, calling elect";
		if(m_isCameraThread || m_updateLeader) 
			electUpdateLeader(this); // reelect an update leader, ignore this drawable if going down
		
		GLDrawable::setGLWidget(widget);
		if(liveStatus())
			setLiveStatus(false);
	}
}

void GLVideoDrawable::setLiveStatus(bool flag)
{
	m_liveStatus = flag;
}

QVariant GLVideoDrawable::itemChange(GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemSceneChange)
	{
		QGraphicsScene *scene = value.value<QGraphicsScene*>();
		if(!scene && liveStatus() && !m_glw)
			setLiveStatus(false);
	}
	return GLDrawable::itemChange(change, value);
 }

void GLVideoDrawable::initGL()
{
	GLDrawable::initGL();
	
	// Just to be safe, dont re-initalize this 'widgets' GL state if already passed thru here once.
	if(m_glInited)
		return;

	if(glWidget())
		glWidget()->makeRenderContextCurrent();

	//qDebug() << "GLVideoDrawable::initGL(): "<<(QObject*)this;

	#ifndef QT_OPENGL_ES
	glActiveTexture = (_glActiveTexture)glWidget()->context()->getProcAddress(QLatin1String("glActiveTexture"));
	#endif

	const GLubyte *str = glGetString(GL_EXTENSIONS);
	QString extensionList = QString((const char*)str);
	
	m_useShaders = extensionList.indexOf("GL_ARB_fragment_shader") > -1;

	if(0)
	{
		qDebug() << "GLVideoDrawable::initGL: Forcing NO GLSL shaders";
		m_useShaders = false;
	}

	if(m_useShaders)
	{
		m_program = new QGLShaderProgram(glWidget()->context(), this);
		m_program2 = new QGLShaderProgram(glWidget()->context(), this);
	}

	m_glInited = true;
	//qDebug() << "GLVideoDrawable::initGL(): "<<objectName();
	setVideoFormat(m_videoFormat);

	// frameReady() updates textures, so it seems to trigger
	// some sort of loop by causing the GLWidget::initializeGL() function to be called again,
	// which calls this class's initGL(), which calls frameReady(), which triggers
	// GLWidget::initalizeGL() again...so we just put it in a singleshot timer to
	// fire as soon as control returns to the event loop
	if(m_source)
		QTimer::singleShot(0,this,SLOT(frameReady()));
	else
		if(m_frame && m_frame->isValid())
			QTimer::singleShot(0,this,SLOT(updateTexture()));


	// create the alpha texture
	glGenTextures(1, &m_alphaTextureId);

	if(m_alphaMask.isNull())
	{
		m_alphaMask = QImage(1,1,QImage::Format_RGB32);
		m_alphaMask.fill(Qt::black);
 		//qDebug() << "GLVideoDrawable::initGL: BLACK m_alphaMask.size:"<<m_alphaMask.size();
 		setAlphaMask(m_alphaMask);
	}
	else
	{
 		//qDebug() << "GLVideoDrawable::initGL: Alpha mask already set, m_alphaMask.size:"<<m_alphaMask.size();
		setAlphaMask(m_alphaMask_preScaled);
	}

	m_time.start();
}

bool GLVideoDrawable::setVideoFormat(const VideoFormat& format, bool secondSource)
{
	bool samePixelFormat = false; //format.pixelFormat == m_videoFormat.pixelFormat;
	if(!secondSource)
		m_videoFormat = format;
	else
		m_videoFormat2 = format;

	//qDebug() << "GLVideoDrawable::setVideoFormat(): "<<(QObject*)this<<" \t frameSize:"<<format.frameSize<<", pixelFormat:"<<format.pixelFormat;

	if(!m_glInited || !glWidget())
		return m_imagePixelFormats.contains(format.pixelFormat);

	//qDebug() << "GLVideoDrawable::setVideoFormat(): \t Initalizing vertex and pixel shaders...";

	glWidget()->makeRenderContextCurrent();

	if(!secondSource)
		m_validShader = false;
	else
		m_validShader2 = false;
	QByteArray fragmentProgram = resizeTextures(format.frameSize, secondSource);

  	if(!samePixelFormat)
  	{
  		if(m_useShaders)
		{
			Q_UNUSED(qt_glsl_warpingVertexShaderProgram);

			QGLShaderProgram *program = !secondSource ? m_program : m_program2;

			if(!program->shaders().isEmpty())
				program->removeAllShaders();
				
			//qDebug() << "Fragment used:"<<fragmentProgram.constData();
				
			//qDebug() << "GLVideoDrawable::setVideoFormat: fragmentProgram:"<<fragmentProgram.constData();
			// This seems to be needed for some reason!!! Grrr....(live camera in director window is black without this...)
			(void*)fragmentProgram.constData();

			if(!QGLShaderProgram::hasOpenGLShaderPrograms())
			{
				qDebug() << "GLSL Shaders Not Supported by this driver, this program will NOT function as expected and will likely crash.";
				return false;
			}

			if (fragmentProgram.isEmpty())
			{
 				//if(format.pixelFormat != QVideoFrame::Format_Invalid)
 					//qDebug() << "GLVideoDrawable: No shader program found - format not supported.";
				return false;
			}
			else
			if (!program->addShaderFromSourceCode(QGLShader::Vertex, qt_glsl_vertexShaderProgram))
			{
				qWarning("GLVideoDrawable: Vertex shader compile error %s",
					qPrintable(program->log()));
				//error = QAbstractVideoSurface::ResourceError;
				return false;

			}
			else
			if (!program->addShaderFromSourceCode(QGLShader::Fragment, fragmentProgram))
			{
				qDebug() << "Fragment used:"<<fragmentProgram.constData();
				qWarning("GLVideoDrawable: Shader compile error %s", qPrintable(program->log()));
				//error = QAbstractVideoSurface::ResourceError;
				program->removeAllShaders();
				return false;
			}
			else
			if(!program->link())
			{
// 				qDebug() << "Vertex used:"<<qt_glsl_vertexShaderProgram;
// 				qDebug() << "Fragment used:"<<fragmentProgram.constData();
				qWarning("GLVideoDrawable: Shader link error %s", qPrintable(program->log()));
				program->removeAllShaders();
				return false;
			}
			else
			{
				//m_handleType = format.handleType();
				m_scanLineDirection = QVideoSurfaceFormat::TopToBottom; //format.scanLineDirection();

				if(!secondSource)
				{
					m_frameSize = format.frameSize;
					glGenTextures(m_textureCount, m_textureIds);
					m_texturesInited = true;
				}
				else
				{
					m_frameSize2 = format.frameSize;
					glGenTextures(m_textureCount2, m_textureIds2);
					m_texturesInited2 = true;
				}
			}

			//qDebug() << "GLVideoDrawable::setVideoFormat(): \t Initalized"<<m_textureCount<<"textures";
		}
		else
		{
			m_scanLineDirection = QVideoSurfaceFormat::TopToBottom; //format.scanLineDirection();

			if(!secondSource)
			{
				m_frameSize = format.frameSize;
				glGenTextures(1, m_textureIds);
				m_texturesInited = true;
			}
			else
			{
				m_frameSize2 = format.frameSize;
				glGenTextures(1, m_textureIds2);
				m_texturesInited2 = true;
			}

			//qDebug() << "GLVideoDrawable::setVideoFormat(): \t Initalized 1 textures, no shader used";
		}
 	}
	if(!secondSource)
		m_validShader = true;
	else
		m_validShader2 = true;
	return true;
}

QByteArray GLVideoDrawable::resizeTextures(const QSize& frameSize, bool secondSource)
{
	const char * fragmentProgram = 0;

	//qDebug() << "GLVideoDrawable::resizeTextures(): "<<objectName()<<" \t frameSize: "<<frameSize<<", format: "<<m_videoFormat.pixelFormat;
	//qDebug() << "GLVideoDrawable::resizeTextures(): secondSource:"<<secondSource;
	if(!secondSource)
		m_frameSize = frameSize;
	else
		m_frameSize2 = frameSize;

	m_packed_yuv = false;
	m_packed_yuv2 = false;
	m_yuv = false;
	m_yuv2 = false;
	
	//qDebug() << "GLVideoDrawable::resizeTextures(): "<<(QObject*)this<<"\t frame size:"<<frameSize;
	
	bool rgb = false;
	
	bool debugShaderName = false;
	switch (m_videoFormat.pixelFormat)
	{
	/// RGB Formats
	case QVideoFrame::Format_RGB32:
 		if(debugShaderName)
 			qDebug() << "GLVideoDrawable::resizeTextures(): "<<(QObject*)this<<"\t Format RGB32, using qt_glsl_xrgbShaderProgram";
		initRgbTextureInfo(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, frameSize, secondSource);
		//fragmentProgram = m_levelsEnabled ? qt_glsl_xrgbLevelsShaderProgram : qt_glsl_xrgbShaderProgram;
		fragmentProgram = qt_glsl_xrgbShaderProgram;
		//fragmentProgram = qt_glsl_xrgbLevelsShaderProgram;
		rgb = true;
		break;
        case QVideoFrame::Format_ARGB32:
         	if(debugShaderName)
         		qDebug() << "GLVideoDrawable::resizeTextures(): "<<(QObject*)this<<"\t Format ARGB, using qt_glsl_argbShaderProgram";
		initRgbTextureInfo(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, frameSize, secondSource);
		//fragmentProgram = m_levelsEnabled ? qt_glsl_argbLevelsShaderProgram : qt_glsl_argbShaderProgram;
		fragmentProgram = qt_glsl_argbShaderProgram;
		rgb = true;
		break;
#ifndef QT_OPENGL_ES
        case QVideoFrame::Format_RGB24:
        	if(debugShaderName)
         		qDebug() << "GLVideoDrawable::resizeTextures(): "<<(QObject*)this<<"\t Format RGB24, using qt_glsl_rgbShaderProgram";
		initRgbTextureInfo(GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, frameSize, secondSource);
		//fragmentProgram = m_levelsEnabled ? qt_glsl_rgbLevelsShaderProgram : qt_glsl_rgbShaderProgram;
		fragmentProgram = qt_glsl_rgbShaderProgram;
		rgb = true;
		break;
#endif
	case QVideoFrame::Format_RGB565:
		if(debugShaderName)
			qDebug() << "GLVideoDrawable::resizeTextures(): "<<(QObject*)this<<"\t Format RGB565, using qt_glsl_rgbShaderProgram";
		initRgbTextureInfo(GL_RGB, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, frameSize, secondSource);
		//fragmentProgram = m_levelsEnabled ? qt_glsl_rgbLevelsShaderProgram : qt_glsl_rgbShaderProgram;
		fragmentProgram = qt_glsl_rgbShaderProgram;
		rgb = true;
		break;
		
	/// YUV Formats
	case QVideoFrame::Format_YV12:
		if(debugShaderName)
			qDebug() << "GLVideoDrawable::resizeTextures(): "<<(QObject*)this<<"\t Format YV12, using qt_glsl_yuvPlanarShaderProgram";
		initYv12TextureInfo(frameSize, secondSource);
		fragmentProgram = qt_glsl_yuvPlanarShaderProgram;
		break;
	case QVideoFrame::Format_YUV420P:
		if(debugShaderName)
			qDebug() << "GLVideoDrawable::resizeTextures(): "<<(QObject*)this<<"\t Format YUV420P, using qt_glsl_yuvPlanarShaderProgram";
		initYuv420PTextureInfo(frameSize, secondSource);
		fragmentProgram = qt_glsl_yuvPlanarShaderProgram;
		break;
		
	case QVideoFrame::Format_YUV444:
		if(debugShaderName)
			qDebug() << "GLVideoDrawable::resizeTextures(): "<<(QObject*)this<<"\t Format YUV444, using qt_glsl_xyuvShaderProgram";
			
		initRgbTextureInfo(GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, frameSize);
		fragmentProgram = qt_glsl_xyuvShaderProgram;
		m_yuv = true;
	break;
	case QVideoFrame::Format_AYUV444:
		if(debugShaderName)
			qDebug() << "GLVideoDrawable::resizeTextures(): "<<(QObject*)this<<"\t Format_AYUV444, using qt_glsl_ayuvShaderProgram";
			
		initRgbTextureInfo(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, frameSize);
		fragmentProgram = qt_glsl_ayuvShaderProgram;
		m_yuv = true;
	break;
	case QVideoFrame::Format_UYVY:
		if(debugShaderName)
			qDebug() << "GLVideoDrawable::resizeTextures(): "<<(QObject*)this<<"\t Format_UYVY, using qt_glsl_uyvyShaderProgram";
		initYuv442TextureInfo(frameSize);
		fragmentProgram = qt_glsl_uyvyShaderProgram;
	break;
	case QVideoFrame::Format_YUYV:
		if(debugShaderName)
			qDebug() << "GLVideoDrawable::resizeTextures(): "<<(QObject*)this<<"\t Format_YUYV, using qt_glsl_yuyvShaderProgram";
		initYuv442TextureInfo(frameSize);
		fragmentProgram = qt_glsl_yuyvShaderProgram;
	break;
				
	default:
		if(debugShaderName)
			qDebug() << "GLVideoDrawable::resizeTextures(): "<<(QObject*)this<<"\t Unknown pixel format:"<<m_videoFormat.pixelFormat;
		break;
	}
	
	//m_filterType = Filter_Sharp;
	//m_levelsEnabled = false;
	
	QString pixelOrder = 
		m_videoFormat.pixelFormat == QVideoFrame::Format_RGB24 || 
		m_videoFormat.pixelFormat == QVideoFrame::Format_RGB565 ? "rgb" : "bgr"; 
		
	if(m_levelsEnabled || m_filterType != Filter_None)
	{
		
		QString convolveSource;
		QString uniformDefs;

		QString programSource(fragmentProgram);
		if(programSource.indexOf("%1") < 0)
		{
			// No placeholders for code insertion in the shader, 
			// so just return the shader as-is
			QByteArray array;
			array.append(programSource);
			return array;
		}
			
		QString fragUniforms;
		QString fragSource;
		
		//m_filterType = ConvSharp;
			
		if(m_filterType != Filter_None)
		{
			if(!rgb)
			{
				qDebug() << "GLVideoDrawable::resizeTextures: Non-rgb texture, convolution filter not implemented.";
			}
			else
			{
				if(m_filterType != Filter_CustomConvultionKernel)
					m_kernelSize = 9;

				int kernelSize = m_kernelSize;
				
				QList<float> kernel;
				float divisor = 1;
					
				if(m_filterType == Filter_Blur)
				{
					kernelSize = 25;
					if(kernelSize == 9)
					{
						kernel << 1 << 2 << 1 <<
							2 << 4 << 2 <<
							1 << 2 << 1;
						divisor = 16;
	
						kernel << 1 << 0 << 1 <<
							0 << 1 << 0 <<
							1 << 0 << 1;
						divisor = -1;
	
	
					}
					else if(kernelSize == 25)
					{
// 						kernel << 1 <<  4 <<  6 <<  4 << 1 << 
// 							4 << 16 << 24 << 16 << 4 << 
// 							6 << 24 << 36 << 24 << 6 << 
// 							4 << 16 << 24 << 16 << 4 << 
// 							1 <<  4 <<  6 <<  4 << 1;
// 						divisor = -1;
	
						//m_blurAmount = 1;
						kernel << 1 << 0.1 << 1 << 0.1 << 1 << 
							0.1 << 1 << 0.1 << 1 << 0.1 << 
							1 << 0.1 << 1 << 0.1 << 1 << 
							0.1 << 1 << 0.1 << 1 << 0.1 << 
							1 << 0.1 << 1 << 0.1 << 1;
						for(int i=0;i<kernelSize;i++)
							kernel[i] = i == kernelSize/2? kernel[i] : kernel[i] * m_blurAmount;
						divisor = -1; // TODO what divisor is right?? 16 is just a guess..
	
					}
				}
				else
				if(m_filterType == Filter_Emboss)
				{
					kernel  << 2 <<  0 << 0
						<< 0 << -1 << 0 
						<< 0 <<  0 << -1;
					divisor = 16;
				}
				else
				if(m_filterType == Filter_Bloom)
				{
					kernel  << 5 <<  0 << 10
						<< 0 << 10 <<  0
						<< 10 << 0 <<  5;
					divisor = 9;
				}
				else
				if(m_filterType == Filter_Mean)
				{
					for(int i=0;i<kernelSize;i++)
						kernel << 1;
					divisor = 9;
				}
				else
				if(m_filterType == Filter_Sharp)
				{
					double sharpAmount = m_sharpAmount;
					//sharpAmount = 1.25;
					if(kernelSize == 9)
					{
						if(sharpAmount <= 1.0)
							kernel  <<  0
								<< -1 * sharpAmount
								<<  0
								
								<< -1 * sharpAmount
								<< (1 * sharpAmount * 4 + 1)
								<< -1 * sharpAmount
								
								<<  0
								<< -1 * sharpAmount
								<<  0;
						else
							kernel  << -1 * sharpAmount
								<< -1 * sharpAmount
								<< -1 * sharpAmount
								
								<< -1 * sharpAmount
								<< (1 * sharpAmount * 8 + 1)
								<< -1 * sharpAmount
								
								<< -1 * sharpAmount
								<< -1 * sharpAmount
								<< -1 * sharpAmount;
						divisor = 1; 
					}
					else
					{
						//TODO 
						//-3, 12, 17, 12, -3
						//or
						//1, -8, 0, 8, -1
						//Convert those weights to a 5x5 matrix by doing vector multiply with its own transpose.
						//See http://prideout.net/archive/bloom/index.php
						
						divisor = 1; // TODO use a divsior or no?
					}
				}
				else
				if(m_filterType == Filter_Edge)
				{
					kernel  << 0 <<  1 << 0
						<< 1 << -4 << 1
						<< 0 <<  1 << 0;
					
					divisor = 16;
				}
				else
				{
					//passthru matrix leaves color unchanged
					for(int i=0;i<kernelSize;i++)
						kernel << (i == kernelSize / 2 ? 1 : 0);
				}
				
				if(m_filterType != Filter_CustomConvultionKernel)
					m_kernelSize = kernelSize;
				
				uniformDefs = QString(
					"#define KERNEL_SIZE %1\n"
					"uniform mediump float kernel[KERNEL_SIZE];\n"
					"uniform highp vec2 offset[KERNEL_SIZE];\n"
					).arg(m_kernelSize);
					
				float step_w = 1.0/frameSize.width();
				float step_h = 1.0/frameSize.height();
					
				m_convOffsets.clear();
				m_convOffsets.resize(kernelSize * 2);
				m_convKernel.resize(kernelSize);
				
				int kernelWidth  = kernelSize ==  9 ? 3 :
						   kernelSize == 25 ? 5 :
						   (int)sqrt(kernelSize);
				int kernelHeight = kernelSize ==  9 ? 3 :
						   kernelSize == 25 ? 5 :
						   (int)sqrt(kernelSize);
				for(int y = 0; y < kernelHeight; ++y) 
				{
					for(int x = 0; x < kernelWidth; ++x) 
					{
						m_convOffsets[(y * kernelWidth + x) * 2]     = step_w * (x - (kernelWidth  / 2));
						m_convOffsets[(y * kernelWidth + x) * 2 + 1] = step_h * (    (kernelHeight / 2) - y);
					}
				}
				
				if(divisor == -1)
				{
					float sum = 0;
					for(int i=0;i<kernelSize;i++)
						sum += i < kernel.size() ? kernel[i] : 0;
						
					for(int i=0;i<kernelSize;i++)
						m_convKernel[i] = i < kernel.size() ? kernel[i] / sum : 0;
				}
				else
				{
					for(int i=0;i<kernelSize;i++)
						m_convKernel[i] = i < kernel.size() ? kernel[i] / divisor : 0;
				
				}
				
				bool compareSideBySide = false;
				
				//convolution setup
				{
					convolveSource +=
					"	highp vec4 color = vec4(0.0);\n";
					//"	vec4 sum = vec4(0.0);\n"
					//"	int i = 0;\n";
				}
				
				if(compareSideBySide)
				{
					convolveSource +=
					"	if(texPoint.s<0.495) {\n";
				}
				
				//core of convolution
				{
					// Try to be smarter than the GLSL compiler and unroll the loop for it
// 					convolveSource += 
// 					"	for( i=0; i<KERNEL_SIZE; i++ )\n"
// 					"	{\n"
// 					//"		vec4 tmp = texture2D(texRgb, texPoint + offset[i]);\n"
// 					"		vec4 tmp = vec4(texture2D(texRgb, texPoint + offset[i])."+pixelOrder+", 1.0);\n"
// 					"		sum += tmp * kernel[i];\n"
// 					"	}\n"
				
					for(int i=0;i<kernelSize;i++)
					{
						//QString num = QString().sprintf("%f",i < m_convKernel.size() ? m_convKernel[i] : 1.0);
						// Not hardcoding kernel value because we want to be able to adjust kernel values live without recompiling the shader
						convolveSource +=
							"	color += kernel["+QString::number(i)+"] * vec4(texture2D(texRgb, texPoint + offset["+QString::number(i)+"])."+pixelOrder+", 1.0);\n";
							//"	color += "+num+" * vec4(texture2D(texRgb, texPoint + offset["+QString::number(i)+"])."+pixelOrder+", 1.0);\n";
					}
				}
				
				if(compareSideBySide)
				{
					convolveSource +=
					"	} else if(texPoint.s>0.505) {\n"
					"		color = vec4(texture2D(texRgb, texPoint)."+pixelOrder+", 1.0);\n"
					"	} else {\n"
					"		color = vec4(1.0, 0.0, 0.0, 1.0);\n" 
					"	}\n";
				}
			}
		}
		else
		{
			convolveSource += 
				"	highp vec4 color = vec4(texture2D(texRgb, texPoint)."+pixelOrder+", 1.0);\n";
		}
		
		convolveSource += "	color = colorMatrix * color;\n";

		
		if(m_levelsEnabled
			#ifdef RECOMPILE_SHADERS_TO_INCLUDE_LEVELS 
			&& (m_blackLevel > 0 ||
			    m_whiteLevel < 255)
			#endif
		  )
		{
			uniformDefs +=
				"uniform mediump float blackLevel;\n"
				"uniform mediump float whiteLevel;\n";
				//  /// TODO add midLevel adjustments
				//"uniform highp float gamma;\n";

			convolveSource += 
				#ifndef RECOMPILE_SHADERS_TO_INCLUDE_LEVELS
					// Since whiteLevel is really the delta between black and white, then inverted, it won't be less than 1.0.
					// For example, the user says white is 220 and black is 18 - thats a delta of 202. 
					// Invert that by doing 255/202 and white is 1.262376.
					// blackLevel really is just black level normalized by 255.
				"	if(whiteLevel > 1.0 || blackLevel > 0.0)\n"
				#endif
				// This levels code is based on the example given in gpu gems, referenced at:
				// http://http.developer.nvidia.com/GPUGems/gpugems_ch22.html.
				// Original formula given:
				// outPixel = (pow(((inPixel * 255.0) - inBlack) / (inWhite - inBlack), inGamma) * (outWhite - outBlack) + outBlack) / 255.0;
				// Since we don't need the gamma correction, and we convert white/black to 0-1 instead of upconverting the color to 0-255,
				// we can simplify things by doing vector subtraction/multilication much more easily in one statement.
				// Additionally, I found that dividing by the delta of (white-black) caused *really wierd* problems when crossfading -
				// it faded in all white at 0-.2ish opacity, then faded correctly near 1. - I never could figure out why!
				// However, I noticed when multiplying by the inverted delta, the problem went away...wierd.
				//"		color =  clamp((color - blackLevel) * whiteLevel, 0.0, 1.0);\n";
				// Moved clamping lower - since we do it for a filter as well, why duplicate clamps?
				"		color =  (color - blackLevel) * whiteLevel;\n";
		}
		
		convolveSource +=
			//clamp is necessary for some filters such as sharpening - doesnt hurt others
			"	color = clamp(color,0.0,1.0);\n";
			
		//Passthru for no filter
		//"    highp vec4 color = vec4(texture2D(texRgb, texPoint).bgr, 1.0);\n"
		
		programSource = QString(programSource).arg(uniformDefs).arg(convolveSource);
		//qDebug() << "GLVideoDrawable::resizeTextures(): prog:"<<programSource;
		

		//qDebug() << "GLVideoDrawable::resizeTextures(): "<<(QObject*)this<<"\t offsets:\n";
// 		for(int i=0;i<m_kernelSize*2;i+=2)
// 			qDebug() << "\t "<<m_convOffsets[i]<<" x "<<m_convOffsets[i+1];
// 			
// 		//qDebug() << "GLVideoDrawable::resizeTextures(): "<<(QObject*)this<<"\t kernel:\n";
// 		int inc = m_kernelSize == 9 ? 3 : 5;
// 		for(int i=0;i<m_kernelSize;i+= inc)
// 			if(m_kernelSize == 9)
// 				qDebug() << "\t { " <<
// 					m_convKernel[i+0] << ", " << 
// 					m_convKernel[i+1] << ", " << 
// 					m_convKernel[i+2] <<
// 					" }";
// 			else
// 				qDebug() << "\t { " <<
// 					m_convKernel[i+0] << ", " << 
// 					m_convKernel[i+1] << ", " << 
// 					m_convKernel[i+2] << ", " << 
// 					m_convKernel[i+3] << ", " << 
// 					m_convKernel[i+4] <<  
// 					" }";
		
		QByteArray out;
		out.append(programSource);
		return out;
	}
	else
	{
		QString programSource = QString(fragmentProgram);
		
		if(programSource.indexOf("%1") > -1)
		{
			programSource = QString(programSource)
				.arg("")
				.arg("highp vec4 color = vec4(texture2D(texRgb, texPoint)."+pixelOrder+", 1.0);\n");
				
			QByteArray out;
			out.append(programSource);
			return out;
		}
		else
		{
			//qDebug() << "GLVideoDrawable::resizeTextures(): "<<(QObject*)this<<"\t fragmentProgram output:\n"<<fragmentProgram<<"\n -- [end] -- \n"; 
			QByteArray shader;
			shader.append(fragmentProgram);
			return shader;
		}
	}
	//return fragmentProgram;
}

void GLVideoDrawable::viewportResized(const QSize& /*newSize*/)
{
	updateAlignment();
	updateRects();
}

void GLVideoDrawable::canvasResized(const QSizeF& /*newSize*/)
{
	updateAlignment();
	updateRects();
}

void GLVideoDrawable::drawableRectChanged(const QRectF& /*newRect*/)
{
	//qDebug() << "GLVideoDrawable::drawableResized()";
	//updateAlignment();
	updateRects();
}

void GLVideoDrawable::updateTextureOffsets()
{
	m_invertedOffset = QPointF(m_textureOffset.x() * 1/m_sourceRect.width(),
				   m_textureOffset.y() * 1/m_sourceRect.height());
}

void GLVideoDrawable::updateRects(bool secondSource)
{
// 	if(!m_glInited)
// 		return;
	if(!m_frame)
		return;

	QRectF sourceRect = m_frame->rect();
	//if(m_frame->rect() != m_sourceRect)

	updateTextureOffsets();

	// force mask to be re-scaled
	//qDebug() << "GLVideoDrawable::updateRects(): "<<(QObject*)this<<",  New source rect: "<<m_sourceRect<<", mask size:"<<m_alphaMask.size()<<", isNull?"<<m_alphaMask.isNull();
	
	// Automatically add "overscan" using 3.5% (approx) based on 'Overscan amounts' recommended at http://en.wikipedia.org/wiki/Overscan 
	if(m_isCameraThread)
	{
// 		double assumedFrameWidth  = sourceRect.width();
// 		double assumedFrameHeight = sourceRect.height();
		//double overscanAmount = .02; //035 * .5; // 3.5% * .5 = .0175
		
		//qDebug() << "m_displayOpts.cropTopLeft:"<<m_displayOpts.cropTopLeft<<", m_displayOpts.cropBottomRight:"<<m_displayOpts.cropBottomRight;
		if(m_displayOpts.cropTopLeft.x() == 0)
			m_displayOpts.cropTopLeft.setX(CAMERA_OVERSCAN);
		if(m_displayOpts.cropTopLeft.y() == 0)
			m_displayOpts.cropTopLeft.setY(CAMERA_OVERSCAN);
		if(m_displayOpts.cropBottomRight.x() == 0)
			m_displayOpts.cropBottomRight.setX(-CAMERA_OVERSCAN);
		if(m_displayOpts.cropBottomRight.y() == 0)
			m_displayOpts.cropBottomRight.setY(-CAMERA_OVERSCAN);
	}


	double x1 = m_displayOpts.cropTopLeft.x();
	double y1 = m_displayOpts.cropTopLeft.y();
	double x2 = m_displayOpts.cropBottomRight.x();
	double y2 = m_displayOpts.cropBottomRight.y();
	double w = sourceRect.width();
	double h =  sourceRect.height();
	double c_x1 = x1 * w;
	double c_y1 = y1 * h;
	double c_x2 = x2 * w;
	double c_y2 = y2 * h;
	
	QRectF adjustedSource = sourceRect.adjusted(
		c_x1,
		c_y1,
		c_x2,
		c_y2);
		
// 	qDebug() << "GLVideoDrawable::updateRects(): "<<(QObject*)this<<",  Source rect: "<<sourceRect<<", adjustedSource:"<<adjustedSource << "\n" //crop:"<<m_displayOpts.cropTopLeft<<"/:"<<m_displayOpts.cropBottomRight;
// 		<< "x1:"<<x1
// 		<< "y1:"<<y1
// 		<< "x2:"<<x2
// 		<< "y2:"<<y2
// 		<< "\n"
// 		<< "w:"<<w
// 		<< "h:"<<h
// 		<< "\n"
// 		<< "c_x1:"<<c_x1
// 		<< "c_y1:"<<c_y1
// 		<< "c_x2:"<<c_x2
// 		<< "c_y2:"<<c_y2
// 		;
// 	m_origSourceRect = m_sourceRect;
//
// 	m_sourceRect.adjust(m_adjustDx1,m_adjustDy1,m_adjustDx2,m_adjustDy2);

	QSizeF nativeSize = adjustedSource.size(); //m_frame->size();

	QRectF targetRect;

	if (nativeSize.isEmpty())
	{
		targetRect = QRectF();
	}
	else
	if (m_aspectRatioMode == Qt::IgnoreAspectRatio)
	{
		targetRect = rect();
	}
	else
	if (m_aspectRatioMode == Qt::KeepAspectRatio)
	{
		QSizeF size = nativeSize;
		size.scale(rect().size(), Qt::KeepAspectRatio);

		targetRect = QRectF(0, 0, size.width(), size.height());
		targetRect.moveCenter(rect().center());
	}
	else
	if (m_aspectRatioMode == Qt::KeepAspectRatioByExpanding)
	{
		targetRect = rect();

		QSizeF size = rect().size();
		size.scale(nativeSize, Qt::KeepAspectRatio);

		sourceRect = QRectF(QPointF(0,0),size);
		sourceRect.moveCenter(QPointF(size.width() / 2, size.height() / 2));
	}

	if(!secondSource)
	{
		m_sourceRect = sourceRect;
		m_targetRect = targetRect;
	}
	else
	{
		m_sourceRect2 = sourceRect;
		m_targetRect2 = targetRect;
	}

	setAlphaMask(m_alphaMask_preScaled);
	//qDebug() << "GLVideoDrawable::updateRects(): "<<(QObject*)this<<m_source<<" m_sourceRect:"<<m_sourceRect<<", adjustedSource:"<<adjustedSource<<", m_targetRect:"<<m_targetRect<<", rect:"<<rect();
}

// float opacity = 0.5;
// 	glColor4f(opacity,opacity,opacity,opacity);

void GLVideoDrawable::setDisplayOptions(const VideoDisplayOptions& opts)
{
	//qDebug() << "GLVideoDrawable::setDisplayOptions: opts.flipVertical:"<<opts.flipVertical<<", opts.flipHorizontal:"<<opts.flipHorizontal;
	m_displayOpts = opts;
	m_colorsDirty = true;
	emit displayOptionsChanged(opts);
	updateGL();
}

void GLVideoDrawable::setTextureOffset(double x, double y)
{
	setTextureOffset(QPointF(x,y));
}

void GLVideoDrawable::setTextureOffset(const QPointF& point)
{
	m_textureOffset = point;
	updateTextureOffsets();
	updateGL();
}

// returns the highest number closest to v, which is a power of 2
// NB! assumes 32 bit ints
int qt_next_power_of_two(int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    ++v;
    return v;
}


// map from Qt's ARGB endianness-dependent format to GL's big-endian RGBA layout
static inline void qgl_byteSwapImage(QImage &img, GLenum pixel_type)
{
    const int width = img.width();
    const int height = img.height();

    if (pixel_type == GL_UNSIGNED_INT_8_8_8_8_REV
        || (pixel_type == GL_UNSIGNED_BYTE && QSysInfo::ByteOrder == QSysInfo::LittleEndian))
    {
        for (int i = 0; i < height; ++i) {
            uint *p = (uint *) img.scanLine(i);
            for (int x = 0; x < width; ++x)
                p[x] = ((p[x] << 16) & 0xff0000) | ((p[x] >> 16) & 0xff) | (p[x] & 0xff00ff00);
        }
    } else {
        for (int i = 0; i < height; ++i) {
            uint *p = (uint *) img.scanLine(i);
            for (int x = 0; x < width; ++x)
                p[x] = (p[x] << 8) | ((p[x] >> 24) & 0xff);
        }
    }
}

//#define QGL_BIND_TEXTURE_DEBUG

static void uploadTexture(GLuint tx_id, const QImage &image)
{
	 GLenum target = GL_TEXTURE_2D;
	 GLint internalFormat = GL_RGBA;
	 QGLContext::BindOptions options;
	 //const qint64 key = 0;

    //Q_Q(QGLContext);

#ifdef QGL_BIND_TEXTURE_DEBUG
    printf("GLVideoDrawable / uploadTexture(), imageSize=(%d,%d), internalFormat =0x%x, options=%x\n",
           image.width(), image.height(), internalFormat, int(options));
    QTime time;
    time.start();
#endif

#ifndef QT_NO_DEBUG
    // Reset the gl error stack...git
    while (glGetError() != GL_NO_ERROR) ;
#endif

    // Scale the pixmap if needed. GL textures needs to have the
    // dimensions 2^n+2(border) x 2^m+2(border), unless we're using GL
    // 2.0 or use the GL_TEXTURE_RECTANGLE texture target
    int tx_w = qt_next_power_of_two(image.width());
    int tx_h = qt_next_power_of_two(image.height());

    QImage img = image;

    if (// !(QGLExtensions::glExtensions() & QGLExtensions::NPOTTextures)
        //&&
        !(QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_ES_Version_2_0)
        && (target == GL_TEXTURE_2D && (tx_w != image.width() || tx_h != image.height())))
    {
        img = img.scaled(tx_w, tx_h);
#ifdef QGL_BIND_TEXTURE_DEBUG
        printf(" - upscaled to %dx%d (%d ms)\n", tx_w, tx_h, time.elapsed());

#endif
    }


    GLuint filtering = options & QGLContext::LinearFilteringBindOption ? GL_LINEAR : GL_NEAREST;

    //GLuint tx_id;
    //glGenTextures(1, &tx_id);
    glBindTexture(target, tx_id);
    glTexParameterf(target, GL_TEXTURE_MAG_FILTER, filtering);

    //qDebug() << "Upload, mark1";

#if defined(QT_OPENGL_ES_2)
    bool genMipmap = false;
#endif

//     if (glFormat.directRendering()
//         && (QGLExtensions::glExtensions() & QGLExtensions::GenerateMipmap)
//         && target == GL_TEXTURE_2D
//         && (options & QGLContext::MipmapBindOption))
//     {
// #ifdef QGL_BIND_TEXTURE_DEBUG
//         printf(" - generating mipmaps (%d ms)\n", time.elapsed());
// #endif
// #if !defined(QT_OPENGL_ES_2)
//         glHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
// #ifndef QT_OPENGL_ES
//         glTexParameteri(target, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
// #else
//         glTexParameterf(target, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
// #endif
// #else
//         glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
//         genMipmap = true;
// #endif
//         glTexParameterf(target, GL_TEXTURE_MIN_FILTER, options & QGLContext::LinearFilteringBindOption
//                         ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST);
//     } else {
        glTexParameterf(target, GL_TEXTURE_MIN_FILTER, filtering);
    //}

    QImage::Format target_format = img.format();
    bool premul = options & QGLContext::PremultipliedAlphaBindOption;
    GLenum externalFormat;
    GLuint pixel_type;
//     if (QGLExtensions::glExtensions() & QGLExtensions::BGRATextureFormat) {
//         externalFormat = GL_BGRA;
//         if (QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_1_2)
//             pixel_type = GL_UNSIGNED_INT_8_8_8_8_REV;
//         else
//             pixel_type = GL_UNSIGNED_BYTE;
//     } else {
        externalFormat = GL_RGBA;
        pixel_type = GL_UNSIGNED_BYTE;
    //}

    switch (target_format) {
    case QImage::Format_ARGB32:
        if (premul) {
            img = img.convertToFormat(target_format = QImage::Format_ARGB32_Premultiplied);
#ifdef QGL_BIND_TEXTURE_DEBUG
            printf(" - converting ARGB32 -> ARGB32_Premultiplied (%d ms) \n", time.elapsed());
#endif
        }
        break;
    case QImage::Format_ARGB32_Premultiplied:
        if (!premul) {
            img = img.convertToFormat(target_format = QImage::Format_ARGB32);
#ifdef QGL_BIND_TEXTURE_DEBUG
            printf(" - converting ARGB32_Premultiplied -> ARGB32 (%d ms)\n", time.elapsed());
#endif
        }
        break;
    case QImage::Format_RGB16:
        pixel_type = GL_UNSIGNED_SHORT_5_6_5;
        externalFormat = GL_RGB;
        internalFormat = GL_RGB;
        break;
    case QImage::Format_RGB32:
        break;
    default:
        if (img.hasAlphaChannel()) {
            img = img.convertToFormat(premul
                                      ? QImage::Format_ARGB32_Premultiplied
                                      : QImage::Format_ARGB32);
#ifdef QGL_BIND_TEXTURE_DEBUG
            printf(" - converting to 32-bit alpha format (%d ms)\n", time.elapsed());
#endif
        } else {
            img = img.convertToFormat(QImage::Format_RGB32);
#ifdef QGL_BIND_TEXTURE_DEBUG
            printf(" - converting to 32-bit (%d ms)\n", time.elapsed());
#endif
        }
    }

    if (options & QGLContext::InvertedYBindOption) {
#ifdef QGL_BIND_TEXTURE_DEBUG
            printf(" - flipping bits over y (%d ms)\n", time.elapsed());
#endif
        if (img.isDetached()) {
            int ipl = img.bytesPerLine() / 4;
            int h = img.height();
            for (int y=0; y<h/2; ++y) {
                int *a = (int *) img.scanLine(y);
                int *b = (int *) img.scanLine(h - y - 1);
                for (int x=0; x<ipl; ++x)
                    qSwap(a[x], b[x]);
            }
        } else {
            // Create a new image and copy across.  If we use the
            // above in-place code then a full copy of the image is
            // made before the lines are swapped, which processes the
            // data twice.  This version should only do it once.
            img = img.mirrored();
        }
    }

    if (externalFormat == GL_RGBA) {
#ifdef QGL_BIND_TEXTURE_DEBUG
            printf(" - doing byte swapping (%d ms)\n", time.elapsed());
#endif
        // The only case where we end up with a depth different from
        // 32 in the switch above is for the RGB16 case, where we set
        // the format to GL_RGB
        Q_ASSERT(img.depth() == 32);
        qgl_byteSwapImage(img, pixel_type);
        //qDebug() << "Upload, mark2 - unable to byte swap - dont hvae helper";
    }
#ifdef QT_OPENGL_ES
    // OpenGL/ES requires that the internal and external formats be
    // identical.
    internalFormat = externalFormat;
#endif
#ifdef QGL_BIND_TEXTURE_DEBUG
    printf(" - uploading, image.format=%d, externalFormat=0x%x, internalFormat=0x%x, pixel_type=0x%x\n",
           img.format(), externalFormat, internalFormat, pixel_type);
#endif

    const QImage &constRef = img; // to avoid detach in bits()...
    glTexImage2D(target, 0, internalFormat, img.width(), img.height(), 0, externalFormat,
                 pixel_type, constRef.bits());
#if defined(QT_OPENGL_ES_2)
    if (genMipmap)
        glGenerateMipmap(target);
#endif
#ifndef QT_NO_DEBUG
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        qWarning(" - texture upload failed, error code 0x%x\n", error);
    }
#endif

#ifdef QGL_BIND_TEXTURE_DEBUG
    static int totalUploadTime = 0;
    totalUploadTime += time.elapsed();
    printf(" - upload done in (%d ms) time=%d\n", time.elapsed(), totalUploadTime);
#endif


    // this assumes the size of a texture is always smaller than the max cache size
//     int cost = img.width()*img.height()*4/1024;
//     QGLTexture *texture = new QGLTexture(q, tx_id, target, options);
//     QGLTextureCache::instance()->insert(q, key, texture, cost);
    //return texture;
}


void GLVideoDrawable::updateTexture(bool secondSource)
{
//   	if(property("-debug").toBool())
   		//qDebug() << "GLVideoDrawable::updateTexture(): "<<(QObject*)this<<" secondSource:"<<secondSource;
		//qDebug() << "GLVideoDrawable::updateTexture(): "<<(QObject*)this;
	if(!secondSource ? (!m_frame || !m_frame->isValid()) : (!m_frame2 || !m_frame2->isValid()))
	{
		//qDebug() << "GLVideoDrawable::updateTexture(): "<<(QObject*)this<<" Frame not valid";
		return;
	}

	if(m_glInited && glWidget())
	{
		//if(objectName() != "StaticBackground")
// 		if(property("-debug").toBool())
//  			qDebug() << "GLVideoDrawable::updateTexture(): "<<(QObject*)this<<" Got a frame, size:"<<m_frame->size();
		//if()
			//m_frameSize = m_frame->size();
 		//qDebug() << "GLVideoDrawable::updateTexture(): "<<objectName()<<" Got frame size:"<<m_frame->size();
 		//qDebug() << "GLVideoDrawable::updateTexture(): "<<objectName()<<" Got frame size:"<<m_frame->size();
		//qDebug() << "GLVideoDrawable::updateTexture(): "<<(QObject*)this<<" Got frame size:"<<m_frame->size();

		if(!secondSource ? (m_frameSize != m_frame->size()   || m_frame->rect() != m_sourceRect   || !m_texturesInited) :
				   (m_frameSize2 != m_frame2->size() || m_frame2->rect() != m_sourceRect2 || !m_texturesInited2))
		{
 			//qDebug() << "GLVideoDrawable::updateTexture(): m_frame->rect():"<<m_frame->rect()<<", m_sourceRect:"<<m_sourceRect<<", m_frame->size():"<<m_frame->size();
//    			if(property("-debug").toBool())
//    				qDebug() << "GLVideoDrawable::updateTexture(): "<<(QObject*)this<<" frame size changed or !m_texturesInited, resizing and adjusting pixels...";
			//if(m_videoFormat.pixelFormat != m_source->videoFormat().pixelFormat)

			if(!secondSource)
			{
				if(m_videoFormat.pixelFormat != m_frame->pixelFormat())
					setVideoFormat(VideoFormat(m_frame->bufferType(), m_frame->pixelFormat(), m_frame->size()), secondSource);
			}
			else
			{
				if(m_videoFormat2.pixelFormat != m_frame2->pixelFormat())
					setVideoFormat(VideoFormat(m_frame2->bufferType(), m_frame2->pixelFormat(), m_frame2->size()), secondSource);
			}

			resizeTextures(!secondSource ? m_frame->size() : m_frame2->size(), secondSource);
			updateRects(secondSource);
			updateAlignment();
		}

		if(secondSource ? !m_validShader2 : !m_validShader)
		{
// 			if(property("-debug").toBool())
// 				qDebug() << "GLVideoDrawable::updateTexture(): "<<(QObject*)this<<" No valid shader, not updating, secondSource:"<<secondSource;
			return;
		}

		glWidget()->makeRenderContextCurrent();

		if(!secondSource ? m_frame->isEmpty() : m_frame2->isEmpty())
		{
			qDebug() << "GLVideoDrawable::updateTexture(): Got empty frame, ignoring.";
		}
		else
		if(!secondSource ? m_frame->isRaw() : m_frame2->isRaw())
		{
//  			if(property("-debug").toBool())
//  				qDebug() << "GLVideoDrawable::updateTexture(): "<<objectName()<<" Mark: raw frame";

			// Raw frames are just a pointer to a block of memory containing the image data 
			// (as opposed to a QImage, which holds to memory internally). Raw frames are 
			// especially useful for things like the YUV formats and derivatives where the
			// conversion from YUV to RGB needs to happen on the GPU
			for (int i = 0; i < (!secondSource ? m_textureCount : m_textureCount2); ++i)
			{
				//qDebug() << "raw: "<<i<<m_textureWidths[i]<<m_textureHeights[i]<<m_textureOffsets[i]<<m_textureInternalFormat<<m_textureFormat<<m_textureType;
				glBindTexture(GL_TEXTURE_2D, (!secondSource ? m_textureIds[i] : m_textureIds2[i]));
				if(m_useShaders)
				{
					// Use different upload blocks because the
					// texture widths/heights/type/formats are stored
					// in different variables. It might be good to switch this code
					// to remove the almost-duplicate blocks and combine them in some way...we'll see...
					if(!secondSource)
					{
						// Use a different upload call for packed YUV because
						// it has different tex format/internal format per
						// texture due to the packed nature of the format.
						if(m_packed_yuv)
						{
							glTexImage2D(
								GL_TEXTURE_2D,
								0,
								m_textureInternalFormats[i],
								m_textureWidths[i],
								m_textureHeights[i],
								0,
								m_textureFormats[i],
								m_textureType,
								//m_frame->byteArray().constData() + m_textureOffsets[i]
								m_frame->pointer() + m_textureOffsets[i]
								//m_frame->bufferType() == VideoFrame::BUFFER_POINTER ? m_frame->data()[i] :
									//(uint8_t*)m_frame->byteArray().constData() + m_textureOffsets[i]
							);
						}
						else
						{
							glTexImage2D(
								GL_TEXTURE_2D,
								0,
								m_textureInternalFormat,
								m_textureWidths[i],
								m_textureHeights[i],
								0,
								m_textureFormat,
								m_textureType,
								//m_frame->byteArray().constData() + m_textureOffsets[i]
								m_frame->pointer() + m_textureOffsets[i]
								//m_frame->bufferType() == VideoFrame::BUFFER_POINTER ? m_frame->data()[i] :
									//(uint8_t*)m_frame->byteArray().constData() + m_textureOffsets[i]
							);
						}
						
						if( m_yuv && m_packed_yuv )
						{
							// In case of packed yuv pixel formats(like UYVY and YUY2)
							// GL_LINEAR filtering doesn't work because pixels are packed
							// inside texture. Unpacking is done fragment shader.
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
						}
						else
						{
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
						}
		// 				
					}
					else
					{
						// Use a different upload call for packed YUV because
						// it has different tex format/internal format per
						// texture due to the packed nature of the format.
						if(m_packed_yuv2)
						{
							glTexImage2D(
								GL_TEXTURE_2D,
								0,
								m_textureInternalFormats[i+3],
								m_textureWidths[i+3],
								m_textureHeights[i+3],
								0,
								m_textureFormats[i+3],
								m_textureType2,
								//m_frame->byteArray().constData() + m_textureOffsets[i]
								m_frame->pointer() + m_textureOffsets[i+3]
								//m_frame->bufferType() == VideoFrame::BUFFER_POINTER ? m_frame->data()[i] :
									//(uint8_t*)m_frame->byteArray().constData() + m_textureOffsets[i]
							);
						}
						else
						{
							glTexImage2D(
								GL_TEXTURE_2D,
								0,
								m_textureInternalFormat2,
								m_textureWidths[i+3],
								m_textureHeights[i+3],
								0,
								m_textureFormat2,
								m_textureType2,
								m_frame2->pointer() + m_textureOffsets[i+3]
								//m_frame->bufferType() == VideoFrame::BUFFER_POINTER ? m_frame->data()[i] :
									//(uint8_t*)m_frame->byteArray().constData() + m_textureOffsets[i]
							);
						}
						
						if( m_yuv2 && m_packed_yuv2 )
						{
							// In case of packed yuv pixel formats(like UYVY and YUY2)
							// GL_LINEAR filtering doesn't work because pixels are packed
							// inside texture. Unpacking is done fragment shader.
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
						}
						else
						{
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
						}
		// 				
					}
				}
				else
				{
					// Not going to worry about the 'packed yuv' formats (or even YUV in general) 
					// because we need GLSL shaders inorder to handle those - without shaders, we only to RGB variants 
					if(!secondSource)
					{
						glTexImage2D(
							GL_TEXTURE_2D,
							0,
							3, //m_textureInternalFormat,
							m_textureWidths[i],
							m_textureHeights[i],
							0,
							GL_BGRA, //m_textureFormat,
							GL_UNSIGNED_BYTE, //m_textureType,
							m_frame->pointer() + m_textureOffsets[i]
							//m_frame->bufferType() == VideoFrame::BUFFER_POINTER ? m_frame->data()[i] :
								//(uint8_t*)m_frame->byteArray().constData() + m_textureOffsets[i]
						);
					}
					else
					{
						glTexImage2D(
							GL_TEXTURE_2D,
							0,
							3, //m_textureInternalFormat,
							m_textureWidths[i+3],
							m_textureHeights[i+3],
							0,
							GL_BGRA, //m_textureFormat,
							GL_UNSIGNED_BYTE, //m_textureType,
							m_frame2->pointer() + m_textureOffsets[i]
							//m_frame->bufferType() == VideoFrame::BUFFER_POINTER ? m_frame->data()[i] :
								//(uint8_t*)m_frame->byteArray().constData() + m_textureOffsets[i]
						);
					}
					
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				}

				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
// 				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

			}
		}
		else
		if(!m_frame->image().isNull())
		{
//   			if(property("-debug").toBool())
//   			{
//   				qDebug() << "GLVideoDrawable::updateTexture(): "<<(QObject*)this<<" Mark: QImage frame";
//   				m_frame->image().save("debug.jpg");
//   			}

			for (int i = 0; i < (!secondSource ? m_textureCount : m_textureCount2); ++i)
			{
// 				if(property("-debug").toBool())
// 					qDebug() << (QObject*)(this) << "normal: "<<i<<m_textureWidths[i]<<m_textureHeights[i]<<m_textureOffsets[i]<<m_textureInternalFormat<<m_textureFormat<<m_textureType;
// 				QImageWriter writer("test.jpg");
// 				writer.write(m_frame->image());

				const QImage &constRef = !secondSource ? m_frame->image() : m_frame2->image(); // avoid detach in .bits()

				glBindTexture(GL_TEXTURE_2D, !secondSource ? m_textureIds[i] : m_textureIds2[i]);
				if(m_useShaders)
				{
					if(!secondSource)
					{
						glTexImage2D(
							GL_TEXTURE_2D,
							0,
							m_textureInternalFormat,
							m_textureWidths[i],
							m_textureHeights[i],
							0,
							m_textureFormat,
							m_textureType,
							constRef.bits() + m_textureOffsets[i]
							);
					}
					else
					{
						glTexImage2D(
							GL_TEXTURE_2D,
							0,
							m_textureInternalFormat2,
							m_textureWidths[i+3],
							m_textureHeights[i+3],
							0,
							m_textureFormat2,
							m_textureType2,
							constRef.bits() + m_textureOffsets[i]
							);
					}
				}
				else
				{
					//m_frame->image() = m_frame->image().convertToFormat(QImage::Format_ARGB32);
// 					if(property("-debug").toBool())
// 						qDebug() << "No shader, custom glTexImage2D arguments";

 					//QImage texGL = m_frame->image();
// 					//glTexImage2D( GL_TEXTURE_2D, 0, 3, texGL.width(), texGL.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, texGL.bits() );
 					//glTexImage2D( GL_TEXTURE_2D, 0, 3, texGL.width(), texGL.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, texGL.bits() );
//  					qDebug() << (QObject*)(this) << "my args:      "<<m_textureInternalFormat<<m_textureFormat<<m_textureType;
//  					qDebug() << (QObject*)(this) << "working args: "<<3<<GL_RGBA<<GL_UNSIGNED_BYTE;
//  					qDebug() << (QObject*)(this) << "image format#:"<<m_frame->image().format();

					// Why does this work?? m_frame->image().format == #4, RGB32, but tex format seems to require BGRA when using non-GLSL texture rendering...wierd...
					m_textureFormat = GL_BGRA;

					if(0)
					{
						m_textureIds[i] = m_glw->bindTexture(m_frame->image());
					}

					if(1)
					{
						uploadTexture(m_textureIds[i],!secondSource ? m_frame->image() : m_frame2->image());
					}

					if(0)
					{
						glTexImage2D(
							GL_TEXTURE_2D,
							0,
							m_textureInternalFormat,
							m_textureWidths[i],
							m_textureHeights[i],
							0,
							m_textureFormat,
							m_textureType,
							(const uchar*)constRef.bits()
							//m_frame->image().scanLine(0)
						);
					}

					//glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
 					//glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
				}

				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
// 				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
// 				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			}
		}
	}
	else
	{
		if(!secondSource ? (m_frame->rect()  != m_sourceRect) :
		                   (m_frame2->rect() != m_sourceRect2))
		{
			updateRects(secondSource);
			updateAlignment(secondSource);
		}

		m_textureUpdateNeeded = true;
	}

// 	if(property("-debug").toBool())
// 		qDebug() << "GLVideoDrawable::updateTexture(): "<<(QObject*)this<<" Done update";
}


void GLVideoDrawable::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
	paintChildren(true, painter, option, widget); // paint children below
	
	m_unrenderedFrames --;
	
// 	painter->setPen(pen);
// 	painter->setBrush(brush);
// 	painter->drawRect(m_contentsRect);

// 	if(m_frame &&
// 		(m_frameSize != m_frame->size() || m_frame->rect() != m_sourceRect || m_targetRect.isEmpty()))
// 	{
// 		updateRects(false);
// 		updateAlignment();
// 	}
//
// 	if(m_frame2 &&
// 		(m_frameSize2 != m_frame2->size() || m_frame2->rect() != m_sourceRect2))
// 	{
// 		updateRects(true);
// 		updateAlignment();
// 	}

	updateAnimations(true);

	aboutToPaint();

	// Since QGraphicsScene/QGraphicsView doesn't have (that I know of) a way to hook into when the scene gets added to the view,
	// then we have to 'guess' by finding out when we start painting here.
	if(!liveStatus())
		setLiveStatus(true);

	QRectF source = m_sourceRect;
	QRectF target = QRectF(m_targetRect.topLeft() - rect().topLeft(),m_targetRect.size());

	source = source.adjusted(
		m_displayOpts.cropTopLeft.x() * source.width(),
		m_displayOpts.cropTopLeft.y() * source.height(),
		m_displayOpts.cropBottomRight.x() * source.width(),
		m_displayOpts.cropBottomRight.y() * source.height());

	//const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat());

	painter->setRenderHint(QPainter::HighQualityAntialiasing, true);
	painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

	double opac = opacity() * (m_fadeActive ? m_fadeValue:1);
	//qDebug() << "GLVideoDrawable::paint(): "<<(QObject*)this<<", target:"<<target; 
	//<<" opac used:"<<opac<<", m_:"<<opacity()<<", fade act:"<<m_fadeActive<<", fade val:"<<m_fadeValue;
	painter->setOpacity(opac);

	if(!m_frame)
		return;

	if(!m_frame->image().isNull())
	{
		painter->drawImage(target,m_frame->image(),source);
 		//qDebug() << "GLVideoDrawable::paint(): "<<(QObject*)this<<" Painted m_frame, size:" << m_frame->image().size()<<", source:"<<source<<", target:"<<target<<", m_targetRect:"<<m_targetRect<<", rect:"<<rect()<<", pos:"<<pos();
	}
	else
	{
		const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(m_frame->pixelFormat());
		if(imageFormat != QImage::Format_Invalid)
		{
			QImage image(m_frame->pointer(),
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

			if(m_displayOpts.flipHorizontal || m_displayOpts.flipVertical)
				image = image.mirrored(m_displayOpts.flipHorizontal, m_displayOpts.flipVertical);

			painter->drawImage(target,image,source);
			//qDebug() << "GLVideoDrawable::paint(): "<<(QObject*)this<<" Painted RAW frame, size:" << image.size()<<", source:"<<source<<", target:"<<target;
		}
		else
		{
			qDebug() << "GLVideoDrawable::paint(): "<<(QObject*)this<<" Unable to convert pixel format to image format, cannot paint frame. Pixel Format:"<<m_frame->pixelFormat();
		}
	}

	if(m_fadeActive)
	{
		QRectF source2 = m_sourceRect2;
		QRectF target2 = QRectF(m_targetRect2.topLeft() - rect().topLeft(),m_targetRect2.size());

		source2 = source2.adjusted(
			m_displayOpts.cropTopLeft.x(),
			m_displayOpts.cropTopLeft.y(),
			m_displayOpts.cropBottomRight.x(),
			m_displayOpts.cropBottomRight.y());



		painter->setOpacity(opacity() * (1.-m_fadeValue));

		if(!m_frame2->image().isNull())
		{
			painter->drawImage(target2,m_frame2->image(),source2);
			//qDebug() << "GLVideoDrawable::paint(): "<<(QObject*)this<<" Painted m_frame2, size:" << m_frame2->image().size()<<", source:"<<source2<<", target:"<<target2;
		}
		else
		{
			const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(m_frame2->pixelFormat());
			if(imageFormat != QImage::Format_Invalid)
			{
				QImage image(m_frame2->pointer(),
					     m_frame2->size().width(),
					     m_frame2->size().height(),
					     m_frame2->size().width() *
						(imageFormat == QImage::Format_RGB16  ||
						imageFormat == QImage::Format_RGB555 ||
						imageFormat == QImage::Format_RGB444 ||
						imageFormat == QImage::Format_ARGB4444_Premultiplied ? 2 :
						imageFormat == QImage::Format_RGB888 ||
						imageFormat == QImage::Format_RGB666 ||
						imageFormat == QImage::Format_ARGB6666_Premultiplied ? 3 :
						4),
					     imageFormat);

				painter->drawImage(target2,image,source2);
				//qDebug() << "GLVideoDrawable::paint(): "<<(QObject*)this<<" Painted RAW frame2, size:" << image.size()<<", source:"<<source<<", target:"<<target;
			}
			else
			{
				//qDebug() << "GLVideoDrawable::paint(): "<<(QObject*)this<<" Unable to convert pixel format to image format, cannot paint frame2. Pixel Format:"<<m_frame->pixelFormat();
			}
		}
	}


	if(!m_frame->captureTime().isNull())
	{
		int msecLatency = m_frame->captureTime().msecsTo(QTime::currentTime());

		m_latencyAccum += msecLatency;
	}

	if (!(m_frameCount % 100))
	{
		m_avgFps = m_frameCount /(m_time.elapsed() / 1000.0);
		
		QString framesPerSecond;
		framesPerSecond.setNum(m_avgFps, 'f', 2);

		QString latencyPerFrame;
		latencyPerFrame.setNum((((double)m_latencyAccum) / ((double)m_frameCount)), 'f', 3);

		if(m_debugFps && framesPerSecond!="0.00")
			qDebug() << "GLVideoDrawable::paint: "<<objectName()<<" FPS: " << qPrintable(framesPerSecond) << (m_frame->captureTime().isNull() ? "" : qPrintable(QString(", Latency: %1 ms").arg(latencyPerFrame)));
		
		m_time.start();
		m_frameCount = 0;
		m_latencyAccum = 0;

		//lastFrameTime = time.elapsed();
		electUpdateLeader();
	}

	m_frameCount ++;

	painter->setOpacity(1.);
	
	GLDrawable::paint(painter, option, widget);
	
	paintChildren(false, painter, option, widget); // paint children above

}

void GLVideoDrawable::aboutToPaint()
{
	// NOOP
}

void GLVideoDrawable::updateAnimations(bool insidePaint)
{
	// Because the QAbsoluteTimeAnimations could potentially call another updateGL()
	// we lock calls to updateGL() via lockUpdates() prior to updating the anim times
	// only if we're insidePaint though. 
	bool lock = false;
	if(insidePaint)
		lock = lockUpdates(true);
	
	updateAbsoluteTimeAnimations();
	
	if(insidePaint)
		lockUpdates(lock);
	
	// Call the fade update function directly for both
	// our local crossfade and the scene crossfade
	if(m_fadeActive)
		xfadeTick(!insidePaint); // dont update GL

	if(glScene() && glScene()->fadeActive())
		glScene()->recalcFadeOpacity(!insidePaint); // dont call setOpacity internally, which calls updateGL on each drawable in the scene
}

void GLVideoDrawable::paintGL()
{
	if(!m_glw)
		return;
		
	paintGLChildren(true); // paint children below 
	
	m_unrenderedFrames --;
	
	if(!m_validShader)
	{
// 		if(property("-debug").toBool())
// 			qDebug() << "GLVideoDrawable::paintGL(): "<<(QObject*)this<<" No valid shader, not painting";
		return;
	}

	if(!m_texturesInited)
	{
		qDebug() << "GLVideoDrawable::paintGL(): "<<(QObject*)this<<" Textures not inited, not painting.";
		return;
	}

	if (m_colorsDirty)
	{
		//qDebug() << "Updating color matrix";
		updateColors(m_displayOpts.brightness, m_displayOpts.contrast, m_displayOpts.hue, m_displayOpts.saturation);
		m_colorsDirty = false;
        }

//  	if(property("-debug").toBool())
//  	{
//  		updateTexture();
//  		qDebug() << "GLVideoDrawable::paintGL():"<<(QObject*)this;
//  	}

	if(m_textureUpdateNeeded)
	{
		updateTexture();
		m_textureUpdateNeeded = false;
	}


	//m_frame->unmap()();

	updateAnimations(true);

	QRectF source = m_sourceRect;
	QRectF target = m_targetRect;
	
	if(parent() && parent()->hasFrameBuffer())
	{
		// offset our target negativly by the difference between our self and our parent
		QPointF diff = target.topLeft() - parent()->coverageRect().topLeft();
		target = QRectF(diff, target.size());//moveBy(-diff.x(), -diff.y());
		qDebug() << "GLVideoDrawable::paintGL():"<<(QObject*)this<<": offset diff: "<<diff<<", old pos:"<<m_targetRect.topLeft()<<", new pos:"<<target.topLeft();
		
	}
	else
	if(hasFrameBuffer())
	{
		QPointF targ = target.topLeft();
		QPointF obj  = coverageRect().topLeft();
		QPointF diff = targ - obj;
		//diff = QPointF(6,310);
		target = QRectF(diff, target.size());
		qDebug() << "GLVideoDrawable::paintGL():"<<(QObject*)this<<": self FB: diff: "<<diff<<", obj pos:"<<obj<<", targ pos:"<<targ<<", new target:"<<target<<", coverage:"<<coverageRect();
		//m_useShaders = false;
	}
	

	source = source.adjusted(
		m_displayOpts.cropTopLeft.x() * source.width(),
		m_displayOpts.cropTopLeft.y() * source.height(),
		m_displayOpts.cropBottomRight.x() * source.width(),
		m_displayOpts.cropBottomRight.y() * source.height());

// 	if(property("-debug").toBool())
// 		qDebug() << "GLVideoDrawable::paintGL():"<<(QObject*)this<<": source:"<<source<<", target:"<<target<<" ( crop:"<<m_displayOpts.cropTopLeft<<"/"<<m_displayOpts.cropBottomRight<<")";


	const int width  = (int)( hasFrameBuffer() ? m_frameBuffer->width() : 
		(parent() && parent()->hasFrameBuffer()) ? parent()->coverageRect().width() : 
		QGLContext::currentContext()->device()->width() );
	const int height = (int)( hasFrameBuffer() ? m_frameBuffer->height() : 
		(parent() && parent()->hasFrameBuffer()) ? parent()->coverageRect().height() :
		QGLContext::currentContext()->device()->height() );

	//QPainter painter(this);
	QTransform transform =  ((parent() && parent()->hasFrameBuffer()) || hasFrameBuffer()) ? QTransform() : m_glw->transform(); //= painter.deviceTransform();
	if(m_lastKnownTransform != transform)
	{
		m_lastKnownTransform = transform;
		QTimer::singleShot(20, this, SLOT(transformChanged()));
	}
	
	//transform = transform.scale(1.25,1.);
	if(!translation().isNull())
		transform *= QTransform().translate(translation().x(),translation().y());

	const GLfloat wfactor =  2.0 / width;
	const GLfloat hfactor = -2.0 / height;

	if(!rotation().isNull())
	{
 		qreal tx = target.width()  * rotationPoint().x() + target.x();
 		qreal ty = target.height() * rotationPoint().y() + target.y();
 		qreal x, y;
 		transform.map(tx,ty,&x,&y);

 		QVector3D rot = rotation();
  		transform *= QTransform()
 			.translate(x,y)
 			.rotate(rot.x(),Qt::XAxis)
 			.rotate(rot.y(),Qt::YAxis)
 			.rotate(rot.z(),Qt::ZAxis)
 			.translate(-x,-y);
	}
	const GLfloat positionMatrix[4][4] =
	{
		{
			/*(0,0)*/ wfactor * transform.m11() - transform.m13(),
			/*(0,1)*/ hfactor * transform.m12() + transform.m13(),
			/*(0,2)*/ 0.0,
			/*(0,3)*/ transform.m13()
		}, {
			/*(1,0)*/ wfactor * transform.m21() - transform.m23(),
			/*(1,1)*/ hfactor * transform.m22() + transform.m23(),
			/*(1,2)*/ 0.0,
			/*(1,3)*/ transform.m23()
		}, {
			/*(2,0)*/ 0.0,
			/*(2,1)*/ 0.0,
			/*(2,2)*/ -1.0,
			/*(2,3)*/ 0.0
		}, {
			/*(3,0)*/ wfactor * transform.dx() - transform.m33(),
			/*(3,1)*/ hfactor * transform.dy() + transform.m33(),
			/*(3,2)*/ 0.0,
			/*(3,3)*/ transform.m33()
		}
	};


	//QVector3D list[] =

	const GLfloat vertexCoordArray[] =
	{
		target.left()     , target.bottom() + 1, //(GLfloat)zIndex(),
		target.right() + 1, target.bottom() + 1, //(GLfloat)zIndex(),
		target.left()     , target.top(), 	//(GLfloat)zIndex(),
		target.right() + 1, target.top()//, 	(GLfloat)zIndex()
	};


// 	QVector3D v1(target.left(),      target.bottom() + 1, 1);
// 	QVector3D v2(target.right() + 1, target.bottom() + 1, 1);
// 	QVector3D v3(target.left(),      target.top(), 1);
// 	QVector3D v4(target.right() + 1, target.top(), 1);
//
// 	if(applyRotTx)
// 	{
// 		v1 = rotTx.map(v1);
// 		v2 = rotTx.map(v2);
// 		v3 = rotTx.map(v3);
// 		v4 = rotTx.map(v4);
// 	}
//
//
// 	const GLfloat vertexCoordArray[] =
// 	{
// 		v1.x(), v1.y(), v1.z(),
// 		v2.x(), v2.y(), v2.z(),
// 		v3.x(), v3.y(), v3.z(),
// 		v4.x(), v4.y(), v4.z(),
// 	};

	//qDebug() << vTop << vBottom;
	//qDebug() << "m_frameSize:"<<m_frameSize;

	const GLfloat txLeft   = m_displayOpts.flipHorizontal ? source.right()  / m_frameSize.width() : source.left()  / m_frameSize.width();
	const GLfloat txRight  = m_displayOpts.flipHorizontal ? source.left()   / m_frameSize.width() : source.right() / m_frameSize.width();

	const GLfloat txTop    = !m_displayOpts.flipVertical //m_scanLineDirection == QVideoSurfaceFormat::TopToBottom
		? source.top()    / m_frameSize.height()
		: source.bottom() / m_frameSize.height();
	const GLfloat txBottom = !m_displayOpts.flipVertical //m_scanLineDirection == QVideoSurfaceFormat::TopToBottom
		? source.bottom() / m_frameSize.height()
		: source.top()    / m_frameSize.height();

	const GLfloat textureCoordArray[] =
	{
		txLeft , txBottom,
		txRight, txBottom,
		txLeft , txTop,
		txRight, txTop
	};
	
	//qDebug() << "GLVideoDrawable::paintGL():"<<(QObject*)this<<": source:"<<source<<", target:"<<target<<" ( crop:"<<m_displayOpts.cropTopLeft<<"/"<<m_displayOpts.cropBottomRight<<"), tx:"<<txRight<<txTop<<txLeft<<txBottom;

	double liveOpacity = m_crossFadeMode == JustFront ?
				 opacity() :
				(opacity() * (m_fadeActive ? m_fadeValue : 1.));

	//qDebug() << "GLVideoDrawable::paintGL(): "<<(QObject*)this<<" opac used:"<<liveOpacity<<", m_:"<<opacity()<<", fade act:"<<m_fadeActive<<", fade val:"<<m_fadeValue;

	//if(m_fadeActive)
//		qDebug() << "GLVideoDrawable::paintGL: "<<(QObject*)this<<m_source<<" liveOpacity: "<<liveOpacity;
// 	if(property("-debug").toBool())
// 		qDebug() << "m_useShaders:"<<m_useShaders;
	if(m_useShaders)
	{

		if(m_validShader)
		{
			m_program->bind();

			m_program->enableAttributeArray("vertexCoordArray");
			m_program->enableAttributeArray("textureCoordArray");

			m_program->setAttributeArray("vertexCoordArray",  vertexCoordArray,  2);
			m_program->setAttributeArray("textureCoordArray", textureCoordArray, 2);

			m_program->setUniformValue("positionMatrix",      positionMatrix);
		// 	QMatrix4x4 mat4(
		// 		positionMatrix[0][0], positionMatrix[0][1], positionMatrix[0][2], positionMatrix[0][3],
		// 		positionMatrix[1][0], positionMatrix[1][1], positionMatrix[1][2], positionMatrix[1][3],
		// 		positionMatrix[2][0], positionMatrix[2][1], positionMatrix[2][2], positionMatrix[2][3],
		// 		positionMatrix[3][0], positionMatrix[3][1], positionMatrix[3][2], positionMatrix[3][3]
		// 		);
		// 	m_program->setUniformValue("positionMatrix",      mat4);

	// 		if(property("-debug").toBool())
	// 			qDebug() << "GLVideoDrawable::paintGL():"<<(QObject*)this<<": rendering with opacity:"<<opacity();
		
			//qDebug() << "GLVideoDrawable::paintGL: "<<(QObject*)this<<m_source<<" painting with shaders, target:"<<target<<", tx:"<<txLeft<<txTop<<txRight<<txBottom; 
			
			m_program->setUniformValue("alpha",               (GLfloat)liveOpacity);
			m_program->setUniformValue("texOffsetX",          (GLfloat)m_invertedOffset.x());
			m_program->setUniformValue("texOffsetY",          (GLfloat)m_invertedOffset.y());
			
			m_program->setUniformValue("width",          (GLfloat)m_frameSize.width());
			m_program->setUniformValue("height",         (GLfloat)m_frameSize.height());

			if(m_levelsEnabled)
			{
//  				m_whiteLevel = 128;
//  				m_blackLevel = 32;
				int delta = m_whiteLevel - m_blackLevel;
				double w = delta == 0 ? 0 : 255. / ((double)delta);
				double b = m_blackLevel == 0 ? 0 : ((double)m_blackLevel) / 255.;
				//qDebug() << "GLVideoDrawable::paintGL():"<<(QObject*)this<<": levels:"<<m_blackLevel<<"/"<<m_whiteLevel<<", g:"<<m_gamma<<", w:"<<w<<",b:"<<b;
				m_program->setUniformValue("whiteLevel", (GLfloat)w);
				m_program->setUniformValue("blackLevel", (GLfloat)b);
				m_program->setUniformValue("gamma",      (GLfloat)m_gamma);
				
				/// TODO
				//m_program->setUniformValue("midLevel",   (GLfloat)midLevel);
				 
				//qDebug() << "GLVideoDrawable::paintGL():"<<(QObject*)this<<": levels:"<<m_blackLevel<<"/"<<m_whiteLevel<<", g:"<<m_gamma; 
			}
			
			if(m_filterType != Filter_None)
			{
// 				qDebug() << "GLVideoDrawable::paintGL(): "<<(QObject*)this<<"\t offsets:\n";
// 				for(int i=0;i<m_kernelSize*2;i+=2)
// 					qDebug() << "\t "<<m_convOffsets[i]<<" x "<<m_convOffsets[i+1];
// 					
// 				qDebug() << "GLVideoDrawable::paintGL(): "<<(QObject*)this<<"\t kernel:\n";
// 				int inc = m_kernelSize == 9 ? 3 : 5;
// 				for(int i=0;i<m_kernelSize;i+= inc)
// 					if(m_kernelSize == 9)
// 						qDebug() << "\t { " <<
// 							m_convKernel[i+0] << ", " << 
// 							m_convKernel[i+1] << ", " << 
// 							m_convKernel[i+2] <<
// 							" }";
// 					else
// 						qDebug() << "\t { " <<
// 							m_convKernel[i+0] << ", " << 
// 							m_convKernel[i+1] << ", " << 
// 							m_convKernel[i+2] << ", " << 
// 							m_convKernel[i+3] << ", " << 
// 							m_convKernel[i+4] <<  
// 							" }";
				m_program->setUniformValueArray("kernel", m_convKernel.data(),  m_kernelSize, 1);
    				m_program->setUniformValueArray("offset", m_convOffsets.data(), m_kernelSize, 2);
			}

			if (m_textureCount == 3)
			{
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, m_textureIds[0]);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, m_textureIds[1]);

				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, m_textureIds[2]);

				glActiveTexture(GL_TEXTURE3);
				glBindTexture(GL_TEXTURE_2D, m_alphaTextureId);

				glActiveTexture(GL_TEXTURE0);

				m_program->setUniformValue("texY", 0);
				m_program->setUniformValue("texU", 1);
				m_program->setUniformValue("texV", 2);
				m_program->setUniformValue("alphaMask", 3);
			}
			else 
			if (m_textureCount == 2) 
			{
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, m_textureIds[0]);
				
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, m_textureIds[1]);
				
				glActiveTexture(GL_TEXTURE0);
			
				m_program->setUniformValue("texY", 0);
				m_program->setUniformValue("texC", 1);
			}
			else
			{
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, m_textureIds[0]);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, m_alphaTextureId);

				glActiveTexture(GL_TEXTURE0);

				m_program->setUniformValue("texRgb", 0);
				m_program->setUniformValue("alphaMask", 1);
			}
			m_program->setUniformValue("colorMatrix", m_colorMatrix);

			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			m_program->release();
		}

	}
	else
	{
		if(0)
		{
			QImage texOrig, texGL;
			if ( !texOrig.load( "me2.jpg" ) )
			//if ( !texOrig.load( "Pm5544.jpg" ) )
			{
				texOrig = QImage( 16, 16, QImage::Format_RGB32 );
				texOrig.fill( Qt::green );
			}

			qDebug() << "Loaded test me2.jpg manually using glTexImage2D";
			// Setup inital texture
			texGL = QGLWidget::convertToGLFormat( texOrig );
			glGenTextures( 1, &m_textureIds[0] );
			glBindTexture( GL_TEXTURE_2D, m_textureIds[0] );
			glTexImage2D( GL_TEXTURE_2D, 0, 3, texGL.width(), texGL.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, texGL.bits() );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		}

		glColor4f(liveOpacity,liveOpacity,liveOpacity,liveOpacity);

		glEnable(GL_TEXTURE_2D);

		glActiveTexture(GL_TEXTURE0);

	// 	glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
	// 	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLoadIdentity(); // Reset The View
		//glTranslatef(0.0f,0.0f,-3.42f);

		glBindTexture(GL_TEXTURE_2D, m_textureIds[0]);

		QPolygonF points = transform.map(QPolygonF(target));
 		//qDebug() << "target: "<<target;
 		//qDebug() << "texture: "<<txLeft<<txBottom<<txTop<<txRight;
		glBegin(GL_QUADS);
// 			qreal
// 				vx1 = target.left(),
// 				vx2 = target.right(),
// 				vy1 = target.bottom(),
// 				vy2 = target.top();

	// 		const GLfloat txLeft   = m_displayOpts.flipHorizontal ? source.right()  / m_frameSize.width() : source.left()  / m_frameSize.width();
	// 		const GLfloat txRight  = m_displayOpts.flipHorizontal ? source.left()   / m_frameSize.width() : source.right() / m_frameSize.width();
	//
	// 		const GLfloat txTop    = !m_displayOpts.flipVertical //m_scanLineDirection == QVideoSurfaceFormat::TopToBottom
	// 			? source.top()    / m_frameSize.height()
	// 			: source.bottom() / m_frameSize.height();
	// 		const GLfloat txBottom = !m_displayOpts.flipVertical //m_scanLineDirection == QVideoSurfaceFormat::TopToBottom
	// 			? source.bottom() / m_frameSize.height()
	// 			: source.top()    / m_frameSize.height();

			glTexCoord2f(txLeft, txBottom); 	glVertex3f(points[2].x(),points[2].y(),  0.0f); // top left
			glTexCoord2f(txRight, txBottom); 	glVertex3f(points[1].x(),points[1].y(),  0.0f); // top right
			glTexCoord2f(txRight, txTop); 		glVertex3f(points[0].x(),points[0].y(),  0.0f); // bottom right
			glTexCoord2f(txLeft, txTop); 		glVertex3f(points[3].x(),points[3].y(),  0.0f); // bottom left

				/*
			glTexCoord2f(0.0f, 0.0f); glVertex3f(vx1,vy1,  0.0f); // bottom left 2
			glTexCoord2f(1.0f, 0.0f); glVertex3f(vx2,vy1,  0.0f); // bottom right 1
			glTexCoord2f(1.0f, 1.0f); glVertex3f(vx2,vy2,  0.0f); // top right 0
			glTexCoord2f(0.0f, 1.0f); glVertex3f(vx1,vy2,  0.0f); // top left 3
	*/
	// 		glTexCoord2f(0,0); glVertex3f( 0, 0,0); //lo
	// 		glTexCoord2f(0,1); glVertex3f(256, 0,0); //lu
	// 		glTexCoord2f(1,1); glVertex3f(256, 256,0); //ru
	// 		glTexCoord2f(1,0); glVertex3f( 0, 256,0); //ro
		glEnd();
	}

 	if(m_fadeActive)
 	{
 		double fadeOpacity = opacity() * (1.0-m_fadeValue);
//		if(fadeOpacity < 0.001)
//			fadeOpacity = 0;

//		qDebug() << "GLVideoDrawable::paintGL: [source2] fadeOpacity: "<<fadeOpacity;

		QRectF source2 = m_sourceRect2;
		QRectF target2 = m_targetRect2;

		source2 = source2.adjusted(
			m_displayOpts.cropTopLeft.x(),
			m_displayOpts.cropTopLeft.y(),
			m_displayOpts.cropBottomRight.x(),
			m_displayOpts.cropBottomRight.y());


		//QPainter painter(this);
		QTransform transform =  m_glw->transform(); //= painter.deviceTransform();
		//transform = transform.scale(1.25,1.);
		if(!translation().isNull())
			transform *= QTransform().translate(translation().x(),translation().y());

		if(!rotation().isNull())
		{
			qreal tx = target2.width()  * rotationPoint().x() + target2.x();
			qreal ty = target2.height() * rotationPoint().y() + target2.y();
			qreal x, y;
			transform.map(tx,ty,&x,&y);

			QVector3D rot = rotation();
			transform *= QTransform()
				.translate(x,y)
				.rotate(rot.x(),Qt::XAxis)
				.rotate(rot.y(),Qt::YAxis)
				.rotate(rot.z(),Qt::ZAxis)
				.translate(-x,-y);
		}
		const GLfloat positionMatrix[4][4] =
		{
			{
				/*(0,0)*/ wfactor * transform.m11() - transform.m13(),
				/*(0,1)*/ hfactor * transform.m12() + transform.m13(),
				/*(0,2)*/ 0.0,
				/*(0,3)*/ transform.m13()
			}, {
				/*(1,0)*/ wfactor * transform.m21() - transform.m23(),
				/*(1,1)*/ hfactor * transform.m22() + transform.m23(),
				/*(1,2)*/ 0.0,
				/*(1,3)*/ transform.m23()
			}, {
				/*(2,0)*/ 0.0,
				/*(2,1)*/ 0.0,
				/*(2,2)*/ -1.0,
				/*(2,3)*/ 0.0
			}, {
				/*(3,0)*/ wfactor * transform.dx() - transform.m33(),
				/*(3,1)*/ hfactor * transform.dy() + transform.m33(),
				/*(3,2)*/ 0.0,
				/*(3,3)*/ transform.m33()
			}
		};


		//QVector3D list[] =

		const GLfloat vertexCoordArray[] =
		{
			target2.left()     , target2.bottom() + 1, //(GLfloat)zIndex(),
			target2.right() + 1, target2.bottom() + 1, //(GLfloat)zIndex(),
			target2.left()     , target2.top(), 	//(GLfloat)zIndex(),
			target2.right() + 1, target2.top()//, 	(GLfloat)zIndex()
		};

		const GLfloat txLeft   = m_displayOpts.flipHorizontal ? source2.right()  / m_frameSize2.width() : source2.left()  / m_frameSize2.width();
		const GLfloat txRight  = m_displayOpts.flipHorizontal ? source2.left()   / m_frameSize2.width() : source2.right() / m_frameSize2.width();

		const GLfloat txTop    = !m_displayOpts.flipVertical //m_scanLineDirection == QVideoSurfaceFormat::TopToBottom
			? source2.top()    / m_frameSize2.height()
			: source2.bottom() / m_frameSize2.height();
		const GLfloat txBottom = !m_displayOpts.flipVertical //m_scanLineDirection == QVideoSurfaceFormat::TopToBottom
			? source2.bottom() / m_frameSize2.height()
			: source2.top()    / m_frameSize2.height();

		const GLfloat textureCoordArray[] =
		{
			txLeft , txBottom,
			txRight, txBottom,
			txLeft , txTop,
			txRight, txTop
		};

		if(m_useShaders)
		{

			if(m_validShader2)
			{
				m_program2->bind();

				m_program2->enableAttributeArray("vertexCoordArray");
				m_program2->enableAttributeArray("textureCoordArray");

				m_program2->setAttributeArray("vertexCoordArray",  vertexCoordArray,  2);
				m_program2->setAttributeArray("textureCoordArray", textureCoordArray, 2);

				m_program2->setUniformValue("positionMatrix",      positionMatrix);
			// 	QMatrix4x4 mat4(
			// 		positionMatrix[0][0], positionMatrix[0][1], positionMatrix[0][2], positionMatrix[0][3],
			// 		positionMatrix[1][0], positionMatrix[1][1], positionMatrix[1][2], positionMatrix[1][3],
			// 		positionMatrix[2][0], positionMatrix[2][1], positionMatrix[2][2], positionMatrix[2][3],
			// 		positionMatrix[3][0], positionMatrix[3][1], positionMatrix[3][2], positionMatrix[3][3]
			// 		);
			// 	m_program->setUniformValue("positionMatrix",      mat4);

				//qDebug() << "GLVideoDrawable::paintGL():"<<this<<", rendering with opacity:"<<opacity();
				m_program2->setUniformValue("alpha",               (GLfloat)fadeOpacity);
				m_program2->setUniformValue("texOffsetX",          (GLfloat)m_invertedOffset.x());
				m_program2->setUniformValue("texOffsetY",          (GLfloat)m_invertedOffset.y());


				if (m_textureCount2 == 3)
				{
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, m_textureIds2[0]);

					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, m_textureIds2[1]);

					glActiveTexture(GL_TEXTURE2);
					glBindTexture(GL_TEXTURE_2D, m_textureIds2[2]);

					glActiveTexture(GL_TEXTURE3);
					glBindTexture(GL_TEXTURE_2D, m_alphaTextureId);

					glActiveTexture(GL_TEXTURE0);

					m_program->setUniformValue("texY", 0);
					m_program->setUniformValue("texU", 1);
					m_program->setUniformValue("texV", 2);
					m_program->setUniformValue("alphaMask", 3);
				}
				else
				{
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, m_textureIds2[0]);

					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, m_alphaTextureId);

					glActiveTexture(GL_TEXTURE0);

					m_program->setUniformValue("texRgb", 0);
					m_program->setUniformValue("alphaMask", 1);
				}
				m_program->setUniformValue("colorMatrix", m_colorMatrix);

	//			if(fadeOpacity > 0.001)
					glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

				m_program->release();
			}

		}
		else
		{
			if(0)
			{
				QImage texOrig, texGL;
				if ( !texOrig.load( "me2.jpg" ) )
				//if ( !texOrig.load( "Pm5544.jpg" ) )
				{
					texOrig = QImage( 16, 16, QImage::Format_RGB32 );
					texOrig.fill( Qt::green );
				}

				qDebug() << "Loaded test me2.jpg manually using glTexImage2D";
				// Setup inital texture
				texGL = QGLWidget::convertToGLFormat( texOrig );
				glGenTextures( 1, &m_textureIds2[0] );
				glBindTexture( GL_TEXTURE_2D, m_textureIds2[0] );
				glTexImage2D( GL_TEXTURE_2D, 0, 3, texGL.width(), texGL.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, texGL.bits() );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			}

			glColor4f(fadeOpacity,fadeOpacity,fadeOpacity,fadeOpacity);

			glEnable(GL_TEXTURE_2D);

			glActiveTexture(GL_TEXTURE0);

		// 	glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
		// 	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glLoadIdentity(); // Reset The View
			//glTranslatef(0.0f,0.0f,-3.42f);

			glBindTexture(GL_TEXTURE_2D, m_textureIds2[0]);

			target2 = transform.mapRect(target2);
			//qDebug() << "target: "<<target;
			//qDebug() << "texture: "<<txLeft<<txBottom<<txTop<<txRight;
			glBegin(GL_QUADS);
				qreal
					vx1 = target2.left(),
					vx2 = target2.right(),
					vy1 = target2.bottom(),
					vy2 = target2.top();

		// 		const GLfloat txLeft   = m_displayOpts.flipHorizontal ? source.right()  / m_frameSize.width() : source.left()  / m_frameSize.width();
		// 		const GLfloat txRight  = m_displayOpts.flipHorizontal ? source.left()   / m_frameSize.width() : source.right() / m_frameSize.width();
		//
		// 		const GLfloat txTop    = !m_displayOpts.flipVertical //m_scanLineDirection == QVideoSurfaceFormat::TopToBottom
		// 			? source.top()    / m_frameSize.height()
		// 			: source.bottom() / m_frameSize.height();
		// 		const GLfloat txBottom = !m_displayOpts.flipVertical //m_scanLineDirection == QVideoSurfaceFormat::TopToBottom
		// 			? source.bottom() / m_frameSize.height()
		// 			: source.top()    / m_frameSize.height();

				glTexCoord2f(txLeft, txBottom); 	glVertex3f(vx1,vy1,  0.0f); // top left
				glTexCoord2f(txRight, txBottom); 	glVertex3f(vx2,vy1,  0.0f); // top right
				glTexCoord2f(txRight, txTop); 		glVertex3f(vx2,vy2,  0.0f); // bottom right
				glTexCoord2f(txLeft, txTop); 		glVertex3f(vx1,vy2,  0.0f); // bottom left

		// 		glTexCoord2f(0,0); glVertex3f( 0, 0,0); //lo
		// 		glTexCoord2f(0,1); glVertex3f(256, 0,0); //lu
		// 		glTexCoord2f(1,1); glVertex3f(256, 256,0); //ru
		// 		glTexCoord2f(1,0); glVertex3f( 0, 256,0); //ro
			glEnd();
		}
 	}


	//renderText(10, 10, qPrintable(QString("%1 fps").arg(framesPerSecond)));

	if(m_frame && !m_frame->captureTime().isNull())
	{
		int msecLatency = m_frame->captureTime().msecsTo(QTime::currentTime());

		m_latencyAccum += msecLatency;
	}

	if (!(m_frameCount % 100))
	{
		m_avgFps = m_frameCount /(m_time.elapsed() / 1000.0);
		
		QString framesPerSecond;
		framesPerSecond.setNum(m_avgFps, 'f', 2);

		QString latencyPerFrame;
		latencyPerFrame.setNum((((double)m_latencyAccum) / ((double)m_frameCount)), 'f', 3);

		if(m_debugFps && framesPerSecond!="0.00")
			qDebug() << "GLVideoDrawable::paintGL: "<<(QObject*)this<<" FPS: " << qPrintable(framesPerSecond) << (!m_frame || m_frame->captureTime().isNull() ? "" : qPrintable(QString(", Latency: %1 ms").arg(latencyPerFrame)));
			
		/*
		if(m_isCameraThread && m_frame)// && !m_frame->isRaw() && !m_frame->image().isNull())
		{
			//CameraThread *source = dynamic_cast<CameraThread*>(m_source);
			QImage image;
			if(m_frame->isRaw())
			{
				const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(m_frame->pixelFormat());
				if(imageFormat != QImage::Format_Invalid)
				{//517-617-2907
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
				}
			}
			else
			{
				image = m_frame->image();
			}
			
			image = image.scaled(QSize(640,480), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
				
			QString name = QString().sprintf("debug/cam-%p.png",(void*)m_source);
			qDebug() << "Writing to "<<name;
			image.save(name);
		}
		*/


		m_time.start();
		m_frameCount = 0;
		m_latencyAccum = 0;
		
		//qDebug() << "GLVideoDrawable::paintGL(): "<<(QObject*)this<<" 100 frames, calling electUpdateLeader";
		electUpdateLeader();

		//lastFrameTime = time.elapsed();
	}

	m_frameCount ++;
	
	paintGLChildren(false); // paint children above

// 	qDebug() << "GLVideoDrawable::paintGL(): "<<(QObject*)this<<" Mark - end";
}

void GLVideoDrawable::transformChanged()
{	
	// NOOP in this class - this is a hook for subclasses
}

void GLVideoDrawable::setCrossFadeMode(CrossFadeMode mode)
{
	m_crossFadeMode = mode;
}

void GLVideoDrawable::setLevelsEnabled(bool flag)
{
	m_levelsEnabled = flag;
	
	// force reload shaders
	setVideoFormat(m_videoFormat);
	
	updateGL();
}

void GLVideoDrawable::setBlackLevel(int x)
{
	#ifdef RECOMPILE_SHADERS_TO_INCLUDE_LEVELS
	int old = m_blackLevel;
	#endif
	m_blackLevel = x;
	// recompile shaders to include levels adjustments if m_levelsEnabled
	#ifdef RECOMPILE_SHADERS_TO_INCLUDE_LEVELS
	if((old <= 0 && x >  0) ||
	   (old >  0 && x <= 0))
		setVideoFormat(m_videoFormat);
	#endif
	updateGL();
}

void GLVideoDrawable::setWhiteLevel(int x)
{
	#ifdef RECOMPILE_SHADERS_TO_INCLUDE_LEVELS
	int old = m_whiteLevel;
	#endif
	m_whiteLevel = x;
	// recompile shaders to include levels adjustments if m_levelsEnabled
	#ifdef RECOMPILE_SHADERS_TO_INCLUDE_LEVELS
	if((old >= 255 && x <  255) ||
	   (old <  255 && x >= 255))
		setVideoFormat(m_videoFormat);
	#endif
	updateGL();
}

void GLVideoDrawable::setMidLevel(int x)
{
	m_midLevel = x;
	updateGL();
}

void GLVideoDrawable::setGamma(double g)
{
	m_gamma = g;
	updateGL();
}

void GLVideoDrawable::setFilterType(FilterType filterType)
{
	if(m_filterType == filterType)
		return;
		
	m_filterType = filterType;
	setVideoFormat(m_videoFormat);
	updateGL();
}
 
void GLVideoDrawable::setSharpAmount(double value)
{
	//qDebug() << "GLVideoDrawable::setSharpAmount: "<<value;
	
	m_sharpAmount = value;
	//setVideoFormat(m_videoFormat);
	if(m_filterType == Filter_Sharp)
	{
		double sharpAmount = m_sharpAmount;
		//sharpAmount = 1.25;
		if(m_kernelSize == 9)
		{
			QList<float> kernel;
			
			if(sharpAmount <= 1.0)
				kernel	<<  0
					<< -1 * sharpAmount
					<<  0
					
					<< -1 * sharpAmount
					<< (1 * sharpAmount * 4 + 1)
					<< -1 * sharpAmount
					
					<<  0
					<< -1 * sharpAmount
					<<  0;
			else
				kernel  << -1 * sharpAmount
					<< -1 * sharpAmount
					<< -1 * sharpAmount
					
					<< -1 * sharpAmount
					<< (1 * sharpAmount * 8 + 1)
					<< -1 * sharpAmount
					
					<< -1 * sharpAmount
					<< -1 * sharpAmount
					<< -1 * sharpAmount;
			 
			m_convKernel.resize(m_kernelSize);
			for(int i=0;i<m_kernelSize;i++)
				m_convKernel[i] = i < kernel.size() ? kernel[i] : 0;
		}
	}
	
	if(m_isCameraThread)
	{
		if(m_updateLeader == this)
			updateGL(true);
	}
	else
	{
		updateGL();
	}
}

void GLVideoDrawable::setBlurAmount(double value)
{
	m_blurAmount = value;
	setVideoFormat(m_videoFormat);
	updateGL();
}

void GLVideoDrawable::setKernelSize(int size)
{
	m_kernelSize = size;
	setVideoFormat(m_videoFormat);
	updateGL();
}

void GLVideoDrawable::setCustomKernel(QVariantList list) 
{
	m_customKernel = list;
	setVideoFormat(m_videoFormat);
	updateGL();
}

	
VideoDisplayOptionWidget::VideoDisplayOptionWidget(GLVideoDrawable *drawable, QWidget *parent)
	: QWidget(parent)
	, m_drawable(drawable)
{
	if(drawable)
	{
		m_opts = drawable->displayOptions();
		connect(this, SIGNAL(displayOptionsChanged(const VideoDisplayOptions&)), drawable, SLOT(setDisplayOptions(const VideoDisplayOptions&)));
		connect(drawable, SIGNAL(displayOptionsChanged(const VideoDisplayOptions&)), this, SLOT(setDisplayOptions(const VideoDisplayOptions&)));
	}

	initUI();
}

VideoDisplayOptionWidget::VideoDisplayOptionWidget(const VideoDisplayOptions& opts, QWidget *parent)
	: QWidget(parent)
{
	m_opts = opts;
	initUI();
}


void VideoDisplayOptionWidget::initUI()
{
	QString name(m_drawable ? m_drawable->objectName() : "");
	setWindowTitle(QString("%1Display Options").arg(name.isEmpty() ? "" : name + " - "));

	QGridLayout *layout = new QGridLayout(this);
	m_optsOriginal = m_opts;

	int row = 0;

	QCheckBox *cb = 0;
	cb = new QCheckBox("Flip Horizontal");
	cb->setChecked(m_opts.flipHorizontal);
	connect(cb, SIGNAL(toggled(bool)), this, SLOT(flipHChanged(bool)));
	layout->addWidget(cb,row,1);
	m_cbFlipH = cb;

	row++;
	cb = new QCheckBox("Flip Vertical");
	cb->setChecked(m_opts.flipVertical);
	connect(cb, SIGNAL(toggled(bool)), this, SLOT(flipVChanged(bool)));
	layout->addWidget(cb,row,1);
	m_cbFlipV = cb;

	row++;
	QSpinBox *spinBox = 0;
	QHBoxLayout *rowLayout = new QHBoxLayout();
	layout->addWidget(new QLabel("Crop Left By:"),row,0);
	spinBox = new QSpinBox;
	spinBox->setSuffix(" px");
	spinBox->setMinimum(-1000);
	spinBox->setMaximum(1000);
	spinBox->setValue((int)m_opts.cropTopLeft.x());
	connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(cropX1Changed(int)));
	rowLayout->addWidget(spinBox);
	rowLayout->addStretch(1);
	layout->addLayout(rowLayout,row,1);
	m_spinCropX1 = spinBox;

	row++;
	rowLayout = new QHBoxLayout();
	layout->addWidget(new QLabel("Crop Right By:"),row,0);
	spinBox = new QSpinBox;
	spinBox->setSuffix(" px");
	spinBox->setMinimum(-1000);
	spinBox->setMaximum(1000);
	spinBox->setValue((int)m_opts.cropBottomRight.x());
	connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(cropX2Changed(int)));
	rowLayout->addWidget(spinBox);
	rowLayout->addStretch(1);
	layout->addLayout(rowLayout,row,1);
	m_spinCropX2 = spinBox;

	row++;
	rowLayout = new QHBoxLayout();
	layout->addWidget(new QLabel("Crop Top By:"),row,0);
	spinBox = new QSpinBox;
	spinBox->setSuffix(" px");
	spinBox->setMinimum(-1000);
	spinBox->setMaximum(1000);
	spinBox->setValue((int)m_opts.cropTopLeft.y());
	connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(cropY1Changed(int)));
	rowLayout->addWidget(spinBox);
	rowLayout->addStretch(1);
	layout->addLayout(rowLayout,row,1);
	m_spinCropY1 = spinBox;

	row++;
	rowLayout = new QHBoxLayout();
	layout->addWidget(new QLabel("Crop Bottom By:"),row,0);
	spinBox = new QSpinBox;
	spinBox->setSuffix(" px");
	spinBox->setMinimum(-1000);
	spinBox->setMaximum(1000);
	spinBox->setValue((int)m_opts.cropBottomRight.y());
	connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(cropY2Changed(int)));
	rowLayout->addWidget(spinBox);
	rowLayout->addStretch(1);
	layout->addLayout(rowLayout,row,1);
	m_spinCropY2 = spinBox;

	row++;
	QSlider *slider =0;
	rowLayout = new QHBoxLayout();
	// add label
	layout->addWidget(new QLabel("Brightness:"),row,0);
	// add slider
	slider = new QSlider;
	slider->setOrientation(Qt::Horizontal);
	slider->setMinimum(-100);
	slider->setMaximum(100);
	slider->setValue(m_opts.brightness);
	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(bChanged(int)));
	rowLayout->addWidget(slider);
	// add spinBox
	spinBox = new QSpinBox;
	spinBox->setMinimum(-100);
	spinBox->setMaximum(100);
	spinBox->setValue(m_opts.brightness);
	connect(spinBox, SIGNAL(valueChanged(int)), slider, SLOT(setValue(int)));
	connect(slider, SIGNAL(valueChanged(int)), spinBox, SLOT(setValue(int)));
	rowLayout->addWidget(spinBox);
	// finish the row
	rowLayout->addStretch(1);
	layout->addLayout(rowLayout,row,1);
	m_spinB = spinBox;

	row++;
	rowLayout = new QHBoxLayout();
	layout->addWidget(new QLabel("Contrast:"),row,0);
	slider = new QSlider;
	slider->setOrientation(Qt::Horizontal);
	slider->setMinimum(-100);
	slider->setMaximum(100);
	slider->setValue(m_opts.contrast);
	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(cChanged(int)));
	rowLayout->addWidget(slider);
	// add spinBox
	spinBox = new QSpinBox;
	spinBox->setMinimum(-100);
	spinBox->setMaximum(100);
	spinBox->setValue(m_opts.brightness);
	connect(spinBox, SIGNAL(valueChanged(int)), slider, SLOT(setValue(int)));
	connect(slider, SIGNAL(valueChanged(int)), spinBox, SLOT(setValue(int)));
	rowLayout->addWidget(spinBox);
	// finish the row
	rowLayout->addStretch(1);
	layout->addLayout(rowLayout,row,1);
	m_spinC = spinBox;

	row++;
	rowLayout = new QHBoxLayout();
	layout->addWidget(new QLabel("Hue:"),row,0);
	slider = new QSlider;
	slider->setOrientation(Qt::Horizontal);
	slider->setMinimum(-100);
	slider->setMaximum(100);
	slider->setValue(m_opts.hue);
	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(hChanged(int)));
	rowLayout->addWidget(slider);
	// add spinBox
	spinBox = new QSpinBox;
	spinBox->setMinimum(-100);
	spinBox->setMaximum(100);
	spinBox->setValue(m_opts.brightness);
	connect(spinBox, SIGNAL(valueChanged(int)), slider, SLOT(setValue(int)));
	connect(slider, SIGNAL(valueChanged(int)), spinBox, SLOT(setValue(int)));
	rowLayout->addWidget(spinBox);
	// finish the row
	rowLayout->addStretch(1);
	layout->addLayout(rowLayout,row,1);
	m_spinH = spinBox;

	row++;
	rowLayout = new QHBoxLayout();
	layout->addWidget(new QLabel("Saturation:"),row,0);
	slider = new QSlider;
	slider->setOrientation(Qt::Horizontal);
	slider->setMinimum(-100);
	slider->setMaximum(100);
	slider->setValue(m_opts.saturation);
	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(sChanged(int)));
	rowLayout->addWidget(slider);
	// add spinBox
	spinBox = new QSpinBox;
	spinBox->setMinimum(-100);
	spinBox->setMaximum(100);
	spinBox->setValue(m_opts.brightness);
	connect(spinBox, SIGNAL(valueChanged(int)), slider, SLOT(setValue(int)));
	connect(slider, SIGNAL(valueChanged(int)), spinBox, SLOT(setValue(int)));
	rowLayout->addWidget(spinBox);
	// finish the row
	rowLayout->addStretch(1);
	layout->addLayout(rowLayout,row,1);
	m_spinS = spinBox;

	row++;
	QPushButton *resetButton = new QPushButton("Undo Changes");
	connect(resetButton, SIGNAL(clicked()), this, SLOT(undoChanges()));
	layout->addWidget(resetButton,row,1);
}

void VideoDisplayOptionWidget::undoChanges()
{
	setDisplayOptions(m_optsOriginal);
}

void VideoDisplayOptionWidget::setDisplayOptions(const VideoDisplayOptions& opts)
{
	m_opts = opts;
	m_cbFlipH->setChecked(opts.flipHorizontal);
	m_cbFlipV->setChecked(opts.flipVertical);
	m_spinCropX1->setValue((int)opts.cropTopLeft.x());
	m_spinCropY1->setValue((int)opts.cropTopLeft.y());
	m_spinCropX2->setValue((int)opts.cropBottomRight.x());
	m_spinCropY2->setValue((int)opts.cropBottomRight.y());
	m_spinB->setValue(opts.brightness);
	m_spinC->setValue(opts.contrast);
	m_spinH->setValue(opts.hue);
	m_spinS->setValue(opts.saturation);

}

void VideoDisplayOptionWidget::flipHChanged(bool value)
{
	m_opts.flipHorizontal = value;
	emit displayOptionsChanged(m_opts);
}

void VideoDisplayOptionWidget::flipVChanged(bool value)
{
	m_opts.flipVertical = value;
	emit displayOptionsChanged(m_opts);
}


void VideoDisplayOptionWidget::cropX1Changed(int value)
{
	m_opts.cropTopLeft.setX(value);
	emit displayOptionsChanged(m_opts);
}


void VideoDisplayOptionWidget::cropY1Changed(int value)
{
	m_opts.cropTopLeft.setY(value);
	emit displayOptionsChanged(m_opts);
}


void VideoDisplayOptionWidget::cropX2Changed(int value)
{
	m_opts.cropBottomRight.setX(value);
	emit displayOptionsChanged(m_opts);
}


void VideoDisplayOptionWidget::cropY2Changed(int value)
{
	m_opts.cropBottomRight.setY(value);
	emit displayOptionsChanged(m_opts);
}


void VideoDisplayOptionWidget::bChanged(int value)
{
	//qDebug() << "b changed: "<<value;
	m_opts.brightness = value;
	emit displayOptionsChanged(m_opts);
}


void VideoDisplayOptionWidget::cChanged(int value)
{
	m_opts.contrast = value;
	emit displayOptionsChanged(m_opts);
}


void VideoDisplayOptionWidget::hChanged(int value)
{
	m_opts.hue = value;
	emit displayOptionsChanged(m_opts);
}


void VideoDisplayOptionWidget::sChanged(int value)
{
	m_opts.saturation = value;
	emit displayOptionsChanged(m_opts);
}


