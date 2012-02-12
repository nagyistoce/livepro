#include "MainWindow.h"

#include "GLWidget.h"
#include "VideoReceiver.h"
#include "GLSceneGroup.h"
#include "GLVideoInputDrawable.h"

#include "HistogramFilter.h"
#include "ExpandableWidget.h"
#include "EditorUtilityWidgets.h"

#include "PlayerConnection.h"

#define PRESET_SETTINGS_FILE "presets.ini"
#define GetPresetsSettings() QSettings settings(PRESET_SETTINGS_FILE,QSettings::IniFormat);

MainWindow::MainWindow()
	: QWidget()
	, m_drw(0)
	, m_group(0)
	, m_filter(0)
	, m_filterDrawable(0)
{
	connect(&m_hintWackTimer, SIGNAL(timeout()), this, SLOT(hintWack()));
	
	QSettings settings;
	// Setup the layout of the window
	QVBoxLayout *vbox = new QVBoxLayout(this);
	//vbox->setContentsMargins(0,0,0,0);
	
	QHBoxLayout *conLay = new QHBoxLayout();
	conLay->addWidget(new QLabel("Server: "));
	//conLay->setContentsMargins(5,5,5,5);
	//conLay->setSpacing(3);

	m_serverBox = new QLineEdit();
	m_serverBox->setText(settings.value("lastServer","192.168.0.16").toString());
	connect(m_serverBox, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));
	conLay->addWidget(m_serverBox);
	
	m_connectBtn = new QPushButton("Connect");
	connect(m_connectBtn, SIGNAL(clicked()), this, SLOT(connectToServer()));
	conLay->addWidget(m_connectBtn);
	
	conLay->addWidget(new QLabel("Input:"));
	m_inputBox = new QComboBox();
	m_inputBox->setEnabled(false); // will enable when rx list of video inputs
	connect(m_inputBox, SIGNAL(currentIndexChanged(int)), this, SLOT(videoSourceChanged(int)));
	conLay->addWidget(m_inputBox, 1);
	
	//conLay->addStretch(1);
	
	vbox->addLayout(conLay);
	
	setWindowTitle("Hint Calibration");
	
	QSplitter *split = new QSplitter(this);
	split->setOrientation(Qt::Vertical);
	vbox->addWidget(split);
	
	// Create the player object
	m_player = new PlayerConnection();
	connect(m_player, SIGNAL(videoInputListReceived(const QStringList&)), this, SLOT(videoInputListReceived(const QStringList&)));
	
	// Create the GLWidget to do the actual rendering
	m_glw = new GLWidget(this);
	//vbox->addWidget(m_glw);
	split->addWidget(m_glw);
	
	//glw->setViewport(QRect(0,0,1000, 750));
	
	QScrollArea *controlArea = new QScrollArea(this);
	controlArea->setWidgetResizable(true);
	//vbox->addWidget(controlArea);
	split->addWidget(controlArea);
	
	QWidget *controlBase = new QWidget(controlArea);
	
	m_layout = new QVBoxLayout(controlBase);
	m_layout->setContentsMargins(0,0,0,0);
	controlArea->setWidget(controlBase);
	
	connectToServer();

}

void MainWindow::createControlUI()
{
	if(!m_drw)
		return;
		
	if(m_filterDrawable)
	{
		m_filterGlw->removeDrawable(m_filterDrawable);
		
		m_filterDrawable->setVideoSource(0);
		delete m_filterDrawable;//->deleteLater();
		m_filterDrawable = 0;
	}
	
	if(m_filter)
	{
		delete m_filter;//->deleteLater();
		m_filter = 0;
	}
	
	while(m_layout->count() > 0)
	{
		QLayoutItem *item = m_layout->itemAt(m_layout->count() - 1);
		m_layout->removeItem(item);
		if(QWidget *widget = item->widget())
		{
			m_layout->removeWidget(widget);
			delete widget;
		}
			
		delete item;
		item = 0;
	}
	
	#define NEW_SECTION(title) \
			ExpandableWidget *expand = new ExpandableWidget(title); \
			expand->setExpandedIfNoDefault(true); \
			QWidget *base = new QWidget(); \
			QFormLayout *form = new QFormLayout(base); \
			expand->setWidget(base); \
			m_layout->addWidget(expand); \
			PropertyEditorFactory::PropertyEditorOptions opts; \
			opts.reset();
		
// 	GLVideoDrawable *item = widgetVid ? widgetVid->drawable() : widgetCam->drawable();
// 	m_vid = item;
	
	GLVideoDrawable *item = m_drw;
	
	// Histo
	{
		NEW_SECTION("Histogram");
		
		GLWidget *vid = new GLWidget();
		vid->setBackgroundColor(palette().color(QPalette::Window));
		vid->setViewport(QRectF(0,0,255*3,128*3)); // approx histogram output size for a 4:3 video frame
		
		HistogramFilter *filter = new HistogramFilter();
		
		GLVideoDrawable *gld = new GLVideoDrawable();
		gld->setVideoSource(filter);
		
		gld->setFilterType(GLVideoDrawable::Filter_Sharp);
		gld->setSharpAmount(1.25); // creates a nice relief/outline effect on the histogram
		
		vid->addDrawable(gld);
		gld->setRect(vid->viewport());
		
		form->addWidget(vid);
		
		filter->setHistoType(HistogramFilter::Gray);
		//filter->setVideoSource(item->videoSource());
		// Use the stream from the GLWidget instead of the original video source
		// because we want the Histogram to reflect the *changes*, such as
		// hue/brightness/saturation - if we rendered the original video stream
		// in the histo, it wouldn't show any changes (since the changes are 
		// actually applied on the GPU, not before.)
		filter->setVideoSource(item->glWidget()->outputStream());
		filter->setIncludeOriginalImage(false);
		filter->setFpsLimit(10);
		filter->setDrawBorder(true);
		
		m_filter = filter;
		m_filterDrawable = gld;
		m_filterGlw = vid;
	}
	
	// Profiles
	{
		NEW_SECTION("Store/Load Preset");
		
		m_settingsCombo = new QComboBox();
		m_settingsList.clear();
		
		GetPresetsSettings();
		
		QVariantList list = settings.value(QString("presets/%1").arg(m_currentConString)).toList();
		QStringList nameList = QStringList(); 
		foreach(QVariant data, list)
		{
			QVariantMap map = data.toMap();
			
			nameList << map["name"].toString();
			m_settingsList << map;
		}
		
		m_settingsCombo->addItems(nameList);
		//connect(combo, SIGNAL(activated(QString)), this, SLOT(loadVidOpts(QString)));
		form->addRow(tr("&Settings:"), m_settingsCombo);
		
		QHBoxLayout *hbox = new QHBoxLayout();
		
		QPushButton *btn1 = new QPushButton("Load");
		connect(btn1, SIGNAL(clicked()), this, SLOT(loadPreset()));
		hbox->addWidget(btn1);
		
		QPushButton *btn2 = new QPushButton("Save As...");
		connect(btn2, SIGNAL(clicked()), this, SLOT(savePreset()));
		hbox->addWidget(btn2);
	
		QPushButton *btn3 = new QPushButton("Delete");
		connect(btn3, SIGNAL(clicked()), this, SLOT(deletePreset()));
		hbox->addWidget(btn3);
		
		form->addRow("",hbox);
		
		//QVariant var = source->property("_currentSetting");
		QVariant var = settings.value(QString("presets/%1-lastPreset").arg(m_currentConString));
		
		int idx = 0;
		if(var.isValid())
			idx = var.toInt();
		
		if(idx > -1 && idx < m_settingsList.size())
		{
			m_settingsCombo->setCurrentIndex(idx);
			loadPreset(false); // false = dont call setSource(...);
		}
	}
	
	// Levels
	{
		NEW_SECTION("White/Black Levels");
		
		opts.min = 0;
		opts.max = 255;
		opts.step = 5;
		
		form->addRow(tr("&Black:"), PropertyEditorFactory::generatePropertyEditor(item, "blackLevel", SLOT(setBlackLevel(int)), opts));
		//form->addRow(tr("&Mid:"),   PropertyEditorFactory::generatePropertyEditor(item, "midLevel",   SLOT(setMidLevel(int)), opts));
		form->addRow(tr("&White:"), PropertyEditorFactory::generatePropertyEditor(item, "whiteLevel", SLOT(setWhiteLevel(int)), opts));
		
// 			opts.min = 0.1;
// 			opts.max = 3.0;
// 			form->addRow(tr("&Gamma:"), PropertyEditorFactory::generatePropertyEditor(item, "gamma", SLOT(setGamma(double)), opts));
// 			
		QPushButton *btn = new QPushButton("Apply to Player");
		connect(btn, SIGNAL(clicked()), this, SLOT(sendVidOpts()));
		form->addRow("",btn);
	}
	
	// Color Adjust
	{
		NEW_SECTION("Color Adjustments");
		
		opts.min = -100;
		opts.max = +100;
		opts.step = 5;
		
		form->addRow(tr("&Brightness:"), 	PropertyEditorFactory::generatePropertyEditor(item, "brightness", SLOT(setBrightness(int)), opts));
		form->addRow(tr("&Contrast:"), 		PropertyEditorFactory::generatePropertyEditor(item, "contrast",   SLOT(setContrast(int)), opts));
		form->addRow(tr("&Saturation:"), 	PropertyEditorFactory::generatePropertyEditor(item, "saturation", SLOT(setSaturation(int)), opts));
		form->addRow(tr("&Hue:"), 		PropertyEditorFactory::generatePropertyEditor(item, "hue",        SLOT(setHue(int)), opts));
		
		QPushButton *btn = new QPushButton("Apply to Player");
		connect(btn, SIGNAL(clicked()), this, SLOT(sendVidOpts()));
		form->addRow("",btn);
	}
	
	// Advanced Filters
	{
		NEW_SECTION("Advanced Filters");
		
		QComboBox *combo = new QComboBox();
		QStringList filterNames = QStringList() 
			<< "(None)"
			<< "Sharp"
			<< "Blur"
			<< "Mean"
			<< "Bloom"
			<< "Emboss"
			<< "Edge";
		combo->addItems(filterNames);
		connect(combo, SIGNAL(activated(QString)), this, SLOT(setAdvancedFilter(QString)));
		form->addRow(tr("&Filter:"), combo);
		
		combo->setCurrentIndex(indexOfFilter((int)item->filterType()));
		
		opts.min = 0;
		opts.max = 2;
		opts.step = 1;//.25;
		
		m_sharpAmountWidget = PropertyEditorFactory::generatePropertyEditor(item, "sharpAmount", SLOT(setSharpAmount(double)), opts);
		m_sharpAmountWidget->setEnabled(item->filterType() == GLVideoDrawable::Filter_Sharp);
		
		form->addRow(tr("&Sharp Amount:"), m_sharpAmountWidget);
		
		QPushButton *btn = new QPushButton("Apply to Player");
		connect(btn, SIGNAL(clicked()), this, SLOT(sendVidOpts()));
		form->addRow("",btn);
	}
		
	// Croping
	{
		NEW_SECTION("Croping/Straightening");
		
		opts.doubleIsPercentage = true;
		opts.suffix = "%";
		opts.type = QVariant::Int;
		
		opts.min = 0;
		opts.max = 100;
		form->addRow(tr("Crop &Left:"),		PropertyEditorFactory::generatePropertyEditor(item, "cropLeft",   SLOT(setCropLeft(int)), opts));
		opts.min = -100;
		opts.max = 0;
		form->addRow(tr("Crop &Right:"),	PropertyEditorFactory::generatePropertyEditor(item, "cropRight",  SLOT(setCropRight(int)), opts));
		opts.min = 0;
		opts.max = 100;
		form->addRow(tr("Crop &Top:"),		PropertyEditorFactory::generatePropertyEditor(item, "cropTop",    SLOT(setCropTop(int)), opts));
		opts.min = -100;
		opts.max = 0;
		form->addRow(tr("Crop &Bottom:"),	PropertyEditorFactory::generatePropertyEditor(item, "cropBottom", SLOT(setCropBottom(int)), opts));
		
		opts.reset();
		//opts.suffix = " deg";
		opts.min = -360.0;
		opts.max =  360.0;
		opts.step = 1;
		opts.defaultValue = 0;
		//opts.type = QVariant::Int;
		opts.type = QVariant::Double;
		opts.value = item->rotation().z();
		form->addRow(tr("&Straighten:"), PropertyEditorFactory::generatePropertyEditor(this, "zRotation", SLOT(setStraightValue(double)), opts));

		
		QPushButton *btn = new QPushButton("Apply to Player");
		connect(btn, SIGNAL(clicked()), this, SLOT(sendVidOpts()));
		form->addRow("",btn);
	}
	
	// A/R Adjust
	{
		NEW_SECTION("A/R Adjust");
		
		opts.doubleIsPercentage = true;
		opts.suffix = "%";
		opts.type = QVariant::Int;
		
		opts.min = 0;
		opts.max = 100;
		form->addRow(tr("Adjust &Left:"),	PropertyEditorFactory::generatePropertyEditor(item, "adjustLeft",   SLOT(setAdjustLeft(int)), opts));
		opts.min = -100;
		opts.max = 0;
		form->addRow(tr("Adjust &Right:"),	PropertyEditorFactory::generatePropertyEditor(item, "adjustRight",  SLOT(setAdjustRight(int)), opts));
		opts.min = 0;
		opts.max = 100;
		form->addRow(tr("Adjust &Top:"),	PropertyEditorFactory::generatePropertyEditor(item, "adjustTop",    SLOT(setAdjustTop(int)), opts));
		opts.min = -100;
		opts.max = 0;
		form->addRow(tr("Adjust &Bottom:"),	PropertyEditorFactory::generatePropertyEditor(item, "adjustBottom", SLOT(setAdjustBottom(int)), opts));
		
		QPushButton *btn = new QPushButton("Apply to Player");
		connect(btn, SIGNAL(clicked()), this, SLOT(sendVidOpts()));
		form->addRow("",btn);
	}
	
	// Flip
	{
		NEW_SECTION("Flip/Mirror");
		
		opts.text = "Flip Horizontal";
		form->addRow(PropertyEditorFactory::generatePropertyEditor(item, "flipHorizontal", SLOT(setFlipHorizontal(bool)), opts));
		
		opts.text = "Flip Vertical";
		form->addRow(PropertyEditorFactory::generatePropertyEditor(item, "flipVertical", SLOT(setFlipVertical(bool)), opts));
		
		QPushButton *btn = new QPushButton("Apply to Player");
		connect(btn, SIGNAL(clicked()), this, SLOT(sendVidOpts()));
		form->addRow("",btn);
	}
	
	
	m_layout->addStretch(1);	
}


void MainWindow::connectToServer()
{
	if(!m_receivers.isEmpty())
	{
		foreach(VideoReceiver *rx, m_receivers)
			rx->release(this); // release receiver and allow to be deleted
		m_receivers.clear();
	}
	
	m_isConnected = false;
	QString server = m_serverBox->text();
	
	m_connectBtn->setEnabled(false);
	m_connectBtn->setText("Connected");
	
	// Store for later use
	QSettings().setValue("lastServer",server);

	// Reset state variables
	m_hasVideoInputsList = false;
	m_lastInputIdx = -1;
	
	// Connect to player
	m_player->setHost(server);
	m_player->connectPlayer();
	
	// Update combo box
	if(m_player->videoIputsReceived())
		videoInputListReceived(m_player->videoInputs());
	
	// Create the controls
	createControlUI();
}

void MainWindow::videoSourceChanged(int idx)
{
	if(m_lastInputIdx == idx)
		return;
		
	m_lastInputIdx = idx;
	if(idx < 0 || idx >= m_videoInputs.size())
	{
		qDebug() << "MainWindow::videoSourceChanged: Invalid idx:"<<idx;
		return; 
	}
	
	QString conString = m_videoInputs[idx];
	m_currentConString = conString; // used to store/load presets
	
	//QUrl url = GLVideoInputDrawable::extractUrl(conString);
		
// 	VideoReceiver *rx = VideoReceiver::getReceiver(host,port);
// 	m_drw->setVideoSource(rx);
	if(m_group)
	{
		m_group->at(0)->detachGLWidget();
		delete m_group;
		
		m_drw = 0;
	}

	GLScene *scene = new GLScene();
	
	// Finally, get down to actually creating the drawables 
	//int frameWidth = 1000, frameHeight = 750, x=0, y=0;
	
	m_drw = new GLVideoInputDrawable();
	//m_drw->setRect(QRectF(x,y,frameWidth,frameHeight));
	m_drw->setVideoConnection(conString); //tr("dev=/dev/video0,input=Composite1,net=%1").arg(server));
	
	// Finally, add the drawable
	scene->addDrawable(m_drw);
	
	// Add the drawables to the GLWidget
	scene->setGLWidget(m_glw);
	
	// Used for setting up the player and deleting when changing sources
	m_group = new GLSceneGroup();
	m_group->addScene(scene);
	
	//m_videoWidget->setVideoSource(rx);
	qDebug() << "MainWindow::videoSourceChanged(): Connecting to" << conString; //url.host()<<url.port();;//<<", rx:"<<rx;
	
	m_drw->loadHintsFromSource();
	
	VideoReceiver *rx = dynamic_cast<VideoReceiver*>(m_drw->videoSource());
	if(rx)
	{
		bool flag = false;
		rx->videoHints(&flag);
		if(!flag)
		{
			m_hintWackTimer.setInterval(500);
			m_hintWackTimer.start();
			
			connect(rx, SIGNAL(currentVideoHints(QVariantMap)), this, SLOT(hintsReceived()));
		}
	}
	
	createControlUI();
}

void MainWindow::hintWack()
{
	VideoReceiver *rx = dynamic_cast<VideoReceiver*>(m_drw->videoSource());
	if(rx)
	{
	
		// Hints not received yet, request hints again from video sender and then rebuild UI when received
		qDebug() << "MainWindow::hintWack(): Hint Wack...";
		rx->queryVideoHints();
		
		// Try again
		bool flag=false;
		rx->videoHints(&flag);
		//qDebug() << "Debug: Try again, flag:"<<flag;
		qDebug() << "MainWindow::hintWack(): Right after query, flag:"<<flag;
		if(flag)
			hintsReceived();
	}
}

void MainWindow::videoInputListReceived(const QStringList& inputs)
{
	m_isConnected = true;
	
	//qDebug() << "MainWindow::videoInputListReceived: "<<inputs;
	
	if(m_hasVideoInputsList)
		return;
	if(inputs.isEmpty())
		return;
		
	m_hasVideoInputsList = true;
//  	int index = 0;
 	
 	m_inputBox->clear();
 	m_inputBox->setEnabled(true);
 	
 	QStringList itemList;
 	foreach(QString con, inputs)
	{
		QHash<QString,QString> map = GLVideoInputDrawable::parseConString(con);
		//itemList << tr("%1 (%2)").arg(map["dev"]).arg(map["net"]);
		itemList << tr("%1").arg(map["dev"]);
		
		VideoReceiver *rx = VideoReceiver::getReceiver( map["net"] );
		rx->registerConsumer(this); // hold reference to this receiver so it doesn't get deleted
		m_receivers << rx; 
	}
	
	m_videoInputs = inputs;
	
	m_inputBox->addItems(itemList);
	m_inputBox->setCurrentIndex(0);
	
	videoSourceChanged(0);
}

void MainWindow::hintsReceived()
{
	if(m_hintWackTimer.isActive())
		m_hintWackTimer.stop();
	
	VideoReceiver *rx = dynamic_cast<VideoReceiver*>(m_drw->videoSource());
	if(rx)
	{
		bool flag = flag;
		QVariantMap hints = rx->videoHints(&flag);
		
		qDebug() << "MainWindow::hintsReceived(): flag:"<<flag<<", hints:"<<hints;
	}
	else
	{
		qDebug() << "MainWindow::hintsReceived(): Got signal, but m_drw source is not a VideoReceiver";
	}
		
		
	// We know hints we're received now (because this slot is triggered by the receiption of the hints),
	// so we tell the drawable to load the hints again now that we know they're here.
	m_drw->loadHintsFromSource();
	
	// Create the controls (which will use the newly received hints)
	createControlUI();
}

void MainWindow::textChanged(QString)
{
	m_connectBtn->setEnabled(true);
	m_connectBtn->setText("Connect");
}

void MainWindow::setAdvancedFilter(QString name)
{
	if(m_sharpAmountWidget)
		m_sharpAmountWidget->setEnabled(name == "Sharp");
	/*	
			QStringList filterNames = QStringList() 
				<< "(None)"
				<< "Sharp"
				<< "Blur"
				<< "Mean"
				<< "Bloom"
				<< "Emboss"
				<< "Edge";
	*/
	
	GLVideoDrawable::FilterType type = 
		name == "(None)" ?	GLVideoDrawable::Filter_None :
		name == "Sharp" ?	GLVideoDrawable::Filter_Sharp :
		name == "Blur" ?	GLVideoDrawable::Filter_Blur :
		name == "Mean" ?	GLVideoDrawable::Filter_Mean :
		name == "Bloom" ?	GLVideoDrawable::Filter_Bloom :
		name == "Emboss" ?	GLVideoDrawable::Filter_Emboss :
		name == "Edge" ?	GLVideoDrawable::Filter_Edge :
		GLVideoDrawable::Filter_None;
	
	m_drw->setFilterType(type);
}

int MainWindow::indexOfFilter(int filter)
{
	GLVideoDrawable::FilterType type = (GLVideoDrawable::FilterType)filter;
	if(type == GLVideoDrawable::Filter_None)
		return 0;
		
	if(type == GLVideoDrawable::Filter_Sharp)
		return 1;
	if(type == GLVideoDrawable::Filter_Blur)
		return 2;
	if(type == GLVideoDrawable::Filter_Mean)
		return 3;
	if(type == GLVideoDrawable::Filter_Bloom)
		return 4;
	if(type == GLVideoDrawable::Filter_Emboss)
		return 5;
	if(type == GLVideoDrawable::Filter_Edge)
		return 6;
	
	return 0;
}	

void MainWindow::sendVidOpts()
{
	if(!m_player || !m_drw)
		return;
	
	QStringList props = QStringList() 
		<< "whiteLevel"
		<< "blackLevel"
// 		<< "midLevel"
// 		<< "gamma"
		<< "brightness"
		<< "contrast" 
		<< "hue"
		<< "saturation"
		<< "flipHorizontal"
		<< "flipVertical"
		<< "cropTop"
		<< "cropBottom"
		<< "cropLeft"
		<< "cropRight"
		<< "adjustTop"
		<< "adjustBottom"
		<< "adjustLeft"
		<< "adjustRight"
		<< "filterType"
		<< "sharpAmount"
		<< "rotation";
		
		
	
	if(m_player->isConnected())
	{
		//if(m_player->lastGroup() != m_group)
 			m_player->setGroup(m_group, m_group->at(0));
		
		foreach(QString prop, props)
		{
			m_player->setUserProperty(m_drw, m_drw->property(qPrintable(prop)), prop);
		}
	}
	
	m_drw->storeHintsToSource();
}


void MainWindow::setStraightValue(double value)
{
	if(!m_drw)
		return;
	
	// here's the easy part
	QVector3D rot = m_drw->rotation(); 
	rot.setZ(value); 
	m_drw->setProperty("rotation", rot);
	
	// now to calculate cropping
	
	QVector3D rotationPoint = m_drw->rotationPoint();
	//QRectF rect = m_drw->rect();
	
	// We're going to set the rect ourselves here
	QSizeF canvas = m_drw->canvasSize();
	QRectF rect(QPointF(0,0),canvas);

	qreal x = rect.width()  * rotationPoint.x() + rect.x();
	qreal y = rect.height() * rotationPoint.y() + rect.y();
	
	QTransform transform = QTransform()
		.translate(x,y)
		.rotate(rot.x(),Qt::XAxis)
		.rotate(rot.y(),Qt::YAxis)
		.rotate(rot.z(),Qt::ZAxis)
		.translate(-x,-y);
	
	QRectF rotRect = transform.mapRect(rect);
	
	//qDebug() << "PropertyEditorWindow::setStraightValue("<<value<<"): orig rect:"<<rect<<", rotated rect:"<<rotRect;
	
	QPointF tld = rotRect.topLeft()     - rect.topLeft();
	QPointF brd = rotRect.bottomRight() - rect.bottomRight();
// 	qDebug() << "\t topLeftDiff:		"<<tld;
// 	qDebug() << "\t bottomRightDiff:	"<<brd;
	 
// 	PropertyEditorWindow::setStraightValue( 19 ): orig rect: QRectF(0,0 1000x750) , rotated rect: QRectF(-94.8473,-142.354 1189.69x1034.71)
//          topLeftDiff:            QPointF(-94.8473, -142.354)
//          bottomRightDiff:        QPointF(94.8473, 142.354)

	// 2.2 is just a "magic number" found by guessing
	
	double rx = tld.x()*2.2;
	double ry = tld.y()*2.2;
	double rw = canvas.width() + fabs(rx*2);
	double rh = canvas.height() + fabs(ry*2);

	QRectF newRect = QRectF(rx,ry,rw,rh);
	m_drw->setProperty("rect", newRect);	
	
	//qDebug() << "\t newRect:        	"<<newRect;
}



void MainWindow::deletePreset()
{
	if(!m_settingsCombo || !m_drw)
		return;
	
	int idx = m_settingsCombo->currentIndex();
	if(idx < 0 || idx >= m_settingsList.size())
		return;
	qDebug() << "MainWindow::deletePreset(): idx:"<<idx;
	QVariantMap map = m_settingsList[idx];

	if(QMessageBox::question(this, "Really Delete?", QString("Are you sure you want to delete '%1'?").arg(map["name"].toString()),
		QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok)
	{
		GetPresetsSettings();
		
		QString key = QString("presets/%1").arg(m_currentConString);
		QVariantList list = settings.value(key).toList(), newList;
		QStringList nameList = QStringList();
		int counter = 0; 
		foreach(QVariant data, list)
		{
			if(counter != idx)
			{
				newList << data;
			}
			
			counter ++;
		}
		
		settings.setValue(key, newList);
	}
	
	createControlUI();
}

void MainWindow::loadPreset(bool setSource)
{
	if(!m_settingsCombo || !m_drw)
		return;
	
	int idx = m_settingsCombo->currentIndex();
	if(idx < 0 || idx >= m_settingsList.size())
		return;
		 
	QVariantMap map = m_settingsList[idx];
	
	QList<QString> props = map.keys();
	
	foreach(QString prop, props)
	{
		m_drw->setProperty(qPrintable(prop), map[prop]);
		
		if(m_player->isConnected())
			m_player->setUserProperty(m_drw, m_drw->property(qPrintable(prop)), prop);
	}
	
	GetPresetsSettings();
	settings.setValue(QString("presets/%1-lastPreset").arg(m_currentConString), idx);
	
	if(setSource)
		createControlUI();
}

void MainWindow::savePreset()
{
	if(!m_settingsCombo || !m_drw)
		return;

	QStringList props = QStringList() 
		<< "whiteLevel"
		<< "blackLevel"
// 		<< "midLevel"
// 		<< "gamma"
		<< "brightness"
		<< "contrast" 
		<< "hue"
		<< "saturation"
		<< "flipHorizontal"
		<< "flipVertical"
		<< "cropTop"
		<< "cropBottom"
		<< "cropLeft"
		<< "cropRight"
		<< "adjustTop"
		<< "adjustBottom"
		<< "adjustLeft"
		<< "adjustRight"
		
		<< "filterType"
		<< "sharpAmount"
		<< "rotation";
		
	QVariantMap map;
	
	foreach(QString prop, props)
	{
		map[prop] = m_drw->property(qPrintable(prop));
	}
	
	int idx = m_settingsCombo->currentIndex();
	
	GetPresetsSettings();
	
	QString name = "";
	if(idx >= 0)
	{
		name = m_settingsCombo->itemText(idx);
	}
	else
	{
		QVariant var = settings.value(QString("presets/%1-lastPreset").arg(m_currentConString));
		if(var.isValid())
		{
			name = m_settingsCombo->itemText(var.toInt());
		}
	}
	
	bool ok;
	QString text = QInputDialog::getText(this, tr("Setting Name"),
						   tr("Setting name:"), QLineEdit::Normal,
						   name, &ok);
	
	if (ok && !text.isEmpty())
	{
		name = text;
		map["name"] = name;
	
		bool found = false;
		QVariantList newList;
		
		QString key = QString("presets/%1").arg(m_currentConString);
		QVariantList list = settings.value(key).toList();
		QStringList nameList = QStringList(); 
		foreach(QVariant data, list)
		{
			QVariantMap existingMap = data.toMap();
			
			if(existingMap["name"].toString() == name)
			{
				newList << QVariant(map);
				found = true;
				//idx = newList.size()-1;
			}
			else
			{
				newList << data;
			}
		}
		
		if(!found)
		{
			newList << QVariant(map);
			idx = newList.size() - 1;
		}
		
		settings.setValue(key, newList);
	}
	
	settings.setValue(QString("presets/%1-lastPreset").arg(m_currentConString), idx);
	createControlUI();
}
