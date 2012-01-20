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
/*	
	// Grab number of inputs from config
	int numItems = settings.value("num-inputs",0).toInt();
	if(numItems <= 0)
	{
		qDebug() << "No inputs listed in "<<SETTINGS_FILE<<", sorry!";
		return;
	}
	
	
	// If user specifies both row and column values, then just use those to layout the inputs
	int cols = settings.value("cols",0).toInt();
	int rows = settings.value("rows",0).toInt();
	
	// Both row & col must be given - if not, then calculate row/cols 
	// based on the sqrt() of the # of items
	if(rows <= 0 || cols <= 0)
	{
		qDebug() << "No rows and/or columns given in "<<SETTINGS_FILE<<", setting up grid using sqrt()";
		#define applySize(a,b) { cols=x; rows=y; }
		double sq = sqrt(numItems);
		if(((int)sq) != sq)
		{
			// first, attempt to round up
			int x = (int)ceil(sq);
			int y = (int)floor(sq);
			
			if(x*y >= numItems)
				// good to go, apply size
				applySize(x,y)
			else
			{
				// add one col then try
				y++;
				if(x*y >= numItems)
					applySize(x,y)
				else
					// just use the sqrt ceil
					applySize(x,x)
			}
		}
		else
		{
			cols = (int)sq;
			rows = (int)sq;
		}
	}
	
	// This is the default canvas size from GLWidget.
	// Really, we could use any values here since the canvas size will be changed 
	// and these are just used for the frame size.
	int defWidth = 1000;
	int defHeight = 750;
	
	int frameWidth = 0; 
	int frameHeight = 0;
	
	// Calculate frame size
	if(cols >= rows)
	{
		frameWidth = defWidth / cols;
		frameHeight = (int)((double)frameWidth * (((double)defHeight)/((double)defWidth)));
	}
	else
	{
		frameHeight = defHeight / rows;
		frameWidth = (int)((double)frameHeight * (((double)defWidth)/((double)defHeight)));
	}
	
	// Calculate the new canvas size
	int realWidth  = frameWidth  * cols;
	int realHeight = frameHeight * rows;
	glw->setCanvasSize(QSizeF((double)realWidth,(double)realHeight));*/
	
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
// 	// and setting their positions
// 	for(int i=0; i<numItems; i++)
// 	{
// 		// Load the connection string
// 		QString con = settings.value(tr("input%1").arg(i)).toString();
// 		if(con.indexOf("=") < 0)
// 			con = tr("net=%1").arg(con);
// 			
// 		qDebug() << "Input "<<i<<": "<<con;
// 		
// 		// Create the drawable
// 		drw = new GLVideoInputDrawable();
// 		drw->setVideoConnection(con);
// 		
// 		// Calculate the position in the grid
// 		double frac = (double)i / (double)cols;
// 		int row = (int)frac;
// 		int col = (int)((frac - row) * cols);
// 		
// 		// Translate to "pixels"
// 		int x = col * frameWidth;
// 		int y = row * frameHeight;
// 		
// 		// Set the position
// 		drw->setRect(QRectF(x,y,frameWidth,frameHeight));
// 		
// 		// Finally, add the drawable
// 		scene->addDrawable(drw);
// 	}

	int frameWidth = 1000, frameHeight = 750, x=0, y=0;
	
	drw = new GLVideoInputDrawable();
	drw->setVideoInput("test:1");///dev/video1");
	drw->setRect(QRectF(x,y,frameWidth,frameHeight));
	
	VideoDisplayOptionWidget *widget = new VideoDisplayOptionWidget(drw);
	widget->show();
	
	// Finally, add the drawable
	scene->addDrawable(drw);
	
	drw = new GLVideoInputDrawable();
	drw->setVideoInput("test:1");///dev/video1");
	drw->setRect(QRectF(x + frameWidth,y,frameWidth,frameHeight));
	scene->addDrawable(drw);
	
	// Add the drawables to the GLWidget
	scene->setGLWidget(glw);
	
	glw->setViewport(QRect(0,0,2000, 750));
	
// 	GLSpinnerDrawable *spinner = new GLSpinnerDrawable();;
// 	spinner->setCycleDuration(1);
// 	int spinSize = 28;
// 	QRectF spinnerRect(10,10, spinSize, spinSize);
// 	spinner->setRect(spinnerRect);
// // 	spinner->setFlipVertical(true);
// // 	spinner->setFlipHorizontal(true);
// 	spinner->setZIndex(10.);
// 	spinner->start();	
// 	
// 	glw->addDrawable(spinner);
}

// Why? Just for easier setting the win-width/-height in the .ini file.
void MainWindow::resizeEvent(QResizeEvent*)
{
	qDebug() << "Window Size: "<<width()<<" x "<<height(); 
}
