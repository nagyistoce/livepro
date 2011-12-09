#ifndef StaticVideoSource_H
#define StaticVideoSource_H

#include "../livemix/VideoFrame.h"
#include "../livemix/VideoSource.h"

class StaticVideoSource : public VideoSource
{
	Q_OBJECT
	
	Q_PROPERTY(QImage image READ image WRITE setImage);

public:
	StaticVideoSource(QObject *parent=0);
	//virtual ~StaticVideoSource() {}

	VideoFormat videoFormat() { return VideoFormat(VideoFrame::BUFFER_IMAGE,QVideoFrame::Format_ARGB32); }
	//VideoFormat videoFormat() { return VideoFormat(VideoFrame::BUFFER_BYTEARRAY,QVideoFrame::Format_ARGB32); }
	
	
	const QImage & image() { return m_image; }

public slots:
	void setImage(const QImage&);
	
signals:
	void frameReady();

protected:
	void run();
	
private:
	QImage m_image;
	VideoFramePtr m_frame;
	bool m_frameUpdated;
};

#endif
