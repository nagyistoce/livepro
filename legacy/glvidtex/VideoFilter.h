#ifndef VideoFilter_H
#define VideoFilter_H

#include <QtGui>

#include "VideoConsumer.h"
#include "../livemix/VideoSource.h"
#include "../livemix/VideoFrame.h"

class VideoFilter : public VideoSource,
		    public VideoConsumer
{
	Q_OBJECT
public:
	VideoFilter(QObject *parent=0);
	virtual ~VideoFilter();
	int fpsLimit() { return m_fpsLimit; }

public slots:
	virtual void setVideoSource(VideoSource*);
	virtual void disconnectVideoSource();
	virtual void setFpsLimit(int);
	
protected slots:
	void frameAvailable();
	
	// Default impl just re-enqueues frame
	virtual void processFrame();

protected:
	// If filter is threaded, when new frameAvailable() is called,
	// it will set m_frameDirty=true and let the run() thread handle it.
	// If filter is NOT threaded, frameAvailable() will start m_processTimer,
	// which will call processFrame() with 0ms timeout to prevent recursion.
	void setIsThreaded(bool);
	bool isThreaded() { return m_isThreaded; }
	virtual void run();
	
	QImage frameImage();
	
protected:	
	VideoFramePtr m_frame;
	
	QTimer m_processTimer;
	QMutex m_frameAccessMutex;
	bool m_frameDirty;
	int m_threadFps;
	int m_fpsLimit;

private:	
	bool m_isThreaded;
	

};

#endif
