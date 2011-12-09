#include "LiveVideoInputLayer.h"
#include "ExpandableWidget.h"
#include "CameraThread.h"
#include "../glvidtex/GLWidget.h"
#include "../glvidtex/GLVideoDrawable.h"

LiveVideoInputLayer::LiveVideoInputLayer(QObject *parent)
	: LiveVideoLayer(parent)
	, m_camera(0)
{
	#ifdef Q_OS_WIN
		QString defaultCamera = "vfwcap://0";
	#else
		QString defaultCamera = "/dev/video0";
	#endif

	qDebug() << "LiveVideoInputLayer::initDrawable: Using default camera:"<<defaultCamera;

	CameraThread *source = CameraThread::threadForCamera(defaultCamera);
	if(source)
	{
		source->setFps(30);
		source->enableRawFrames(true);
		
		m_cameraDev = defaultCamera;
		setCamera(source);
		
		m_props["deinterlace"] = source->deinterlace();
	}
	
	setAnimatePropFlags(QStringList()
		<< "deinterlace",
		false);

}

LiveVideoInputLayer::~LiveVideoInputLayer()
{
	// TODO close camera
}

GLDrawable *LiveVideoInputLayer::createDrawable(GLWidget *widget, bool flag)
{
	// We overrride createDrawable here just for future expansiosn sake
	GLDrawable *draw = LiveVideoLayer::createDrawable(widget, flag);
	//draw->setRotation(QVector3D(0,45,0));
	return draw;
}

void LiveVideoInputLayer::initDrawable(GLDrawable *drawable, bool isFirst)
{
	//qDebug() << "LiveVideoInputLayer::setupDrawable: drawable:"<<drawable<<", copyFrom:"<<copyFrom;
	LiveVideoLayer::initDrawable(drawable, isFirst);

	if(m_camera)
	{
		drawable->setObjectName(qPrintable(m_camera->inputName()));
		setCamera(m_camera);
	}
}

void LiveVideoInputLayer::setInputDevice(const QString& dev)
{
	m_cameraDev = dev;
	CameraThread *thread = CameraThread::threadForCamera(dev);
	if(m_camera == thread)
		return;
	setCamera(thread);
}

void LiveVideoInputLayer::setCamera(CameraThread *camera)
{
	qDebug() << "LiveVideoInputLayer::setCamera: "<<camera;
	setVideoSource(camera);
	m_camera = camera;
	foreach(GLDrawable *drawable, m_drawables)
		drawable->setObjectName(qPrintable(m_camera->inputName()));
	foreach(GLDrawable *drawable, m_secondaryDrawables)
		drawable->setObjectName(qPrintable(m_camera->inputName()));
	setInstanceName(camera->inputName());
}

void LiveVideoInputLayer::selectCameraIdx(int idx)
{
	QStringList rawDevices = CameraThread::enumerateDevices();
	if(idx < 0 || idx >= rawDevices.size()) 
		return;
	
	CameraThread *source = CameraThread::threadForCamera(rawDevices[idx]);
	source->setFps(30);
	source->enableRawFrames(true);
	
	m_cameraDev = rawDevices[idx];
	setCamera(source);
}

QWidget * LiveVideoInputLayer::createLayerPropertyEditors()
{
	QWidget * base = new QWidget();
	QVBoxLayout *blay = new QVBoxLayout(base);
	blay->setContentsMargins(0,0,0,0);
	
	ExpandableWidget *groupContent = new ExpandableWidget("Video Input",base);
	blay->addWidget(groupContent);
	
	QWidget *groupContentContainer = new QWidget;
	QGridLayout *gridLayout = new QGridLayout(groupContentContainer);
	gridLayout->setContentsMargins(3,3,3,3);
	
	groupContent->setWidget(groupContentContainer);
	
	int row = 0;
	{
		QStringList rawDevices = CameraThread::enumerateDevices();
		
		if(rawDevices.isEmpty())
		{
			gridLayout->addWidget(new QLabel("<b>No video input devices found.</b>"), row, 0, 1, 2);
		}
		else
		{
			QStringList devices;
			int counter = 1;
			int idx = 0;
			foreach(QString dev, rawDevices)
			{
				if(m_camera && m_camera->inputName() == dev)
					idx = counter-1;
				devices << QString("Camera # %1").arg(counter++);
			}
			
			QComboBox *cameraBox = new QComboBox();
			cameraBox->addItems(devices);
			cameraBox->setCurrentIndex(idx);
			connect(cameraBox, SIGNAL(activated(int)), this, SLOT(selectCameraIdx(int)));
			
			gridLayout->addWidget(new QLabel("Camera:"), row, 0);
			gridLayout->addWidget(cameraBox, row, 1);
		}
	}
	
	row++;
	{
		gridLayout->addWidget(generatePropertyEditor(this, "deinterlace", SLOT(setDeinterlace(bool))), row, 0, 1, 2);
	}
 	
 	groupContent->setExpandedIfNoDefault(true);
	
	/////////////////////////////////////////
	
	QWidget *basics =  LiveVideoLayer::createLayerPropertyEditors();
	blay->addWidget(basics);
	
	return base;
}

void LiveVideoInputLayer::setDeinterlace(bool flag)
{
	if(m_camera)
		m_camera->setDeinterlace(flag);
}

void LiveVideoInputLayer::setLayerProperty(const QString& key, const QVariant& value)
{
	if(key == "deinterlace")
	{
		setDeinterlace(value.toBool());
	}
	else
	if(m_camera)
	{
		const char *keyStr = qPrintable(key);
		if(m_camera->metaObject()->indexOfProperty(keyStr)>=0)
		{
			m_camera->setProperty(keyStr, value);
		}
		
	}
	
	LiveLayer::setLayerProperty(key,value);
}
