#include "MainWindow.h"

#include "RectSelectVideoWidget.h"
#include "TrackingFilter.h"

#include "CameraThread.h"
#include "VideoThread.h"
#include "VideoReceiver.h"

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
	
	#if 0
	VideoReceiver *source = VideoReceiver::getReceiver("192.168.0.18",7755); 
	#endif
	
	
	#if 1
	// Use a video file for input
	VideoThread * source = new VideoThread();
	source->setVideo("../data/20120115/sermon.wmv");
	//source->setVideo("../data/20120115/webcam4.mp4");
	source->start();
	//qApp->beep();
	// 380
	source->seek(550 * 1000, 0);
	#endif
	
	#if 0
	CameraThread *source = CameraThread::threadForCamera("/dev/video0");
	#endif
	
	TrackingFilter *filter = new TrackingFilter();
	filter->setVideoSource(source);
	
	videoWidget->setVideoSource(filter);
	
	vbox->addWidget(videoWidget);
	
	connect(videoWidget, SIGNAL(rectSelected(QRect)), filter, SLOT(setTrackingRect(QRect)));
	
	// Adjust window for a 4:3 aspect ratio
	resize(640, 480);
}

