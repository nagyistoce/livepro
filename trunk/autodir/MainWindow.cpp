#include "MainWindow.h"

#include "GLWidget.h"
#include "GLSceneGroup.h"
#include "GLVideoInputDrawable.h"
#include "QtVideoSource.h"
#include "GLVideoFileDrawable.h"
#include "GLVideoDrawable.h"
#include "VideoReceiver.h"

#include "PlayerConnection.h"

MainWindow::MainWindow()
	: QWidget()
{
	// Create the GLWidget to do the actual rendering
	GLWidget *glw = new GLWidget(this);
	
	// Setup the layout of the window
	QVBoxLayout *vbox = new QVBoxLayout(this);
	vbox->addWidget(glw);
	vbox->setContentsMargins(0,0,0,0);
	
	
	PlayerConnection *player = new PlayerConnection();
	player->setHost("localhost");
	
	player->connectPlayer();
	
	player->setScreenRect(QRect(1200,600,320,240));
	
	QStringList inputs = QStringList() 
		<< "10.10.9.90:7755"
		<< "10.10.9.90:7756"
		<< "10.10.9.90:7757";
	
 	GLScene *scene = new GLScene();
 	
 	int frameWidth  = 1000,
 	    frameHeight =  750,
 	    x = 0,
 	    y = 0;
 	
 	foreach(QString connection, inputs)
 	{
		QStringList parts = connection.split(":");
		QString host = parts[0];
		int port = parts[1].toInt();
		
		VideoReceiver *rx = VideoReceiver::getReceiver(host,port);
		
		GLVideoDrawable *drw = new GLVideoDrawable();
		drw->setVideoSource(rx);
		
		drw->setRect(QRectF(x,y,frameWidth,frameHeight));
		x += frameWidth;
		
		// Finally, add the drawable
		scene->addDrawable(drw);
	}
	
	// Add the drawables to the GLWidget
	scene->setGLWidget(glw);
	
	glw->setViewport(QRect(0,0,frameWidth * inputs.size(), 750));
	resize( 320 * inputs.size(), 240 );
}

void MainWindow::resizeEvent(QResizeEvent*)
{
	//qDebug() << "Window Size: "<<width()<<" x "<<height(); 
}
