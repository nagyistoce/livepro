#include "GLImageHttpDrawable.h"

#include "VideoReceiver.h"

GLImageHttpDrawable::GLImageHttpDrawable(QObject *parent)
	: GLImageDrawable("",parent)
	, m_url("")
	, m_pollDviz(false)
	, m_updateTime(1000)
	, m_isDataPoll(false)
	, m_slideId(-1)
	, m_slideName("")
	, m_rx(0)
{
	//setImage(QImage("Pm5544.jpg"));
	setImage(QImage("dot.gif"));
	
	connect(&m_pollDvizTimer, SIGNAL(timeout()), this, SLOT(initDvizPoll()));
	connect(&m_pollImageTimer, SIGNAL(timeout()), this, SLOT(initImagePoll()));
	
	setUpdateTime(m_updateTime);
}

void GLImageHttpDrawable::setUpdateTime(int ms)
{
	m_updateTime = ms;
	m_pollDvizTimer.setInterval(ms/2);
	m_pollImageTimer.setInterval(ms);
}

GLImageHttpDrawable::~GLImageHttpDrawable()
{
	releaseVideoReceiver();
}

void GLImageHttpDrawable::releaseVideoReceiver()
{
	//qDebug() << "GLImageHttpDrawable::releaseVideoReceiver()";
	if(m_rx)
	{
		m_rx->release(this);
		disconnect(m_rx, 0, this, 0);
		m_rx = 0;
	}
}

void GLImageHttpDrawable::setUrl(const QString& url)
{
	m_url = url;
	if(url.isEmpty())
		return;
		
	QUrl parsedUrl(url);
	//qDebug() << "GLImageHttpDrawable::setUrl: URL Sceme:"<<parsedUrl.scheme();
	if(parsedUrl.scheme() == "raw")
	{
		qDebug() << "GLImageHttpDrawable::setUrl: Connecting to RAW host: "<<parsedUrl;
		
		releaseVideoReceiver();
		
		m_rx = VideoReceiver::getReceiver(parsedUrl.host(), parsedUrl.port());
		if(m_rx)
		{
			m_rx->registerConsumer(this);
			connect(m_rx, SIGNAL(frameReady()), this, SLOT(videoRxFrameReady()));
		}
		
		qDebug() << "GLImageHttpDrawable::setUrl: Got receiver:"<<m_rx;
		m_pollDvizTimer.stop();
		m_pollImageTimer.stop();
		
		// Pull in first frame if one is available
		videoRxFrameReady();
	}
	else
	//if(liveStatus())
	{
		releaseVideoReceiver();
		
		qDebug() << "GLImageHttpDrawable::setUrl: Connecting to HTTP host: "<<parsedUrl;
		if(m_pollDviz)
			initDvizPoll();
		else
			initImagePoll();
	}
}

void GLImageHttpDrawable::setPollDviz(bool flag)
{
	m_pollDviz = flag;
	
	if(!liveStatus())
	{
		// If not live, only do one poll
		if(flag)
			initDvizPoll();
		else
			initImagePoll();
	}
	else
	{
		QUrl parsedUrl(m_url);
		if(parsedUrl.scheme() == "raw")
		{
			//qDebug() << "GLImageHttpDrawable::setPollDviz: RAW host, stopping poll timers";
			m_pollDvizTimer.stop();
			m_pollImageTimer.stop();
		}
		else
		if(flag)
		{
			//qDebug() << "GLImageHttpDrawable::setPollDviz: true flag, starting dviz, stopping image";
			m_pollDvizTimer.start();
			m_pollImageTimer.stop();
		}
		else
		{
			//qDebug() << "GLImageHttpDrawable::setPollDviz: false flag, stopping dviz, starting image";
			m_pollDvizTimer.stop();
			m_pollImageTimer.start();
		}
	}
}

void GLImageHttpDrawable::setLiveStatus(bool flag)
{
	GLVideoDrawable::setLiveStatus(flag);
	
	if(flag)
	{
		// start correct timers
		setPollDviz(pollDviz());
	}
	else
	{
		m_pollDvizTimer.stop();
		m_pollImageTimer.stop();
	}
}

void GLImageHttpDrawable::initDvizPoll()
{
	QString url = m_url;
	QUrl parser(url);
	QString newUrl = QString("http://%1:%2/poll?slide_id=%3&slide_name=%4")
		.arg(parser.host())
		.arg(parser.port())
		.arg(m_slideId)
		//.arg(QString(QUrl::toPercentEncoding(m_slideName)));
		.arg(m_slideName);

	qDebug()<< "GLImageHttpDrawable::initDvizPoll(): url:"<<newUrl;
	m_isDataPoll = true;
	loadUrl(newUrl);
}

void GLImageHttpDrawable::initImagePoll()
{
	m_isDataPoll = false;
	
	QString imageUrl = m_url;
	if(m_pollDviz)
	{
		QString newUrl = QString("%1?size=%2x%3")
			.arg(imageUrl)
			.arg((int)rect().width())
			.arg((int)rect().height());
	
		qDebug()<< "GLImageHttpDrawable::initImagePoll(): url:"<<newUrl;
		
		imageUrl = newUrl;
	}
	
	loadUrl(imageUrl);
}

void GLImageHttpDrawable::loadUrl(const QString &location) 
{
	QUrl url(location);
	
	qDebug() << "GLImageHttpDrawable::loadUrl(): url:"<<url;

	QNetworkAccessManager *manager = new QNetworkAccessManager(this);
	connect(manager, SIGNAL(finished(QNetworkReply*)),
		this, SLOT(handleNetworkData(QNetworkReply*)));
	manager->get(QNetworkRequest(url));
}

void GLImageHttpDrawable::handleNetworkData(QNetworkReply *networkReply) 
{
	QUrl url = networkReply->url();
	if (!networkReply->error())
	{
		if(m_isDataPoll)
		{
			QByteArray ba = networkReply->readAll();
			QString str = QString::fromUtf8(ba);
			//qDebug() << "GLImageHttpDrawable::handleNetworkData(): [DEBUG] Result:"<<str;
			//QVariantMap data = m_parser.parse(ba).toMap();
			if(str.indexOf("no_change") > -1) //data["no_change"].toString() == "true")
			{
				// do nothing
				//qDebug() << "GLImageHttpDrawable::handleNetworkData(): [DEBUG] No change, current slide:"<<m_slideName;
			}
			else
			if(str.indexOf("slide_id") > -1)
			{
				qDebug() << "GLImageHttpDrawable::handleNetworkData(): [DEBUG] Result:"<<str;
				static QRegExp dataExtract("slide_id:(-?\\d+),slide_name:\"([^\"]+)\"",Qt::CaseInsensitive);
				if(dataExtract.indexIn(str))
				{
					m_slideId   = dataExtract.cap(1).toInt();
					m_slideName = dataExtract.cap(2);
				}
				
				qDebug() << "GLImageHttpDrawable::handleNetworkData(): [DEBUG] Changed to slide:"<<m_slideName<<", m_slideId:"<<m_slideId<<", polling image";
				initImagePoll();
			}
			
		}
		else
		{
			QImage image = QImage::fromData(networkReply->readAll());
			if(image.isNull())
			{
				qDebug() << "GLImageHttpDrawable::handleNetworkData(): Invalid image";
			}
			else
			{
				//qDebug() << "GLImageHttpDrawable::handleNetworkData(): [DEBUG] Received image, size:"<<image.size();
				setImage(image);
			}
		}
	}
	networkReply->deleteLater();
	networkReply->manager()->deleteLater();
}

void GLImageHttpDrawable::videoRxFrameReady()
{
	if(!m_rx)
		return;
		
	VideoFramePtr frame = m_rx->frame();
	if(!frame)
		return;
		
	//qDebug() << "GLImageHttpDrawable::videoRxFrameReady(): Got image";
	setImage(frame->toImage());
}

