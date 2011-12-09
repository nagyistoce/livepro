#include "LiveVideoLayer.h"
#include "ExpandableWidget.h"
#include "VideoSource.h"
#include "../glvidtex/GLWidget.h"
#include "../glvidtex/GLVideoDrawable.h"

LiveVideoLayer::LiveVideoLayer(QObject *parent)
	: LiveLayer(parent)
	, m_source(0)
{
	setBrightness(0);
	setContrast(0);
	setHue(0);
	setSaturation(0);
	setFlipHorizontal(false);
	setFlipVertical(false);
	setCropTopLeft(QPointF(0,0));
	setCropBottomRight(QPointF(0,0));
	setTextureOffset(QPointF(0,0));
	setAspectRatioMode(Qt::KeepAspectRatio);
	
	setAnimatePropFlags(QStringList()
		<< "flipHorizontal" 
		<< "flipVertical"
		<< "aspectRatioMode"
		<< "cropTopLeft"
		<< "cropBottomRight",
		false);
}

LiveVideoLayer::~LiveVideoLayer()
{
	// TODO close source
}

GLDrawable *LiveVideoLayer::createDrawable(GLWidget *widget, bool)
{
	GLVideoDrawable *drawable = new GLVideoDrawable();
	//drawable->setVideoSource(source);
	//drawable->setRect(QRectF(0,0,1000,750));
	drawable->setRect(widget->viewport());

	drawable->addShowAnimation(GLDrawable::AnimFade);
	drawable->addHideAnimation(GLDrawable::AnimFade);

	connect(this, SIGNAL(videoSourceChanged(VideoSource*)), drawable, SLOT(setVideoSource(VideoSource*)));
	
	return drawable;
}


void LiveVideoLayer::initDrawable(GLDrawable *drawable, bool isFirst)
{
	//qDebug() << "LiveVideoLayer::initDrawable: drawable:"<<drawable<<", calling parent";
	LiveLayer::initDrawable(drawable, isFirst);
	
// 	GLVideoDrawable *vid = dynamic_cast<GLVideoDrawable*>(drawable);
// 	if(!vid)
// 		return;
// 		

	QStringList props = QStringList()
		<< "brightness"
		<< "contrast"
		<< "hue"
		<< "saturation"
		<< "flipHorizontal"
		<< "flipVertical"
		<< "cropTopLeft"
		<< "cropBottomRight"
		<< "textureOffset"
		<< "aspectRatioMode";
		
	//qDebug() << "LiveVideoLayer::initDrawable: drawable:"<<drawable<<", props list:"<<props;
	applyLayerPropertiesToObject(drawable, props);
	if(m_source)
		setVideoSource(m_source);
}

void LiveVideoLayer::setVideoSource(VideoSource *source)
{
// 	if(source)
// 		qDebug() << "LiveVideoLayer::setVideoSource: "<<source;
	emit videoSourceChanged(source);
	m_source = m_source;
}

QWidget * LiveVideoLayer::createLayerPropertyEditors()
{
	QWidget * base = new QWidget();
	QVBoxLayout *blay = new QVBoxLayout(base);
	blay->setContentsMargins(0,0,0,0);
	
	/////////////////////////////////////////
	
	ExpandableWidget *groupDisplay = new ExpandableWidget("Display Options",base);
	blay->addWidget(groupDisplay);
	
	QWidget *groupDisplayContainer = new QWidget;
	QGridLayout *displayLayout = new QGridLayout(groupDisplayContainer);
	//gridLayout->setDisplaysMargins(3,3,3,3);
	
	groupDisplay->setWidget(groupDisplayContainer);
	
	//int row = 0;
	PropertyEditorOptions opts;
	
	opts.min = -100;
	opts.max =  100;
	opts.defaultValue = 0;
	
	int row=0;
	displayLayout->addWidget(new QLabel(tr("Brightness:")), row, 0);
	displayLayout->addWidget(generatePropertyEditor(this, "brightness", SLOT(setBrightness(int)), opts), row, 1);
	
	row++;
	displayLayout->addWidget(new QLabel(tr("Contrast:")), row, 0);
	displayLayout->addWidget(generatePropertyEditor(this, "contrast", SLOT(setContrast(int)), opts), row, 1);
	
	row++;
	displayLayout->addWidget(new QLabel(tr("Hue:")), row, 0);
	displayLayout->addWidget(generatePropertyEditor(this, "hue", SLOT(setHue(int)), opts), row, 1);
	
	row++;
	displayLayout->addWidget(new QLabel(tr("Saturation:")), row, 0);
	displayLayout->addWidget(generatePropertyEditor(this, "saturation", SLOT(setSaturation(int)), opts), row, 1);
	
	opts.reset();
	
	row++;
	displayLayout->addWidget(generatePropertyEditor(this, "flipHorizontal", SLOT(setFlipHorizontal(bool))), row, 0, 1, 2);
	row++;
	displayLayout->addWidget(generatePropertyEditor(this, "flipVertical", SLOT(setFlipVertical(bool))), row, 0, 1, 2);
	
	row++;
	displayLayout->addWidget(new QLabel(tr("Crop Top-Left:")), row, 0);
	displayLayout->addWidget(generatePropertyEditor(this, "cropTopLeft", SLOT(setCropTopLeft(const QPointF&))), row, 1);
	
	row++;
	displayLayout->addWidget(new QLabel(tr("Crop Bottom-Right:")), row, 0);
	displayLayout->addWidget(generatePropertyEditor(this, "cropBottomRight", SLOT(setCropBottomRight(const QPointF&))), row, 1);
	
	row++;
	displayLayout->addWidget(new QLabel(tr("Video Offset:")), row, 0);
	displayLayout->addWidget(generatePropertyEditor(this, "textureOffset", SLOT(setTextureOffset(const QPointF&))), row, 1);
	
	opts.reset();
	opts.stringIsFile = true;
	opts.fileTypeFilter = tr("Image Files with Transparency (*.png *.gif);;Any File (*.*)");
	
	row++;
	displayLayout->addWidget(new QLabel(tr("Alpha Mask File:")), row, 0);
	displayLayout->addWidget(generatePropertyEditor(this, "alphaMaskFile", SLOT(setAlphaMaskFile(const QString&)), opts), row, 1);
	
	row++;
	displayLayout->addWidget(new QLabel(tr("Overlay Mask File:")), row, 0);
	displayLayout->addWidget(generatePropertyEditor(this, "overlayMaskFile", SLOT(setOverlayMaskFile(const QString&)), opts), row, 1);
	
	QStringList modeList = QStringList()
		<< "Ignore Aspect Ratio"
		<< "Keep Aspect Ratio"
		<< "Keep by Expanding";
		
	QComboBox *modeListBox = new QComboBox();
	modeListBox->addItems(modeList);
	modeListBox->setCurrentIndex(m_props["aspectRatioMode"].toInt());
	connect(modeListBox, SIGNAL(activated(int)), this, SLOT(setAspectRatioMode(int)));
	
	row++;
	displayLayout->addWidget(new QLabel(tr("Aspect Ratio Mode:")), row, 0);
	displayLayout->addWidget(modeListBox, row, 1);
	

// 	QFormLayout *formLayout = new QFormLayout(groupDisplayContainer);
// 	formLayout->setDisplaysMargins(3,3,3,3);
// 	
// 	groupDisplay->setWidget(groupDisplayContainer);
// 	
// 	formLayout->addRow("", generatePropertyEditor(this, "deinterlace", SLOT(setDeinterlace(bool))));
// 	
 	groupDisplay->setExpandedIfNoDefault(true);
 	
 	/////////////////////////////////////////
	
	QWidget *basics =  LiveLayer::createLayerPropertyEditors();
	blay->addWidget(basics);
	
	return base;
}

void LiveVideoLayer::setLayerProperty(const QString& key, const QVariant& value)
{
	if(key == "alphaMaskFile" || key == "overlayMaskFile")
	{
		m_props[key] = value;

		QImage image(value.toString());
		if(image.isNull() && !value.toString().isEmpty())
		{
			qDebug() << "LiveVideoLayer: "<<key<<": Unable to load file "<<value.toString();
			image = QImage();
		}
			
		foreach(GLWidget *widget, m_drawables.keys())
		{
			GLVideoDrawable *vid = dynamic_cast<GLVideoDrawable*>(m_drawables[widget]);
			if(vid)
			{
				if(key == "alphaMaskFile")
					vid->setAlphaMask(image);
				
				// Not Implemented yet
				//else
				//	m_drawables[widget]->setOverlayMask(image);
			}
		}
	}
	else
	{
		if(m_source)
		{
			const char *keyStr = qPrintable(key);
			if(m_source->metaObject()->indexOfProperty(keyStr)>=0)
			{
				m_source->setProperty(keyStr, value);
			}
			
			LiveLayer::setLayerProperty(key,value);
		}
		
		// Only call parent function if NOT an alphaMask or overlayMask, since it will cause a recursive loop due to the Q_PROPERTY declaration for both the overlay and alpha masks
		LiveLayer::setLayerProperty(key,value);
	}

}
