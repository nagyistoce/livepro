#ifndef GLVideoThread_H
#define GLVideoThread_H
 
#include <QThread>
#include <QSize>
#include <QtGui>
#include <QtOpenGL>
#include <QMutex>
#include <QMutexLocker>
#include <QTime>

#include "VideoFrame.h"
class VideoOutputWidget;
class VideoSource;

class GLVideoThread : public QThread
{
	Q_OBJECT
public:
	GLVideoThread(VideoOutputWidget *glWidget);
	void resizeViewport(const QSize &size);
	void run();
	void stop();

	VideoSource * getVideoSource() { return videoSource; }
	
public slots:
	void setVideoSource(VideoSource *);

private slots:
	void frameReady();

private:
	bool doRendering;
	bool doResize;
	int w;
	int h;
	int rotAngle;
	VideoOutputWidget *glw;
	
	GLuint	texture[1]; // Storage For One Texture
	
	float xrot, yrot, zrot;
	float xscale, yscale, zscale;
	float xscaleInc, yscaleInc, zscaleInc;
	
	VideoSource *videoSource;
	QMutex videoMutex;
	QMutex resizeMutex;
	QMutex videoSourceMutex;
	
	VideoFrame frame;
	int lastFrameTime;
	bool newFrame;
	QTime time;
	int frames;
	
// 	GLfloat	xrot;				// X Rotation ( NEW )
// 	GLfloat	yrot;				// Y Rotation ( NEW )
// 	GLfloat	zrot;				// Z Rotation ( NEW )

};

#endif
