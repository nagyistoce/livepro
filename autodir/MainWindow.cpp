#include "MainWindow.h"

#include "VideoReceiver.h"
#include "AnalysisFilter.h"
#include "PointTrackingFilter.h"
#include "PlayerConnection.h"
#include "GLSceneGroup.h"
#include "GLVideoInputDrawable.h"

// List of commands recognized by the original player server, also used in the json server
#include "../livemix/ServerCommandNames.h"

#include "VideoSender.h"


#include "VideoWidget.h"

#define ENABLE_VIDEO_SENDERS
#define ENABLE_LIVEVIEW_PROXY

//#define ENABLE_V4L_OUTPUT

#ifdef ENABLE_V4L_OUTPUT
// Output to a V4L device (used primarily with vloopback)
#include "V4LOutput.h"
#define FIRST_V4L_NUM 0
#endif

#define SETTINGS_FILE "inputs.ini"
#define GetSettingsObject() QSettings settings(SETTINGS_FILE,QSettings::IniFormat);

MainWindow::MainWindow()
	: QWidget()
	, m_lastHighNum(-1)
	, m_ignoreCountdown(60)
{
	m_hbox = new QHBoxLayout(this);
	m_hbox->setContentsMargins(0,0,0,0);
	
	GetSettingsObject();
	
	QString host = settings.value("host").toString(); //"10.10.9.90";
	//QString host = "localhost";
	if(host.isEmpty())
	{
		qDebug() << "Error: 'host' value in "<<SETTINGS_FILE<<" is empty, exiting.";
		exit(-1);
	}
	
	m_jsonServer = new VideoSwitcherJsonServer(9979, this);
	connect(m_jsonServer, SIGNAL(showCon(const QString&, int)), this, SLOT(showJsonCon(const QString&, int)));
	
	// setFadeSpeed: Not implemented yet
	//connect(jsonServer, SIGNAL(setFadeSpeed(int)), this, SLOT(setFadeSpeed(int)));
	
	
	PlayerConnection *player = new PlayerConnection();
	player->setHost(host);
	m_player = player;
	
	player->connectPlayer();
	
	//player->setScreenRect(QRect(1280,600,320,240));
	//player->setScreenRect(QRect(600,200,640,480));
	QString rectStr = settings.value("rect","0,0,640,480").toString();
	QStringList rectStrList = rectStr.split(",");
	QRect rect(rectStrList[0].toInt(), rectStrList[1].toInt(), rectStrList[2].toInt(), rectStrList[3].toInt());
	
	player->setScreenRect(rect); //QRect(0,0,640,480));
	player->setCrossfadeSpeed(settings.value("xfade",150).toInt());
	//player->setCrossfadeSpeed(0);
	
/*	
	QStringList inputs = QStringList() 
// 		<< tr("%1:7755").arg(host)
// 		<< tr("%1:7756").arg(host)
// 		<< tr("%1:7757").arg(host)
// 		<< tr("%1:7758").arg(host)
// 		<< tr("10.10.9.90:7759");

		<< tr("%1:7755").arg(host)
		<< tr("%1:7756").arg(host)
		<< tr("%1:7757").arg(host)
		<< tr("%1:7758").arg(host);
		
	
	m_cons = QStringList()
// 		<< tr("dev=test:/opt/livepro/devel/data/2012-01-08 SS Test/test1.mpg,input=Default,net=%1:7755").arg(host)
// 		<< tr("dev=test:/opt/livepro/devel/data/2012-01-08 SS Test/test1-orig.mpg,input=Default,net=%1:7756").arg(host)
// 		<< tr("dev=test:/opt/livepro/devel/data/2012-01-08 SS Test/test2.mpg,input=Default,net=%1:7757").arg(host)
// 		<< tr("dev=test:/opt/livepro/devel/data/2012-01-08 SS Test/test3.mpg,input=Default,net=%1:7758").arg(host)
// 		<< tr("dev=/dev/null,input=Default,net=10.10.9.90:7759");

		<< tr("dev=/dev/video0,input=Default,net=%1:7755").arg(host)
		<< tr("dev=test:1,input=Default,net=%1:7756").arg(host)
		<< tr("dev=test:2,input=Default,net=%1:7757").arg(host)
		<< tr("dev=test:3,input=Default,net=%1:7758").arg(host);*/
		
	int numInputs = settings.value("num-inputs",0).toInt();
	if(numInputs <= 0)
	{
		qDebug() << "Error: Zero inputs listed in "<<SETTINGS_FILE<<", exiting.";
		exit(-1);
	}
	
	for(int i=0; i<numInputs; i++)
	{
		QString input = settings.value(tr("input%1").arg(i)).toString();
		if(input.indexOf("$host") > -1)
		{
			QString original = input;
			input = input.replace("$host","%1");
			input = QString(input).arg(host);
			qDebug() << "Input "<<i<<": Auto-replaced host, orig:"<<original<<", final:"<<input;
		}
		
		m_cons << input;

		m_weights << settings.value(tr("weight%1").arg(i), "1.0").toDouble();

		QUrl url = GLVideoInputDrawable::extractUrl(input);
		QString host = url.host();
		int port = url.port();
		
		// Start connection
		VideoReceiver *rx = VideoReceiver::getReceiver(host,port);
		rx->registerConsumer(this);
		
	}
	
	m_host = host;
	m_numInputs = numInputs;
	qDebug() << "Waiting 10 sec to load filters...\n";
	QTimer::singleShot(10000, this, SLOT(setupFilters()));
}

void MainWindow::setupFilters()
{	
 	QString ipAddress = VideoSender::ipAddress();
 	QStringList myVideoPorts;
 	
 	int counter = 0;
 	foreach(QString connection, m_cons)
 	{
// 		QStringList parts = connection.split(":");
// 		QString host = parts[0];
// 		int port = parts[1].toInt();

		QUrl url = GLVideoInputDrawable::extractUrl(connection);
		QString host = url.host();
		int port = url.port();
		
		
		VideoReceiver *rx = VideoReceiver::getReceiver(host,port);
		
		#ifdef ENABLE_V4L_OUTPUT
		// Multiply by two because V4L pipes go (in,out), eg:
		// /dev/video0 (input) goes to /dev/video1 (output) - zeroth item in inputs
		// /dev/video2 (input) goes to /dev/video3 (output) - first item in inputs
		// /dev/video4 (input) goes to /dev/video5 (output) - second item in inputs
		QString outputDev = tr("/dev/video%1").arg((counter * 2) + FIRST_V4L_NUM);
		qDebug() << "Outputing connecting "<<connection<<" to video device "<<outputDev;
		V4LOutput *output = new V4LOutput(outputDev);
		output->setVideoSource(rx);
		#endif
		
		VideoWidget *drw = new VideoWidget(this);
		m_hbox->addWidget(drw);
		
		connect(drw, SIGNAL(clicked()), this, SLOT(widgetClicked()));
		drw->setProperty("con", connection);
		
		//AnalysisFilter *filter = new AnalysisFilter();
		PointTrackingFilter *filter = new PointTrackingFilter();
		
// 		QString file = tr("input%1-mask.png").arg(port);
// 		if(QFileInfo(file).exists())
// 			filter->setMaskImage(QImage(file));
		
		filter->setVideoSource(rx);
		drw->setVideoSource(filter);
		
		filter->setProperty("num", counter++);
		//connect(filter, SIGNAL(motionRatingChanged(int)), this, SLOT(motionRatingChanged(int)));
		connect(filter, SIGNAL(averageMovement(int)), this, SLOT(motionRatingChanged(int)));
		
		m_filters << filter;
		m_ratings << 0;
		
		
		
		#ifdef ENABLE_VIDEO_SENDERS
		VideoSender *sender = new VideoSender();
		sender->setVideoSource(filter);
		sender->start();
		
		QString con = QString("dev=%1,net=%2:%3")
				.arg(counter-1)
				.arg(ipAddress)
				.arg(sender->serverPort());
		
		qDebug() << "Sending filter output from server "<<host<<":"<<port<<" out on con "<<con;
		myVideoPorts << con;
		#endif
		
		//filter->setOutputImagePrefix(tr("input%1").arg(port));
		//drw->setVideoSource(rx);
		
		
	}
	
	
	#ifdef ENABLE_VIDEO_SENDERS
	m_jsonServer->setInputList(myVideoPorts);
	#endif
	
	#ifdef ENABLE_LIVEVIEW_PROXY
	VideoReceiver *liveRx = VideoReceiver::getReceiver(m_host, 9978);
	VideoSender *liveSend = new VideoSender(this);
	liveSend->setVideoSource(liveRx);
	liveSend->listen(QHostAddress::Any,9978);
	#endif
	
	resize( 320 * m_numInputs, 240 );
}

void MainWindow::resizeEvent(QResizeEvent*)
{
	//qDebug() << "Window Size: "<<width()<<" x "<<height(); 
}

void MainWindow::motionRatingChanged(int rating)
{
	//AnalysisFilter *filter = dynamic_cast<AnalysisFilter*>(sender());
	PointTrackingFilter *filter = dynamic_cast<PointTrackingFilter*>(sender());
	if(!filter)
		return;
	int num = filter->property("num").toInt();
	
	if(rating < 0)
		rating = 0;
	
	m_ratings[num] += rating * m_weights[num];
	//qDebug() << num << rating;
	m_filters[num]->setDebugText(tr("%2 %1").arg(m_ratings[num]).arg(m_lastHighNum == num ? " ** LIVE **":""));

	// Hold the camera for approx 7 seconds
	int frameLimit = (int)(filter->calculatedFps() * 7.0);
	
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
		
		//maxNum = 0;
		
		if(m_lastHighNum != maxNum)
		{
			QString highestRatedCon = m_cons[maxNum];
			showCon(highestRatedCon);
// 			GLSceneGroup *group = new GLSceneGroup();
// 			GLScene *scene = new GLScene();
// 			GLVideoInputDrawable *vidgld = new GLVideoInputDrawable();
// 			vidgld->setVideoConnection(highestRatedCon);
// 			vidgld->loadHintsFromSource(); // Honor video hints pre-configured for the input on the server - use the 'hintcal' project to configure hints interactivly
// 			scene->addDrawable(vidgld);
// 			group->addScene(scene);
// 			
// 			m_player->setGroup(group, group->at(0));
			//player->setUserProperty(gld, m_con, "videoConnection");
			
			m_lastHighNum = maxNum;
			
			m_filters[maxNum]->setDebugText(tr("** LIVE ** %1").arg(rating));
			m_ratings[maxNum] = 0;
			
			m_ignoreCountdown = frameLimit * m_ratings.size(); // ignore next X frames * num sources
			qDebug() << "MainWindow::motionRatingChanged: Switching to num:"<<maxNum<<", rated:"<<max<<", not switching for: "<<m_ignoreCountdown;
			
			//group->deleteLater();
		}
		else
		{
			//for(int i=0; i<m_ratings.size(); i++)
				m_ratings[maxNum] = 0; 
				
			m_ignoreCountdown = frameLimit * m_ratings.size(); // ignore next X frames * num sources
			qDebug() << "MainWindow::motionRatingChanged: Max num the same:"<<maxNum<<", rated:"<<max<<", not switching for: "<<m_ignoreCountdown;
		}
	}
	else
	{
		//qDebug() << "m_ignoreCountdown:"<<m_ignoreCountdown;
	}
}
	
void MainWindow::widgetClicked()
{
	QString con = sender()->property("con").toString();
	showCon(con);

	for(int i=0; i<m_ratings.size(); i++)
		m_ratings[i] = 0; 
		
}

void MainWindow::showCon(const QString& con)
{
	
	GLSceneGroup *group = new GLSceneGroup();
	GLScene *scene = new GLScene();
	GLVideoInputDrawable *vidgld = new GLVideoInputDrawable();
	vidgld->setVideoConnection(con);
	vidgld->loadHintsFromSource(); // Honor video hints pre-configured for the input on the server - use the 'hintcal' project to configure hints interactivly
	scene->addDrawable(vidgld);
	group->addScene(scene);
	
	m_player->setGroup(group, group->at(0));
	
	int idx = m_cons.indexOf(con);
	if(idx > -1)
	{
		m_lastHighNum = idx;
		for(int i=0; i<m_ratings.size(); i++)
			if(i!=idx)
				m_ratings[i] = 0; 


		/*for(int i=0; i<m_ratings.size(); i++)
			m_ratings[i] = 0; 
		*/	
		//m_ratings[idx] = 0;	
	}
	
	qDebug() << "MainWindow::showCon: "<<con<<", idx:"<<idx;
	
	group->deleteLater();
}

void MainWindow::showJsonCon(const QString& con, int /*ms*/)
{
	QHash<QString, QString> map = GLVideoInputDrawable::parseConString(con);
	int conNum = map["dev"].toInt();
	if(conNum < 0 || conNum >= m_cons.size())
	{
		qDebug() << "MainWindow::showJsonCon: Invalid con num in con string:"<<con;
		return;
	}
	
	showCon(m_cons[conNum]);

		
}

/// VideoSwitcherJsonServer
VideoSwitcherJsonServer::VideoSwitcherJsonServer(quint16 port, QObject *parent)
	: HttpServer(port,parent)
	, m_jsonOut(new QJson::Serializer())
{

}

void VideoSwitcherJsonServer::dispatch(QTcpSocket *socket, const QStringList &pathElements, const QStringMap &map)
{
	//QString pathStr = path.join("/");

	QStringList path = pathElements;

	QString cmd = path.isEmpty() ? "" : path.takeFirst();


	//QString cmd = map["cmd"].toString();
	qDebug() << "VideoSwitcherJsonServer::receivedMap: [COMMAND]: "<<cmd;

	if(cmd == Server_SetCrossfadeSpeed)
	{
		int ms = map["ms"].toInt();
		emit setFadeSpeed(ms);

		sendReply(socket, QVariantList()
				<< "cmd" << cmd
				<< "status" << true);
	}
	else
	if(cmd == Server_ShowVideoConnection)
	{
		QString con = map["con"];
		
		QString speedStr = map["ms"];
		int fadeSpeedOverride = -1;
		if(!speedStr.isEmpty())
			fadeSpeedOverride = speedStr.toInt();
		
		emit showCon(con, fadeSpeedOverride);
	}
	else
	if(cmd == Server_ListVideoInputs)
	{
		QStringList inputs = m_inputList;
		QVariantList varList;
		foreach(QString str, inputs)
			varList << str;
		QVariant list = varList; // convert to a varient so sendReply() doesnt try to add each element to the map
		sendReply(socket, QVariantList()
				<< "cmd"  << cmd
				<< "list" << list);

	}
	else
	{
		sendReply(socket, QVariantList()
				<< "cmd"     << cmd
				<< "status"  << "error"
				<< "message" << "Unknown command.");
	}
}


void VideoSwitcherJsonServer::sendReply(QTcpSocket *socket, QVariantList reply)
{
	QVariantMap map;
	if(reply.size() % 2 != 0)
	{
		qDebug() << "VideoSwitcherJsonServer::sendReply: [WARNING]: Odd number of elelements in reply: "<<reply;
	}
	
	// Add the API version on all outgoing JSON replies
	reply << "api" << "0.6";

	for(int i=0; i<reply.size(); i+=2)
	{
		if(i+1 >= reply.size())
			continue;

		QString key = reply[i].toString();
		QVariant value = reply[i+1];

		map[key] = value;
	}

	QByteArray json = m_jsonOut->serialize( map );
	qDebug() << "VideoSwitcherJsonServer::sendReply: [DEBUG] map:"<<map<<", json:"<<json;

	Http_Send_Response(socket,"HTTP/1.0 200 OK\r\nContent-Type: text/javascript\r\n\r\n") << json;
}
