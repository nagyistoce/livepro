#include "BMDOutput.h"

#include "../livemix/VideoSource.h"
	

#ifdef ENABLE_DECKLINK_CAPTURE


#include "DeckLinkAPI.h"

/// \class BMDOutputDelegate
///	Provides integration with Blackmagic DeckLink-compatible device (such as the Intensity Prso) for
///	outputing {VideoFrame}s.

class BMDOutputDelegate : public IDeckLinkVideoOutputCallback
{
private:
	BMDOutputDelegate(IDeckLink*);
	
public:
	virtual ~BMDOutputDelegate() {}
	
	// *** DeckLink API implementation of IDeckLinkVideoOutputCallback *** //
	
	// IUnknown needs only a dummy implementation
	virtual HRESULT STDMETHODCALLTYPE	QueryInterface (REFIID /*iid*/, LPVOID */*ppv*/)	{return E_NOINTERFACE;}
	virtual ULONG STDMETHODCALLTYPE		AddRef ()	{return 1;}
	virtual ULONG STDMETHODCALLTYPE		Release ()	{return 1;}
	
	virtual HRESULT STDMETHODCALLTYPE	ScheduledFrameCompleted (IDeckLinkVideoFrame* completedFrame, BMDOutputFrameCompletionResult result);
	virtual HRESULT STDMETHODCALLTYPE	ScheduledPlaybackHasStopped ();
	
	static QStringList enumDeviceNames(bool forceReload=false);
	static BMDOutputDelegate *openDevice(QString deviceName);
	
	void SetCurrentFrame(VideoFramePtr);
	void StopRunning();

private:	
	void ScheduleNextFrame(bool preroll);
	void StartRunning();
	void PrepareFrame();
	IDeckLinkDisplayMode *GetDisplayModeByIndex(int selectedIndex);
	
	static QStringList	s_knownDevices;
	
	IDeckLink *		m_deckLink;
	IDeckLinkOutput *	   m_deckLinkOutput;
	IDeckLinkVideoConversion * m_deckLinkConverter;
	
// 	AVFrame *allocPicture(enum PixelFormat pix_fmt, int width, int height);
// 	struct SwsContext *	m_swsContext;
// 	QSize 			m_swsInitSize;
// 	AVFrame *		m_yuvPicture;
// 	AVFrame *		m_rgbPicture;

	IDeckLinkMutableVideoFrame *m_rgbFrame;
	IDeckLinkMutableVideoFrame *m_yuvFrame;
	
	VideoFramePtr		m_frame;
	
	bool 			m_running;
	
	unsigned long		m_totalFramesScheduled;
	
	unsigned long		m_frameWidth;
	unsigned long		m_frameHeight;
	BMDTimeValue		m_frameDuration;
	BMDTimeScale		m_frameTimescale;
	unsigned long		m_framesPerSecond;
	
	QImage			m_image;
	
	bool			m_frameSet;
	int			m_frameReceivedCount;
	
};

QStringList BMDOutputDelegate::s_knownDevices = QStringList();


HRESULT BMDOutputDelegate::ScheduledFrameCompleted (IDeckLinkVideoFrame* /*completedFrame*/, BMDOutputFrameCompletionResult /*result*/)
{
	// When a video frame has been released by the API, schedule another video frame to be output
	m_frameSet = false;
	ScheduleNextFrame(false);
	return S_OK;
}

HRESULT BMDOutputDelegate::ScheduledPlaybackHasStopped ()
{
	return S_OK;
}

QStringList BMDOutputDelegate::enumDeviceNames(bool forceReload)
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
	IDeckLinkInput	*deckLinkOutput;
	
	int index = 0;
	
	// Enumerate all cards in this system
	while (deckLinkIterator->Next(&deckLink) == S_OK)
	{
		if (deckLink->QueryInterface(IID_IDeckLinkOutput, (void**)&deckLinkOutput) == S_OK)
		{
			s_knownDevices << QString("bmd:%1").arg(index);
			
			deckLinkOutput->Release();
			deckLinkOutput = NULL;
		}
		
		
		index ++;
		
		// Release the IDeckLink instance when we've finished with it to prevent leaks
		deckLink->Release();
	}
	
	deckLinkIterator->Release();
	
	return s_knownDevices;
}


BMDOutputDelegate *BMDOutputDelegate::openDevice(QString deviceName)
{
	if(enumDeviceNames().indexOf(deviceName) < 0)
		return 0;
		
	QString name = deviceName.replace("bmd:","");
	int cardNum = name.toInt();
	
	IDeckLinkIterator *deckLinkIterator = CreateDeckLinkIteratorInstance();
	if (deckLinkIterator == NULL)
	{
		qDebug() << "BMDOutputDelegate::openDevice: A DeckLink iterator could not be created.  The DeckLink drivers may not be installed.";
		return 0;
	}
		
	
	int index = 0;
	
	IDeckLink* deckLink;
	
	// Find the card requested
	while (deckLinkIterator->Next(&deckLink) == S_OK)
	{
		if(index == cardNum)
			return new BMDOutputDelegate(deckLink);
		
		index ++;
		
		// Release the IDeckLink instance when we've finished with it to prevent leaks
		deckLink->Release();
	}
	
	deckLinkIterator->Release();
		
	return 0;
}

BMDOutputDelegate::BMDOutputDelegate(IDeckLink* link)
	: m_deckLink(link)
	, m_deckLinkConverter(0)
	, m_rgbFrame(0)
	, m_yuvFrame(0)
	, m_running(false)
	, m_frameSet(false)
	, m_frameReceivedCount(0)
{
	// Obtain the audio/video output interface (IDeckLinkOutput)
	if (m_deckLink->QueryInterface(IID_IDeckLinkOutput, (void**)&m_deckLinkOutput) != S_OK)
		goto bail;
		
	#ifdef Q_OS_WIN
	if(CoCreateInstance(CLSID_CDeckLinkVideoConversion, NULL, CLSCTX_ALL,IID_IDeckLinkVideoConversion, (void**)&m_deckLinkConverter) != S_OK)
	#else
	if(!(m_deckLinkConverter = CreateVideoConversionInstance()))
	#endif
 	{
 		qDebug() << "BMDCaptureDelegate: Cannot create an instance of IID_IDeckLinkVideoConversion, therefore cannot convert YUV->RGB, therefore we will not emit proper RGB frames now.";
 		m_deckLinkConverter = NULL;
 		goto bail;
 	}

	// Provide this class as a delegate to the audio and video output interfaces
	m_deckLinkOutput->SetScheduledFrameCompletionCallback(this);
	
	// Start.
	StartRunning();
	
	return;
		
bail:
	if (m_running == true)
	{
		StopRunning();
	}
	else
	{
		// Release any resources that were partially allocated
		if (m_deckLinkOutput != NULL)
		{
			m_deckLinkOutput->Release();
			m_deckLinkOutput = NULL;
		}
		//
		if (m_deckLink != NULL)
		{
			m_deckLink->Release();
			m_deckLink = NULL;
		}
	}
	
	return ;
	
};


IDeckLinkDisplayMode *BMDOutputDelegate::GetDisplayModeByIndex(int selectedIndex)
{
	// Populate the display mode combo with a list of display modes supported by the installed DeckLink card
	IDeckLinkDisplayModeIterator*		displayModeIterator;
	IDeckLinkDisplayMode*			deckLinkDisplayMode;
	IDeckLinkDisplayMode*			selectedMode = NULL;
	int index = 0;

	if (m_deckLinkOutput->GetDisplayModeIterator(&displayModeIterator) != S_OK)
		goto bail;
	while (displayModeIterator->Next(&deckLinkDisplayMode) == S_OK)
	{
		const char *modeName;
	
		if (deckLinkDisplayMode->GetName(&modeName) == S_OK)
		{
				if (index == selectedIndex)
				{
					printf("Selected mode: %s\n", modeName);
					selectedMode = deckLinkDisplayMode;
					goto bail;
				}
		}
		index++;
	}
bail:
	displayModeIterator->Release();
	return selectedMode; 
}

void BMDOutputDelegate::StartRunning ()
{
	IDeckLinkDisplayMode*	videoDisplayMode = NULL;
	
	// Get the display mode for 1080i 59.95 - mode 6
	// Changed to NTSC 23.98 - JB 20110215
	videoDisplayMode = GetDisplayModeByIndex(1);

	if (!videoDisplayMode)
		return;

	m_frameWidth = videoDisplayMode->GetWidth();
	m_frameHeight = videoDisplayMode->GetHeight();
	videoDisplayMode->GetFrameRate(&m_frameDuration, &m_frameTimescale);
	
	// Calculate the number of frames per second, rounded up to the nearest integer.  For example, for NTSC (29.97 FPS), framesPerSecond == 30.
	m_framesPerSecond = (unsigned long)((m_frameTimescale + (m_frameDuration-1))  /  m_frameDuration);
	
	
	QImage image(m_frameWidth,m_frameHeight, QImage::Format_ARGB32);
	image.fill(Qt::green);
	//m_frame = VideoFramePtr(new VideoFrame(image, 1000/30));
	
	HRESULT res;
	
	// Set the video output mode
	if (m_deckLinkOutput->EnableVideoOutput(videoDisplayMode->GetDisplayMode(), bmdVideoOutputFlagDefault) != S_OK)
	{
		//fprintf(stderr, "Failed to enable video output\n");
		qDebug() << "BMDOutputDelegate::StartRunning(): Failed to EnableVideoOutput()";
		goto bail;
	}
	
	res = m_deckLinkOutput->CreateVideoFrame(
		m_frameWidth,
		m_frameHeight,
		m_frameWidth * 4,
		bmdFormat8BitBGRA, 
		bmdFrameFlagDefault,
		&m_rgbFrame);
	if(res != S_OK)
	{
		qDebug() << "BMDOutputDelegate::StartRunning: Error creating RGB frame, res:"<<res;
		goto bail;
	}
	
	res = m_deckLinkOutput->CreateVideoFrame(
		m_frameWidth,
		m_frameHeight,
		m_frameWidth * 2,
		bmdFormat8BitYUV, 
		bmdFrameFlagDefault,
		&m_yuvFrame);
	if(res != S_OK)
	{
		qDebug() << "BMDOutputDelegate::StartRunning: Error creating YUV frame, res:"<<res;
		goto bail;
	}


// 	// Generate a frame of black
// 	if (m_deckLinkOutput->CreateVideoFrame(m_frameWidth, m_frameHeight, m_frameWidth*2, bmdFormat8BitYUV, bmdFrameFlagDefault, &m_videoFrameBlack) != S_OK)
// 	{
// 		fprintf(stderr, "Failed to create video frame\n");	
// 		goto bail;
// 	}
// 	FillBlack(m_videoFrameBlack);
// 	
// 	// Generate a frame of colour bars
// 	if (m_deckLinkOutput->CreateVideoFrame(m_frameWidth, m_frameHeight, m_frameWidth*2, bmdFormat8BitYUV, bmdFrameFlagDefault, &m_videoFrameBars) != S_OK)
// 	{
// 		fprintf(stderr, "Failed to create video frame\n");
// 		goto bail;
// 	}
// 	FillColourBars(m_videoFrameBars);
	
	
	// Begin video preroll by scheduling a second of frames in hardware
	m_totalFramesScheduled = 0;
  	for (unsigned i = 0; i < m_framesPerSecond; i++)
  	{
  		PrepareFrame();
 		ScheduleNextFrame(true);
 	}
	
	// Args: startTime, timeScale, playback speed (1.0 = normal)
	m_deckLinkOutput->StartScheduledPlayback(0, 100, 1.0);
	
	m_running = true;
	
	return;
	
bail:
	// *** Error-handling code.  Cleanup any resources that were allocated. *** //
	StopRunning();
}


void BMDOutputDelegate::StopRunning ()
{
	// Stop the audio and video output streams immediately
	m_deckLinkOutput->StopScheduledPlayback(0, NULL, 0);
	//
	//m_deckLinkOutput->DisableAudioOutput();
	m_deckLinkOutput->DisableVideoOutput();
	
	if (m_yuvFrame != NULL)
		m_yuvFrame->Release();
	m_yuvFrame = NULL;
	
	if (m_rgbFrame != NULL)
		m_rgbFrame->Release();
	m_rgbFrame = NULL;
	
	// Success; update the UI
	m_running = false;
}

void BMDOutputDelegate::ScheduleNextFrame(bool preroll)
{
	if(!preroll)
	{
		// If not prerolling, make sure that playback is still active
		if (m_running == false)
			return;
	}
	
	if(m_frameSet)
	{
		//qDebug() << "m_frameSet: not setting";
		return;
	}
	
	m_frameSet = true;
	
	QTime t;
	t.start();
// 		if ((m_totalFramesScheduled % m_framesPerSecond) == 0)
// 		{
// 			// On each second, schedule a frame of black
// 			if (m_deckLinkOutput->ScheduleVideoFrame(m_videoFrameBlack, (m_totalFramesScheduled * m_frameDuration), m_frameDuration, m_frameTimescale) != S_OK)
// 				return;
// 		}
			
	
	
	//qDebug() << "BMDOutputDelegate::ScheduleNextFrame: [3] Frame Repaint: "<<t.restart()<<" ms";
	
	if(!m_image.isNull())
	{
		void *frameBytes;
		m_rgbFrame->GetBytes(&frameBytes);
		
		int maxBytes = m_frameWidth * m_frameHeight * 4;
		memcpy(frameBytes, (const uchar*)m_image.bits(), m_image.byteCount() > maxBytes ? maxBytes : m_image.byteCount());
		
		//qDebug() << "BMDOutputDelegate::ScheduleNextFrame: [4] Load BMD Frame with RGB: "<<t.restart()<<" ms";
	
		
		// 	Pixel conversions
		//  Source frame      Target frame
		//  bmdFormat8BitRGBA bmdFormat8BitYUV
		//                    bmdFormat8BitARGB
		//  bmdFormat8BitBGRA bmdFormat8BitYUV
		//  bmdFormat8BitARGB bmdFormat8BitYUV
		if(m_deckLinkConverter)
		{
			m_deckLinkConverter->ConvertFrame(m_rgbFrame, m_yuvFrame);
			
			//qDebug() << "BMDOutputDelegate::ScheduleNextFrame: [5] RGB->YUV: "<<t.restart()<<" ms";
			
			if (m_deckLinkOutput->ScheduleVideoFrame(m_yuvFrame, 
				(m_totalFramesScheduled * m_frameDuration), 
				m_frameDuration, 
				m_frameTimescale) != S_OK)
				return;
				
			//qDebug() << "BMDOutputDelegate::ScheduleNextFrame: [6] ScheduleVideoFrame(): "<<t.restart()<<" ms";
		}
		else
		{
			qDebug() << "BMDOutputDelegate::ScheduleNextFrame: No m_deckLinkConverter available, unable to convert frame.";
		}
	}
	
		
	m_totalFramesScheduled += 1;
}

void BMDOutputDelegate::PrepareFrame()
{
	if(!m_frame || !m_frame->isValid())
		return;
		
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
				
			//image = image.copy();
			//qDebug() << "Downscaled image from "<<image.byteCount()<<"bytes to "<<scaledImage.byteCount()<<"bytes, orig ptr len:"<<m_frame->pointerLength()<<", orig ptr:"<<m_frame->pointer();
		}
		else
		{
			qDebug() << "BMDOutputDelegate::frameImage: Unable to convert pixel format to image format, cannot scale frame. Pixel Format:"<<m_frame->pixelFormat();
		}
	}
	
	//qDebug() << "BMDOutputDelegate::ScheduleNextFrame: [1] VideoFrame->QImage: "<<t.restart()<<" ms";
	
	if(image.format() != QImage::Format_ARGB32)
		image = image.convertToFormat(QImage::Format_ARGB32);
		
	//qDebug() << "BMDOutputDelegate::ScheduleNextFrame: [2] Format Conversion: "<<t.restart()<<" ms";
		
	if(image.width()  != (int)m_frameWidth || 
	   image.height() != (int)m_frameHeight)
	{
		image = image.scaled(m_frameWidth, m_frameHeight, Qt::IgnoreAspectRatio); //, Qt::SmoothTransformation);
		
// 		image = image.scaled(m_frameWidth, m_frameHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
// 		
// 		QImage frame(m_frameWidth, m_frameHeight, QImage::Format_ARGB32);
// 		frame.fill(Qt::black);
// 		QPainter p(&frame);
// 		QRect rect = image.rect();
// 		
// 		rect.moveCenter(frame.rect().center());
// 		
// 		p.drawImage(rect.topLeft(), image);
// 		p.end();
// 		
// 		image = frame;
	}
	
	m_image = image.copy();
}

void BMDOutputDelegate::SetCurrentFrame(VideoFramePtr ptr)
{
	m_frame = ptr;
	
	PrepareFrame();
	
	if((++ m_frameReceivedCount % m_framesPerSecond) == 0)
		m_frameSet = false;
		
	ScheduleNextFrame(false);
	
	
	
	//if(m_totalFramesScheduled == 0)
	
	
	
	//ScheduleNextFrame(false);
	//qDebug() << "BMDOutputDelegate::SetCurrentFrame(): Got frame, size:"<<ptr->size();
	
	/*
	
// 	if(image.width()  != FRAME_WIDTH ||
// 	   image.height() != FRAME_HEIGHT)
// 	{
// 		image = image.scaled(FRAME_WIDTH,FRAME_HEIGHT,Qt::IgnoreAspectRatio);
// 	}
	
	if(image.format() != V4L_QIMAGE_FORMAT)
		image = image.convertToFormat(V4L_QIMAGE_FORMAT);
		
	// QImage and V4L's def of RGB24 differ in that they have the R & B bytes swapped compared to each other 
	// - so we swap them here to appear correct in output. 
	if(V4L_NATIVE_FORMAT == VIDEO_PALETTE_RGB24 &&
	   V4L_QIMAGE_FORMAT == QImage::Format_RGB888)
	{
		uchar  tmp;
		uchar *bits = image.scanLine(0);
		for(int i=0; i<image.byteCount(); i+=3)
		{
			tmp       = bits[i];
			bits[i]   = bits[i+2];
			bits[i+2] = tmp;
		}
	}
	
	if(write(m_BMDOutputDev, (const uchar*)image.bits(), image.byteCount()) != image.byteCount()) 
	{
		qDebug() << "BMDOutput::readFrame(): Error writing to "<<m_bmdOutputName<<" (bytes written does not match bytes requested), V4L error: " << strerror(errno); 
	}
	else
	{
		//qDebug() << "DVizSharedMemoryThread::readFrame(): Wrote "<<outImg.byteCount()<<" bytes to "<<V4L_OUTPUT;
	} */
	
}

#endif


BMDOutput::BMDOutput(QString key, QObject *parent)
	: QThread(parent)
	, m_bmdOutputName(key)
	, m_frameReady(false)
	, m_started(false)
	, m_bmd(0)
	, m_killed(false)
{
	m_source = 0;
	start();
}

BMDOutput::~BMDOutput()
{
	setVideoSource(0);
	if(!m_killed)
	{
		m_killed = true;
		quit();
		wait();
	}
	
	#ifdef ENABLE_DECKLINK_CAPTURE
	if(m_bmd)
	{
		m_bmd->StopRunning();
		delete m_bmd;
		m_bmd = 0;
	}
	#endif
}
	
void BMDOutput::setVideoSource(VideoSource *source)
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
		qDebug() << "BMDOutput::setVideoSource(): "<<this<<" Source is NULL";
	}

}

void BMDOutput::disconnectVideoSource()
{
	if(!m_source)
		return;
	disconnect(m_source, 0, this, 0);
	m_source = 0;
}


void BMDOutput::frameReady()
{
	if(!m_source)
		return;
		
	m_frameReady = true;
	if(!m_started)
		getFrame();
}

void BMDOutput::getFrame()
{
	VideoFramePtr frame = m_source->frame();
	if(!frame || !frame->isValid())
	{
		//qDebug() << "BMDOutput::frameReady(): Invalid frame or no frame";
		return;
	}

	#ifdef ENABLE_DECKLINK_CAPTURE	
	if(!m_bmd)
	{
		m_bmd = BMDOutputDelegate::openDevice(m_bmdOutputName);
		if(!m_bmd)
			qDebug() << "BMDOutput::getFrame: Unable to open"<<m_bmdOutputName<<"for output, output will be disabled.";
		else
			qDebug() << "BMDOutput::getFrame: Opened"<<m_bmdOutputName<<"with delegate:"<<m_bmd;
	}
	
	if(m_bmd)
		m_bmd->SetCurrentFrame(frame);
	#endif
}

void BMDOutput::run()
{
	m_started = true;
	while(!m_killed)
	{
		if(m_frameReady)
			getFrame();
		
		msleep(15);
	}
}


