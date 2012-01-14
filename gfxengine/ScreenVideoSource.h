#ifndef ScreenVideoSource_H
#define ScreenVideoSource_H

#include <QtGui>

#include "VideoSource.h"


class ScreenVideoSource : public VideoSource
{
	Q_OBJECT
	
public:
	ScreenVideoSource(QObject *parent=0);
	
	// VideoSource::
	VideoFormat videoFormat() { return VideoFormat(VideoFrame::BUFFER_IMAGE,QVideoFrame::Format_ARGB32); }
	
	QRect captureRect() { return m_captureRect; }
	int fps() { return m_fps; }

public slots:
	void setCaptureRect(const QRect& rect) { m_captureRect = rect; }
	void setFps(int);
	
signals:
	void frameReady();

private slots:
	void captureScreen();

protected:
	void run();
	
private:
	QRect m_captureRect;
	int m_fps;
	QTimer m_captureTimer;
};


#endif
