#include "MainWindow.h"

#include "GLWidget.h"
#include "GLSceneGroup.h"
#include "GLVideoInputDrawable.h"
#include "QtVideoSource.h"
#include "GLVideoFileDrawable.h"
#include "GLVideoDrawable.h"
#include "ScreenVideoSource.h"
#include "VideoSender.h"

MainWindow::MainWindow()
	: QWidget()
{
	// Create the GLWidget to do the actual rendering
	GLWidget *glw = new GLWidget(this);
	
	// Setup the layout of the window
	QVBoxLayout *vbox = new QVBoxLayout(this);
	vbox->addWidget(glw);
	vbox->setContentsMargins(0,0,0,0);
	
	
	GLScene *scene = new GLScene();
	
	int frameWidth = 1000, frameHeight = 750, x=0, y=0;
	
	//GLVideoInputDrawable *drw = 0;
	//drw = new GLVideoInputDrawable();
	//drw->setVideoInput("/dev/video1");
	
	//GLVideoFileDrawable *drw = new GLVideoFileDrawable("../data/2012-01-08 SS Test/test1.mpg");
	
	GLVideoDrawable *drw = new GLVideoDrawable();
// 	QtVideoSource *src = new QtVideoSource();
// 	src->setFile("../data/2012-01-08 SS Test/test1.mpg");
// 	src->start();
	ScreenVideoSource *src = new ScreenVideoSource();
	drw->setVideoSource(src);
	
	VideoSender *sender = new VideoSender();
	//src->registerConsumer(sender);
	sender->setVideoSource(src);
	sender->setTransmitSize(src->captureRect().size());
	//sender->setTransmitFps(src->fps());
	sender->start();
	
	
	QString ipAddress = VideoSender::ipAddress();
	qDebug() << "Started video sender "<<tr("%1:%2").arg(ipAddress).arg(sender->serverPort()) << " for rect "<<src->captureRect();
		
	
	drw->setRect(QRectF(x,y,frameWidth,frameHeight));
	
	// Finally, add the drawable
	scene->addDrawable(drw);
	
	// Add the drawables to the GLWidget
	scene->setGLWidget(glw);
	
	//glw->setViewport(QRect(0,0,1000, 750));
}

void MainWindow::resizeEvent(QResizeEvent*)
{
	//qDebug() << "Window Size: "<<width()<<" x "<<height(); 
}
