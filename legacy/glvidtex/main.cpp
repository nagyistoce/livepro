#include <QApplication>

#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

#include "GLWidget.h"

#include "GLImageDrawable.h"
#include "GLVideoLoopDrawable.h"
#include "GLVideoInputDrawable.h"
#include "GLVideoFileDrawable.h"
#include "GLVideoReceiverDrawable.h"
#include "GLTextDrawable.h"
#include "GLVideoMjpegDrawable.h"

#include "MetaObjectUtil.h"

#include "PlayerWindow.h"

#include "../livemix/VideoWidget.h"
#include "../livemix/CameraThread.h"

#include "VideoSender.h"
#include "VideoReceiver.h"

#include "../livemix/VideoThread.h"
#include "StaticVideoSource.h"
#include "HistogramFilter.h"
#include "FaceDetectFilter.h"
#include "VideoDifferenceFilter.h"

#include "VideoInputColorBalancer.h"

int main(int argc, char *argv[])
{

	QApplication app(argc, argv);
	
	qApp->setApplicationName("GLVidTex");
	qApp->setOrganizationName("Josiah Bryan");
	qApp->setOrganizationDomain("mybryanlife.com");
	
	MetaObjectUtil_Register(GLImageDrawable);
	MetaObjectUtil_Register(GLTextDrawable);
	MetaObjectUtil_Register(GLVideoFileDrawable);
	MetaObjectUtil_Register(GLVideoInputDrawable);
	MetaObjectUtil_Register(GLVideoLoopDrawable);
	MetaObjectUtil_Register(GLVideoReceiverDrawable);

 	PlayerWindow *glw = new PlayerWindow();
	
	//VideoWidget *glw = new VideoWidget();
//   	GLWidget *glw = new GLWidget();
	//VideoInputColorBalancer *glw = new VideoInputColorBalancer();
   	//glw->resize(640,480);
   	//glw->adjustSize();
//  	glw->setViewport(QRectF(0,0,1000,750));
 	
//  	CameraThread *source = CameraThread::threadForCamera("/dev/video1");
//  	//source->setInput("S-Video");
//  	source->setFps(30);
//  	//source->setDeinterlace(true);
// 	source->registerConsumer(glw);
// 	source->enableRawFrames(true);
// 	
// 	//glw->setVideoSource(source);
// 	
// 	VideoSender *sender = new VideoSender();
// 	sender->setVideoSource(source);
// 	if(!sender->listen(QHostAddress::Any,8899))
// 	{
// 		qDebug() << "Unable to setup server";
// 		return -1;
// 	}
// 	
	//VideoReceiver *source = VideoReceiver::getReceiver("192.168.0.17",7755);
	//glw->setVideoSource(source);
	
/* 	
	{
		GLVideoInputDrawable *gld = new GLVideoInputDrawable();
		gld->setVideoInput("/dev/video0");
		gld->setCardInput("S-Video");
		//gld->setDeinterlace(true);
		gld->setRect(glw->viewport());
		gld->setVisible(true);
		gld->setObjectName("camera");
		glw->addDrawable(gld);
	}

	GLVideoDrawable *vid = new GLVideoDrawable();
	vid->setVideoSource(glw->outputStream());
	vid->setRect(QRectF(600,0,400,300));
	vid->setVisible(true);
	vid->setObjectName("stream");
	glw->addDrawable(vid);*/
	
// 	VideoThread * source = new VideoThread();
// 	source->setVideo("../data/Seasons_Loop_3_SD.mpg");
// 	source->start();

// 	StaticVideoSource *source = new StaticVideoSource();
//  	//source->setImage(QImage("colors.png"));
//  	source->setImage(QImage("me2.jpg"));
//  	source->start();

	
	//HistogramFilter *histo = new HistogramFilter();
	//histo->setVideoSource(source);
	
// 	FaceDetectFilter *faceFilter = new FaceDetectFilter();
// 	faceFilter->setVideoSource(source);
	
// 	VideoDifferenceFilter *diffFilter = new VideoDifferenceFilter();
// 	diffFilter->setVideoSource(source);
	
	/*
	HsvInfoFilter *hsvInfo = new HsvInfoFilter();
	hsvInfo->setVideoSource(source);*/
	
	
	//glw->setVideoSource(hsvInfo);
	//glw->setVideoSource(histo);
	//glw->setVideoSource(faceFilter);
	//glw->setVideoSource(diffFilter);
	
// 	GLVideoDrawable *vid = new GLVideoDrawable();
// 	vid->setVideoSource(histo);
// 	vid->setRect(glw->viewport());
// 	vid->setVisible(true);
// 	vid->setObjectName("histo");
// 	glw->addDrawable(vid);
	
	
	
// 	loop->setRect(QRectF(0,0,1000,750));
// 	loop->setZIndex(-10);
// 	loop->setVisible(true);

	

//  	{
// 		GLImageDrawable *gld = new GLImageDrawable("Pm5544.jpg");
// 		gld->setRect(QRectF(0,0,400,300));
// 		gld->setVisible(true);
// 		glw->addDrawable(gld);
// 	}
	
// 	GLImageDrawable *gld2 = new GLImageDrawable("Pm5544.jpg");
// 	gld2->setRect(QRectF(0,0,1000,750));
// 	gld2->setZIndex(-10);
// 	gld2->setVisible(true);
// 	glw->addDrawable(gld2);
	


// 	GLTextDrawable *gld = new GLTextDrawable("Hello 1");
// 	gld->setRect(glw->viewport());
// 	gld->setVisible(true);
// 	glw->addDrawable(gld);
// 	
// 	gld = new GLTextDrawable("Hello 2");
// 	gld->setRect(glw->viewport().translated(100,100));
// 	gld->setVisible(true);
// 	glw->addDrawable(gld);
// 		
 	glw->show();
		
	int x = app.exec();
	delete glw;
	return x;
}
