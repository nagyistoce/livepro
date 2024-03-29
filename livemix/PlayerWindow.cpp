#include "PlayerWindow.h"

#include <QtGui>

// OpenGl-based canvas
#include "GLWidget.h"
// All our OpenGL-based drawables
#include "GLDrawables.h"

// Very basic scene management for our OpenGL-based canvas
#include "GLSceneGroup.h"

// Expandable list of custom scene types (currently includes news, weather, random video/images, etc.)
#include "GLSceneTypes.h"

// Generic QVariantMap-based server framework
#include "VariantMapServer.h"

// Our command names we'll recognize 
#include "ServerCommandNames.h"

// Video transmission and encoding utilities
#include "VideoInputSenderManager.h"
#include "VideoEncoder.h"
#include "VideoSender.h"

//#include "SharedMemorySender.h"

// Needed to access CameraThread::enableTestSignalGenerator 
#include "CameraThread.h"

#ifndef Q_OS_WIN
// Output to a V4L device (used primarily with vloopback)
#include "V4LOutput.h"
#endif

// Output for a BlackMagic card
#include "BMDOutput.h"

#include <QTimer>
#include <QApplication>

class ScaledGraphicsView : public QGraphicsView
{
public:
	ScaledGraphicsView(QWidget *parent=0) : QGraphicsView(parent)
	{
		setFrameStyle(QFrame::NoFrame);
		setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform );
		setCacheMode(QGraphicsView::CacheBackground);
		setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
		setOptimizationFlags(QGraphicsView::DontSavePainterState);
		setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
		setTransformationAnchor(AnchorUnderMouse);
		setResizeAnchor(AnchorViewCenter);
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	}

protected:
	void resizeEvent(QResizeEvent *)
	{
		adjustViewScaling();
	}

	void adjustViewScaling()
	{
		if(!scene())
			return;

		float sx = ((float)width()) / scene()->width();
		float sy = ((float)height()) / scene()->height();

		float scale = qMin(sx,sy);
		setTransform(QTransform().scale(scale,scale));
		//qDebug("Scaling: %.02f x %.02f = %.02f",sx,sy,scale);
		update();
		//m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatioByExpanding);
		//m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
	}

};



/// PlayerCompatOutputStream

PlayerCompatOutputStream::PlayerCompatOutputStream(PlayerWindow *parent)
	: VideoSource(parent)
	, m_win(parent)
	, m_fps(5)
{
 	setIsBuffered(false);
	setImage(QImage("dot.gif"));

	connect(&m_frameReadyTimer, SIGNAL(timeout()), this, SLOT(renderScene()));
	setFps(m_fps);

	//m_frameReadyTimer.start();

	setAutoDestroy(false);
}

void PlayerCompatOutputStream::consumerRegistered(QObject*)
{
	if(!m_frameReadyTimer.isActive())
		m_frameReadyTimer.start();
}

void PlayerCompatOutputStream::consumerReleased(QObject*)
{
	if(m_consumerList.isEmpty())
		m_frameReadyTimer.stop();
}

void PlayerCompatOutputStream::setImage(QImage img)
{
	m_image = img;

	//qDebug() << "PlayerCompatOutputStream::setImage(): Received frame buffer, size:"<<m_image.size()<<", img format:"<<m_image.format();
	enqueue(new VideoFrame(m_image,1000/m_fps, QTime::currentTime()));

	emit frameReady();
}

void PlayerCompatOutputStream::renderScene()
{
	QImage image(320,240,QImage::Format_ARGB32);
	memset(image.scanLine(0),0,image.byteCount());
	QPainter p(&image);

	//qDebug() << "PlayerCompatOutputStream::renderScene(): Image size:"<<image.size();


	if(m_win->graphicsScene())
		m_win->graphicsScene()->render(&p);
	else
		qDebug() << "PlayerCompatOutputStream::renderScene: No graphics scene available, unable to render.";

	p.end();

// 	qDebug() << "PlayerCompatOutputStream::renderScene: Image size:"<<image.size();
// 	image.save("comapt.jpg");

	setImage(image);
}

void PlayerCompatOutputStream::setFps(int fps)
{
	m_fps = fps;
	m_frameReadyTimer.setInterval(1000/m_fps);
}


/// PlayerWindow

PlayerWindow::PlayerWindow(QWidget *parent)
	: QWidget(parent)
	, m_playerVersionString(tr("LiveMix %1").arg(PLAYER_API_VER))
	, m_playerVersion(15)
	, m_col(0)
	, m_group(0)
	, m_oldGroup(0)
	, m_preloadGroup(0)
	, m_scene(0)
	, m_oldScene(0)
	, m_graphicsView(0)
	, m_glWidget(0)
	, m_useGLWidget(true)
	, m_vidSendMgr(0)
	, m_outputEncoder(0)
	, m_xfadeSpeed(300)
	, m_compatStream(0)
	, m_isBlack(false)
	, m_blackScene(new GLScene)
	, m_configLoaded(false)
	, m_lastConGroup(0)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0,0,0,0);
}


void PlayerWindow::loadConfig(const QString& configFile, bool verbose)
{
	if(verbose)
		qDebug() << "PlayerWindow: Reading settings from "<<configFile;

	QSettings settings(configFile,QSettings::IniFormat);

	QString str;
	QStringList parts;
	QPoint point;

	QString activeGroup = settings.value("config").toString();

	str = settings.value("verbose").toString();
	if(!str.isEmpty())
		verbose = str == "true";

	if(verbose && !activeGroup.isEmpty())
		qDebug() << "PlayerWindow: Using config:"<<activeGroup;

	#define READ_STRING(key,default) \
		(!activeGroup.isEmpty() ? \
			(!(str = settings.value(QString("%1/%2").arg(activeGroup).arg(key)).toString()).isEmpty() ?  str : \
				settings.value(key,default).toString()) : \
			settings.value(key,default).toString())

	#define READ_POINT(key,default) \
		str = READ_STRING(key,default); \
		parts = str.split("x"); \
		point = QPoint(parts[0].toInt(),parts[1].toInt()); \
		if(verbose) qDebug() << "PlayerWindow: " key ": " << point;
		
	
	bool testEnab = READ_STRING("enab-test-gen","false") == "true";
	if(testEnab)
	{
		int numTestChan = READ_STRING("num-test-chan","2").toInt();
		CameraThread::enableTestSignalGenerator(testEnab, numTestChan);
	}
	else
	{
		CameraThread::enableTestSignalGenerator(false,0);
	}
	
	
	bool vidSendEnab = READ_STRING("input-monitor-enabled","true") == "true";
	//WinXP - m_vidSendMgr causes the app to freeze
	#ifdef Q_OS_WIN
        //	if(READ_STRING("force-input-monitor-enabled","false") == "false")
        //		vidSendEnab = false;
 	#endif
 	
 	if(vidSendEnab)
 	{
                qDebug() << "PlayerWindow: Starting Video Input Manager...";
 		m_vidSendMgr = new VideoInputSenderManager();
 		m_vidSendMgr->setSendingEnabled(true);
 	}
        else
        {
                qDebug() << "PlayerWindow: NOT starting input manager.";
        }

	m_useGLWidget = READ_STRING("compat","false") == "false";
	if(m_useGLWidget)
	{
// 		QGLFormat format(QGL::SampleBuffers | QGL::AlphaChannel);
// 		format.setSwapInterval(1); // sync to vblank (vsync)
// 		m_glWidget = new GLWidget(this, 0, format);
		m_glWidget = new GLWidget(this);
		layout()->addWidget(m_glWidget);
		qDebug() << "PlayerWindow: Using OpenGL to provide high-quality graphics.";

		m_glWidget->setCursor(Qt::BlankCursor);
	}
	else
	{
		m_graphicsView = new ScaledGraphicsView();
		m_graphicsScene = new QGraphicsScene();
		m_graphicsView->setScene(m_graphicsScene);
		//m_graphicsView->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
		m_graphicsScene->setSceneRect(QRectF(0,0,1000.,750.));
		m_graphicsView->setBackgroundBrush(Qt::black);
		layout()->addWidget(m_graphicsView);

		qDebug() << "PlayerWindow: Using vendor-provided stock graphics engine for compatibility with older hardware.";

		m_graphicsView->setCursor(Qt::BlankCursor);
	}


	// Window position and size
	READ_POINT("window-pos","10x10");
	QPoint windowPos = point;

	READ_POINT("window-size","640x480");
	QPoint windowSize = point;

	if(verbose)
		qDebug() << "PlayerWindow: pos:"<<windowPos<<", size:"<<windowSize;

	resize(windowSize.x(),windowSize.y());
	move(windowPos.x(),windowPos.y());

	bool frameless = READ_STRING("frameless","true") == "true";
	if(frameless)
		setWindowFlags(Qt::FramelessWindowHint);// | Qt::ToolTip);

	if(m_useGLWidget)
	{

		// Keystoning / Corner Translations
		READ_POINT("key-tl","0x0");
		m_glWidget->setTopLeftTranslation(point);

		READ_POINT("key-tr","0x0");
		m_glWidget->setTopRightTranslation(point);

		READ_POINT("key-bl","0x0");
		m_glWidget->setBottomLeftTranslation(point);

		READ_POINT("key-br","0x0");
		m_glWidget->setBottomRightTranslation(point);

		// Brightness/Contrast, Hue/Sat
		m_glWidget->setBrightness(READ_STRING("brightness","0").toInt());
		m_glWidget->setContrast(READ_STRING("contrast","0").toInt());
		m_glWidget->setHue(READ_STRING("hue","0").toInt());
		m_glWidget->setSaturation(READ_STRING("saturation","0").toInt());

		// Flip H/V
		bool fliph = READ_STRING("flip-h","false") == "true";
		if(verbose)
			qDebug() << "PlayerWindow: flip-h: "<<fliph;
		m_glWidget->setFlipHorizontal(fliph);

		bool flipv = READ_STRING("flip-v","false") == "true";
		if(verbose)
			qDebug() << "PlayerWindow: flip-v: "<<flipv;
		m_glWidget->setFlipVertical(flipv);

		// Rotate
		int rv = READ_STRING("rotate","0").toInt();
		if(verbose)
			qDebug() << "PlayerWindow: rotate: "<<rv;

		if(rv != 0)
			m_glWidget->setCornerRotation(rv == -1 ? GLRotateLeft  :
						      rv ==  1 ? GLRotateRight :
							         GLRotateNone);

		// Aspet Ratio Mode
		m_glWidget->setAspectRatioMode(READ_STRING("ignore-ar","false") == "true" ? Qt::IgnoreAspectRatio : Qt::KeepAspectRatio);

		// Alpha Mask
		QString alphaFile = READ_STRING("alphamask","");
		if(!alphaFile.isEmpty())
		{
			QImage alphamask(alphaFile);
			if(alphamask.isNull())
				qDebug() << "PlayerWindow: Error loading alphamask "<<alphaFile;
			else
				m_glWidget->setAlphaMask(alphamask);
		}
	}

	m_validUser = READ_STRING("user","player");
	m_validPass = READ_STRING("pass","player");

	// Canvas Size
	READ_POINT("canvas-size","1000x750");
	if(m_useGLWidget)
	{
		m_glWidget->setCanvasSize(QSizeF((qreal)point.x(),(qreal)point.y()));
	}
	else
	{
		QRectF r = QRectF(0,0,(qreal)point.x(),(qreal)point.y());
		m_graphicsScene->setSceneRect(r);
	}

	m_xfadeSpeed = READ_STRING("xfade-speed",300).toInt();
	//qDebug() << "PlayerWindow: Crossfade speed: "<<m_xfadeSpeed;


	m_server = new VariantMapServer();

	if (!m_server->listen(QHostAddress::Any,9977))
	{
		qDebug() << "VariantMapServer could not start: "<<m_server->errorString();
	}
	else
	{
		qDebug() << "VariantMapServer listening on "<<m_server->myAddress();
	}

	connect(m_server, SIGNAL(receivedMap(QVariantMap)), this, SLOT(receivedMap(QVariantMap)));



// 	QString outputFile = READ_STRING("output","output.avi");
// 	if(!outputFile.isEmpty())
// 	{
// 		if(!m_glWidget)
// 		{
// 			qDebug() << "PlayerWindow: Unable to output video to"<<outputFile<<" because OpenGL is not enabled. (Set comapt=false in player.ini to use OpenGL)";
// 		}
//
// 		QFileInfo info(outputFile);
// 		if(info.exists())
// 		{
// 			int counter = 1;
// 			QString newFile;
// 			while(QFileInfo(newFile = QString("%1-%2.%3")
// 				.arg(info.baseName())
// 				.arg(counter)
// 				.arg(info.completeSuffix()))
// 				.exists())
//
// 				counter++;
//
// 			qDebug() << "PlayerWindow: Video output file"<<outputFile<<"exists, writing to"<<newFile<<"instead.";
// 			outputFile = newFile;
// 		}
//
// 		m_outputEncoder = new VideoEncoder(outputFile, this);
// 		m_outputEncoder->setVideoSource(m_glWidget->outputStream());
// 		//m_outputEncoder->startEncoder();
// 	}

	if(!m_glWidget)
                m_compatStream = new PlayerCompatOutputStream(this);

	// TODO Why disable V4L/BMD output if not using OpenGL...? - JB 20110223
// 	if(m_glWidget) // || m_compatStream)
// 	{
		//m_shMemSend = new SharedMemorySender("PlayerWindow-2",this);
		//m_shMemSend->setVideoSource(m_glWidget->outputStream());
		m_v4lOutput = 0;

		#ifndef Q_OS_WIN
		bool v4l_out_enab = READ_STRING("v4l-output-enabled","false") == "true";

		/// NB: v4l output is designed to be used with vloopback (http://www.lavrsen.dk/svn/vloopback/trunk or ./vloopback)
		/// To use with vloopback, first build the kernel mod in vloopback and do 'insmod vloopback.ko', then
		/// make sure you adjust the output device to the device shown in dmesg or /var/log/messages as the output
		/// device.
		
		// Just testing CPU usage...
		//(void*)m_glWidget->outputStream();

		if(v4l_out_enab)
		{
			QString v4lOutputDev = READ_STRING("v4l-output","/dev/video2");

			m_v4lOutput = new V4LOutput(v4lOutputDev);
			m_v4lOutput->setVideoSource(m_glWidget ? (VideoSource*)m_glWidget->outputStream() : (VideoSource*)m_compatStream);

			qDebug() << "PlayerWindow: Streaming to V4L device: "<<v4lOutputDev;
		}
		#else
		qDebug() << "PlayerWindow: Unable to stream via V4L because this player was compiled for Windows.";
		#endif
		
		bool bmd_out_enab = READ_STRING("bmd-output-enabled","false") == "true";
		
		if(bmd_out_enab)
		{
			QString bmdOutputDev = READ_STRING("bmd-output","bmd:0");
		
			BMDOutput *bmd = new BMDOutput(bmdOutputDev);
			bmd->setVideoSource(m_glWidget ? (VideoSource*)m_glWidget->outputStream() : (VideoSource*)m_compatStream);
			qDebug() << "PlayerWindow: Streaming to BMD output: "<<bmdOutputDev;
		}
		
// 	}
// 	else
// 	{
// 		//m_shMemSend = 0;
// 		//qDebug() << "Playerwindow: Unable to stream output via shared memory because OpenGL is not enabled.";
// 
// 		m_v4lOutput = 0;
// 		qDebug() << "Playerwindow: Unable to stream output via V4L because OpenGL is not enabled or no output stream available (m_compatStream)";
// 	}


	QString loadGroup = READ_STRING("load-group","");
	if(!loadGroup.isEmpty())
	{
		QFile file(loadGroup);
		if (!file.open(QIODevice::ReadOnly))
		{
			qDebug() << "PlayerWindow: Unable to read group file: "<<loadGroup;
		}
		else
		{
			QByteArray array = file.readAll();

			GLSceneGroup *group = new GLSceneGroup();
			group->fromByteArray(array);
			setGroup(group);

			GLScene *scene = group->at(0);
			if(scene)
			{
				//scene->setGLWidget(this);
				displayScene(scene);
				qDebug() << "PlayerWindow: [DEBUG]: Loaded File: "<<loadGroup<<", GroupID: "<<group->groupId()<<", SceneID: "<< scene->sceneId();

				if(m_outputEncoder &&
				  !m_outputEncoder->encodingStarted())
					m_outputEncoder->startEncoder();

			}
			else
			{
				qDebug() << "PlayerWindow: [DEBUG]: Loaded File: "<<loadGroup<<", GroupID: "<<group->groupId()<<" - no scenes found at index 0";
			}
		}
	}
	else
	{
		QString loadGroup = READ_STRING("collection","");
		if(!loadGroup.isEmpty())
		{
			QFile file(loadGroup);
			if (!file.open(QIODevice::ReadOnly))
			{
				qDebug() << "PlayerWindow: Unable to read group file: "<<loadGroup;
			}
			else
			{
				QByteArray array = file.readAll();

				GLSceneGroupCollection *collection = new GLSceneGroupCollection();
				collection->fromByteArray(array);
				setGroup(collection->at(0));

				if(m_group)
				{
					GLScene *scene = m_group->at(0);
					if(scene)
					{
// 						GLSceneTypeCurrentWeather *weather = new GLSceneTypeCurrentWeather();
// 						weather->setLocation("47390");
//
// 						GLSceneType::AuditErrorList errors = weather->auditTemplate(scene);
// 						if(!errors.isEmpty())
// 						{
// 							qDebug() << "PlayerWindow: [DEBUG]: Errors found attaching weather type to scene:";
// 							foreach(GLSceneType::AuditError error, errors)
// 							{
// 								qDebug() << "PlayerWindow: [DEBUG]: \t" << error.toString();
// 							}
// 						}
//
// 						if(!weather->attachToScene(scene))
// 						{
// 							qDebug() << "PlayerWindow: [DEBUG]: weather->attachToScene() returned FALSE";
// 						}

						//scene->setGLWidget(this);
						
						//// JUST for TESTING video preroll performance
						//m_testScene = scene;
						//qDebug() << "PlayerWindow: [DEBUG]: Starting 5sec timer till scene goes live to allow video to settle";
						//QTimer::singleShot(5000, this, SLOT(setTestScene()));
						
						displayScene(scene);
						
						qDebug() << "PlayerWindow: [DEBUG]: Loaded File: "<<loadGroup<<", GroupID: "<<m_group->groupId()<<", SceneID: "<< scene->sceneId();

						if(m_outputEncoder &&
						  !m_outputEncoder->encodingStarted())
							m_outputEncoder->startEncoder();
					}
					else
					{
						qDebug() << "PlayerWindow: [DEBUG]: Loaded File: "<<loadGroup<<", GroupID: "<<m_group->groupId()<<" - no scenes found at index 0";
					}
				}
			}
		}
	}

	int outputPort = READ_STRING("output-port",9978).toInt();

	if(outputPort > 0 )
	{
		VideoSender *sender = new VideoSender(this);
		sender->setTransmitFps(15);
		sender->setTransmitSize(320,240);
		//sender->setTransmitFps(5);
		sender->setVideoSource(m_compatStream ? (VideoSource*)m_compatStream : (VideoSource*)m_glWidget->outputStream());
		if(sender->listen(QHostAddress::Any,outputPort))
		{
			qDebug() << "PlayerWindow: Live monitor available on port"<<outputPort;
		}
		else
		{
			qDebug() << "PlayerWindow: [ERROR] Unable start Live Monitor server on port"<<outputPort;
		}
	}

	int jsonPort = READ_STRING("json-port",9979).toInt();

	if(jsonPort > 0 )
		m_jsonServer = new PlayerJsonServer(jsonPort, this);


// 	// Send test map back to self
// 	QTimer::singleShot(50, this, SLOT(sendTestMap()));

	/// NB REMOVE FOR PRODUCTION
	/// NB REMOVE FOR PRODUCTION
	/// NB REMOVE FOR PRODUCTION
	/// NB REMOVE FOR PRODUCTION
	/// NB REMOVE FOR PRODUCTION
	/// NB REMOVE FOR PRODUCTION
	/// NB REMOVE FOR PRODUCTION
	/// NB REMOVE FOR PRODUCTION

	/// THIS IS ONLY TO TEST FILE SIZES AND ENCODER PERFORMANCE

	//QTimer::singleShot(1000 * 60, qApp, SLOT(quit()));

	/// THIS IS ONLY TO TEST FILE SIZES AND ENCODER PERFORMANCE

	/// NB REMOVE FOR PRODUCTION
	/// NB REMOVE FOR PRODUCTION
	/// NB REMOVE FOR PRODUCTION
	/// NB REMOVE FOR PRODUCTION
	/// NB REMOVE FOR PRODUCTION
	/// NB REMOVE FOR PRODUCTION
	/// NB REMOVE FOR PRODUCTION
	/// NB REMOVE FOR PRODUCTION

	//QTimer::singleShot(1000, this, SLOT(setBlack()));
	
	
 	m_overlays = new GLSceneGroup();
 	
 	QString dev = "test:/opt/livepro/devel/data/2012-01-08 SS Test/test1.mpg";
 	
// 	GLScene *scene = new GLScene();
// 	//GLImageDrawable *img = new GLImageDrawable();
// 	//img->setImageFile("Pm5544.jpg");
// 	//img->setRect(QRectF(10,10,200,200));
// 	GLVideoInputDrawable *img = new GLVideoInputDrawable(dev);
// 	img->setRect(QRectF(10,10,200,200));
// 	
// 	scene->addDrawable(img);
// 	
// 	addOverlay(scene);
	
// 	QString con = tr("dev=%1").arg(dev); //,net=localhost:7755";
// 	
// 	qDebug() << "Composing test scene group";
// 	
// 	GLSceneGroup *group = new GLSceneGroup();
// 	GLScene *scene = new GLScene();
// 	GLVideoInputDrawable *vidgld = new GLVideoInputDrawable(); //dev);
// 	vidgld->setVideoConnection(con);
// 	vidgld->setRect(QRectF(10,10,200,200));
// 	scene->addDrawable(vidgld);
// 	
// 	qDebug() << "Done composing, adding overlay";
// 	
// 	int fadeSpeedOverride = 0;
// 	if(setGroup(group))
// 		setScene(scene, fadeSpeedOverride);
// 	//addOverlay(scene);
// 	
}

void PlayerWindow::setTestScene()
{
	qDebug() << "PlayerWindow::setTestScene: [DEBUG]: Loading test scene now";
	displayScene(m_testScene);
	
}

void PlayerWindow::sendReply(QVariantList reply)
{
	QVariantMap map;
	if(reply.size() % 2 != 0)
	{
		qDebug() << "PlayerWindow::sendReply: [WARNING]: Odd number of elelements in reply: "<<reply;
	}
	
	// Add the API version on all outgoing replies
	reply << "api" << PLAYER_API_VER;

	for(int i=0; i<reply.size(); i+=2)
	{
		if(i+1 >= reply.size())
			continue;

		QString key = reply[i].toString();
		QVariant value = reply[i+1];

		map[key] = value;
	}


	//qDebug() << "PlayerWindow::sendReply: [DEBUG] map:"<<map;
	m_server->sendMap(map);
}


//! Process the QVariantMap received from a client connected to the VariantMapServer.
// Start by processing the 'cmd' parameter as a string, matching it to a command name defined in ServerCommandNames.h
void PlayerWindow::receivedMap(QVariantMap map)
{
	//qDebug() << "PlayerWindow::receivedMap: "<<map;

	QString cmd = map["cmd"].toString();
	
	if(cmd != Server_DeadPing)
		qDebug() << "PlayerWindow::receivedMap: [COMMAND]: "<<cmd;

	if(cmd == Server_Login)
	{
		if(map["user"].toString() != m_validUser ||
		   map["pass"].toString() != m_validPass)
		{
			qDebug() << "PlayerWindow::receivedMap: ["<<cmd<<"] Invalid user/pass combo";

			sendReply(QVariantList()
				<< "cmd" << Server_Login
				<< "status" << "error"
				<< "message" << "Invalid username/password.");
		}
		else
		{
			m_loggedIn = true;
			sendReply(QVariantList()
				<< "cmd" << Server_Login
				<< "status" << "success"
				<< "version" << m_playerVersionString
				<< "ver" << m_playerVersion);


			if(m_outputEncoder &&
			  !m_outputEncoder->encodingStarted())
				m_outputEncoder->startEncoder();

		}
	}
	else
	if(cmd == Server_Ping)
	{
		if(m_vidSendMgr)
		{
			QStringList inputs = m_vidSendMgr->videoConnections();

			sendReply(QVariantList()
					<< "cmd" << Server_Ping
					<< "version" << m_playerVersionString
					<< "ver" << m_playerVersion
					<< "inputs" << inputs.size());
		}
		else
		{
			sendReply(QVariantList()
					<< "cmd" << Server_Ping
					<< "version" << m_playerVersionString
					<< "ver" << m_playerVersion
					<< "inputs" << -1);
		}
	}
	else
	if(!m_loggedIn)
	{
		sendReply(QVariantList()
				<< "cmd" << Server_Login
				<< "status" << "error"
				<< "message" << "Not logged in, command will not succeed.");
	}
	else
	if(cmd == Server_SetBlackout)
	{
// 		if(m_glWidget)
// 			m_glWidget->fadeBlack(map["flag"].toBool());
		bool flag = map["flag"].toBool();
		setBlack(flag);

		sendReply(QVariantList()
				<< "cmd" << Server_SetBlackout
				<< "status" << true);
	}
	else
	if(cmd == Server_SetCrossfadeSpeed)
	{
		int ms = map["ms"].toInt();
		setCrossfadeSpeed(ms);

		sendReply(QVariantList()
				<< "cmd" << cmd
				<< "status" << true);
	}
	else
	if(cmd == Server_AddOverlay)
	{
		QByteArray ba = map["data"].toByteArray();
		GLScene *scene = new GLScene(ba);
		
		if(GLScene *old = m_overlays->lookupScene(scene->sceneId()))
		{
			//m_overlays->removeScene(old);
			removeOverlay(old);
		}
		
		addOverlay(scene);

		sendReply(QVariantList()
				<< "cmd" << cmd
				<< "status" << true);
	}
	else
	if(cmd == Server_RemoveOverlay)
	{
		int id = map["id"].toInt();
		
		if(GLScene *old = m_overlays->lookupScene(id))
		{
			removeOverlay(old);
			sendReply(QVariantList()
					<< "cmd" << cmd
					<< "status" << true);
		}
		else
		{
			sendReply(QVariantList()
					<< "cmd" << cmd
					<< "status" << "error"
					<< "message" << "Invalid overlay ID");
		
		}
	}
	else
	if(cmd == Server_LoadSlideGroup ||
	   cmd == Server_PreloadSlideGroup)
	{
		QByteArray ba = map["data"].toByteArray();
		
		if(cmd == Server_PreloadSlideGroup)
		{
			if(m_preloadGroup)
			{
				delete m_preloadGroup;
				m_preloadGroup = 0;
			}
			
			m_preloadGroup = new GLSceneGroup(ba);
		}
		else
		{
			GLSceneGroup *group  = 0;
			if(m_preloadGroup)
			{
				// If we preloaded a group,
				// check to see if this LoadSlideGroupCall is for the group we preloaded.
				// If it *IS* then use the preloaded group pointer and DON'T even 
				// create a new GLSceneGroup.
				// If the groupId's DO NOT match, then delete the preloaded 
				// group and move on with life.
				if(map.contains("groupid") &&
				   map["groupid"].toInt() == m_preloadGroup->groupId())
				{
					group = m_preloadGroup;
					m_preloadGroup = 0;
				}
				else
				{
					delete m_preloadGroup;
					m_preloadGroup = 0;
				}
			}
			
			if(!group)
				group = new GLSceneGroup(ba);
			
			if(setGroup(group) && 
			   group->size() > 0)
				setScene(group->at(0));
		}
		
		sendReply(QVariantList()
				<< "cmd" << cmd
				<< "status" << true);
	}
	else
	if(cmd == Server_SetSlide)
	{
		if(!m_group)
		{
			sendReply(QVariantList()
					<< "cmd" << Server_SetSlide
					<< "status" << "error"
					<< "message" << "No group loaded. First transmit Server_LoadSlideGroup before calling Server_SetSlide.");
		}
		else
		{
			int id = map["sceneid"].toInt();
			GLScene *scene = m_group->lookupScene(id);
			if(!scene)
			{
				sendReply(QVariantList()
					<< "cmd" << Server_SetSlide
					<< "status" << "error"
					<< "message" << "Invalid SceneID");
			}
			else
			{
				setScene(scene);

				sendReply(QVariantList()
					<< "cmd" << Server_SetSlide
					<< "status" << true);
			}
		}
	}
	else
	if(cmd == Server_SetLayout)
	{
		/// TODO implement layout setting
		sendReply(QVariantList()
				<< "cmd" << Server_SetLayout
				<< "status" << true);
	}
	else
	if(cmd == Server_SetUserProperty    ||
	   cmd == Server_SetVisibility      ||
	   cmd == Server_QueryProperty      ||
	   cmd == Server_SetPlaylistPlaying ||
	   cmd == Server_UpdatePlaylist     ||
	   cmd == Server_SetPlaylistTime)
	{
		if(!m_scene)
		{
			sendReply(QVariantList()
					<< "cmd" << cmd
					<< "status" << "error"
					<< "message" << QString("No scene selected. First transmit Server_SetSlide before sending %1").arg(cmd));
		}
		else
		{
			int id = map["drawableid"].toInt();
			GLDrawable *gld = m_scene->lookupDrawable(id);
			if(!gld)
			{
				foreach(GLScene *scene, m_overlays->sceneList())
					if((gld = scene->lookupDrawable(id)) != NULL)
						break;
			}
			
			if(!gld)
			{
				sendReply(QVariantList()
					<< "cmd" << cmd
					<< "status" << "error"
					<< "message" << "Invalid DrawableID");
			}
			else
			{
				if(cmd == Server_SetPlaylistPlaying)
				{
				 	bool flag = map["flag"].toBool();
				 	gld->playlist()->setIsPlaying(flag);

				}
				else
				if(cmd == Server_UpdatePlaylist)
				{
					QByteArray ba = map["data"].toByteArray();
					gld->playlist()->fromByteArray(ba);
				}
				else
				if(cmd == Server_SetPlaylistTime)
				{
					double time = map["time"].toDouble();
					gld->playlist()->setPlayTime(time);
				}
				else
				{
					QString name =
						cmd == Server_SetVisibility ? "isVisible" :
						map["name"].toString();

					if(name.isEmpty())
					{
						name = QString(gld->metaObject()->userProperty().name());
					}

					if(name.isEmpty())
					{
						sendReply(QVariantList()
							<< "cmd" << cmd
							<< "status" << "error"
							<< "message" << "No property name given in 'name', and could not find a USER-flagged Q_PROPERTY on the GLDrawable requested by 'drawableid'.");
					}
					else
					{
						if(cmd == Server_QueryProperty)
						{
							sendReply(QVariantList()
								<< "cmd" << cmd
								<< "drawableid" << id
								<< "name" << name
								<< "value" << gld->property(qPrintable(name)));
						}
						else
						{
							QVariant value = map["value"];

							GLVideoDrawable *vid = dynamic_cast<GLVideoDrawable*>(gld);
							if(vid)
								vid->setXFadeLength(m_xfadeSpeed);
								
							QVariant::Type propType = gld->metaObject()->property(gld->metaObject()->indexOfProperty(qPrintable(name))).type();
							if((propType == QVariant::Int ||
							    propType == QVariant::Double ||
							    propType == QVariant::PointF ||
							    propType == QVariant::Point ||
							    propType == QVariant::RectF ||
							    propType == QVariant::Rect) 
							    /*&&
							    (name != "sharpAmount")*/)
							{
								QAbsoluteTimeAnimation *anim = new QAbsoluteTimeAnimation(gld, qPrintable(name));
								anim->setEndValue(value);
								anim->setDuration((int)xfadeSpeed());
								anim->start(QAbstractAnimation::DeleteWhenStopped);
								
								gld->registerAbsoluteTimeAnimation(anim);
								qDebug() << "PlayerWindow: Set Property:"<<name<<" - "<<value<<" - Animated, Duration:"<<xfadeSpeed();
							
							}
							else
							{
								
								qDebug() << "PlayerWindow: Set Property:"<<name<<" - "<<value;
								gld->setProperty(qPrintable(name), value);
							}	

							//gld->setProperty(qPrintable(name), value);

							sendReply(QVariantList()
								<< "cmd" << cmd
								<< "status" << true);
						}
					}
				}
			}
		}
	}
	else
	if(cmd == Server_DownloadFile)
	{
		/// TODO implement download file
		sendReply(QVariantList()
				<< "cmd" << Server_DownloadFile
				<< "status" << true);
	}
	else
	if(cmd == Server_QueryLayout)
	{
		/// TODO implement query layout
		sendReply(QVariantList()
				<< "cmd" << Server_QueryLayout
				<< "status" << true);
	}
	else
	if(cmd == Server_SetViewport)
	{
		QRectF rect = map["viewport"].toRectF();
		setViewport(rect);

		sendReply(QVariantList()
				<< "cmd" << cmd
				<< "status" << true);
	}
	else
	if(cmd == Server_SetCanvasSize)
	{
		QSizeF size = map["canvas"].toSizeF();
		setCanvasSize(size);

		sendReply(QVariantList()
				<< "cmd" << cmd
				<< "status" << true);
	}
	else
	if(cmd == Server_SetIgnoreAR)
	{
		bool flag = map["flag"].toBool();
		setIgnoreAR(flag);

		sendReply(QVariantList()
				<< "cmd" << cmd
				<< "status" << true);
	}
	else
	if(cmd == Server_SetScreen)
	{
		QRect size = map["rect"].toRect();
		setScreen(size);

		sendReply(QVariantList()
				<< "cmd" << cmd
				<< "status" << true);
	}
	else
	if(cmd == Server_AddSubview)
	{
		if(m_glWidget)
		{
			GLWidgetSubview *view = new GLWidgetSubview();

			QByteArray ba = map["data"].toByteArray();
			view->fromByteArray(ba);

			addSubview(view);
		}

		sendReply(QVariantList()
				<< "cmd" << cmd
				<< "status" << true);
	}
	else
	if(cmd == Server_RemoveSubview)
	{
		if(m_glWidget)
		{
			int subviewId = map["subviewid"].toInt();
			if(subviewId>0)
			{
				removeSubview(subviewId);
			}
			else
			{
				GLWidgetSubview *view = new GLWidgetSubview();

				QByteArray ba = map["data"].toByteArray();
				view->fromByteArray(ba);

				removeSubview(view->subviewId());

				delete view;
			}
		}

		sendReply(QVariantList()
				<< "cmd" << cmd
				<< "status" << true);
	}
	else
	if(cmd == Server_ClearSubviews)
	{
		clearSubviews();

		sendReply(QVariantList()
				<< "cmd" << cmd
				<< "status" << true);
	}
	else
	if(cmd == Server_ListVideoInputs)
	{
		QStringList inputs = videoInputs();
		QVariantList varList;
		foreach(QString str, inputs)
			varList << str;
		QVariant list = varList; // convert to a varient so sendReply() doesnt try to add each element to the map
		sendReply(QVariantList()
				<< "cmd"  << cmd
				<< "list" << list);

	}
	else
	if(cmd == Server_DeadPing)
	{
		sendReply(QVariantList()
				<< "cmd" << cmd);
	}
	else
	{
		sendReply(QVariantList()
				<< "cmd" << cmd
				<< "status" << "error"
				<< "message" << "Unknown command.");
	}


// 	if(map["cmd"].toString() != "ping")
// 	{
// 		QVariantMap map2;
// 		map2["cmd"] = "ping";
// 		map2["text"] = "Got a map!";
// 		map2["bday"] = 101010;
//
// 		qDebug() << "PlayerWindow::receivedMap: Sending response: "<<map2;
// 		m_server->sendMap(map2);
// 	}
}

void PlayerWindow::addSubview(GLWidgetSubview *view)
{
	if(GLWidgetSubview *oldSubview = m_glWidget->subview(view->subviewId()))
	{
		qDebug() << "PlayerWindow::addSubview: Deleting oldSubview: "<<oldSubview;
		m_glWidget->removeSubview(oldSubview);
		delete oldSubview;
	}
	else
	{
		qDebug() << "PlayerWindow::addSubview: NO OLD SUBVIEW DELETED, id:"<<view->subviewId();
	}

	qDebug() << "PlayerWindow::addSubview: Added subview: "<<view<<", id: "<<view->subviewId();
	m_glWidget->addSubview(view);
}

void PlayerWindow::removeSubview(int id)
{
	if(!m_glWidget)
		return;

	if(GLWidgetSubview *oldSubview = m_glWidget->subview(id))
	{
		m_glWidget->removeSubview(oldSubview);
		delete oldSubview;
	}
}

QStringList PlayerWindow::videoInputs()
{
	return m_vidSendMgr ?
		m_vidSendMgr->videoConnections() :
		QStringList();
}

void PlayerWindow::clearSubviews()
{
	if(!m_glWidget)
		return;

	QList<GLWidgetSubview*> views = m_glWidget->subviews();
	foreach(GLWidgetSubview *view, views)
	{
		m_glWidget->removeSubview(view);
		delete view;
	}
}

void PlayerWindow::setCanvasSize(const QSizeF& size)
{
	if(m_glWidget)
		m_glWidget->setCanvasSize(size);
	else
		m_graphicsScene->setSceneRect(QRectF(QPointF(0,0),size));
}

void PlayerWindow::setViewport(const QRectF& rect)
{
	if(m_glWidget)
		m_glWidget->setViewport(rect);
	//else
		//m_graphicsScene->setSceneRect(QRectF(0,0,(qreal)point.x(),(qreal)point.y()));
}

void PlayerWindow::setScreen(QRect size)
{
	if(!size.isEmpty())
	{
		move(size.x(),size.y());
		resize(size.width(),size.height());
		raise();
	}
}

void PlayerWindow::setIgnoreAR(bool flag)
{
	if(m_glWidget)
		m_glWidget->setAspectRatioMode(flag ? Qt::IgnoreAspectRatio : Qt::KeepAspectRatio);
}

void PlayerWindow::setScene(GLScene *scene, int fadeSpeedOverride)
{
	if(m_isBlack)
		m_scenePreBlack = scene;
	else
		displayScene(scene, fadeSpeedOverride);
}

void PlayerWindow::setCrossfadeSpeed(int ms)
{
	//qDebug() << "PlayerWindow::setCrossfadeSpeed: "<<ms;
	if(m_glWidget)
		m_glWidget->setCrossfadeSpeed(ms);

	if(m_scene)
	{
		GLDrawableList list = m_scene->drawableList();
		foreach(GLDrawable *drawable, list)
			if(GLVideoDrawable *vid = dynamic_cast<GLVideoDrawable*>(drawable))
				vid->setXFadeLength(ms);
	}
	
	foreach(GLScene *scene, m_overlays->sceneList())
	{
		GLDrawableList list = scene->drawableList();
		foreach(GLDrawable *drawable, list)
			if(GLVideoDrawable *vid = dynamic_cast<GLVideoDrawable*>(drawable))
				vid->setXFadeLength(ms);
	}

	m_xfadeSpeed = ms;
}

void PlayerWindow::setBlack(bool flag)
{
        //qDebug() << "PlayerWindow::setBlack: blackout flag:"<<flag<<", m_isBlack:"<<m_isBlack;
	if(flag == m_isBlack)
		return;

	m_isBlack = flag;

	if(flag)
	{
		m_scenePreBlack = m_scene;
		displayScene(m_blackScene);
		
		foreach(GLScene *scene, m_overlays->sceneList())
			scene->setOpacity(0,true,m_xfadeSpeed); // animate fade out
	}
	else
	{
		displayScene(m_scenePreBlack);
		
		foreach(GLScene *scene, m_overlays->sceneList())
			scene->setOpacity(1,true,m_xfadeSpeed); // animate fade in
	}
}

// Given a video connection string, display it on screen
// Used by the JSON server
void PlayerWindow::showVideoConnection(const QString& con, int fadeSpeedOverride)
{
	// Don't create a new group/scene if the last one we made matches this con
	if(m_lastConGroup &&
	   m_lastConGroup->at(0) && // get first scene
	   m_lastConGroup->at(0)->at(0) && // get first drawable
	   m_lastConGroup
	   	->at(0) // scene
	   	->at(0) // drawable
	   	->property("videoConnection") // load videoConnection property
	   	 .toString() == con) // convert to string and compare
	{
		// Don't re-call setGroup() if this con's group is already live
		if(m_group &&
		   m_lastConGroup == m_group &&
		   m_lastConGroup->groupId() == m_group->groupId())
		   	return;
		
		// Set this con's group live
		if(setGroup(m_lastConGroup))
			setScene(m_lastConGroup->at(0));
		
		return;
	}
	
	GLSceneGroup *group = new GLSceneGroup();
	GLScene *scene = new GLScene();
	GLVideoInputDrawable *vidgld = new GLVideoInputDrawable();
	vidgld->setVideoConnection(con);
	scene->addDrawable(vidgld);
	
	if(setGroup(group))
		setScene(scene, fadeSpeedOverride);
		
	m_lastConGroup = group;
}

// void PlayerWindow::sendTestMap()
// {
// 	qDebug() << "PlayerWindow::sendTestMap: Connecting...";
// 	m_client = new VariantMapClient();
// 	m_client->connectTo("localhost",9977);
//
// 	connect(m_client, SIGNAL(socketConnected()), this, SLOT(slotConnected()));
// 	connect(m_client, SIGNAL(receivedMap(QVariantMap)), this, SLOT(receivedMap(QVariantMap)));
// }
//
// void PlayerWindow::slotConnected()
// {
// 	QVariantMap map;
// 	map["cmd"] = "testConn";
// 	map["text"] = "Hello, World!";
// 	map["bday"] = 103185;
//
// 	qDebug() << "PlayerWindow::slotConnected: Connected, sending map: "<<map;
// 	m_client->sendMap(map);
// }

void PlayerWindow::setCollection(GLSceneGroupCollection *col)
{
	if(m_col)
	{
		delete m_col;
		m_col = 0;
	}

	m_col = col;
}

bool PlayerWindow::setGroup(GLSceneGroup *group)
{
	//qDebug() << "PlayerWindow::setGroup(): start";
	if(m_group && group &&
	   m_group->groupId() == group->groupId() &&
	   m_group != group)
	{
		// They are trying to set the same group,
		// group ID's match, but pointers differ.
		// Since ::setGroup() is supposed to take
		// ownership of the group, we will delete
		// the new group here so we don't leak.
		if(group != m_group)
			delete group;
			
		group = 0;
		return false;
	}
	
	if(group == m_group)
		return false;
			
	if(m_group)
	{
		disconnect(m_group->playlist(), 0, this, 0);
		if(m_scene && 
		   m_group->lookupScene(m_scene->sceneId()))
		{
			// active scene is in this group, dont delete till active scene released;
			if(m_oldGroup && 
			   m_oldGroup != group)
			{
				//qDebug()<<"PlayerWindow::setGroup: leaking memory via m_oldGroup - "<<m_group<<" !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
				m_oldGroup->deleteLater();
			}
			
			m_oldGroup = m_group;
			m_group = 0; // dont delete because it will be deleted via m_oldgroup later
			
//			qDebug()<<"PlayerWindow::setGroup: m_scene "<<m_scene<<" found in group "<<m_group<<", delaying deletion.";
		}
		else
		{
//			qDebug()<<"PlayerWindow::setGroup: m_scene "<<m_scene<<" not in this group "<<m_group<<", deleting.";
			delete m_group;
			m_group = 0;
			m_oldGroup = 0;
		}
	}

	m_group = group;

	if(m_group)
	{
		connect(m_group->playlist(), SIGNAL(currentItemChanged(GLScene*)), this, SLOT(setScene(GLScene*)));
		m_group->playlist()->play();
	}
	
	//qDebug() << "PlayerWindow::setGroup(): end";
	return true;
}

void PlayerWindow::displayScene(GLScene *scene, int fadeSpeedOverride)
{
	//qDebug() << "PlayerWindow::displayScene: New scene:"<<scene;
	if(scene == m_scene)
	{
		qDebug() << "PlayerWindow::displayScene: Scene pointers match, not setting new scene";
		return;
	}

// 	if(scene   && 
// 	   m_scene &&
// 	   scene->sceneId() == m_scene->sceneId())
// 	{
// 		qDebug() << "PlayerWindow::displayScene: Scene IDs match, not setting new scene.";
// 		return;
// 	}

	if(m_oldScene)
	{
		//qDebug() << "PlayerWindow::displayScene: Has old scene, finishing that one first";
		opacityAnimationFinished(m_oldScene);
	}

	if(m_scene)
		m_oldScene = m_scene;
		
	m_scene = scene;

	if(m_group)
		m_group->playlist()->setCurrentItem(m_scene);

	int oldFadeSpeed = -1;
	if(fadeSpeedOverride > -1)
	{
		oldFadeSpeed = m_xfadeSpeed; 
		m_xfadeSpeed = fadeSpeedOverride;
	}
	
	//qDebug() << "PlayerWindow::displayScene: Adding in new scene";
	addScene(m_scene, 1, true, m_oldScene);
	//qDebug() << "PlayerWindow::displayScene: starting removal of old scene";
	removeScene(m_oldScene);//, m_scene); // it will use m_scene as the 'fade leader' - eg wait for m_scene to start fading in before fading out
	
	if(fadeSpeedOverride > -1)
		m_xfadeSpeed = oldFadeSpeed;
	//qDebug() << "PlayerWindow::displayScene: Done with displayScene()";
}

void PlayerWindow::addOverlay(GLScene *scene)
{
	if(!scene)
		return;
		
	if(m_overlays->lookupScene(scene->sceneId()))
	{
		qDebug() << "PlayerWindow::addOverlay: overlay"<<scene<<"already present in window, not readding";
		return;
	}
	
	m_overlays->addScene(scene);
	
	addScene(scene, m_overlays->size() + 1, !m_isBlack);
}

void PlayerWindow::removeOverlay(GLScene *scene)
{
	if(!m_overlays->lookupScene(scene->sceneId()))
	{
		qDebug() << "PlayerWindow::addOverlay: overlay"<<scene<<"not present in window, nothing to remove";
		return;
	}
	
	m_overlays->removeScene(scene);
	removeScene(scene);
}

void PlayerWindow::removeScene(GLScene *scene, GLScene *fadeLeader)
{
	if(scene)
	{
		GLDrawableList list = scene->drawableList();
		foreach(GLDrawable *gld, list)
			if(gld->playlist()->size() > 0)
				gld->playlist()->stop();
				
		if(m_xfadeSpeed > 0)
		{
			qDebug() << "PlayerWindow::removeScene: fade out scene:"<<scene<<" at "<<m_xfadeSpeed<<"ms, fadeLeader: "<<fadeLeader;
			scene->setFadeSyncLeader(fadeLeader);
			scene->setOpacity(0,true,m_xfadeSpeed); // animate fade out
			// remove drawables from oldScene in finished slot
			connect(scene, SIGNAL(opacityAnimationFinished()), this, SLOT(opacityAnimationFinished()));
		}
		else
		{
			opacityAnimationFinished(scene);
		}
	}
	else
	{
// 		if(m_glWidget)
// 		{
// 			QList<GLDrawable*> items = m_glWidget->drawables();
// 			foreach(GLDrawable *drawable, items)
// 			{
// 				disconnect(drawable->playlist(), 0, this, 0);
// 				m_glWidget->removeDrawable(drawable);
// 			}
// 		}
// 		else
// 		{
// 			QList<QGraphicsItem*> items = m_graphicsScene->items();
// 			foreach(QGraphicsItem *item, items)
// 			{
// 				if(GLDrawable *drawable = dynamic_cast<GLDrawable*>(item))
// 				{
// 					disconnect(drawable->playlist(), 0, this, 0);
// 					m_graphicsScene->removeItem(drawable);
// 				}
// 			}
// 		}
	}
	
	//qDebug() << "PlayerWindow::removeScene: Done with method";
}

void PlayerWindow::addScene(GLScene *scene, int zmod/*=1*/, bool fadeInOpac/*=true*/, GLScene *fadeLeader)
{
	if(!scene)
		return;
	
	if(m_xfadeSpeed > 0)
		scene->setOpacity(0); // no anim yet...

	if(scene->sceneType())
		scene->sceneType()->setLiveStatus(true);

	GLDrawableList newSceneList = scene->drawableList();

	//qDebug() << "PlayerWindow::addScene: Adding scene "<<scene<<" at layer"<<zmod;

	
	if(m_glWidget)
	{
		//qDebug() << "PlayerWindow::displayScene: Set new scene opac to 0";

// 		//double maxZIndex = -100000;
		scene->setGLWidget(m_glWidget, zmod);

		foreach(GLDrawable *drawable, newSceneList)
		{
			//drawable->setZIndexModifier(zmod);
			
			connect(drawable->playlist(), SIGNAL(currentItemChanged(GLPlaylistItem*)), this, SLOT(currentPlaylistItemChanged(GLPlaylistItem*)));
			connect(drawable->playlist(), SIGNAL(playerTimeChanged(double)), this, SLOT(playlistTimeChanged(double)));
			//m_glWidget->addDrawable(drawable);
			qDebug() << "PlayerWindow::displayScene: Adding drawable:"<<(QObject*)drawable;

			if(GLVideoDrawable *vid = dynamic_cast<GLVideoDrawable*>(drawable))
			{
				qDebug() << "GLWidget mode, item:"<<(QObject*)drawable<<", xfade length:"<<m_xfadeSpeed;
				vid->setXFadeLength(m_xfadeSpeed);
			}

// 			if(drawable->zIndex() > maxZIndex)
// 				maxZIndex = drawable->zIndex();
		}

		//qDebug() << "PlayerWindow::displayScene: Animating fade in
		
	}
	else
	{
		
		foreach(GLDrawable *drawable, newSceneList)
		{
			drawable->setZIndexModifier(zmod);
			
			connect(drawable->playlist(), SIGNAL(currentItemChanged(GLPlaylistItem*)), this, SLOT(currentPlaylistItemChanged(GLPlaylistItem*)));
			connect(drawable->playlist(), SIGNAL(playerTimeChanged(double)), this, SLOT(playlistTimeChanged(double)));
			m_graphicsScene->addItem(drawable);
			
			if(drawable->rect().isNull())
				//m_rect = QRectF(QPointF(0,0),canvasSize());
				drawable->setRect(m_graphicsScene->sceneRect());

	
			if(GLVideoDrawable *vid = dynamic_cast<GLVideoDrawable*>(drawable))
			{
				qDebug() << "QGraphicsView mode, item:"<<(QObject*)drawable<<", xfade length:"<<m_xfadeSpeed;
				vid->setXFadeLength(m_xfadeSpeed);
			}
		}
	}

	if(fadeInOpac && m_xfadeSpeed>0)
	{
		//qDebug() << "PlayerWindow::addScene: fade in scene:"<<scene<<" at "<<m_xfadeSpeed<<"ms";
		qDebug() << "PlayerWindow::addScene: fade in scene:"<<scene<<" at "<<m_xfadeSpeed<<"ms, fadeLeader: "<<fadeLeader;
		scene->setFadeSyncLeader(fadeLeader);
		scene->setOpacity(1,true,m_xfadeSpeed); // animate fade in
	}
	

	GLDrawableList list = scene->drawableList();
	foreach(GLDrawable *gld, list)
		if(gld->playlist()->size() > 0)
			gld->playlist()->play();
}

void PlayerWindow::opacityAnimationFinished(GLScene *scene)
{
	if(!scene)
		scene = dynamic_cast<GLScene*>(sender());
		
	if(!scene)
	{
 		qDebug() << "PlayerWindow::opacityAnimationFinished: No scene given, nothing removed.";
 		return;
	}

	//disconnect(drawable, 0, this, 0);

// 	if(!m_oldScene)
// 	{
// 		qDebug() << "PlayerWindow::opacityAnimationFinished: No m_oldScene, nothing removed.";
// 		return;
// 	}

	GLDrawableList list = scene->drawableList();
	//qDebug() << "PlayerWindow::opacityAnimationFinished: Found "<<list.size()<<" drawables to remove";
	foreach(GLDrawable *drawable, list)
	{
		if(!m_glWidget)
		//	m_glWidget->removeDrawable(drawable);
		//else
			m_graphicsScene->removeItem(drawable);

		qDebug() << "PlayerWindow::opacityAnimationFinished: removing drawable:"<<(QObject*)drawable;
	}
	
	if(m_glWidget)
		scene->detachGLWidget();

	disconnect(scene, 0, this, 0);

	if(scene->sceneType())
		scene->sceneType()->setLiveStatus(false);

	if(scene == m_oldScene)
		m_oldScene = 0;
	else
	if(m_oldScene)
		qDebug() << "PlayerWindow::opacityAnimationFinished: Has m_oldScene pointer, but not our scene, so not setting to 0.";
	
	if(scene->sceneGroup() == m_oldGroup)
	{
		//qDebug() << "PlayerWindow::opacityAnimationFinished: deleting old group:"<<(QObject*)m_oldGroup;
		delete m_oldGroup;
		//m_oldGroup->deleteLater();
		m_oldGroup = 0;
	}
	
	if(m_glWidget)
	{
		// This is necessary to do a final repaint after fading out objects...
		// Without this, fading out (for example) an overlay will leave a barely-visible (like 5%) opacity
		// version visible only in the output stream, not on screen. This forces a redraw which
		// sends a 'clean' frame out....need to debug the root of the problem later.
		QTimer::singleShot(100,m_glWidget,SLOT(updateGL()));
	}
}

void PlayerWindow::currentPlaylistItemChanged(GLPlaylistItem* item)
{
	GLDrawablePlaylist *playlist = dynamic_cast<GLDrawablePlaylist*>(sender());
	if(!playlist)
		return;

	GLDrawable *drawable = playlist->drawable();
	if(!drawable)
		return;

	sendReply(QVariantList()
			<< "cmd"	<< Server_CurrentPlaylistItemChanged
			<< "drawableid"	<< drawable->id()
			<< "itemid"	<< item->id());
}

void PlayerWindow::playlistTimeChanged(double time)
{
	GLDrawablePlaylist *playlist = dynamic_cast<GLDrawablePlaylist*>(sender());
	if(!playlist)
		return;

	GLDrawable *drawable = playlist->drawable();
	if(!drawable)
		return;

	sendReply(QVariantList()
			<< "cmd"	<< Server_PlaylistTimeChanged
			<< "drawableid"	<< drawable->id()
			<< "time"	<< time);
}

QRectF PlayerWindow::stringToRectF(const QString& string)
{
	QStringList z = string.split(",");
	return QRectF(	z[0].toDouble(),
			z[1].toDouble(),
			z[2].toDouble(),
			z[3].toDouble());
}

QPointF PlayerWindow::stringToPointF(const QString& string)
{
	QStringList z = string.split(",");
	return QPointF(	z[0].toDouble(),
			z[1].toDouble());
}

/// PlayerJsonServer
PlayerJsonServer::PlayerJsonServer(quint16 port, PlayerWindow* parent)
	: HttpServer(port,parent)
	, m_win(parent)
	, m_jsonOut(new QJson::Serializer())
{

}

void PlayerJsonServer::dispatch(QTcpSocket *socket, const QStringList &pathElements, const QStringMap &map)
{
	//QString pathStr = path.join("/");

	QStringList path = pathElements;

	QString cmd = path.isEmpty() ? "" : path.takeFirst();


	//QString cmd = map["cmd"].toString();
	qDebug() << "PlayerJsonServer::receivedMap: [COMMAND]: "<<cmd;

// 	if(cmd == Server_Login)
// 	{
// 		if(map["user"].toString() != m_validUser ||
// 		   map["pass"].toString() != m_validPass)
// 		{
// 			qDebug() << "PlayerWindow::receivedMap: ["<<cmd<<"] Invalid user/pass combo";
//
// 			sendReply(QVariantList()
// 				<< "cmd" << Server_Login
// 				<< "status" << "error"
// 				<< "message" << "Invalid username/password.");
// 		}
// 		else
// 		{
// 			m_loggedIn = true;
// 			sendReply(QVariantList()
// 				<< "cmd" << Server_Login
// 				<< "status" << "success"
// 				<< "version" << m_playerVersionString
// 				<< "ver" << m_playerVersion);
//
//
// 			if(m_outputEncoder &&
// 			  !m_outputEncoder->encodingStarted())
// 				m_outputEncoder->startEncoder();
//
// 		}
// 	}
// 	else
// 	if(cmd == Server_Ping)
// 	{
// 		if(m_vidSendMgr)
// 		{
// 			QStringList inputs = m_vidSendMgr->videoConnections();
//
// 			sendReply(QVariantList()
// 					<< "cmd" << Server_Ping
// 					<< "version" << m_playerVersionString
// 					<< "ver" << m_playerVersion
// 					<< "inputs" << inputs.size());
// 		}
// 	}
// 	else
// 	if(!m_loggedIn)
// 	{
// 		sendReply(QVariantList()
// 				<< "cmd" << Server_Login
// 				<< "status" << "error"
// 				<< "message" << "Not logged in, command will not succeed.");
// 	}
// 	else
	if(cmd == Server_SetBlackout)
	{
// 		if(m_glWidget)
// 			m_glWidget->fadeBlack(map["flag"].toBool());
		bool flag = map["flag"] == "true";
		m_win->setBlack(flag);

		sendReply(socket, QVariantList()
				<< "cmd" << Server_SetBlackout
				<< "status" << true);
	}
	else
	if(cmd == Server_SetCrossfadeSpeed)
	{
		int ms = map["ms"].toInt();
		m_win->setCrossfadeSpeed(ms);

		sendReply(socket, QVariantList()
				<< "cmd" << cmd
				<< "status" << true);
	}
	else
	if(cmd == Server_LoadSlideGroup)
	{
// 		QByteArray ba = map["data"].toByteArray();
// 		GLSceneGroup *group = new GLSceneGroup(ba);
// 		setGroup(group);
//
// 		if(group->size() > 0)
// 			setScene(group->at(0));

		qDebug() << "PlayerJsonServer: Command Not Implemented: "<<cmd;

		sendReply(socket, QVariantList()
				<< "cmd" << Server_LoadSlideGroup
				<< "status" << true);
	}
	else
	if(cmd == Server_SetSlide)
	{
		if(!m_win->group())
		{
			sendReply(socket, QVariantList()
					<< "cmd" << Server_SetSlide
					<< "status" << "error"
					<< "message" << "No group loaded. First transmit Server_LoadSlideGroup before calling Server_SetSlide.");
		}
		else
		{
			int id = map["sceneid"].toInt();
			GLScene *scene = m_win->group()->lookupScene(id);
			if(!scene)
			{
				sendReply(socket, QVariantList()
					<< "cmd" << Server_SetSlide
					<< "status" << "error"
					<< "message" << "Invalid SceneID");
			}
			else
			{
				m_win->setScene(scene);

				sendReply(socket, QVariantList()
					<< "cmd" << Server_SetSlide
					<< "status" << true);
			}
		}
	}
	else
	if(cmd == Server_SetLayout)
	{
		/// TODO implement layout setting
		sendReply(socket, QVariantList()
				<< "cmd" << Server_SetLayout
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
		
		m_win->showVideoConnection(con, fadeSpeedOverride);
	}
	else
	if(cmd == Server_SetUserProperty    ||
	   cmd == Server_SetVisibility      ||
	   cmd == Server_QueryProperty      ||
	   cmd == Server_SetPlaylistPlaying ||
	   cmd == Server_UpdatePlaylist     ||
	   cmd == Server_SetPlaylistTime)
	{
		if(!m_win->scene())
		{
			sendReply(socket, QVariantList()
					<< "cmd" << cmd
					<< "status" << "error"
					<< "message" << QString("No scene selected. First transmit Server_SetSlide before sending %1").arg(cmd));
		}
		else
		{
			int id = map["drawableid"].toInt();
			GLDrawable *gld = m_win->scene()->lookupDrawable(id);
			if(!gld)
			{
				foreach(GLScene *scene, m_win->overlays()->sceneList())
					if((gld = scene->lookupDrawable(id)) != NULL)
						break;
			}
			
			if(!gld)
			{
				sendReply(socket, QVariantList()
					<< "cmd" << cmd
					<< "status" << "error"
					<< "message" << "Invalid DrawableID");
			}
			else
			{
				if(cmd == Server_SetPlaylistPlaying)
				{
				 	bool flag = map["flag"] == "true";
				 	gld->playlist()->setIsPlaying(flag);

				}
				else
				if(cmd == Server_UpdatePlaylist)
				{
					//QByteArray ba = map["data"].toByteArray();
					//gld->playlist()->fromByteArray(ba);
					qDebug() << "PlayerJsonServer: Command Not Implemented: "<<cmd;
				}
				else
				if(cmd == Server_SetPlaylistTime)
				{
					double time = map["time"].toDouble();
					gld->playlist()->setPlayTime(time);
				}
				else
				{
					QString name =
						cmd == Server_SetVisibility ? "isVisible" :
						map["name"];

					if(name.isEmpty())
					{
						name = QString(gld->metaObject()->userProperty().name());
					}

					if(name.isEmpty())
					{
						sendReply(socket, QVariantList()
							<< "cmd" << cmd
							<< "status" << "error"
							<< "message" << "No property name given in 'name', and could not find a USER-flagged Q_PROPERTY on the GLDrawable requested by 'drawableid'.");
					}
					else
					{
						if(cmd == Server_QueryProperty)
						{
							sendReply(socket, QVariantList()
								<< "cmd" << cmd
								<< "drawableid" << id
								<< "name" << name
								<< "value" << gld->property(qPrintable(name)));
						}
						else
						// Server_SetUserProperty
						{
							QString string = map["value"];
							QString type = map["type"];

							QVariant value = stringToVariant(string,type);

							if(GLVideoDrawable *vid = dynamic_cast<GLVideoDrawable*>(gld))
								vid->setXFadeLength((int)m_win->xfadeSpeed());

							QVariant::Type propType = gld->metaObject()->property(gld->metaObject()->indexOfProperty(qPrintable(name))).type();
							if(propType == QVariant::Int ||
							   propType == QVariant::Double ||
							   propType == QVariant::PointF ||
							   propType == QVariant::Point ||
							   propType == QVariant::RectF ||
							   propType == QVariant::Rect)
							{
								QAbsoluteTimeAnimation *anim = new QAbsoluteTimeAnimation(gld, qPrintable(name));
								anim->setEndValue(value);
								anim->setDuration((int)m_win->xfadeSpeed());
								anim->start(QAbstractAnimation::DeleteWhenStopped);
								
								gld->registerAbsoluteTimeAnimation(anim);
							}
							else
							{
								gld->setProperty(qPrintable(name), value);
							}

							sendReply(socket, QVariantList()
								<< "cmd" << cmd
								<< "status" << true);
						}
					}
				}
			}
		}
	}
	else
	if(cmd == Server_DownloadFile)
	{
		/// TODO implement download file
		sendReply(socket, QVariantList()
				<< "cmd" << Server_DownloadFile
				<< "status" << true);
	}
	else
	if(cmd == Server_QueryLayout)
	{
		/// TODO implement query layout
		sendReply(socket, QVariantList()
				<< "cmd" << Server_QueryLayout
				<< "status" << true);
	}
	else
	if(cmd == Server_SetViewport)
	{
		QString pair = map["viewport"];
		m_win->setViewport(m_win->stringToRectF(pair));

		sendReply(socket, QVariantList()
				<< "cmd" << cmd
				<< "status" << true);
	}
	else
	if(cmd == Server_SetCanvasSize)
	{
		QString size = map["canvas"];
		QPointF point = m_win->stringToPointF(size);
		m_win->setCanvasSize(QSizeF(point.x(), point.y()));

		sendReply(socket, QVariantList()
				<< "cmd" << cmd
				<< "status" << true);
	}
	else
	if(cmd == Server_SetIgnoreAR)
	{
		bool flag = map["flag"] == "true";
		m_win->setIgnoreAR(flag);

		sendReply(socket, QVariantList()
				<< "cmd" << cmd
				<< "status" << true);
	}
	else
	if(cmd == Server_SetScreen)
	{
		QString str = map["rect"];
		m_win->setScreen(m_win->stringToRectF(str).toRect());

		sendReply(socket, QVariantList()
				<< "cmd" << cmd
				<< "status" << true);
	}
	else
	if(cmd == Server_AddSubview)
	{
// 		if(m_glWidget)
// 		{
// 			GLWidgetSubview *view = new GLWidgetSubview();
//
// 			QByteArray ba = map["data"].toByteArray();
// 			view->fromByteArray(ba);
//
// 			addSubview(view);
// 		}
		qDebug() << "PlayerJsonServer: Command Not Implemented: "<<cmd;

		sendReply(socket, QVariantList()
				<< "cmd" << cmd
				<< "status" << true);
	}
	else
	if(cmd == Server_RemoveSubview)
	{
		int subviewId = map["subviewid"].toInt();
		if(subviewId>0)
		{
			m_win->removeSubview(subviewId);
		}

		sendReply(socket, QVariantList()
				<< "cmd" << cmd
				<< "status" << true);
	}
	else
	if(cmd == Server_ClearSubviews)
	{
		m_win->clearSubviews();

		sendReply(socket, QVariantList()
				<< "cmd" << cmd
				<< "status" << true);
	}
	else
	if(cmd == Server_ListVideoInputs)
	{
		QStringList inputs = m_win->videoInputs();
		QVariantList varList;
		foreach(QString str, inputs)
			varList << str;
		QVariant list = varList; // convert to a varient so sendReply() doesnt try to add each element to the map
		sendReply(socket, QVariantList()
				<< "cmd"  << cmd
				<< "list" << list);

	}
	else
	if(cmd == Server_ExamineCurrentScene)
	{
		// foreach drawable in scene, list prop

		QVariantList gldlist = examineScene();

		sendReply(socket, QVariantList()
				<< "cmd"    << cmd
				<< "status" << "true"
				<< "items"  << QVariant(gldlist));
	}
	else
	if(cmd == Server_ExamineCurrentGroup)
	{
		QVariantList scenelist = examineGroup();

		sendReply(socket, QVariantList()
				<< "cmd"    << cmd
				<< "status" << "true"
				<< "scenes"  << QVariant(scenelist));
	}
	else
	if(cmd == Server_ExamineCollection ||
	   cmd == Server_LoadGroupFromCollection)
	{
		GLSceneGroupCollection *col = new GLSceneGroupCollection();
		if(!col->readFile(map["file"]))
		{
			//qDebug() << qPrintable(name()) << ": Error reading collection file"<<arg;
			sendReply(socket, QVariantList()
				<< "cmd"     << cmd
				<< "status"  << "error"
				<< "message" << "Unable to read collection file.");
		}
		else
		{
			if(cmd == Server_LoadGroupFromCollection)
			{
				int groupId = map["id"].toInt();
				GLSceneGroup *group = col->lookupGroup(groupId);
				if(!group)
				{
					sendReply(socket, QVariantList()
						<< "cmd"     << cmd
						<< "status"  << "error"
						<< "message" << "Invalid groupId");
				}
				else
				{
					m_win->setCollection(col);
					m_win->setGroup(group);

					if(group->size() > 0)
					{
						GLScene *scene = group->at(0);
						m_win->setScene(scene);
					}

					sendReply(socket, QVariantList()
						<< "cmd"    << cmd
						<< "status" << "true");
				}
			}
			else
			{
				//Server_ExamineCollection
				QVariantList groups;
				foreach(GLSceneGroup *group, col->groupList())
				{
					//qDebug() << group->groupId() << "\"" << qPrintable(group->groupName().replace("\"","\\\"")) << "\"";
					//qDebug() << group->groupId() << group->groupName();
					QVariantMap map;

					map["id"] = group->groupId();
					map["name"] = group->groupName();

					map["scenes"] = examineGroup(group);

					groups << map;
				}

				sendReply(socket, QVariantList()
					<< "cmd"    << cmd
					<< "status" << "true"
					<< "groups"  << QVariant(groups));

				delete col;
			}
		}
	}
	else
	if(cmd == Server_WriteCollection)
	{
		QString file = map["file"];
		if(!m_win->collection())
		{
			sendReply(socket, QVariantList()
				<< "cmd"     << cmd
				<< "status"  << "error"
				<< "message" << "No collection loaded.");
		}
		else
		{
			if(!m_win->collection()->writeFile(file))
				sendReply(socket, QVariantList()
					<< "cmd"     << cmd
					<< "status"  << "error"
					<< "message" << "Error writing collection.");
			else
				sendReply(socket, QVariantList()
					<< "cmd"     << cmd
					<< "status"  << "true");
		}
	}
	else
	if(cmd == Server_SetSceneProperty)
	{
		int id = map["sceneid"].toInt();
		GLScene *scene = m_win->group()->lookupScene(id);
		if(!scene)
		{
			sendReply(socket, QVariantList()
				<< "cmd" << cmd
				<< "status" << "error"
				<< "message" << "Invalid SceneID");
		}
		else
		{
			QString name   = map["name"];
			QString string = map["value"];
			QString type   = map["type"];

			QVariant value = stringToVariant(string,type);

			name = name == "autodur"  ? "autoDuration" :
			       name == "datetime" ? "scheduledTime" :
			       name == "autotime" ? "autoSchedule" :
			       name;

			type = name == "scheduledTime" ? "datetime" :
			       name == "autodur" || name == "autotime" ? "bool" :
			       name == "duration" ? "double" :
			       type;

			scene->setProperty(qPrintable(name), value);

			sendReply(socket, QVariantList()
				<< "cmd"     << cmd
				<< "status"  << "true");
		}
	}
	else
	if(cmd == Server_RemovePlaylistItem)
	{
		int id = map["drawableid"].toInt();
		GLDrawable *gld = m_win->scene()->lookupDrawable(id);
		if(!gld)
		{
			sendReply(socket, QVariantList()
				<< "cmd" << cmd
				<< "status" << "error"
				<< "message" << "Invalid DrawableID");
		}
		else
		{
			int itemId = map["itemid"].toInt();
			GLPlaylistItem *item = gld->playlist()->lookup(itemId);
			if(!item)
			{
				sendReply(socket, QVariantList()
					<< "cmd" << cmd
					<< "status" << "error"
					<< "message" << "Invalid ItemID");
			}
			else
			{
				gld->playlist()->removeItem(item);
				sendReply(socket, QVariantList()
					<< "cmd"     << cmd
					<< "status"  << "true");
			}
		}
	}
	else
	if(cmd == Server_AddPlaylistItem)
	{
		int id = map["drawableid"].toInt();
		GLDrawable *gld = m_win->scene()->lookupDrawable(id);
		if(!gld)
		{
			sendReply(socket, QVariantList()
				<< "cmd" << cmd
				<< "status" << "error"
				<< "message" << "Invalid DrawableID");
		}
		else
		{
			GLPlaylistItem *item = new GLPlaylistItem();

			item->setTitle(		map["title"]);
			item->setValue(		stringToVariant(map["value"],map["type"]));
			item->setDuration(	map["duration"].toDouble());
			item->setAutoDuration(	map["autodur"] == "true");
			item->setScheduledTime(	stringToVariant(map["datetime"],"datetime").toDateTime());
			item->setAutoSchedule(	map["autotime"] == "true");

			gld->playlist()->addItem(item);

			sendReply(socket, QVariantList()
				<< "cmd"     << cmd
				<< "status"  << "true"
				<< "itemid"  << item->id());
		}
	}
	else
	{
		sendReply(socket, QVariantList()
				<< "cmd"     << cmd
				<< "status"  << "error"
				<< "message" << "Unknown command.");
	}
}

QVariant PlayerJsonServer::stringToVariant(QString string, QString type)
{
	QVariant value = string;
	if(!type.isEmpty())
	{
		if(type == "string")
		{
			// Not needed, default type
		}
		else
		if(type == "int")
		{
			value = string.toInt();
		}
		else
		if(type == "double")
		{
			value = string.toDouble();
		}
		else
		if(type == "bool")
		{
			value = string == "true";
		}
		else
		if(type == "color")
		{
			QStringList z = string.split(",");
			value = QColor(	z[0].toInt(),
					z[1].toInt(),
					z[2].toInt(),
					z.size() >= 4 ? z[3].toInt() : 255);
		}
		else
		if(type == "point")
		{
			QStringList z = string.split(",");
			value = QPointF(z[0].toDouble(),
					z[1].toDouble());
		}
		else
		if(type == "rect")
		{
			QStringList z = string.split(",");
			value = QRectF(	z[0].toDouble(),
					z[1].toDouble(),
					z[2].toDouble(),
					z[3].toDouble());
		}
		else
		if(type == "datetime")
		{
			value = QDateTime::fromString(string, "yyyy-MM-dd HH:mm:ss");
		}
		else
		{
			qDebug() << "PlayerJsonServer::stringToVariant: Unknown data type:"<<type<<", defaulting to 'string'";
		}
	}
	return value;
}

QVariantList PlayerJsonServer::examineGroup(GLSceneGroup *group)
{
	if(!group)
		group = m_win->group();

	QVariantList outlist;
	foreach(GLScene *scene, group->sceneList())
	{
		QVariantMap map;

		map["id"]       = scene->sceneId();
		map["name"]     = scene->sceneName();
		map["duration"] = scene->duration();
		map["autodur"]  = scene->autoDuration();
		map["datetime"] = scene->scheduledTime();
		map["autotime"] = scene->autoSchedule();

		map["items"] = examineScene(scene);

		outlist << map;
	}

	qDebug() << "PlayerJsonServer::examineGroup: list:"<<outlist;

	return outlist;
}

QVariantList PlayerJsonServer::examineScene(GLScene *scene)
{
	if(!scene)
		scene = m_win->scene();
	
	QVariantList gldlist;

	if(scene)
	{
	
		GLDrawableList list = scene->drawableList();
		if(list.isEmpty())
			qDebug() << "Warning: Found "<<list.size()<<"items in scene "<<scene->sceneId();
	
		
		foreach(GLDrawable *gld, list)
		{
			QVariantMap map;
			QVariantList proplist;
	
			QString typeName = gld->metaObject()->className();
			typeName = typeName.replace(QRegExp("^GL"),"");
			typeName = typeName.replace(QRegExp("Drawable$"),"");
			//qDebug() << gld->id() << qPrintable(typeName) << gld->itemName();
			map["id"] = gld->id();
			map["type"] = typeName;
			map["name"] = gld->itemName();
	
			const QMetaObject *metaobject = gld->metaObject();
			int count = metaobject->propertyCount();
			for (int i=0; i<count; ++i)
			{
				QVariantMap map2;
	
				QMetaProperty meta = metaobject->property(i);
				const char *name = meta.name();
				QVariant value = gld->property(name);
	
				//qDebug() << QString(name) << meta.type() << value.toString();
				map2["name"] = QString(name);
				map2["type"] = QVariant::typeToName(meta.type());
				map2["value"] = value.toString();
	
				proplist << map2;
			}
	
			map["props"] = proplist;
	
			QVariantList playlist;
			QList<GLPlaylistItem*> items = gld->playlist()->items();
			foreach(GLPlaylistItem *item, items)
			{
				QVariantMap map2;
				map2["id"] = item->id();
				map2["title"] = item->title();
				map2["value"] = item->value();
				map2["duration"] = item->duration();
				map2["autodur"] = item->autoDuration();
				map2["datetime"] = item->scheduledTime();
				map2["autotime"] = item->autoSchedule();
	
				playlist << map2;
			}
	
			map["playlist"] = playlist;
	
	
			gldlist << map;
		}
	}

	qDebug() << "PlayerJsonServer::examineScene: list:"<<gldlist;

	return gldlist;
}

void PlayerJsonServer::sendReply(QTcpSocket *socket, QVariantList reply)
{
	QVariantMap map;
	if(reply.size() % 2 != 0)
	{
		qDebug() << "PlayerJsonServer::sendReply: [WARNING]: Odd number of elelements in reply: "<<reply;
	}
	
	// Add the API version on all outgoing JSON replies
	reply << "api" << PLAYER_API_VER;

	for(int i=0; i<reply.size(); i+=2)
	{
		if(i+1 >= reply.size())
			continue;

		QString key = reply[i].toString();
		QVariant value = reply[i+1];

		map[key] = value;
	}

	QByteArray json = m_jsonOut->serialize( map );
	qDebug() << "PlayerJsonServer::sendReply: [DEBUG] map:"<<map<<", json:"<<json;

	Http_Send_Response(socket,"HTTP/1.0 200 OK\r\nContent-Type: text/javascript\r\n\r\n") << json;
}



// 	PlayerWindow * m_win;
// 	QJson::Serializer m_jsonOut;
