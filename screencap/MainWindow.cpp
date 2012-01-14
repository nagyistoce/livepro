#include "MainWindow.h"

#include "GLWidget.h"
#include "GLSceneGroup.h"
#include "GLVideoInputDrawable.h"
#include "QtVideoSource.h"
#include "GLVideoFileDrawable.h"
#include "GLVideoDrawable.h"
#include "ScreenVideoSource.h"
#include "VideoSender.h"

#define SETTINGS_FILE "inputs.ini"
#define GetSettingsObject() QSettings settings(SETTINGS_FILE,QSettings::IniFormat);

#include <QApplication>

MainWindow::MainWindow()
	: QWidget()
{
	// Create the GLWidget to do the actual rendering
	GLWidget *glw = new GLWidget(this);
	
	// Load the config from SETTINGS_FILE
	GetSettingsObject();

	// Grab number of inputs from config
	int numItems = settings.value("num-inputs",0).toInt();
	if(numItems <= 0)
	{
		qDebug() << "No inputs listed in "<<SETTINGS_FILE<<", sorry!";
		QTimer::singleShot(0,qApp,SLOT(quit()));
		return;
	}
		
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
	
	// Finally, get down to actually creating the drawables 
	// and setting their positions
	for(int i=1; i<=numItems; i++)
	{
		// Tell QSettings to read from the current input
		QString group = tr("input%1").arg(i);
		qDebug() << "Reading input "<<group;
		settings.beginGroup(group);
		
		// Read capture rectangle
		QString rectString = settings.value("rect","").toString();
		if(rectString.isEmpty())
		{
			qDebug() << "Error: No rect found for input "<<i<<", assuming default rect of 0 0 320 240";
			rectString = "0 0 320 240";
		}
		
		// Create the QRect for capture
		QRect captureRect;
		QStringList points = rectString.split(" ");
		if(points.size() != 4)
		{
			qDebug() << "Error: Invalid rectangle definition for input "<<i<<" (not exactly four numbers seperated by three spaces): "<<rectString<<", assuming default rect";
			captureRect = QRect(0,0,320,240);
		}
		else
		{
			captureRect = QRect(
				points[0].toInt(),
				points[1].toInt(),
				points[2].toInt(),
				points[3].toInt()
			);
		}
		
		// Read transmit size
		QSize xmitSize = captureRect.size();
		QString xmitSizeString = settings.value("xmit-size","").toString();
		if(!xmitSizeString.isEmpty())
		{
			QStringList list = xmitSizeString.split(" ");
			if(list.size() != 2)
			{
				qDebug() << " Error: Invalid xmit-size string for input "<<i<<" (not exactly two numbers seperated by a space): "<<xmitSizeString<<", assuming xmit same size as captureRect: "<<captureRect.size();
			}
			else
			{
				xmitSize = QSize(list[0].toInt(), list[1].toInt());
			}
		}
		
		// Read capture fps
		int captureFps = settings.value("fps",-1).toInt();
		
		// Read transmit fps
		int xmitFps = settings.value("xmit-fps",10).toInt();
		
		// If captureFps not specified, default to same as xmit-fps
		if(captureFps < 0)
			captureFps = xmitFps;
		
		// Read transmit port
		int port = settings.value("port",-1).toInt(); // default of -1 means it will auto-allocate
		
		// Create the screen capture object
		ScreenVideoSource *src = new ScreenVideoSource();
		src->setCaptureRect(captureRect);
		src->setFps(captureFps);
		
		// Create the preview drawable
		GLVideoDrawable *drw = new GLVideoDrawable();
		drw->setVideoSource(src);
		
		// Create the video transmitter object
		VideoSender *sender = new VideoSender();
		sender->setVideoSource  (src);
		sender->setTransmitSize (xmitSize);
		sender->setTransmitFps  (xmitFps);
		sender->start           (port); // this actually starts the transmission
		
		// Output confirmation
		QString ipAddress = VideoSender::ipAddress();
		qDebug() << "Success: Input "<<i<<": Started video sender "<<tr("%1:%2").arg(ipAddress).arg(sender->serverPort()) << " for rect "<<src->captureRect();
			
		// Position the preview drawable
		drw->setRect(QRectF(x,y,frameWidth,frameHeight));
		x += frameWidth;
		
		// Finally, add the drawable
		scene->addDrawable(drw);
		
		// Return to top level of settings file for next group
		settings.endGroup();
	}
	
	// Add the drawables to the GLWidget
	scene->setGLWidget(glw);
	
	// Adjust size of viewport and window to match number of items we have
	glw->setViewport(QRect(0,0,frameWidth * numItems, 750));
	resize( 320 * numItems, 240 );
}

void MainWindow::resizeEvent(QResizeEvent*)
{
	//qDebug() << "Window Size: "<<width()<<" x "<<height(); 
}
