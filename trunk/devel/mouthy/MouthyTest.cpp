#include "MouthyTest.h"

#include "VideoWidget.h"
#include "CameraThread.h"
#include "VideoThread.h"
#include "FaceDetectFilter.h"

MouthyTest::MouthyTest(QWidget *parent)
	: QWidget(parent)
	, m_videoWidget(0)
	, m_source(0)
{
	// Setup the layout of the window
	QVBoxLayout *m_vbox = new QVBoxLayout(this);
	m_vbox->setContentsMargins(0,0,0,0);
	m_vbox->setSpacing(0);
	
	#if 0
	// Use a video file for input
	VideoThread * source = new VideoThread();
	source->setVideo("../data/dsc_3282.avi");
	source->start();
	
	#else
	// Use camera as input
	//QString dev = "test:/opt/livepro/devel/data/2012-01-08 SS Test/stest1.mpg"; ///dev/video0";
	QString dev = "test:1";
	CameraThread *source = CameraThread::threadForCamera(dev);
	if(!source)
	{
		qDebug() << "Unable to find camera "<<dev<<", exiting";
		exit(-1);
	}
		
	source->setFps(5);
	
// 	dev = "test:/opt/livepro/devel/data/2012-01-08 SS Test/test3.mpg";
// 	CameraThread *source2 = CameraThread::threadForCamera(dev);
	
	#endif
	
	// Create the viewer widget
	m_videoWidget = new VideoWidget(this);
	m_vbox->addWidget(m_videoWidget);
	
	#if 1
	// Add face filter
	m_filter = new FaceDetectFilter(this);
	m_filter->setVideoSource(source);
	m_videoWidget->setVideoSource(m_filter);
	
	#else
	// Just show the source directly
	m_videoWidget->setVideoSource(source);
	
	#endif
	
	resize(640,480);

}
