#include "SwitchMonWidget.h"

#include "VideoWidget.h"
#include "VideoReceiver.h"

#include "../3rdparty/qjson/parser.h"

SwitchMonWidget::SwitchMonWidget(QWidget *parent)
	: QWidget(parent)
	, m_ua(0)
	, m_lastReqType(T_UNKNOWN)
	, m_viewerLayout(0)
	, m_isConnected(false)
	, m_parser(new QJson::Parser)
	, m_resendLastCon(false)
	, m_serverApiVer(-1)
	, m_lastLiveWidget(0)
	, m_cutFlag(false)
{
	QSettings settings;
	
	//setStyleSheet("background-color:rgb(50,50,50); color:#FFFFFFFF");
	
	// Setup the layout of the window
	m_vbox = new QVBoxLayout(this);
	m_vbox->setContentsMargins(0,0,0,0);
	m_vbox->setSpacing(0);
	
	QHBoxLayout *conLay = new QHBoxLayout();
	conLay->addWidget(new QLabel("Server: "));
	conLay->setContentsMargins(5,5,5,5);
	conLay->setSpacing(3);

	m_serverBox = new QLineEdit();
	m_serverBox->setText(settings.value("lastServer","10.10.9.90").toString());
	connect(m_serverBox, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));
	conLay->addWidget(m_serverBox);
	
	m_connectBtn = new QPushButton("Connect");
	connect(m_connectBtn, SIGNAL(clicked()), this, SLOT(connectToServer()));
	conLay->addWidget(m_connectBtn);
	
	conLay->addStretch(1);
	m_vbox->addLayout(conLay);
	
	m_bottomRow = new QHBoxLayout();
	m_bottomRow->setContentsMargins(5,5,5,5);
	m_bottomRow->setSpacing(3);
	
	m_cutBtn = new QPushButton("Hard Cut");
	m_cutBtn->setCheckable(true);
	m_cutBtn->setFont(QFont ( "", 20 ));
	connect(m_cutBtn, SIGNAL(toggled(bool)), this, SLOT(setCutFlag(bool)));
	m_bottomRow->addWidget(m_cutBtn);
	
	m_bottomRow->addStretch(1);
	
	m_bottomRow->addWidget(new QLabel("X-Speed: 0"));
	
	QSlider *slider = new QSlider(this);
	slider->setMinimum(0);
	slider->setMaximum(2000);
	slider->setOrientation(Qt::Horizontal);
	m_bottomRow->addWidget(slider);
	
	m_bottomRow->addWidget(new QLabel("2s"));
	
	m_fadeSpeedBox = new QSpinBox(this);
	m_fadeSpeedBox->setMinimum(0);
	m_fadeSpeedBox->setMaximum(2000);
	m_fadeSpeedBox->setSuffix("ms");
	connect(slider, SIGNAL(valueChanged(int)), m_fadeSpeedBox, SLOT(setValue(int)));
	connect(m_fadeSpeedBox, SIGNAL(valueChanged(int)), slider, SLOT(setValue(int)));
	connect(m_fadeSpeedBox, SIGNAL(valueChanged(int)), this, SLOT(setFadeSpeed(int)));
	m_bottomRow->addWidget(m_fadeSpeedBox);
	
	m_fadeSpeedBox->setValue(settings.value("fadespeed",300).toInt());
	
	m_vbox->addLayout(m_bottomRow);
	
	setWindowTitle("Camera Monitor");
	
	connectToServer();
	
	// TODO
	// Add viewer for the live output on port 9978
}

SwitchMonWidget::~SwitchMonWidget()
{
	delete m_parser;	
}

void SwitchMonWidget::connectToServer()
{
	m_isConnected = false;
	m_host = m_serverBox->text();
	
	m_connectBtn->setEnabled(false);
	m_connectBtn->setText("Connecting...");
	
	QSettings().setValue("lastServer",m_host);

	// This will create at least one viewer (the live monitor)
	// It will automatically update the list of viewers once
	// we receive the list of video inputs
	createViewers();
	
	// Query the server for a list of video inputs that we
	// can then display in a set of VideoWidgets
	m_lastReqType = T_EnumInputs;
	loadUrl(tr("http://%1:9979/ListVideoInputs").arg(m_host));
}

void SwitchMonWidget::textChanged(const QString&)
{
	m_connectBtn->setEnabled(true);
	m_connectBtn->setText("Connect");
}

void SwitchMonWidget::vidWidgetClicked()
{
	QObject *obj = sender();
	QString con = obj->property("con").toString();

	qDebug() << "SwitchMonWidget::vidWidgetClicked: clicked connection: "<<con;

	if(con.isEmpty())
		return;
		
	sendCon(con);
	
	if(m_lastLiveWidget)
		m_lastLiveWidget->setProperty("videoBorderColor",QColor()); // invalid color means dont draw a border
	
	obj->setProperty("videoBorderColor",QColor(Qt::red));
	m_lastLiveWidget = obj;
	m_lastLiveCon = con;
}

void SwitchMonWidget::setCutFlag(bool flag)
{
	m_cutFlag = flag;
	
	m_cutBtn->setText(flag ? "Cross-Fade" : "Hard Cut");
	
	if(m_serverApiVer < 0.6)
	{
		m_lastReqType = T_UNKNOWN;
		loadUrl(tr("http://%1:9979/SetCrossfadeSpeed?ms=%3").arg(m_host).arg(m_cutFlag ? 0 : m_fadeSpeedBox->value()));
	}
}

void SwitchMonWidget::setFadeSpeed(int value)
{
	if(m_fadeSpeedBox->value() != value)
		m_fadeSpeedBox->setValue(value);
		
	QSettings settings;
	settings.setValue("fadespeed",value);
		
	if(m_serverApiVer < 0.6)
	{
		if(!m_cutFlag)
		{
			m_lastReqType = T_UNKNOWN;
			loadUrl(tr("http://%1:9979/SetCrossfadeSpeed?ms=%3").arg(m_host).arg(m_fadeSpeedBox->value()));
		}
	}
}

void SwitchMonWidget::sendCon(const QString& con)
{
	//http://localhost:9979/SetUserProperty?drawableid=1293&name=videoInput&value=/dev/video1&type=string
	
	if(m_serverApiVer >= 0.6)
	{
		m_lastReqType = T_ShowCon;
		loadUrl(tr("http://%1:9979/ShowVideoConnection?con=%2&ms=%3").arg(m_host).arg(con).arg(m_cutFlag ? 0 : m_fadeSpeedBox->value()));
	}
	else
	{
		// Store con so we can re-send if we encounter a fixable error
		m_lastCon = con;
		m_lastReqType = T_SetProperty;
		loadUrl(tr("http://%1:9979/SetUserProperty?drawableid=%2&name=videoConnection&value=%3&type=string").arg(m_host).arg(m_drawableId).arg(con));
	}
}

// Adjust for changes in orientation of the window (e.g. from Portrait to Landscape, etc.)
void SwitchMonWidget::resizeEvent(QResizeEvent*)
{
	if(!m_viewerLayout)
		return;

	QString layoutClassName = m_viewerLayout->metaObject()->className();
		
	//qDebug() << "Window Size: "<<width()<<" x "<<height(); 
	if(width() > height())
	{
		if(layoutClassName == "QHBoxLayout")
			return;
		
		qDebug() << "Window changed to Horizontal, setting up UI";
		
		// Temporarily turn off the autoDestroy so that the video stream 
		// doesn't disconnect/reconnect immediately - no need, since we know
		// we're reusing the same receiver streams, just in a different layout
		QList<VideoReceiver*> receivers = VideoReceiver::receivers();
		foreach(VideoReceiver *rx, receivers)
			rx->setAutoDestroy(false);
		
		clearViewerLayout();
		
		m_vbox->removeItem(m_viewerLayout);
		m_vbox->removeItem(m_bottomRow);
		delete m_viewerLayout;
		
		m_viewerLayout = new QHBoxLayout();
		m_viewerLayout->setContentsMargins(0,0,0,0);
		m_viewerLayout->setSpacing(0);
		
		m_vbox->addLayout(m_viewerLayout);
		m_vbox->addLayout(m_bottomRow);
		
		createViewers();
		
		foreach(VideoReceiver *rx, receivers)
			rx->setAutoDestroy(true);
	}
	else
	{
		if(layoutClassName == "QVBoxLayout")
		    return;
			
		qDebug() << "Window changed to Vertical, setting up UI";
		
		// Temporarily turn off the autoDestroy so that the video stream 
		// doesn't disconnect/reconnect immediately - no need, since we know
		// we're reusing the same receiver streams, just in a different layout
		QList<VideoReceiver*> receivers = VideoReceiver::receivers();
		foreach(VideoReceiver *rx, receivers)
			rx->setAutoDestroy(false);
		
		clearViewerLayout();
		
		m_vbox->removeItem(m_viewerLayout);
		m_vbox->removeItem(m_bottomRow);
		delete m_viewerLayout;
		
		m_viewerLayout = new QVBoxLayout();
		m_viewerLayout->setContentsMargins(0,0,0,0);
		m_viewerLayout->setSpacing(0);
		
		m_vbox->addLayout(m_viewerLayout);
		m_vbox->addLayout(m_bottomRow);
		
		createViewers();
		
		foreach(VideoReceiver *rx, receivers)
			rx->setAutoDestroy(false);
	}
}


void SwitchMonWidget::loadUrl(const QString &location) 
{
	QUrl url(location);
	
	qDebug() << "SwitchMonWidget::loadUrl(): url:"<<url;

	if(!m_ua)
	{
		m_ua = new QNetworkAccessManager(this);
		connect(m_ua, SIGNAL(finished(QNetworkReply*)), this, SLOT(handleNetworkData(QNetworkReply*)));
	}
	
	m_ua->get(QNetworkRequest(url));
}

void SwitchMonWidget::handleNetworkData(QNetworkReply *networkReply) 
{
	QUrl url = networkReply->url();
	if (!networkReply->error())
	{
		// TODO Pass a QVariantMap to all the process* functions instead of a QByteArray
		
		QByteArray bytes = networkReply->readAll();
		//m_isConnected = true;
		
		switch(m_lastReqType)
		{
			case T_EnumInputs:
				processInputEnumReply(bytes);
				break;
			case T_ExamineScene:
				processExamineSceneReply(bytes);
				break;
			case T_SetProperty:
				processSetPropReply(bytes);
				break;
			default:
				// nothing right now
				break;
		};
		
	}
	
	networkReply->deleteLater();
	//networkReply->manager()->deleteLater();
}

void SwitchMonWidget::processSetPropReply(const QByteArray &bytes)
{
	bool ok = false;
	QVariant reply = m_parser->parse(bytes,&ok);
	
	// In 0.6, we added the "ShowVideoConnection" command,
	// so the error code below is no longer applicable since we
	// don't need a drawable ID to show the camera
	if(m_serverApiVer >= 0.6)
		return;
	
	// Check for error condition from switch request:
	// 	<< "cmd" << cmd
	// 	<< "status" << "error"
	// 	<< "message" << "Invalid DrawableID");
	
	QVariantMap replyMap = reply.toMap();
	if(replyMap["status"].toString()  == "error" &&
	   replyMap["message"].toString() == "Invalid DrawableID")
	{
		qDebug() << "Unable to change video input due to scene change on server, re-examining scene.";
		
		// Scene may have changed, find a new video drawable to control
		m_connectBtn->setEnabled(false);
		m_connectBtn->setText("Checking...");
		
		// processExamineSceneReply() will use this flag to re-send the connection string after receiving examine response
		m_resendLastCon = true;
		
		// Request scene breakdown
		m_lastReqType = T_ExamineScene;
		loadUrl(tr("http://%1:9979/ExamineCurrentScene").arg(m_host));
		
	}
}

void SwitchMonWidget::createViewers()
{
	// If the layout isnt created yet, then we create it - 
	// We've delayed initial creation of the layout till here in order that we can check
	// the window orientation (Horizontal or Vertical) and use the appros layout.
	// Note that if window orientation changes later, resizeEvent() will handle changing
	// the layout type.
	if(!m_viewerLayout)
	{
		m_vbox->removeItem(m_bottomRow);
		
		m_viewerLayout = width() > height() ? (QLayout*)(new QHBoxLayout()) : (QLayout*)(new QVBoxLayout());
		m_viewerLayout->setContentsMargins(0,0,0,0);
		m_viewerLayout->setSpacing(0);
		m_vbox->addLayout(m_viewerLayout);
		m_vbox->addLayout(m_bottomRow);
	}

	//if(m_inputList.isEmpty())
	//    // Add in the "Live" output
	//    m_inputList.prepend(tr("net=%1:9978").arg(m_host));
	
	// Add in the "Live" output because when we generate a layout with 
	// no viewers at all to start out with, the layout doesn't
	// automatically expand when we receive the list of streams from the server.
	// By adding in a known stream to start out with (which gets removed when
	// we get the list from the server) we "prop up" the layout till the list arrives.
	//if(m_inputList.isEmpty())
	//	m_inputList.append(tr("net=%1:9978").arg(m_host));
	//if(!m_inputList.isEmpty())
	    //m_inputList.takeFirst();

	qDebug() << "SwitchMonWidget::createViewers: Creating viewers for: "<<m_inputList;
	
	int idx = 0;
	foreach(QVariant entry, m_inputList)
	{
		QString con = entry.toString();
		
		QHash<QString,QString> map;
		QStringList opts = con.split(",");
		foreach(QString pair, opts)
		{
			QStringList values = pair.split("=");
			if(values.size() < 2)
			{
				qDebug() << "SwitchMonWidget::processInputEnumReply: Parse error for option:"<<pair;
				continue;
			}
	
			QString name = values[0].toLower();
			QString value = values[1];
	
			map[name] = value;
		}
		
		QStringList parts = map["net"].split(":");
		QString host = parts[0];
		int port = parts[1].toInt();
		//qDebug() << "Input "<<idx<<": host:"<<host<<", port:"<<port;
		
		VideoWidget *widget = new VideoWidget();
		VideoReceiver *receiver = VideoReceiver::getReceiver(host,port);
		
		//widget->setVideoBackgroundColor(QColor(50,50,50));
		widget->setVideoSource(receiver);
		widget->setOverlayText(idx == 0 ? tr("Live") : tr("Cam %1").arg(idx));
		if(port != 9978)
		    widget->setProperty("con", con);
		
		if(con == m_lastLiveCon)
		{
			widget->setVideoBorderColor(QColor(Qt::red));
			m_lastLiveWidget = widget;
		}
		
		connect(widget, SIGNAL(clicked()), this, SLOT(vidWidgetClicked()));
		
		m_viewerLayout->addWidget(widget);
		
		// Dont leave empty receivers visible - mainly for the first live viewer
		if(!idx && (!receiver || !receiver->isConnected()))
			widget->hide();
		
		idx++;
	}
	
	//adjustSize();

}

void SwitchMonWidget::clearViewerLayout()
{
	if(!m_viewerLayout)
		return;
		
	// Clear the slider grid of old controls
	while(m_viewerLayout->count() > 0)
	{
		QLayoutItem *item = m_viewerLayout->itemAt(m_viewerLayout->count() - 1);
		m_viewerLayout->removeItem(item);
		if(QWidget *widget = item->widget())
		{
			// disconnect any slots
			disconnect(widget, 0, this, 0);
			
			m_viewerLayout->removeWidget(widget);
			delete widget;
		}
			
		delete item;
		item = 0;
	}	
}

void SwitchMonWidget::processInputEnumReply(const QByteArray &bytes)
{
	clearViewerLayout();
	
	bool ok = false;
	QVariant reply = m_parser->parse(bytes,&ok);
	// Sample: { "cmd" : "ListVideoInputs", "list" : [ "dev=/dev/video0,input=Composite1,net=10.10.9.90:7755", "dev=/dev/video1,input=Composite1,net=10.10.9.90:7756" ] }
	
	QVariantMap replyMap = reply.toMap();
	
	QString apiString = replyMap["api"].toString();
	if(apiString.isEmpty()) // assume older code (pre-0.6 didn't advertise API ver)
		apiString = "0.5";
	
	m_serverApiVer = apiString.toDouble();
	qDebug() << "SwitchMonWidget::processInputEnumReply: Server API Version: "<<m_serverApiVer;
	
	m_inputList = replyMap["list"].toList();
	
	// Add in the "Live" output
	//m_inputList.prepend(tr("net=%1:9978").arg(m_host));
	
	createViewers();
	
	// In 0.6, we added the "ShowVideoConnection" command,
	// so we don't need to examine the current scene for
	// a drawable ID inorder to show the camera.
	if(m_serverApiVer >= 0.6)
	{
		m_connectBtn->setEnabled(false);
		m_connectBtn->setText("Connected");
		return;
	}
	
	
	m_lastReqType = T_ExamineScene;
	loadUrl(tr("http://%1:9979/ExamineCurrentScene").arg(m_host));
	
	m_connectBtn->setEnabled(false);
	m_connectBtn->setText("Checking...");
}

void SwitchMonWidget::processExamineSceneReply(const QByteArray &bytes)
{
	bool ok = false;
	QVariant reply = m_parser->parse(bytes,&ok);
	// Sample: { "cmd" : "ListVideoInputs", "list" : [ "dev=/dev/video0,input=Composite1,net=10.10.9.90:7755", "dev=/dev/video1,input=Composite1,net=10.10.9.90:7756" ] }
	QVariantMap replyMap = reply.toMap();
// 	- GET http://localhost:9979/ExamineCurrentScene
// 		- RESPONSE:
// 			{ "cmd" : "ExamineCurrentScene", 
// 				"items" : 
// 				[ 
// 					{ 
// 						"id" : 1293, "name" : "", "playlist" : [  ], 
// 						"props" : 
// 						[ 
// 							{ "name" : "objectName", "type" : "QString", "value" : "" }
// 							, { "name" : "id", "type" : "int", "value" : "1293" }
// 							...
// 							, { "name" : "videoConnection", "type" : "QString", "value" : "dev=/dev/video0,input=Composite1,net=10.10.9.90:7755" }
// 							, { "name" : "videoInput", "type" : "QString", "value" : "/dev/video0" }
// 						]
// 						, "type" : "VideoInput" 
// 					}
// 				]
// 				, "status" : "true" 
// 			}
// 			
// 			var item = obj.items[0];
// 			var vidItemID = item.id if item.type == "VideoInput";
// 			
	QVariantList itemList = replyMap["items"].toList();
	
	//qDebug() << "SwitchMonWidget::processExamineSceneReply: found "<<itemList.size()<<" items";
	
	m_drawableId = -1;
	foreach(QVariant itemData, itemList)
	{
		if(m_drawableId < 0)
		{
			QVariantMap itemMap = itemData.toMap();
			
			QString type = itemMap["type"].toString();
			int id = itemMap["id"].toInt();
			
			if(type == "VideoInput")
				m_drawableId = id;
			
			//qDebug() << "SwitchMonWidget::processExamineSceneReply: checking itemId:"<<id<<", type:"<<type<<", final m_drawableId:"<<m_drawableId;
		}
	}
		
	m_connectBtn->setEnabled(false);
	m_connectBtn->setText("Connected");
	
	if(m_resendLastCon)
	{
		m_resendLastCon = false;
		sendCon(m_lastCon);
	}
}
