#include "OutputWindow.h"

#define GET_CONFIG() QSettings config("stagedisplay.ini", QSettings::IniFormat);

OutputWindow::OutputWindow()
	: QGraphicsView(new QGraphicsScene())
	, m_url("")
	, m_pollDviz(false)
	, m_updateTime(1000)
	, m_isDataPoll(false)
	, m_slideId(-1)
	, m_slideName("")
	//, m_startStopButton(0)
        , m_countValue(0)
{
	// Setup graphics view
	setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform );
	setCacheMode(QGraphicsView::CacheBackground);
	setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
	setOptimizationFlags(QGraphicsView::DontSavePainterState);
	setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
	setTransformationAnchor(AnchorUnderMouse);
	setResizeAnchor(AnchorViewCenter);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		
	// Add pixmap item for background
	QPixmap bluePixmap(32,23);
	bluePixmap.fill(Qt::blue);
	m_pixmap = scene()->addPixmap(bluePixmap);
	
        // Background behind text
        m_counterBgRect = scene()->addRect(0,0,100,60,QPen(), Qt::black);
        // Add text (for use with clock)
        QFont font("Monospace", 50, 600);
        m_counterText = new QGraphicsSimpleTextItem("0");
	m_counterText->setFont(font);
	m_counterText->setPen(QPen(Qt::black));
	m_counterText->setBrush(Qt::white);
        m_counterText->setPos(0, -13);
	scene()->addItem(m_counterText);
	
	
	// Background behind text
        m_overlayBgRect = scene()->addRect(0,300,100,60,QPen(), Qt::black);
        // Add text (for use with clock)
        //QFont font("Monospace", 50, 600);
        m_overlayText = new QGraphicsSimpleTextItem("Hello, World!");
	m_overlayText->setFont(font);
	m_overlayText->setPen(QPen(Qt::black));
	m_overlayText->setBrush(Qt::white);
        m_overlayText->setPos(0, 300-13);
	scene()->addItem(m_overlayText);
	
	m_blinkOverlay = false;
	connect(&m_blinkOverlayTimer, SIGNAL(timeout()), this, SLOT(blinkOverlaySlot()));
	
	GET_CONFIG();
	
	QPoint windowPos(1024,0);
	QPoint windowSize(1024,768);

	//QPoint windowPos(10,10);
	//QPoint windowSize = QPoint(640,480);
	QString windowGeomString = config.value("geom","1024,0,1024,768").toString();
	if(!windowGeomString.isEmpty())
	{
		QStringList list = windowGeomString.split(",");
		windowPos  = QPoint(list[0].toInt(), list[1].toInt());
		windowSize = QPoint(list[2].toInt(), list[3].toInt());
	}
	

	scene()->setSceneRect(0,0,windowSize.x(),windowSize.y());
	m_pixmap->setPixmap(bluePixmap.scaled(windowSize.x(),windowSize.y()));
	
// 	if(verbose)
// 		qDebug() << "SlideShowWindow: pos:"<<windowPos<<", size:"<<windowSize;
	
	resize(windowSize.x(),windowSize.y());
	move(windowPos.x(),windowPos.y());
	
	bool frameless = config.value("frameless","true").toString() == "true";
	if(frameless)
		setWindowFlags(Qt::FramelessWindowHint);// | Qt::ToolTip);

	connect(&m_pollDvizTimer, SIGNAL(timeout()), this, SLOT(initDvizPoll()));
	//connect(&m_pollImageTimer, SIGNAL(timeout()), this, SLOT(initImagePoll()));
	
	setUpdateTime(m_updateTime);
	
        //setUrl("http://10.10.9.90:8081/image");
        //setUrl("");
	//setPollDviz(true);
	
	QString source = config.value("source","dviz://192.168.0.10:8081/image").toString();
	
	if(source.startsWith("dviz:"))
	{
		setUrl(source.replace("dviz:","http:"));
		setPollDviz(true);
	}
	else
	if(source.startsWith("tx://"))
	{
		// ex: tx://192.168.0.18:7758
		source = source.replace("tx://","");
		QStringList hostPort = source.split(":");
		QString host = hostPort[0];
		int port     = hostPort[1].toInt();
		
		m_rx = VideoReceiver::getReceiver(host, port);
		
		if(!m_rx)
		{
			qDebug() << "OutputWindow: Error connecting to source "<<source;
		}
		
		m_drw = new GLVideoDrawable();
		m_drw->setVideoSource(m_rx);
		m_drw->setPos(0,0);
		m_drw->setRect(scene()->sceneRect());
		
		m_pixmap->hide();
	}
	else
	{
		// assume it's a capture device
// 		CameraThread *cam = CameraThread::threadForCamera(source);
// 		if(!cam)
// 		{
// 			qDebug() << "OutputWindow: Unable to locate input source: "<<source;
// 		}
// 		
// 		m_drw = new GLDrawable();
// 		m_drw->setVideoSource(cam);
// 		m_drw->setPos(0,0);
// 		m_drw->setRect(scene()->sceneRect());
// 		
// 		m_pixmap->hide();
	}
	
	
// 	m_startStopButton = new QPushButton("Start Counter");
// 	connect(m_startStopButton, SIGNAL(clicked()), this, SLOT(toggleCounter()));
// 	connect(&m_counterTimer, SIGNAL(timeout()), this, SLOT(counterTick()));
// 	m_counterTimer.setInterval(1000);
// 	
// 	m_startStopButton->show();

	//toggleCounter();
	setCounterActive(false);
		
	
	setBlinkOverlay(config.value("blink-overlay","false").toString() == "true", config.value("blink-speed", 333).toInt());
	setOverlayVisible(config.value("overlay-visible", "true").toString() == "true");
	setCounterVisible(config.value("counter-visible", "true").toString() == "true");
	
	setCounterAlignment((Qt::Alignment)config.value("counter-alignment", (int)(Qt::AlignLeft | Qt::AlignTop)).toInt());
	setOverlayAlignment((Qt::Alignment)config.value("overlay-alignment", (int)(Qt::AlignCenter | Qt::AlignBottom)).toInt());
	
	setOverlayText(config.value("overlay-text").toString());
}

void OutputWindow::setCounterAlignment(Qt::Alignment align)
{
	GET_CONFIG();
	config.setValue("counter-alignment", (int)align);
	
	/// TODO
}

void OutputWindow::setOverlayAlignment(Qt::Alignment align)
{
	GET_CONFIG();
	config.setValue("overlay-alignment", (int)align);
	
	/// TODO
}

void OutputWindow::setOverlayText(QString text)
{
	m_overlayText->setText(text);
	
	GET_CONFIG();
	config.setValue("overlay-text", text);
	
	m_overlayBgRect->setRect(m_overlayText->boundingRect().adjusted(0,13,0,0));
}


void OutputWindow::setBlinkOverlay(bool flag, int speed)
{
	m_blinkOverlayTimer.setInterval(speed);
	m_blinkOverlay = flag;
	if(flag)
	{
		m_blinkOverlayTimer.start();
	}
	else
	{
		m_blinkOverlayTimer.stop();
		if(m_overlayVisible);
			m_overlayText->show();
	}
	
	GET_CONFIG();
	config.setValue("blink-overlay", flag ? "true" : "false");
	config.setValue("blink-speed", speed);
}

void OutputWindow::setCounterVisible(bool flag)
{
	m_counterVisible = flag;
	m_counterText->setVisible(flag);
	m_counterBgRect->setVisible(flag);
	
	GET_CONFIG();
	config.setValue("counter-visible", flag ? "true" : "false");
}

void OutputWindow::setOverlayVisible(bool flag)
{
	m_overlayVisible = flag;
	m_overlayText->setVisible(flag);
	m_overlayBgRect->setVisible(flag);
	
	GET_CONFIG();
	config.setValue("overlay-visible", flag ? "true" : "false");
}

void OutputWindow::counterTick()
{
	m_countValue ++;

        double fsec = ((double)m_countValue);

        double fmin = 0;
        if(fsec > 60)
        {
            fmin = ((double)m_countValue) / 60.0;
            fsec = (fsec - ((int)fsec)) * 60;
        }

        double fhour = 0;
	if(fmin > 60)
	{
		fhour = fmin / 60.0;
		fmin = (fhour - ((int)fhour)) * 60;
	}
	
	int hour = ((int)fhour);
	int min = ((int)fmin);
        int sec = ((int)fsec);
	 
        qDebug() << "counterTick: "<<m_countValue<<fhour<<fmin<<fsec;
        /*
        m_text->setText(QString("%1%2:%3%4:%5%6")
                            .arg(hour < 10 ? "0": "")
                            .arg(hour)
                            .arg(min < 10 ? "0" : "")
                            .arg(min)
                            .arg(sec < 10 ? "0" : "")
                            .arg(sec)
                            );
        */
        m_counterText->setText(QString("%1").arg(int(m_countValue/60)));
}

void OutputWindow::setCounterActive(bool flag)
{
	//if(m_counterTimer.isActive())
	if(flag)
	{
		//m_startStopButton->setText("Stop Counter");
		m_counterTimer.start();
		counterTick();
	}
	else
	{
		//m_startStopButton->setText("Start Counter");
		m_counterTimer.stop();
		
	}
}

void OutputWindow::setImage(QImage image)
{
	QPixmap pix = QPixmap::fromImage(image).scaled(width(),height());
	m_pixmap->setPixmap(pix);
}

void OutputWindow::setUpdateTime(int ms)
{
	m_updateTime = ms;
	m_pollDvizTimer.setInterval(ms/2);
	m_pollImageTimer.setInterval(ms);
}

void OutputWindow::setUrl(const QString& url)
{
	m_url = url;
	if(url.isEmpty())
		return;

	//if(liveStatus())
	{
		if(m_pollDviz)
			initDvizPoll();
// 		else
// 			initImagePoll();
	}
}

void OutputWindow::setPollDviz(bool flag)
{
	m_pollDviz = flag;
	
// 	if(!liveStatus())
// 	{
// 		// If not live, only do one poll
// 		if(flag)
// 			initDvizPoll();
// 		else
// 			initImagePoll();
// 	}
// 	else
// 	{
		if(flag)
		{
			m_pollDvizTimer.start();
			//m_pollImageTimer.stop();
		}
		else
		{
			m_pollDvizTimer.stop();
			//m_pollImageTimer.start();
		}
//	}
}
/*
void OutputWindow::setPollDvizEnabled(bool flag)
{
	//GLVideoDrawable::setLiveStatus(flag);
	
	m_isDvizPollEnabled = flag;
	
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
}*/

void OutputWindow::initDvizPoll()
{
	QString url = m_url;
	QUrl parser(url);
	QString newUrl = QString("http://%1:%2/poll?slide_id=%3&slide_name=%4")
		.arg(parser.host())
		.arg(parser.port())
		.arg(m_slideId)
		//.arg(QString(QUrl::toPercentEncoding(m_slideName)));
		.arg(m_slideName);

	//qDebug()<< "OutputWindow::initDvizPoll(): url:"<<newUrl;
	m_isDataPoll = true;
	loadUrl(newUrl);
}

void OutputWindow::initImagePoll()
{
	m_isDataPoll = false;
	
	QString imageUrl = m_url;
	if(m_pollDviz)
	{
		QString newUrl = QString("%1?size=%2x%3")
			.arg(imageUrl)
			.arg(640)
			.arg(480)
			//.arg((int)rect().width())
			//.arg((int)rect().height())
			;
		
		//qDebug()<< "OutputWindow::initImagePoll(): url:"<<newUrl;
		
		imageUrl = newUrl;
	}
	
	loadUrl(imageUrl);
}

void OutputWindow::loadUrl(const QString &location) 
{
	QUrl url(location);
	
	//qDebug() << "OutputWindow::loadUrl(): url:"<<url;

	QNetworkAccessManager *manager = new QNetworkAccessManager(this);
	connect(manager, SIGNAL(finished(QNetworkReply*)),
		this, SLOT(handleNetworkData(QNetworkReply*)));
	manager->get(QNetworkRequest(url));
}

void OutputWindow::handleNetworkData(QNetworkReply *networkReply)
{
        //qDebug() << "OutputWindow::handleNetworkData(): [DEBUG] mark1";
	QUrl url = networkReply->url();
	if (!networkReply->error())
	{
                if(m_isDataPoll)
		{
			QByteArray ba = networkReply->readAll();
			QString str = QString::fromUtf8(ba);
                        qDebug() << "OutputWindow::handleNetworkData(): [DEBUG] Result:"<<str;
			//QVariantMap data = m_parser.parse(ba).toMap();
			if(str.indexOf("no_change") > -1) //data["no_change"].toString() == "true")
			{
				// do nothing
				//qDebug() << "OutputWindow::handleNetworkData(): [DEBUG] No change, current slide:"<<m_slideName;
			}
			else
			{
				static QRegExp dataExtract("slide_id:(-?\\d+),slide_name:\"([^\"]+)\"",Qt::CaseInsensitive);
				if(dataExtract.indexIn(str))
				{
					m_slideId   = dataExtract.cap(1).toInt();
					m_slideName = dataExtract.cap(2);
				}
				
				//qDebug() << "OutputWindow::handleNetworkData(): [DEBUG] Changed to slide:"<<m_slideName<<", m_slideId:"<<m_slideId<<", polling image";
				initImagePoll();
			}
			
		}
		else
		{
			QImage image = QImage::fromData(networkReply->readAll());
			if(image.isNull())
			{
				qDebug() << "OutputWindow::handleNetworkData(): Invalid image";
			}
			else
			{
				//qDebug() << "OutputWindow::handleNetworkData(): [DEBUG] Received image, size:"<<image.size();
				setImage(image);
			}
		}
	}
        else
        {
            qDebug() << "OutputWindow::handleNetworkData(): [DEBUG] error: "<<(int)networkReply->error();

        }
	networkReply->deleteLater();
	networkReply->manager()->deleteLater();
}

void OutputWindow::blinkOverlaySlot()
{
	/// TODO
}
