#include "MainWindow.h"

#include "GLWidget.h"
#include "GLSceneGroup.h"
#include "GLVideoInputDrawable.h"

// Debugging
#include "GLSpinnerDrawable.h"

#define SETTINGS_FILE "inputs.ini"
#define GetSettingsObject() QSettings settings(SETTINGS_FILE,QSettings::IniFormat);

MainWindow::MainWindow()
	: QWidget()
{
	// Load the config from SETTINGS_FILE
	GetSettingsObject();
	
	// Create the GLWidget to do the actual rendering
	GLWidget *glw = new GLWidget(this);
	
	// Setup the layout of the window
	QVBoxLayout *vbox = new QVBoxLayout(this);
	vbox->addWidget(glw);
	vbox->setContentsMargins(0,0,0,0);
	
	// Allow user to change window title if desired
	QString windowTitle = settings.value("window-title","Camera Monitor").toString();
	setWindowTitle(windowTitle);
	
	// If user gives us a window size, use it - otherwise make it the same as the canvas size
	int winWidth  = settings.value("win-width",0).toInt();
	int winHeight = settings.value("win-height",0).toInt();
// 	if(winWidth <= 0 || winHeight <= 0)
// 		resize(realWidth, realHeight);
// 	else
// 		resize(winWidth, winHeight);

	if(winWidth > 0 && winHeight > 0)
		resize(winWidth, winHeight);
	
	// No default window position. 
	// Only set position if both X and Y are given 
	int winX = settings.value("win-x",-1).toInt();
	int winY = settings.value("win-y",-1).toInt();
 	if(winX >= 0 && winY >= 0)
 		move(winX, winY);
	
	//qDebug() << "Layout: "<<cols<<"x"<<rows<<", size:"<<frameWidth<<"x"<<frameHeight;
	
	GLScene *scene = new GLScene();
	GLVideoInputDrawable *drw = 0;
	
	// Finally, get down to actually creating the drawables 
	int frameWidth = 1000, frameHeight = 750, x=0, y=0;
	
	drw = new GLVideoInputDrawable();
	drw->setVideoInput("/dev/video1");
	drw->setRect(QRectF(x,y,frameWidth,frameHeight));
	
	// Finally, add the drawable
	scene->addDrawable(drw);
	
/*	drw = new GLVideoInputDrawable();
	drw->setVideoInput("/dev/video1");
	drw->setRect(QRectF(x + frameWidth,y,frameWidth,frameHeight));
	scene->addDrawable(drw);
*/
	// Add the drawables to the GLWidget
	scene->setGLWidget(glw);
	
	glw->setViewport(QRect(0,0,1000, 750));
}

// Why? Just for easier setting the win-width/-height in the .ini file.
void MainWindow::resizeEvent(QResizeEvent*)
{
	qDebug() << "Window Size: "<<width()<<" x "<<height(); 
}
