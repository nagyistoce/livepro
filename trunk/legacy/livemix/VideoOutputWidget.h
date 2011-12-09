#ifndef VideoOutputWidget_H
#define VideoOutputWidget_H

#include "GLVideoThread.h"
#include <QGLWidget>

class VideoSource;

class VideoOutputWidget : public QGLWidget
{
public:
	VideoOutputWidget(QWidget *parent);
	void startRendering();
	void stopRendering();
	
	VideoSource *videoSource() { return m_videoSource; }

public slots:
	void setVideoSource(VideoSource *);
	void applyGeometry(const QRect& rect, bool isFullScreen = true);
	
protected:
	void resizeEvent(QResizeEvent *evt);
	void paintEvent(QPaintEvent *);
	void closeEvent(QCloseEvent *evt);
	
	GLVideoThread m_thread;
	VideoSource * m_videoSource;
};


#endif
