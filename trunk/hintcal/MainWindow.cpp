#include "MainWindow.h"

#include "GLWidget.h"
#include "VideoReceiver.h"
#include "GLSceneGroup.h"
#include "GLVideoInputDrawable.h"

#include "HistogramFilter.h"
#include "ExpandableWidget.h"
#include "EditorUtilityWidgets.h"

#include "PlayerConnection.h"

MainWindow::MainWindow()
	: QWidget()
{
	QSettings settings;
	// Setup the layout of the window
	QVBoxLayout *vbox = new QVBoxLayout(this);
	//vbox->setContentsMargins(0,0,0,0);
	
	QHBoxLayout *conLay = new QHBoxLayout();
	conLay->addWidget(new QLabel("Server: "));
	//conLay->setContentsMargins(5,5,5,5);
	//conLay->setSpacing(3);

	m_serverBox = new QLineEdit();
	m_serverBox->setText(settings.value("lastServer","192.168.0.16:7755").toString());
	connect(m_serverBox, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));
	conLay->addWidget(m_serverBox);
	
	m_connectBtn = new QPushButton("Connect");
	connect(m_connectBtn, SIGNAL(clicked()), this, SLOT(connectToServer()));
	conLay->addWidget(m_connectBtn);
	
	conLay->addStretch(1);
	
	vbox->addLayout(conLay);
	
	setWindowTitle("Hint Calibration");
	
	// Create the player object
	m_player = new PlayerConnection();
	
	// Create the GLWidget to do the actual rendering
	GLWidget *glw = new GLWidget(this);
	vbox->addWidget(glw);
	
	GLScene *scene = new GLScene();
	
	// Finally, get down to actually creating the drawables 
	int frameWidth = 1000, frameHeight = 750, x=0, y=0;
	
	m_drw = new GLVideoInputDrawable();
	m_drw->setRect(QRectF(x,y,frameWidth,frameHeight));
	
	// Finally, add the drawable
	scene->addDrawable(m_drw);
	
	// Add the drawables to the GLWidget
	scene->setGLWidget(glw);
	
	// Used for setting up the player
	m_group = new GLSceneGroup();
	m_group->addScene(scene);
	
	glw->setViewport(QRect(0,0,1000, 750));
	
	
	
	#define NEW_SECTION(title) \
			ExpandableWidget *expand = new ExpandableWidget(title); \
			expand->setExpandedIfNoDefault(true); \
			QWidget *base = new QWidget(); \
			QFormLayout *form = new QFormLayout(base); \
			expand->setWidget(base); \
			vbox->addWidget(expand); \
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
		filter->setVideoSource(item->glWidget()->outputStream());
		filter->setIncludeOriginalImage(false);
		filter->setFpsLimit(10);
		filter->setDrawBorder(true);
	}
	
// 	// Profiles
// 	{
// 		NEW_SECTION("Store/Load Settings");
// 		
// 		m_settingsCombo = new QComboBox();
// 		m_settingsList.clear();
// 		
// 		QSettings s;
// 		QVariantList list = s.value(QString("vidopts/%1").arg(source->windowTitle())).toList();
// 		QStringList nameList = QStringList(); 
// 		foreach(QVariant data, list)
// 		{
// 			QVariantMap map = data.toMap();
// 			
// 			nameList << map["name"].toString();
// 			m_settingsList << map;
// 		}
// 		
// 		m_settingsCombo->addItems(nameList);
// 		//connect(combo, SIGNAL(activated(QString)), this, SLOT(loadVidOpts(QString)));
// 		form->addRow(tr("&Settings:"), m_settingsCombo);
// 		
// 		QHBoxLayout *hbox = new QHBoxLayout();
// 		
// 		QPushButton *btn1 = new QPushButton("Load");
// 		connect(btn1, SIGNAL(clicked()), this, SLOT(loadVidOpts()));
// 		hbox->addWidget(btn1);
// 		
// 		QPushButton *btn2 = new QPushButton("Save As...");
// 		connect(btn2, SIGNAL(clicked()), this, SLOT(saveVidOpts()));
// 		hbox->addWidget(btn2);
// 	
// 		QPushButton *btn3 = new QPushButton("Delete");
// 		connect(btn3, SIGNAL(clicked()), this, SLOT(deleteVidOpt()));
// 		hbox->addWidget(btn3);
// 		
// 		form->addRow("",hbox);
// 		
// 		
// 		QVariant var = source->property("_currentSetting");
// 		if(var.isValid())
// 		{
// 			int idx = var.toInt();
// 			m_settingsCombo->setCurrentIndex(idx);
// 			loadVidOpts(false); // false = dont call setSource(...);
// 		}
// 		
// 	}
	
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
	
	// Croping
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
	
	
	vbox->addStretch(1);

	
	
	
	connectToServer();
}


void MainWindow::connectToServer()
{
// 	m_isConnected = false;
	QString server = m_serverBox->text();
	
	m_connectBtn->setEnabled(false);
	m_connectBtn->setText("Connected");
	
	QSettings().setValue("lastServer",server);

	QStringList list = server.split(":");
	QString host = list[0];
	int port = list[1].toInt();
		
// 	VideoReceiver *rx = VideoReceiver::getReceiver(host,port);
// 	m_drw->setVideoSource(rx);
	m_drw->setVideoConnection(tr("dev=/dev/video0,input=Composite1,net=%1").arg(server));
	//m_videoWidget->setVideoSource(rx);
	qDebug() << "Connected to "<<host<<port;//<<", rx:"<<rx;
	
	m_drw->loadHintsFromSource();
	
	// Connect to player
	m_player->setHost(host);
	m_player->connectPlayer();
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