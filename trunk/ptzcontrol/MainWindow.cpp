#include "MainWindow.h"

#include "VideoReceiver.h"
#include "GLSceneGroup.h"
#include "GLVideoInputDrawable.h"
#include "VideoWidget.h"

#define SETTINGS_FILE "config.ini"
#define GetSettingsObject() QSettings settings(SETTINGS_FILE,QSettings::IniFormat);

#define EXIT_MISSING(field) { qDebug() << "Error: '" field "' value in "<<SETTINGS_FILE<<" is empty, exiting."; exit(-1); }
	

MainWindow::MainWindow()
	: QWidget()
{
	QHBoxLayout *hbox = new QHBoxLayout(this);
	hbox->setContentsMargins(0,0,0,0);
	
	GetSettingsObject();
	
	QString vidhost = settings.value("vidhost").toString();
	int     vidport = settings.value("vidport").toInt();
	
	QString ptzhost = settings.value("ptzhost").toString();
	int     ptzport = settings.value("ptzport").toInt();
	
	if(vidhost.isEmpty())	EXIT_MISSING("vidhost");
	if(vidport <= 0)	EXIT_MISSING("vidport");
	
	if(ptzhost.isEmpty())	EXIT_MISSING("ptzhost");
	if(ptzport <= 0)	EXIT_MISSING("ptzport");
	
	VideoReceiver *rx = VideoReceiver::getReceiver(vidhost,vidport);
	m_src = rx;
	
	VideoWidget *drw = new VideoWidget(this);
	hbox->addWidget(drw);
	
	drw->setVideoSource(rx);
	
	connect(drw, SIGNAL(pointClicked(QPoint)), this, SLOT(pointClicked(QPoint)));
	//drw->setProperty("con", connection);
	
	resize( 320, 240 );
}

void MainWindow::pointClicked(QPoint pnt)
{
	/*QString con = sender()->property("con").toString();
	showCon(con);

	for(int i=0; i<m_ratings.size(); i++)
		m_ratings[i] = 0;*/ 
	//qDebug() << "Clicked: "<<pnt;
	
	VideoFramePtr frame = m_src->frame();
	if(frame && frame->isValid())
	{
		QImage img = frame->toImage();
		QRect rect(pnt - QPoint(12,12), QSize(25,25));
		QImage subImg = img.copy(rect);
		qDebug() << "Zoom rect:"<<rect<<", pnt:"<<pnt;
		
		QLabel *label = new QLabel();
		label->setPixmap(QPixmap::fromImage(subImg.scaled(320,240,Qt::KeepAspectRatio)));
		label->show();
		
// 		label = new QLabel();
// 		label->setPixmap(QPixmap::fromImage(img));
// 		label->show();
	}
}

