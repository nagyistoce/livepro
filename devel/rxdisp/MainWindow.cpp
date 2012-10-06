#include "MainWindow.h"

// This is a simple template app, for you to use as a starting point for something else. 
// See README for copying instructions.

// You can use either a GLWidget to render, or a VideoWidget - to use GLWidget, just comment out the following #define

//#define USE_VIDEO_WIDGET

#include "VideoReceiver.h"
#include "GLImageHttpDrawable.h"

#ifdef USE_VIDEO_WIDGET
	#include "VideoWidget.h"
	#include "CameraThread.h"
#else
	#include "GLWidget.h"
	#include "GLSceneGroup.h"
	#include "GLVideoInputDrawable.h"
#endif

MainWindow::MainWindow()
	: QWidget()
{
	// Setup the layout of the window
	QVBoxLayout *vbox = new QVBoxLayout(this);
	vbox->setContentsMargins(0,0,0,0);
	
	//QString host = "localhost"; //192.168.0.19"; //192.168.0.17";
	QString host = "192.168.0.10";
	int port = 8093;
	//int port = 9978;
	//int port = 7756;
		
	#ifdef USE_VIDEO_WIDGET
		
		// Use a VideoWidget to handle the rendering.
		// VideoWidgets only support a single source (but they do support cross-fading),
		// but they don't use OpenGL for rendering, which makes them work under less-than-ideal circumstances.
		VideoWidget *videoWidget = new VideoWidget(this);
		
		//CameraThread *camera = CameraThread::threadForCamera("/dev/video0");
		//videoWidget->setVideoSource(camera);
		VideoReceiver *rx = VideoReceiver::getReceiver(host,port);
		videoWidget->setVideoSource(rx);
		
		vbox->addWidget(videoWidget);
	#else
	
		// Create the GLWidget to do the actual rendering
		GLWidget *glw = new GLWidget(this);
		vbox->addWidget(glw);
		
		GLScene *scene = new GLScene();
		
		//GLVideoInputDrawable *drw = 0;
		
		// Create a video input drawable - GLVideoInputDrawable inherits from GLVideoDrawable,
		// and it acquires a CameraThread for the given video input and connects it to the
		// parent GLVideoDrawable. GLVideoInput drawable also can handle network video sources
		// via VideoReceiver - see GLVideoInputDrawable::setVideoConnection()
		//drw = new GLVideoInputDrawable();
		//drw->setVideoInput("/dev/video0");
		/*GLVideoDrawable *drw = new GLVideoDrawable();
		drw->setVideoSource(rx);*/ 
		
		GLImageHttpDrawable *drw = new GLImageHttpDrawable();
		drw->setUrl(QString("raw://%1:%2/").arg(host).arg(port));
		
		connect(drw, SIGNAL(videoReceiverChanged(VideoReceiver *)), this, SLOT(videoReceiverChanged(VideoReceiver *)));
		
		// Add the drawable to the scene
		scene->addDrawable(drw);
		m_drw = drw;
		
		// setGLWidget() adds all the drawables in the scene to the GLWidget
		scene->setGLWidget(glw);
		
		m_glw = glw;
		m_glw->setCrossfadeSpeed(750);
		
		// Setting the drawable rect and viewport is not strictly needed,
		// since the viewport defaults to 0,0 by 1000x750, and when
		// a drawable with a null rect is added to the GLWidget, it gets
		// its rect set to the viewport rect. However, the code is present
		// here for illustration purposes if needed.
		
		//int frameWidth = 1000, frameHeight = 750, x=0, y=0;
		//drw->setRect(QRectF(x,y,frameWidth,frameHeight));
		//glw->setViewport(QRect(0,0,1000, 750));
	
	#endif
	
	// Adjust window for a 4:3 aspect ratio
	//resize(640, 480);
	resize(1024,768);
}

void MainWindow::videoReceiverChanged(VideoReceiver *rx)
{
	//qDebug() << "MainWindow:: connecting to customSignal() on: "<<rx;
	connect(rx, SIGNAL(customSignal(QString, QVariant)), this, SLOT(customSignal(QString, QVariant)));
}

void MainWindow::customSignal(QString key, QVariant value)
{
	if(key == "setFadeSpeed")
	{
		int speed = value.toInt();
		qDebug() << "MainWindow::customSignal: setFadeSpeed: "<<speed<<"ms";
		//m_glw->setCrossfadeSpeed(speed);
		m_drw->setXFadeLength(speed);
	}
}
