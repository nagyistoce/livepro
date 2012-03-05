#include "MainWindow.h"

#include "VideoReceiver.h"
#include "VideoWidget.h"
#include "V4LOutput.h"

#define V4L_OUTPUT_NUM 1

MainWindow::MainWindow()
	: QWidget()
{
	// Setup the layout of the window
	QHBoxLayout *hbox = new QHBoxLayout(this);
	hbox->setContentsMargins(0,0,0,0);
	
	QString host = "localhost"; //192.168.0.19"; //192.168.0.17";
	int port = 8092;
	//int port = 9978;
	//int port = 7756;
		
	VideoReceiver *rx = VideoReceiver::getReceiver(host,port);
		
	// V4L pipes go pairs (in,out), eg:
	// /dev/video0 (input) goes to /dev/video1 (output) 
	QString outputDev = tr("/dev/video%1").arg(V4L_OUTPUT_NUM);
	qDebug() << "Outputing connecting "<<tr("%1:%2").arg(host).arg(port)<<" to video device "<<outputDev;
	
// 	V4LOutput *output = new V4LOutput(outputDev);
// 	output->setVideoSource(rx);
		
	VideoWidget *drw = new VideoWidget(this);
	drw->setVideoSource(rx);
	drw->setRenderFps(true);
	
	hbox->addWidget(drw);
	
	
	resize( 320, 240 );
}

void MainWindow::resizeEvent(QResizeEvent*)
{
	//qDebug() << "Window Size: "<<width()<<" x "<<height(); 
}
