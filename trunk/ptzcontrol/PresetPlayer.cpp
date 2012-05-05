
#include "MidiInputSettingsDialog.h"
#include "PresetPlayer.h"

#include "PanTiltClient.h"

#include "VideoReceiver.h"
#include "GLSceneGroup.h"
#include "GLVideoInputDrawable.h"
#include "VideoWidget.h"

#include <QApplication>
#include <QDebug>

#define ENABLE_VIDEO_RX

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	app.setApplicationName(argv[0]);
	app.setOrganizationName("Josiah Bryan");
	app.setOrganizationDomain("mybryanlife.com");
	
	PresetPlayer p;
	p.show();
	
	return app.exec();
}
	
#define SETTINGS_FILE "config.ini"
#define GetSettingsObject() QSettings settings(SETTINGS_FILE,QSettings::IniFormat);

#define EXIT_MISSING(field) { qDebug() << "Error: '" field "' value in "<<SETTINGS_FILE<<" is empty, exiting."; exit(-1); }

// Values from Servo.h Arduino library
#define SERVO_MIN 544
#define SERVO_MAX 2400


#define SETUP_SERVO_SPINBOX(spin,val) \
	spin->setMinimum(0); \
	spin->setMaximum(2500); \
	spin->setValue(val);

#define SETUP_ZOOM_SPINBOX(spin,val) \
	spin->setMinimum(1832); \
	spin->setMaximum(2296); \
	spin->setValue(val);
	
#define SETUP_WIN_SPINBOX(spin,val) \
	spin->setMinimum(1); \
	spin->setMaximum(SERVO_MAX); \
	spin->setValue(val);
	
namespace NumberUtils
{
// 	static double mapValue(double mapFromA, double mapFromB, double mapToA, double mapToB, double valueToMap);
// 	static double mapValue(int    mapFromA, int    mapFromB, int    mapToA, int    mapToB, int    valueToMap);
	
// 	double mapValue(double mapFromA, double mapFromB, double mapToA, double mapToB, double valueToMap)
// 	{
// 		// Simple line formulas - find line slope, find y intercept, calculate new X value from "y" (curFps)
// 		// slope: (y2-y1)/(x2-x1)
// 		double slope = 
// 			((((double)mapFromB) - ((double)mapFromA)) /
// 			(((double)mapToB)   - ((double)mapToA))   );
// 		// intercept: y1-slope*x1
// 		double intercept = mapFromA - slope * mapToA;
// 		// new value: slope * x3 + intercept
// 		double newValue = slope * ((double)valueToMap) + intercept;
// 		return newValue;
// 	}
// 	
	int mapValue(int valueToMap,  int mapFromA, int mapFromB,  int mapToA, int mapToB)
	{
		//return (int)mapValue((double)mapFromA, (double)mapFromB, (double)mapToA, (double)mapToB, (double)valueToMap);
		int dist = abs(mapFromB - mapFromA);
		double partial = (double)valueToMap / (double)dist;
		
		int destDist = mapToB - mapToA;
		int finalVal = (int)(destDist * partial) + mapToA;
		
		//qDebug() << "mapValue("<<valueToMap<<","<<mapFromA<<","<<mapFromB<<","<<mapToA<<","<<mapToB<<"): dist:"<<dist<<", partial:"<<partial<<", destDist:"<<destDist<<", finalVal:"<<finalVal;
		
		return finalVal;
	}
}
	
using namespace NumberUtils;



PresetPlayer::PresetPlayer()
	: QWidget()
	, m_lockSendValues(false)
{
	setWindowTitle("Pan/Tilt Control");
	
	GetSettingsObject();
	
	m_vidhost = settings.value("vidhost").toString();
	m_vidport = settings.value("vidport").toInt();
	
	m_ptzhost = settings.value("ptzhost").toString();
	m_ptzport = settings.value("ptzport").toInt();
	
	if(m_vidhost.isEmpty())	EXIT_MISSING("vidhost");
	if(m_vidport <= 0)	EXIT_MISSING("vidport");
	
	if(m_ptzhost.isEmpty())	EXIT_MISSING("ptzhost");
	if(m_ptzport <= 0)	EXIT_MISSING("ptzport");
	
	#ifdef ENABLE_VIDEO_RX
	m_src = VideoReceiver::getReceiver(m_vidhost,m_vidport);
	
	// assuming ssh tunnel, reduce fps to lower bandwidth usage
	if(m_ptzhost ==  "localhost")
		m_src->setFPS(1);
	#endif
	
	setupClient();
	setupGui();
	loadPresets();
	
	//m_updateLiveTimer.start();
	
	//(void*)PresetMidiInputAdapter::instance(this);
	
}

PresetPlayer::~PresetPlayer()
{
	if(m_ptzhost ==  "localhost")
		m_src->setFPS(15);
}

void PresetPlayer::setupClient() 
{
	m_client = new PanTiltClient();
	m_client->start(m_ptzhost, m_ptzport);
	
	m_zoom = new PlayerZoomAdapter(m_client);
}

void PresetPlayer::closeEvent(QCloseEvent *)
{
	QSettings().setValue("mainwindow/splitter",m_split->saveState());
        QSettings().setValue("mainwindow/master-splitter",m_masterSplitter->saveState());
	
}
/*
class View : public QGraphicsView
{
public:
    View(QGraphicsScene *scene) : QGraphicsView(scene) { }

protected:
    void resizeEvent(QResizeEvent *event)
    {
        QGraphicsView::resizeEvent(event);
        fitInView(sceneRect(), Qt::KeepAspectRatio);
    }
};*/

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



class Pixmap : public QGraphicsPixmapItem
{
public:
	Pixmap(const QPixmap &pix, QSpinBox *xBox, QSpinBox *yBox, QSpinBox *zBox)
		: QGraphicsPixmapItem(pix)
		, m_xBox(xBox)
		, m_yBox(yBox)
		, m_zBox(zBox)
		, m_x(xBox->value())
		, m_y(yBox->value())
		, m_z(zBox->value())
	{
		setCacheMode(DeviceCoordinateCache);
	}
	
	void mousePressEvent ( QGraphicsSceneMouseEvent * event )
	{
		m_xBox->setValue(m_x);
		m_yBox->setValue(m_y);
		m_zBox->setValue(m_z);
	}
	
	QSpinBox *m_xBox;
	QSpinBox *m_yBox;
	QSpinBox *m_zBox;
	int m_x;
	int m_y;
	int m_z;
};


void PresetPlayer::sceneSnapshot()
{
	#ifdef ENABLE_VIDEO_RX
	VideoFramePtr frame = m_src->frame();
	QImage image;
	if(frame && frame->isValid())
		image = frame->toImage();
		
	int pan  = m_client->lastPan();
	int tilt = m_client->lastTilt();
	
	if(pan < SERVO_MIN)
	{
		if(pan < 0)   pan = 0;
		if(pan > 180) pan = 180;
		pan = mapValue(pan, 0, 180, SERVO_MIN, SERVO_MAX);
	}
	
	if(tilt < SERVO_MIN)
	{
		if(tilt < 0)   tilt = 0;
		if(tilt > 180) tilt = 180;
		tilt = mapValue(tilt, 0, 180, SERVO_MIN, SERVO_MAX);
	}
	
	double zVal = (double)m_zBox->value();
	double zFrac = 1. - (zVal / 15.);
	double intervalX = 362;//* zFrac;//image.size().width();
	double intervalY = 272;//* zFrac;//image.size().height();
	
	double x = ((int)( ((double)pan + 200)  / intervalX ) ) * intervalX;
	double y = ((int)( ((double)(SERVO_MAX - tilt + SERVO_MAX * .3 + zVal * (intervalY*1.))) / intervalY ) ) * intervalY;
	
	QString key = tr("%1.%2").arg(x).arg(y);
	
	image = image.scaled(intervalX,intervalY);
	
	if(m_images.contains(key))
	{
		m_images.value(key)->setPixmap(QPixmap::fromImage(image));
	}
	else
	{
		Pixmap *item = new Pixmap(QPixmap::fromImage(image),m_xBox,m_yBox,m_zBox);
		m_images[key] = item;
		item->setPos(x,y);
		m_scene->addItem(item);
	}
	#endif
}

void PresetPlayer::updateLive()
{
// 	VideoFramePtr frame = m_src->frame();
// 	QImage image;
// 	if(frame && frame->isValid())
// 		image = frame->toImage();
// 	
// 	double intervalX = 362;//image.size().width();
// 	double intervalY = 272;//image.size().height();
// 	image = image.scaled(intervalX, intervalY);
// 	
// 	m_live->setPixmap(QPixmap::fromImage(image));;
// 	
// 	int pan  = m_client->lastPan();
// 	int tilt = m_client->lastTilt();
// 	
// 	if(pan < SERVO_MIN)
// 	{
// 		if(pan < 0)   pan = 0;
// 		if(pan > 180) pan = 180;
// 		pan = mapValue(pan, 0, 180, SERVO_MIN, SERVO_MAX);
// 	}
// 	
// 	if(tilt < SERVO_MIN)
// 	{
// 		if(tilt < 0)   tilt = 0;
// 		if(tilt > 180) tilt = 180;
// 		tilt = mapValue(tilt, 0, 180, SERVO_MIN, SERVO_MAX);
// 	}
// 	
// // 	double intervalX = image.size().width();
// // 	double intervalY = image.size().height();
// 	
// 	
// 	double x = pan; //((int)( ((double)pan)  / intervalX ) ) * intervalX;
// 	double y = (SERVO_MAX - tilt + SERVO_MAX * .3 + m_zBox->value() * (intervalY*1.));
// 	
// 	m_live->setPos(x,y);
}

void PresetPlayer::setupGui()
{	
	QVBoxLayout *vbox = new QVBoxLayout(this);
        //m_masterSplitter = new QSplitter();
// 	vbox->addWidget(m_masterSplitter);
	
// 	QWidget *mainBase = new QWidget();
// 	m_masterSplitter->addWidget(mainBase);
	
// 	m_scene = new QGraphicsScene(SERVO_MIN, SERVO_MIN, SERVO_MAX, SERVO_MAX);
// 	QGraphicsView *view = new ScaledGraphicsView();
// 	view->setScene(m_scene);
	//m_masterSplitter->addWidget(view);
	
/*	m_live = new QGraphicsPixmapItem();
	m_live->setPos((SERVO_MAX - SERVO_MIN) / 2, (SERVO_MAX - SERVO_MIN) / 2);
	
	m_scene->addItem(m_live);
	connect(&m_updateLiveTimer, SIGNAL(timeout()), this, SLOT(updateLive()));
	m_updateLiveTimer.setInterval(250);*/
	
// 	connect(&m_snapshotTimer, SIGNAL(timeout()), this, SLOT(sceneSnapshot()));
// 	m_snapshotTimer.setInterval(1500);
// 	m_snapshotTimer.setSingleShot(true);
	
	
// 	vbox = new QVBoxLayout(mainBase);
	QSplitter *split = new QSplitter();
	split->setOrientation(Qt::Vertical);
	vbox->addWidget(split);
	m_split = split; // store ptr for saving state
	
	QSettings set;
	
	QHBoxLayout *hbox;

        m_masterSplitter = new QSplitter();
	
        QWidget *topBase = new QWidget();
        hbox = new QHBoxLayout(topBase);

        m_masterSplitter->addWidget(topBase);
	
        #ifdef ENABLE_VIDEO_RX
        VideoWidget *drw = new VideoWidget(topBase);
        hbox->addWidget(drw);
        #endif
	
	QVBoxLayout *vbox2 = new QVBoxLayout();
	
	QSpinBox *zoom;
	vbox2->addWidget(new QLabel("Click Tune: "));
	int w1,h1,w2,h2;
	
	zoom = new QSpinBox();
	zoom->setPrefix("(+W) ");
	SETUP_WIN_SPINBOX(zoom, w1 = set.value("winMaxW", SERVO_MAX).toInt());
	connect(zoom, SIGNAL(valueChanged(int)), this, SLOT(setWinMaxW(int)));
	vbox2->addWidget(zoom);
	
	zoom = new QSpinBox();
	zoom->setPrefix("(+H) ");
	SETUP_WIN_SPINBOX(zoom, h1 = set.value("winMaxH", SERVO_MAX).toInt());
	connect(zoom, SIGNAL(valueChanged(int)), this, SLOT(setWinMaxH(int)));
	vbox2->addWidget(zoom);
	
	zoom = new QSpinBox();
	zoom->setPrefix("(-W) ");
	SETUP_WIN_SPINBOX(zoom, w2 = set.value("winMinW", SERVO_MAX).toInt());
	connect(zoom, SIGNAL(valueChanged(int)), this, SLOT(setWinMinW(int)));
	vbox2->addWidget(zoom);
	
	zoom = new QSpinBox();
	zoom->setPrefix("(-H) ");
	SETUP_WIN_SPINBOX(zoom, h2 = set.value("winMinW", SERVO_MAX).toInt());
	connect(zoom, SIGNAL(valueChanged(int)), this, SLOT(setWinMinW(int)));
	vbox2->addWidget(zoom);
	
	vbox2->addStretch(1);
	
	hbox->addLayout(vbox2);
	
	RelDragWidget *reldrag = new RelDragWidget();
        //hbox->addWidget(reldrag);
        m_masterSplitter->addWidget(reldrag);
	
	
	
	m_winMax = QSize(w1,h1);
	m_winMin = QSize(w2,h2);



        //split->addWidget(topBase);
        split->addWidget(m_masterSplitter);
	
	#ifdef ENABLE_VIDEO_RX
        connect(drw, SIGNAL(pointClicked(QPoint)), this, SLOT(pointClicked(QPoint)));
        drw->setVideoSource(m_src);
        #endif
	
	int lastX = 129; //set.value("lastX",127).toInt();
	int lastY = 115; //set.value("lastY",127).toInt();
	int lastZ = 0; //set.value("lastZ",0).toInt();
	
	
	QWidget *bottomBase = new QWidget();
	vbox = new QVBoxLayout(bottomBase);
	split->addWidget(bottomBase);
	
	///
	
	hbox = new QHBoxLayout();
	hbox->addStretch(1);
	
	hbox->addWidget(new QLabel("X: "));
	m_xBox = new QSpinBox();
	SETUP_SERVO_SPINBOX(m_xBox,lastX);
	connect(m_xBox, SIGNAL(valueChanged(int)), this, SLOT(xChanged(int)));
	hbox->addWidget(m_xBox);
	
	hbox->addWidget(new QLabel("Y: "));
	m_yBox = new QSpinBox();
	SETUP_SERVO_SPINBOX(m_yBox,lastY);
	connect(m_yBox, SIGNAL(valueChanged(int)), this, SLOT(yChanged(int)));
	hbox->addWidget(m_yBox);
	
	hbox->addWidget(new QLabel("Z: "));
	m_zBox = new QSpinBox();
	SETUP_SERVO_SPINBOX(m_zBox,lastZ);
	m_zBox->setMaximum(100);
	connect(m_zBox, SIGNAL(valueChanged(int)), this, SLOT(zChanged(int)));
	hbox->addWidget(m_zBox);
	
	reldrag->setXBox(m_xBox);
	reldrag->setYBox(m_yBox);
	reldrag->setZBox(m_zBox);
	
	
	
	if(m_zoom)
	{
		QPushButton *zero = new QPushButton("0");
		connect(zero, SIGNAL(clicked()), this, SLOT(zero()));
		hbox->addWidget(zero);
	}
	
	QPushButton *btn;
	btn = new QPushButton("Save");
	connect(btn, SIGNAL(clicked()), this, SLOT(addPreset()));
	hbox->addWidget(btn);
	
	hbox->addStretch(1);
	
	btn = new QPushButton("Setup MIDI...");
	connect(btn, SIGNAL(clicked()), this, SLOT(setupMidi()));
	hbox->addWidget(btn);
	
	vbox->addLayout(hbox);
	///
	
	QFrame *frame = new QFrame();
	frame->setFrameStyle(QFrame::HLine);
	vbox->addWidget(frame);
	///
	
	m_presetLayout = new QGridLayout();
	vbox->addLayout(m_presetLayout);
	///
	
	frame = new QFrame();
	frame->setFrameStyle(QFrame::HLine);
	vbox->addWidget(frame);
	///
	
	hbox = new QHBoxLayout();
	hbox->addStretch(1);
	
	if(m_zoom)
	{
		hbox->addWidget(new QLabel("Zoom Config: "));
		zoom = new QSpinBox();
		zoom->setPrefix("(-) ");
		SETUP_ZOOM_SPINBOX(zoom, m_zoom->zoomMinusVal());
		connect(zoom, SIGNAL(valueChanged(int)), m_zoom, SLOT(setZoomMinusVal(int)));
		hbox->addWidget(zoom);
		
		zoom = new QSpinBox();
		zoom->setPrefix("(+) ");
		SETUP_ZOOM_SPINBOX(zoom, m_zoom->zoomPlusVal());
		connect(zoom, SIGNAL(valueChanged(int)), m_zoom, SLOT(setZoomPlusVal(int)));
		hbox->addWidget(zoom);
		
		zoom = new QSpinBox();
		zoom->setPrefix("(|) ");
		SETUP_ZOOM_SPINBOX(zoom, m_zoom->zoomMidVal());
		connect(zoom, SIGNAL(valueChanged(int)), m_zoom, SLOT(setZoomMidVal(int)));
		hbox->addWidget(zoom);
	}
	
	vbox->addLayout(hbox);
	vbox->addStretch(1);
	
	split->restoreState(QSettings().value("mainwindow/splitter").toByteArray());
        m_masterSplitter->restoreState(QSettings().value("mainwindow/master-splitter").toByteArray());
}

void PresetPlayer::zero()
{
	m_zBox->setValue(0);
	m_zoom->zero();
}

void PresetPlayer::loadPresets()
{
	QSettings set;
	QVariantList list = set.value("presets").toList();
	
	foreach(QVariant var, list)
	{
		Preset *p = new Preset();
		p->loadMap(var.toMap());
		addPreset(p);
	}
}

void PresetPlayer::savePresets()
{
	QSettings set;
	QVariantList list;
	foreach(Preset *p, m_presets)
		list << p->toMap();
		
	set.setValue("presets", list);
}

void PresetPlayer::addPreset(Preset *preset)
{
	m_presets << preset;
	rebuildPresetGui();
	
	savePresets();
	
	//PresetMidiInputAdapter *inst = PresetMidiInputAdapter::instance();
	//inst->reloadActionList();
}

void PresetPlayer::rebuildPresetGui()
{
	// Clear the layout of old controls
	while(m_presetLayout->count() > 0)
	{
		QLayoutItem *item = m_presetLayout->itemAt(m_presetLayout->count() - 1);
		m_presetLayout->removeItem(item);
		if(QWidget *widget = item->widget())
		{
			// disconnect any slots
			disconnect(widget, 0, this, 0);
			
			m_presetLayout->removeWidget(widget);
			delete widget;
		}
			
		delete item;
		item = 0;
	}
	
	int idx=0;
	foreach(Preset *p, m_presets)
	{
		int col = 0;
		
		QLineEdit *edit = new QLineEdit();
		edit->setText(p->name);
		edit->setProperty("idx", idx);
		connect(edit, SIGNAL(textChanged(QString)), this, SLOT(presetNameChanged(QString)));
		m_presetLayout->addWidget(edit, idx, col++);
		
		QSpinBox *spin;
		spin = new QSpinBox();
		SETUP_SERVO_SPINBOX(spin, p->x);
		spin->setProperty("idx", idx);
		connect(spin, SIGNAL(valueChanged(int)), this, SLOT(presetXChanged(int)));
		m_presetLayout->addWidget(spin, idx, col++);
		
		spin = new QSpinBox();
		SETUP_SERVO_SPINBOX(spin, p->y);
		spin->setProperty("idx", idx);
		connect(spin, SIGNAL(valueChanged(int)), this, SLOT(presetYChanged(int)));
		m_presetLayout->addWidget(spin, idx, col++);
		
		spin = new QSpinBox();
		SETUP_SERVO_SPINBOX(spin, p->z);
		spin->setProperty("idx", idx);
		connect(spin, SIGNAL(valueChanged(int)), this, SLOT(presetZChanged(int)));
		m_presetLayout->addWidget(spin, idx, col++);
		
		QPushButton *btn;
		btn = new QPushButton("Go >>");
		btn->setProperty("idx", idx);
		connect(btn, SIGNAL(clicked()), this, SLOT(presetPlayBtn()));
		m_presetLayout->addWidget(btn, idx, col++);
		
		btn = new QPushButton("X");
		btn->setProperty("idx", idx);
		connect(btn, SIGNAL(clicked()), this, SLOT(presetDeleteBtn()));
		m_presetLayout->addWidget(btn, idx, col++);
		
		idx ++;
	}
}


void PresetPlayer::addPreset()
{
	Preset *p = new Preset();
	p->x = m_xBox->value();
	p->y = m_yBox->value();
	p->z = m_zBox->value();
	addPreset(p);
}

#define GET_PRESET() \
	QObject *senderObject = sender(); \
	int num = senderObject ? senderObject->property("idx").toInt() : -1; \
	if(num < 0) \
	{ \
		qDebug() << "Error reading preset index, nothing changed."; \
		return; \
	} \
	Preset *p = m_presets[num]; 

void PresetPlayer::presetNameChanged(QString name)
{
	GET_PRESET();
	p->name = name;
	savePresets();
	
	PresetMidiInputAdapter *inst = PresetMidiInputAdapter::instance();
	inst->reloadActionList();
}

void PresetPlayer::presetXChanged(int x)
{
	GET_PRESET();
	p->x = x;
	savePresets();
}

void PresetPlayer::presetYChanged(int y)
{
	GET_PRESET();
	p->y = y;
	savePresets();
}

void PresetPlayer::presetZChanged(int z)
{
	GET_PRESET();
	p->z = z;
	savePresets();
}

void PresetPlayer::presetPlayBtn()
{
	GET_PRESET();
	sendServoValues(p->x,p->y,p->z);
}

void PresetPlayer::sendPreset(Preset *p)
{
	sendServoValues(p->x,p->y,p->z);
}

int PresetPlayer::presetIndex(Preset *p)
{
	return m_presets.indexOf(p);
}

void PresetPlayer::presetDeleteBtn()
{
	GET_PRESET();
	m_presets.removeAll(p);
	savePresets();
	rebuildPresetGui();
}

void PresetPlayer::xChanged(int x)
{
	sendServoValues(x, m_yBox->value(), m_zBox->value(), false);
}

void PresetPlayer::yChanged(int y)
{
	sendServoValues(m_xBox->value(), y, m_zBox->value(), false);
}

void PresetPlayer::zChanged(int z)
{
	sendServoValues(m_xBox->value(), m_yBox->value(), z, false); // don't zero 
}

QPoint PresetPlayer::frameToPanTilt(QPoint pnt)
{
	#ifdef ENABLE_VIDEO_RX
	VideoFramePtr frame = m_src->frame();
	if(frame && frame->isValid())
		m_frameSize = frame->toImage().size();
	
	//pnt = QPoint( m_frameSize.width() /2, m_frameSize.height() /2 );
	
	int curWidth  = mapValue(m_zoom->zoom(), 0, 15, m_winMax.width(),  m_winMin.width());
	int curHeight = mapValue(m_zoom->zoom(), 0, 15, m_winMax.height(), m_winMin.height());
	
	int pan  = m_client->lastPan();
	int tilt = m_client->lastTilt();
	
	if(pan < SERVO_MIN)
	{
		if(pan < 0)   pan = 0;
		if(pan > 180) pan = 180;
		pan = mapValue(pan, 0, 180, SERVO_MIN, SERVO_MAX);
	}
	
	if(tilt < SERVO_MIN)
	{
		if(tilt < 0)   tilt = 0;
		if(tilt > 180) tilt = 180;
		tilt = mapValue(tilt, 0, 180, SERVO_MIN, SERVO_MAX);
	}
	
	QRect ptWin(pan  - curWidth/2,
		    tilt - curHeight/2,
		    curWidth, curHeight);
	
	qDebug() << "PresetPlayer::frameToPanTilt("<<pnt<<"): cur window size:"<<curWidth<<" x "<<curHeight<<", TL:"<<ptWin.topLeft()<<", BR:"<<ptWin.bottomRight();
	
	int curX = mapValue(pnt.x(), 0, m_frameSize.width(),  ptWin.left(), ptWin.right());
	int curY = mapValue(pnt.y(), 0, m_frameSize.height(), ptWin.bottom(),  ptWin.top());
	
	qDebug() << "PresetPlayer::frameToPanTilt("<<pnt<<"): final point: "<<curX<<","<<curY;
	
	return QPoint(curX, curY);
	#else
	
	return QPoint();
	#endif
}

void PresetPlayer::pointClicked(QPoint pnt)
{
	QPoint panTilt = frameToPanTilt(pnt);
	
	sendServoValues(panTilt.x(), panTilt.y(), m_zBox->value(), false);
		

// 	VideoFramePtr frame = m_src->frame();
// 	if(frame && frame->isValid())
// 	{
// 		QImage img = frame->toImage();
// 		QRect rect(pnt - QPoint(12,12), QSize(25,25));
// 		QImage subImg = img.copy(rect);
// 		qDebug() << "Zoom rect:"<<rect<<", pnt:"<<pnt;
// 		
// 		QLabel *label = new QLabel();
// 		label->setPixmap(QPixmap::fromImage(subImg.scaled(320,240,Qt::KeepAspectRatio)));
// 		label->show();

// 		label = new QLabel();
// 		label->setPixmap(QPixmap::fromImage(img));
// 		label->show();
// 	}
}

void PresetPlayer::sendServoValues(int x, int y, int z, bool zero)
{
	if(m_lockSendValues)
		return;
	if(!m_client)
		return;
	if(!m_zoom)
		return;
	m_lockSendValues = true;
	
// 	if(m_snapshotTimer.isActive())
// 		m_snapshotTimer.stop();
// 	m_snapshotTimer.start();
	
	qDebug() << "PresetPlayer::sendServoValues(): "<<x<<y<<z<<zero;
	
	if(m_yBox->value() != y)
		m_yBox->setValue(y);
	if(m_xBox->value() != x)
		m_xBox->setValue(x);
	if(m_zBox->value() != z)
		m_zBox->setValue(z);
		
	QSettings set;
	set.setValue("lastX",x);
	set.setValue("lastY",y);
	set.setValue("lastZ",z);
	
	/*
	QString cmd = QString("%1s%2w%3u").arg(x).arg(y).arg(z);
	qDebug() << "Writing cmd: "<<cmd;
	for(int i=0;i<3;i++)
		m_port->write(qPrintable(cmd), cmd.length());
	*/
	m_client->setPan(x);
	m_client->setTilt(y);
	//m_client->setZoom(z);
	m_zoom->setZoom(z,zero);
		
	m_lockSendValues = false;
}

/** PlayerZoomAdapter **/

PlayerZoomAdapter::PlayerZoomAdapter(PanTiltClient *client)
	: QObject()
	, m_zeroTime(4000) // sec to zero
	, m_pulseTime(250) // ms to pulse
	, m_zoomMinusVal(135)
	, m_zoomPlusVal(155)
	, m_zoomMidVal(145)
	, m_zoom(0)
	, m_client(client)
	, m_hasZeroed(false)
{
	QSettings s;
	m_zoomMidVal   = s.value("zoom/mid", 145).toInt();
	m_zoomMinusVal = s.value("zoom/minus", 135).toInt();
	m_zoomPlusVal  = s.value("zoom/plus", 155).toInt();

}
void PlayerZoomAdapter::setZoomMinusVal(int v) 
{
	m_zoomMinusVal = v;
	QSettings().setValue("zoom/minus", v);
}

void PlayerZoomAdapter::setZoomPlusVal(int v)
{
	m_zoomPlusVal = v;
	QSettings().setValue("zoom/plus", v);
}

void PlayerZoomAdapter::setZoomMidVal(int v)
{
	m_zoomMidVal = v;
	QSettings().setValue("zoom/mid", v);
}


void PlayerZoomAdapter::zero()
{
	setZoom(0, true);
}

void PlayerZoomAdapter::setZoom(int zoom, bool zero)
{
	if(!m_client)
		return;
		
	if(zoom > 100)
		zoom = 100;
	if(zoom < 0)
		zoom = 0;
	
	m_client->sendRaw(QString("%1z%2a%3c%4b")
		.arg(m_pulseTime)
		.arg(m_zoomMinusVal)
		.arg(m_zoomPlusVal)
		.arg(m_zoomMidVal)
	);
	
	// zero the first time around
	if(!m_hasZeroed)
	{
		zero = true;
		m_hasZeroed = true;
		
		/*if(m_zoomMinusVal < 0)   m_zoomMinusVal = 0;
		if(m_zoomMinusVal > 180) m_zoomMinusVal = 180;
		int zMin = mapValue(m_zoomMinusVal, 0, 180, SERVO_MIN, SERVO_MAX);
		
		if(m_zoomPlusVal < 0)   m_zoomPlusVal = 0;
		if(m_zoomPlusVal > 180) m_zoomPlusVal = 180;
		int zPlus = mapValue(m_zoomPlusVal, 0, 180, SERVO_MIN, SERVO_MAX);
		
		if(m_zoomMidVal < 0)   m_zoomMidVal = 0;
		if(m_zoomMidVal > 180) m_zoomMidVal = 180;
		int zMid = mapValue(m_zoomMidVal, 0, 180, SERVO_MIN, SERVO_MAX);
		
		qDebug() << "Mapped zoom values: -/+: "<<zMin<<","<<zPlus<<", mid:"<<zMid;*/ 
	}
	
	if(zero)
	{
		// offset by 100 to encode +/- around 100
		m_client->setZoom((m_zeroTime / m_pulseTime)*-1 + 100);
		m_zoom = 0; 
	}
	
	int delta = zoom - m_zoom;
	if(delta != 0)
		m_client->setZoom(delta + 100); // arduino will take -100 off the value
	m_zoom = zoom;
}

/** Back to other stuff ... **/

void PresetPlayer::setupMidi()
{
	PresetMidiInputAdapter *inst = PresetMidiInputAdapter::instance();
	inst->reloadActionList();
	
	MidiInputSettingsDialog *d = new MidiInputSettingsDialog(inst,this);
	d->exec();
}


namespace PresetMidiAction
{
	static PresetPlayer *mw = 0;
	
	class PresetButton : public MidiInputAction
	{
	public:
		PresetButton(Preset *_p) : p(_p) {} 
		QString id() { return tr("64258ee6-f50e-4f94-ad6a-589a93ac2d40-%1").arg(mw->presetIndex(p)); }
		QString name() { return tr("Go to '%1'").arg(p->name); }
		void trigger(int)
		{
			mw->sendPreset(p);	
		}
		
		Preset *p;
	};
};

void PresetMidiInputAdapter::setupActionList()
{
	m_actionList.clear();
	qDeleteAll(m_actionList);
	foreach(Preset *p, PresetMidiAction::mw->presets())
		m_actionList << new PresetMidiAction::PresetButton(p);
		
	qDebug() << "PresetMidiInputAdapter::setupActionList(): Added "<<m_actionList.size() <<" actions to the MIDI Action List";
}

PresetMidiInputAdapter *PresetMidiInputAdapter::m_inst = 0;

PresetMidiInputAdapter::PresetMidiInputAdapter(PresetPlayer *player)
	: MidiInputAdapter()
{
	PresetMidiAction::mw = player;
	setupActionList();
	loadSettings();
}

PresetMidiInputAdapter *PresetMidiInputAdapter::instance(PresetPlayer *player)
{
	if(!m_inst)
		m_inst = new PresetMidiInputAdapter(player);
	return m_inst;
}

void PresetMidiInputAdapter::reloadActionList()
{
	setupActionList();
	loadSettings();
}





RelDragWidget::RelDragWidget()
{
	m_mouseIsDown = false;
	m_xBox = 0;
	m_yBox = 0;
}

void RelDragWidget::setXBox(QSpinBox *box)
{
	m_xBox = box;
}

void RelDragWidget::setYBox(QSpinBox *box)
{
	m_yBox = box;
}
	
void RelDragWidget::setZBox(QSpinBox *box)
{
	m_zBox = box;
}

QSize RelDragWidget::sizeHint () const { return QSize(160,120); }

void RelDragWidget::paintEvent(QPaintEvent *pe)
{
	QPainter p(this);
	p.fillRect(rect(), Qt::green);
}

void RelDragWidget::mousePressEvent(QMouseEvent *evt)
{
	if(!m_xBox || !m_yBox)
		return;
		
	m_mouseIsDown = true;
	m_mouseDownAt = evt->pos();
	m_mouseDownValues = QPoint(m_xBox->value(), m_yBox->value());
}

void RelDragWidget::mouseMoveEvent(QMouseEvent *evt)
{
	if(m_mouseIsDown)
	{
		QPoint dist = evt->pos() - m_mouseDownAt;
		//qDebug() << "Dist: "<<dist;
		
		double zFactor = ((double)(m_zBox->value()) / 15.);
		double zScale = 1.0 - zFactor;
		//qDebug() << "zScale:"<<zScale<<", zFactor:"<<zFactor;
		
		if(zScale < 0.0)
			zScale = 0.1;
		if(zScale > 1.0)
			zScale = 1.0;
		
		double xScale = 1.5 * zScale;
		double yScale = 1.5 * zScale;
		
		if(xScale < 0.51)
			xScale = 0.51;
		if(yScale < 0.51)
			yScale = 0.51;
		
		
		qDebug() << "xScale:"<<xScale<<", zScale:"<<zScale;
		m_xBox->setValue((int)( dist.x() * xScale) + m_mouseDownValues.x());
		m_yBox->setValue((int)( dist.y() * yScale * -1 ) + m_mouseDownValues.y());
	}
}

void RelDragWidget::mouseReleasEvent(QMouseEvent *evt)
{
	m_mouseIsDown = false;
}

void RelDragWidget::wheelEvent(QWheelEvent *event)
{
	int numDegrees = event->delta() / 8;
	int numSteps = numDegrees / 15;
	//qDebug() << "Wheel numSteps: "<<numSteps<<", numDegrees:"<<numDegrees;
	m_zBox->setValue(m_zBox->value() + numSteps);

}

	
// 	QSpinBox *m_xBox;
// 	QSpinBox *m_yBox;
// 	
// 	bool m_mouseIsDown;
// 	
// 	QPoint m_mouseDownAt;
// 	QPoint m_mouseDownValues;
