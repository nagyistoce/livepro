#ifndef VideoSource_H
#define VideoSource_H

#include <QThread>
#include <QImage>
#include <QQueue>
#include <QPointer>
#include <QMutex>

#include "VideoFrame.h"

class VideoFormat
{
public:
	VideoFormat()
	{
		bufferType = VideoFrame::BUFFER_INVALID;
		pixelFormat = QVideoFrame::Format_Invalid;
	};
	
	VideoFormat(    VideoFrame::BufferType buffer, 
			QVideoFrame::PixelFormat pixel,
			QSize size = QSize(640,480))
	{
		bufferType = buffer;
		pixelFormat = pixel;
		frameSize = size;
	}
	
	bool isValid() { return pixelFormat != QVideoFrame::Format_Invalid && !frameSize.isNull() && bufferType != VideoFrame::BUFFER_INVALID; }
	
	VideoFrame::BufferType bufferType;
	QVideoFrame::PixelFormat pixelFormat;
	QSize frameSize;
};

class VideoWidget;
class VideoSource : public QThread
{
	Q_OBJECT

public:
	VideoSource(QObject *parent=0);
	virtual ~VideoSource();

	virtual void registerConsumer(QObject *consumer);
	virtual void release(QObject *consumer=0);

	virtual VideoFramePtr frame();
	
	bool isBuffered() { return m_isBuffered; }
	void setIsBuffered(bool);
	
	virtual VideoFormat videoFormat() { return VideoFormat(); }
	
	void setAutoDestroy(bool);
	bool autoDestroy() { return m_autoDestroy; }
	
signals:
	void frameReady();

protected slots:
	void consumerDestroyed();
	
protected:
	// subclass hook
	virtual void consumerRegistered(QObject*) {}
	// subclass hook 
	virtual void consumerReleased(QObject*) {}
	
	virtual void run();
	virtual void enqueue(VideoFrame*);
	virtual void enqueue(VideoFramePtr);
	virtual void destroySource();
	
	bool m_killed;
	
	static void initAV();
	static bool isLibAVInit;

	//QQueue<VideoFrame> m_frameQueue;
	VideoFrameQueue m_frameQueue;
	QList<QObject*> m_consumerList;
	bool m_isBuffered;
	VideoFramePtr m_singleFrame;
	QMutex m_queueMutex;
	
	bool m_autoDestroy;
};



#endif
