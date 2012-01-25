#include "MainWindow.h"

#include "RectSelectVideoWidget.h"
#include "CameraThread.h"
#include "TrackingFilter.h"

MainWindow::MainWindow()
	: QWidget()
{
	// Setup the layout of the window
	QVBoxLayout *vbox = new QVBoxLayout(this);
	vbox->setContentsMargins(0,0,0,0);
		
	// Use a VideoWidget to handle the rendering.
	// VideoWidgets only support a single source (but they do support cross-fading),
	// but they don't use OpenGL for rendering, which makes them work under less-than-ideal circumstances.
	RectSelectVideoWidget *videoWidget = new RectSelectVideoWidget(this);
	
	CameraThread *camera = CameraThread::threadForCamera("/dev/video0");
	
	TrackingFilter *filter = new TrackingFilter();
	filter->setVideoSource(camera);
	
	videoWidget->setVideoSource(filter);
	
	vbox->addWidget(videoWidget);
	
	connect(videoWidget, SIGNAL(rectSelected(QRect)), filter, SLOT(setTrackingRect(QRect)));
	
	// Adjust window for a 4:3 aspect ratio
	resize(640, 480);
}

