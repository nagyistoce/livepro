#include "MainWindow.h"

#include "VideoReceiver.h"
#include "AnalysisFilter.h"
#include "PlayerConnection.h"
#include "GLSceneGroup.h"
#include "GLVideoInputDrawable.h"

/// One or the other, not both!
//#define PREVIEW_OPENGL
#define PREVIEW_STOCK

#ifdef PREVIEW_STOCK
#include "VideoWidget.h"
#else
#include "GLWidget.h"
#include "GLVideoDrawable.h"
#endif

MainWindow::MainWindow()
	: QWidget()
	, m_lastHighNum(-1)
	, m_ignoreCountdown(0)
{
	#ifdef PREVIEW_OPENGL
	// Create the GLWidget to do the actual rendering
	GLWidget *glw = new GLWidget(this);
	#endif
	
	// Setup the layout of the window
	#ifdef PREVIEW_STOCK
	QHBoxLayout *hbox = new QHBoxLayout(this);
	hbox->setContentsMargins(0,0,0,0);
	
	#else
	QVBoxLayout *vbox = new QVBoxLayout(this);
	vbox->addWidget(glw);
	vbox->setContentsMargins(0,0,0,0);
	#endif
	
	QString host = "10.10.9.90";
	//QString host = "localhost";
	
	PlayerConnection *player = new PlayerConnection();
	player->setHost(host);
	m_player = player;
	
	player->connectPlayer();
	
	//player->setScreenRect(QRect(1280,600,320,240));
	//player->setScreenRect(QRect(600,200,640,480));
	player->setScreenRect(QRect(0,0,640,480));
	player->setCrossfadeSpeed(150);
	//player->setCrossfadeSpeed(0);
	
	
	QStringList inputs = QStringList() 
		<< tr("%1:7755").arg(host)
		<< tr("%1:7756").arg(host)
		<< tr("%1:7757").arg(host)
		<< tr("%1:7758").arg(host)
		<< tr("10.10.9.90:7759");
	
	m_cons = QStringList()
		<< tr("dev=test:/opt/livepro/devel/data/2012-01-08 SS Test/test1.mpg,input=Default,net=%1:7755").arg(host)
		<< tr("dev=test:/opt/livepro/devel/data/2012-01-08 SS Test/test1-orig.mpg,input=Default,net=%1:7756").arg(host)
		<< tr("dev=test:/opt/livepro/devel/data/2012-01-08 SS Test/test2.mpg,input=Default,net=%1:7757").arg(host)
		<< tr("dev=test:/opt/livepro/devel/data/2012-01-08 SS Test/test3.mpg,input=Default,net=%1:7758").arg(host)
		<< tr("dev=/dev/null,input=Default,net=10.10.9.90:7759");
		
 	#ifdef PREVIEW_OPENGL
 	GLScene *scene = new GLScene();
 	
 	int frameWidth  = 1000,
 	    frameHeight =  750,
 	    x = 0,
 	    y = 0;
 	#endif
 	
 	int counter = 0;
 	foreach(QString connection, inputs)
 	{
		QStringList parts = connection.split(":");
		QString host = parts[0];
		int port = parts[1].toInt();
		
		VideoReceiver *rx = VideoReceiver::getReceiver(host,port);
		
		#ifdef PREVIEW_OPENGL
		GLVideoDrawable *drw = new GLVideoDrawable();
		#else
		VideoWidget *drw = new VideoWidget(this);
		hbox->addWidget(drw);
		#endif
		
		#if 1
		AnalysisFilter *filter = new AnalysisFilter();
		
		QString file = tr("input%1-mask.png").arg(port);
		if(QFileInfo(file).exists())
			filter->setMaskImage(QImage(file));
		
		filter->setVideoSource(rx);
		drw->setVideoSource(filter);
		
		filter->setProperty("num", counter++);
		connect(filter, SIGNAL(motionRatingChanged(int)), this, SLOT(motionRatingChanged(int)));
		
		m_filters << filter;
		m_ratings << 0;
		
		//filter->setOutputImagePrefix(tr("input%1").arg(port));
		#else
		drw->setVideoSource(rx);
		#endif
		
		#ifdef PREVIEW_OPENGL
		drw->setRect(QRectF(x,y,frameWidth,frameHeight));
		x += frameWidth;
		
		// Finally, add the drawable
		scene->addDrawable(drw);
		#endif
	}
	
	#ifdef PREVIEW_OPENGL
	// Add the drawables to the GLWidget
	scene->setGLWidget(glw);
	
	glw->setViewport(QRect(0,0,frameWidth * inputs.size(), 750));
	#endif
	
	resize( 320 * inputs.size(), 240 );
}

void MainWindow::resizeEvent(QResizeEvent*)
{
	//qDebug() << "Window Size: "<<width()<<" x "<<height(); 
}

void MainWindow::motionRatingChanged(int rating)
{
	AnalysisFilter *filter = dynamic_cast<AnalysisFilter*>(sender());
	if(!filter)
		return;
	int num = filter->property("num").toInt();
	
	m_ratings[num] += rating;
	m_filters[num]->setDebugText(tr("%2 %1").arg(m_ratings[num]).arg(m_lastHighNum == num ? " ** LIVE **":""));

	int frameLimit = 69;
			
	if(m_ignoreCountdown-- <= 0)
	{
		
		int max=-1, maxNum = -1;
		int counter = 0;
		foreach(int rating, m_ratings)
		{
			if(rating > max)
			{
				max = rating;
				maxNum = counter;
			}
			
			//m_ratings[counter] = 0;
				
			counter ++;
		}
		
		//maxNum = 4;
		
		if(m_lastHighNum != maxNum)
		{
			QString highestRatedCon = m_cons[maxNum];
			
			GLSceneGroup *group = new GLSceneGroup();
			GLScene *scene = new GLScene();
			GLVideoInputDrawable *vidgld = new GLVideoInputDrawable();
			vidgld->setVideoConnection(highestRatedCon);
			scene->addDrawable(vidgld);
			group->addScene(scene);
			
			m_player->setGroup(group, group->at(0));
			//player->setUserProperty(gld, m_con, "videoConnection");
			
			m_lastHighNum = maxNum;
			
			m_filters[maxNum]->setDebugText(tr("** LIVE ** %1").arg(rating));
			m_ratings[maxNum] = 0;
			
			m_ignoreCountdown = frameLimit * m_ratings.size(); // ignore next X frames * num sources
			qDebug() << "MainWindow::motionRatingChanged: Switching to num:"<<maxNum<<", rated:"<<max;
			
			group->deleteLater();
		}
		else
		{
			//for(int i=0; i<m_ratings.size(); i++)
				m_ratings[maxNum] = 0; 
				
			m_ignoreCountdown = frameLimit * m_ratings.size(); // ignore next X frames * num sources
			qDebug() << "MainWindow::motionRatingChanged: Max num the same:"<<maxNum<<", rated:"<<max<<", not switching!";
		}
	}
}
	
