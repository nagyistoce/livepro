#include "MainWindow.h"

#include "GLWidget.h"
#include "GLSceneGroup.h"
#include "GLVideoInputDrawable.h"

#define SETTINGS_FILE "inputs.ini"
#define GetSettingsObject() QSettings settings(SETTINGS_FILE,QSettings::IniFormat);

MainWindow::MainWindow()
	: QWidget()
{
	// Create GLWidget
	// Create qsettings object for config
	// Create video listeners based on config
		// Figure out what object to use
		// Create object
		// Calculate grid position using the securitycamviewer grid code
		// Add to GLScene
	// Annnnd....go!
	
	GetSettingsObject();
	
	QVBoxLayout *vbox = new QVBoxLayout(this);
	
	GLWidget *glw = new GLWidget();
	vbox->addWidget(glw);
	
	GLScene *scene = new GLScene();
	
	GLVideoInputDrawable *drw = 0;
	
	QString windowTitle = settings.value("window-title","Camera Monitor").toString();
	setWindowTitle(windowTitle);
	
	int numItems = settings.value("num-inputs",0).toInt();
	if(numItems <= 0)
	{
		qDebug() << "No inputs listed in "<<SETTINGS_FILE<<", sorry!";
		return;
	}
	
	int cols = 0, rows = 0;
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
			// add one row then try
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
	
	int maxWidth = 1000;
	int maxHeight = 750;
	int frameWidth = maxWidth / cols;
	int frameHeight = frameWidth * (((double)maxHeight)/((double)maxWidth));
	int top = (maxHeight - frameHeight * rows)/2;
	int left = (maxWidth - frameWidth * cols)/2; 
	qDebug() << "Layout: "<<cols<<"x"<<rows<<", size:"<<frameWidth<<"x"<<frameHeight;
	
	for(int i=0; i<numItems; i++)
	{
		QString con = settings.value(tr("input%1").arg(i)).toString();
		qDebug() << "Input "<<i<<": "<<con;
		
		drw = new GLVideoInputDrawable();
		drw->setVideoConnection(con);
		
		double frac = (double)i / (double)cols;
		int row = (int)frac;
		int col = (int)((frac - row) * cols);
		
		int x = col * frameWidth + left;
		int y = row * frameHeight + top;
		// qDebug() << "updateFrames(): Painted image index "<<i<<" at row"<<row<<", col"<<col<<", coords "<<x<<"x"<<y;
		//painter.drawImage(x,y, image);
		drw->setRect(QRectF(x,y,frameWidth,frameHeight));
		
		scene->addDrawable(drw);
	}
	
	scene->setGLWidget(glw);
}
