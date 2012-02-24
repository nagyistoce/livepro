#include "GLVideoInputDrawable.h"

#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

#include "CameraThread.h"
#include "VideoThread.h"

#include "VideoReceiver.h"
#include <QNetworkInterface>

GLVideoInputDrawable::GLVideoInputDrawable(QString file, QObject *parent)
	: GLVideoDrawable(parent)
	, m_source(0)
	, m_rx(0)
	, m_useNetworkSource(false)
{
	if(!file.isEmpty())
		setVideoInput(file);

        //QTimer::singleShot(2500, this, SLOT(testXfade()));
}

GLVideoInputDrawable::~GLVideoInputDrawable()
{
	setVideoSource(0);
	if(m_rx)
	{
		m_rx->release(this);
		//delete m_rx;
	}
}

QHash<QString,QString> GLVideoInputDrawable::parseConString(const QString& con)
{
	QHash<QString,QString> map;
	QStringList opts = con.split(",");
	foreach(QString pair, opts)
	{
		QStringList values = pair.split("=");
		if(values.size() < 2)
		{
			qDebug() << "GLVideoInputDrawable::parseConString: Parse error for option:"<<pair;
			continue;
		}

		QString name = values[0].toLower();
		QString value = values[1];

		map[name] = value;
	}
	
	return map;
}

QUrl GLVideoInputDrawable::extractUrl(const QString& con)
{
	QString netString;
	if(con.indexOf("=") > -1)
	{
		QHash<QString,QString> map = parseConString(con);
		netString = map["net"];
	}
	else
	{
		netString = con;
	}
	
	if(netString.isEmpty())
		return QUrl();
		
	QStringList url = netString.split(":");
	QString host = url[0];
	int port = url.size() > 1 ? url[1].toInt() : 7755;
	
	QUrl urlObject;
	urlObject.setHost(host);
	urlObject.setPort(port);
	return urlObject;
}


void GLVideoInputDrawable::setVideoConnection(const QString& con)
{
	m_videoConnection = con;

	//qDebug() << "GLVideoInputDrawable::setVideoConnection: con:"<<con;
	
	// Example: dev=/dev/video0,input=S-Video0,net=10.0.1.70:8877
	if(con.isEmpty())
		return;

	QHash<QString,QString> map = parseConString(con);

	m_videoInput = map["dev"];
	
	setNetworkSource(map["net"]);

	//qDebug() << "GLVideoInputDrawable::setVideoConnection: dev:"<<map["dev"]; 
	if(!map["dev"].isEmpty() &&
	   (m_isLocal[map["net"]] || map["net"].isEmpty()) &&
	   !m_localHasError[map["dev"]])
		setVideoInput(map["dev"]);

	setCardInput(map["input"]);
}

void GLVideoInputDrawable::setNetworkSource(const QString& src)
{
	m_networkSource = src;

	if(src.isEmpty())
		return;

	QUrl url = extractUrl(src);
	QString host = url.host();
	
// 	QStringList url = src.split(":");
// 	QString host = url[0];
// 	int port = url.size() > 1 ? url[1].toInt() : 7755;

// 	QUrl url(src);
// 	if(!url.isValid())
// 	{
// 		qDebug() << "GLVideoInputDrawable: URL:"<<src<<", Error: Sorry, the URL you entered is not a properly-formatted URL. Please try again.";
// 		return ;
// 	}

	//if(!m_thread->connectTo(url.host(), url.port(), url.path(), url.userName(), url.password()))

	bool isLocalHost = false;
        //#ifndef Q_OS_WIN32

	QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
	// use the first non-localhost IPv4 address
	for (int i = 0; i < ipAddressesList.size(); ++i)
	{
		if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
		    ipAddressesList.at(i).toIPv4Address())
			if(!isLocalHost)
				isLocalHost = host == ipAddressesList.at(i).toString();
	}
	
	// if we did not find one, use IPv4 localhost
	if (!isLocalHost)
		isLocalHost = host == QHostAddress(QHostAddress::LocalHost).toString();

	if(!isLocalHost &&
	   host == "localhost")
		isLocalHost = true;
	
	// If we already tried opening a local device, force to use the network
	if((m_source && m_source->hasError())/* ||
	   m_videoInput.startsWith("test:")*/)
	{
		qDebug() << "GLVideoInputDrawable::setNetworkSource: Forcing to use the network for src:"<<src<<" due to local device error.";
		isLocalHost = false;
		m_localHasError[m_videoInput] = true;
	}
        /*
	#else
	qDebug() << "GLVideoInputDrawable::setNetworkSource: Forcing to use the network for src:"<<src<<" due to Win32 OS";
	isLocalHost = false;
	m_localHasError[m_videoInput] = true;
	#endif
        */

	setUseNetworkSource(!isLocalHost);

	m_isLocal[src] = isLocalHost;

	qDebug() << "GLVideoInputDrawable::setNetworkSource: src:"<<src<<", isLocalHost:"<<isLocalHost<<", m_videoInput:"<<m_videoInput;
}

void GLVideoInputDrawable::setUseNetworkSource(bool flag)
{
	m_useNetworkSource = flag;

	if(m_networkSource.isEmpty())
		return;

	if(flag)
	{
		VideoReceiver *oldRx = m_rx;
// 		if(m_rx)
// 			m_rx->release(this);

		//QUrl url(m_networkSource);
// 		QStringList url = m_networkSource.split(":");
// 		QString host = url[0];
// 		int port = url.size() > 1 ? url[1].toInt() : 7755;
		QUrl url = extractUrl(m_networkSource);
		QString host = url.host();
		int port = url.port();

		m_rx = VideoReceiver::getReceiver(host,port);
		if(!m_rx)
		{
			qDebug() << "GLVideoInputDrawable::setUseNetworkSource: Error connecting to: "<<host<<":"<<port;
		}
		else
		if(m_rx == oldRx)
		{
			// do nothing
		}
		else
		{
			m_rx->registerConsumer(this);
			if(oldRx)
				oldRx->release(this);

			setVideoSource(m_rx);
		}
	}
	else
	{
		setVideoInput(videoInput());
		setCardInput(cardInput());
	}
}

void GLVideoInputDrawable::testXfade()
{
	qDebug() << "GLVideoInputDrawable::testXfade(): loading video thread";
// 	setVideoInput("/dev/video1");
// 	setCardInput("S-Video");
	QString file = "../data/Seasons_Loop_3_SD.mpg";

	VideoThread *m_videoThread = new VideoThread();
	m_videoThread->setVideo(file);

	// Assuming duration in seconds
	//m_videoLength = m_videoThread->duration(); // / 1000.;

	//source->setVideo("../samples/BlueFish/EssentialsVol05_Abstract_Media/HD/Countdowns/Abstract_Countdown_3_HD.mp4");
	//source->setVideo("../samples/BlueFish/EssentialsVol05_Abstract_Media/SD/Countdowns/Abstract_Countdown_3_SD.mpg");

	m_videoThread->start(true); // true = start paused

	setVideoSource(m_videoThread);
}

bool GLVideoInputDrawable::setVideoInput(const QString& camera)
{
	if(camera.isEmpty())
		return false;
		
// 	testXfade();
// 	return true;
	#ifndef Q_OS_WIN32

	m_videoInput = camera;

	CameraThread *source = CameraThread::threadForCamera(camera.isEmpty() ?  DEFAULT_INPUT : camera);

	if(!source)
	{
		qDebug() << "GLVideoInputDrawable::setVideoInput: "<<camera<<" does not exist!";
		return false;
	}

	if(m_source == source)
		return true;

	qDebug() << "GLVideoInputDrawable::setVideoInput(): camera:"<<camera;
	if(m_source &&
	   m_source != source)
	{
		m_source->release(this);
	}

	m_source = source;
	m_source->setFps(30);
	m_source->registerConsumer(this);

	// Just for testing...
	//if(camera == "/dev/video1")
	//	source->setInput("S-Video");
	
	//usleep(750 * 1000); // This causes a race condition to manifist itself reliably, which causes a crash every time instead of intermitently.
	// With the crash reproducable, I can now work to fix it.
	
	// The 'raw frames' mode uses V4L to get the frames instead of LibAV
	// Of course, V4L isn't supported on windows, so we don't enable raw frames on windows.
	// In the future, I suppose I could find code to use the appros Windows API to
	// connect to the capture card - but I just don't have a need for that level of performance
	// on windows right now, so I'll put it off until I need it or someone really wants it.
	// For now, the high-performance capture use is on Linux (for me), so that's where I'll focus. 
	#ifndef Q_OS_WINDOWS
	m_source->enableRawFrames(true);
	#endif

	setVideoSource(source);

	return true;
	#else
	return false;
	#endif
}

QString GLVideoInputDrawable::cardInput()
{
	if(!m_source)
		return QString();
	QStringList list = cardInputs();
	return list[m_source->input()];
}

QStringList GLVideoInputDrawable::cardInputs()
{
	if(!m_source)
		return QStringList("default");
	return m_source->inputs();
}

bool GLVideoInputDrawable::deinterlace()
{
	return m_source ? m_source->deinterlace() : false;
}

void GLVideoInputDrawable::setDeinterlace(bool flag)
{
	if(m_source)
		m_source->setDeinterlace(flag);
}

bool GLVideoInputDrawable::setCardInput(const QString& input)
{
	if(!m_source)
		return false;
	return m_source->setInput(input);
}
