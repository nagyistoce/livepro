
#include "LiveLayer.h"
#include "LiveScene.h"
#include "../glvidtex/GLDrawable.h"
#include "../glvidtex/GLWidget.h"
#include <QPropertyAnimation>

#include "ExpandableWidget.h"
#include "EditorUtilityWidgets.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QDirModel>
#include <QCompleter>


PositionWidget::PositionWidget(LiveLayer *layer)
	: QWidget()
	, m_layer(layer)
{
	m_lockAspectRatio = false;
	m_lockToAR = 0.0;
	m_lockValueUpdates = false;
	
	QGridLayout *grid = new QGridLayout(this);
	int row =0;
	
	QHBoxLayout *hbox=0;
	QWidget *box=0;
	
	#define NEW_PANEL {\
		box = new QWidget(this); \
		hbox = new QHBoxLayout(box); \
		hbox->setContentsMargins(0,0,0,0); \
		}
	
	NEW_PANEL;
	
	//////////////////////
	// Fist row: Display type
	
	NEW_PANEL;
	row++;
	
	QComboBox *m_aspectRatioBox = new QComboBox();
	QStringList arNames;
	
	int idx = 0;
	
	QPointF ar = m_layer->desiredAspectRatio();
	
	int count = 0;
	#define AddRatio(name,a,r) \
		arNames << name; \
		m_arList << QPoint(a,r); \
		if(a == (int)ar.x() && r == (int)ar.y()) \
			idx = count; \
		count ++;
		
		
	AddRatio("Manual (Any Ratio)", 0,0);
	AddRatio("User Specified", -1,-1);
	AddRatio("4:3   - Standard Screen", 4,3);
	AddRatio("16:9  - HDTV",16,9);
	AddRatio("16:10 - Widescreen Monitor", 16,10);
	
	#undef AddRatio
	
	m_aspectRatioBox->addItems(arNames);
	connect(m_aspectRatioBox, SIGNAL(activated(int)), this, SLOT(setAspectRatioIdx(int)));
	hbox->addWidget(m_aspectRatioBox);
	
	m_arLeft = new QSpinBox(box);
	m_arLeft->setAlignment(Qt::AlignRight);
 	connect(m_arLeft, SIGNAL(valueChanged(int)), this, SLOT(setARLeft(int)));
	hbox->addWidget(m_arLeft);
	
	hbox->addWidget(new QLabel(":"));
	
	m_arRight = new QSpinBox(box);
	m_arRight->setAlignment(Qt::AlignLeft);
 	connect(m_arRight, SIGNAL(valueChanged(int)), this, SLOT(setARRight(int)));
	hbox->addWidget(m_arRight);
	
	hbox->addStretch(1);
	
	                  // row col rspan cspan
	grid->addWidget(box, row, 1, 1, 3);
	
	
	//////////////////////
	// 2nd row: position
	
	row++;
	grid->addWidget(new QLabel("Position (X-Y):"), row, 0);
	
	NEW_PANEL;
	
	m_posX = new QDoubleSpinBox(box);
	connect(m_posX, SIGNAL(valueChanged(double)), this, SLOT(setLayerX(double)));
	hbox->addWidget(m_posX);
	
	m_posY = new QDoubleSpinBox(box);
	connect(m_posY, SIGNAL(valueChanged(double)), this, SLOT(setLayerY(double)));
	hbox->addWidget(m_posY);
	
	
	grid->addWidget(box, row, 1);
	
	//////////////////////
	// 3nd row: size
	
	row++;
	grid->addWidget(new QLabel("Size (W-H):"), row, 0);
	
	NEW_PANEL;
	
	m_sizeWidth = new QDoubleSpinBox(box);
	connect(m_sizeWidth, SIGNAL(valueChanged(double)), this, SLOT(setLayerWidth(double)));
	hbox->addWidget(m_sizeWidth);
	
	m_sizeHeight = new QDoubleSpinBox(box);
	connect(m_sizeHeight, SIGNAL(valueChanged(double)), this, SLOT(setLayerHeight(double)));
	hbox->addWidget(m_sizeHeight);
	
	grid->addWidget(box, row, 1);
	
	#undef NEW_PANEL
	
// 	setEditPixels(false);


	#define CHANGE_SPINBOX_PIXELS(spin) \
		spin->setMinimum(-Q_MAX(m_layer->scene()->canvasSize().width(),m_layer->scene()->canvasSize().height()) * 10); \
		spin->setMaximum( Q_MAX(m_layer->scene()->canvasSize().width(),m_layer->scene()->canvasSize().height()) * 10); \
		spin->setSuffix(" px"); \
		spin->setDecimals(0); \
		spin->setAlignment(Qt::AlignRight);


	m_lockValueUpdates = true;
	CHANGE_SPINBOX_PIXELS(m_posY);
	CHANGE_SPINBOX_PIXELS(m_posX);
	CHANGE_SPINBOX_PIXELS(m_sizeWidth);
	CHANGE_SPINBOX_PIXELS(m_sizeHeight);
	
	QRectF r = m_layer->rect();
	m_posY->setValue(r.y());
	m_posX->setValue(r.x());
	m_sizeWidth->setValue(r.width());
	m_sizeHeight->setValue(r.height());
	
	m_aspectRatioBox->setCurrentIndex(idx);
	setAspectRatioIdx(idx);
	
	
	m_lockValueUpdates = false;
	
	
	#undef CHANGE_SPINBOX_PIXELS
		
	
	// watch for position changes and update UI accordingly
	//connect(m_layer, SIGNAL(layerPropertyChanged(const QString& propertyId, const QVariant& value, const QVariant& oldValue)), this, SLOT(layerPropertyChanged(const QString& propertyId, const QVariant& value, const QVariant& oldValue)));
	
}

void PositionWidget::setAspectRatioIdx(int idx)
{
	if(idx<0 || idx>=m_arList.size())
		return;
		
	QPoint ar = m_arList[idx];
	
	m_arLeft->setEnabled(idx == 1);
	m_arRight->setEnabled(idx == 1);
	
	m_lockAspectRatio = idx > 0;
	
	if(idx > 1)
	{
		m_arLeft->setValue(ar.x());
		m_arRight->setValue(ar.y());
		
		m_layer->setDesiredAspectRatio(QPointF(ar.x(),ar.y()));
	}

}

void PositionWidget::layerPropertyChanged(const QString& /*propertyId*/, const QVariant& /*value*/, const QVariant& /*oldValue*/)
{

}


	
void PositionWidget::setLayerY(double value)
{
	if(m_lockValueUpdates)
	{
		//qDebug() << "PositionWidget::setLayerTop(): m_lockValueUpdates, returning";
		return;
	}
	m_lockValueUpdates = true;
		
	m_layer->setY(value);
	
	m_lockValueUpdates = false;
}

void PositionWidget::setLayerX(double value)
{
	if(m_lockValueUpdates)
		return;
	m_lockValueUpdates = true;
	
	m_layer->setX(value);
	
	m_lockValueUpdates = false;
}
	
void PositionWidget::setLayerWidth(double value)
{
	if(m_lockValueUpdates)
		return;
	m_lockValueUpdates = true;
	
	QRectF r = m_layer->rect();
		
	if(m_lockAspectRatio)
	{
		double newHeight = heightFromWidth(value);
		m_layer->setRect(r.x(), r.y(), value, newHeight);
		m_sizeHeight->setValue(newHeight);
	}
	else
	{
		m_layer->setRect(r.x(), r.y(), value, r.height());
	}
	
	
	m_lockValueUpdates = false;
}

void PositionWidget::setLayerHeight(double value)
{
	if(m_lockValueUpdates)
		return;
	m_lockValueUpdates = true;

	QRectF r = m_layer->rect();
		
	if(m_lockAspectRatio)
	{
		double newWidth = widthFromHeight(value);
		m_layer->setRect(r.x(), r.y(), newWidth, value);
		m_sizeWidth->setValue(newWidth);
	}
	else
	{
		m_layer->setRect(r.x(), r.y(), r.width(), value);
	}


	
	
	m_lockValueUpdates = false;
}
	
double PositionWidget::heightFromWidth(double value)
{
	double ar = ((double)m_arLeft->value()) / ((double)m_arRight->value());
	return value/ar;
}
	
double PositionWidget::widthFromHeight(double value)
{
	double ar = ((double)m_arLeft->value()) / ((double)m_arRight->value());
	return value*ar;
}

void PositionWidget::setARLeft(int /*val*/)
{
	if(m_lockValueUpdates)
		return;
	m_lockValueUpdates = true;
		
// 	// AR = height/width
// 	double ar = ((double)m_arLeft->value()) / ((double)val);
	
	// AR left = width ratio, so update with respective to height
	// eg width for height using new AR
	
	QRectF r = m_layer->rect();
	double newHeight = heightFromWidth(r.width());
	m_layer->setRect(r.x(), r.y(), r.width(), newHeight);
	m_sizeHeight->setValue(newHeight);
	
	m_lockValueUpdates = false;
}

void PositionWidget::setARRight(int /*val*/)
{
	if(m_lockValueUpdates)
		return;
	m_lockValueUpdates = true;
	
	QRectF r = m_layer->rect();
	double newHeight = heightFromWidth(r.width());
	m_layer->setRect(r.x(), r.y(), r.width(), newHeight);
	m_sizeHeight->setValue(newHeight);
		
	m_lockValueUpdates = false;
}


//////////////////////////////////////////////////////////////////////////////

// Translated from a perl function I wrote to do basically
// the same thing for an ERP project a few years back.
QString LiveLayer::guessTitle(QString field)
{
	static const QRegExp rUpperCase = QRegExp("([a-z])([A-Z])");
	static const QRegExp rFirstLetter = QRegExp("([a-z])");
	static const QRegExp rLetterNumber = QRegExp("([a-z])([0-9])");
	//$name =~ s/([a-z])_([a-z])/$1.' '.uc($2)/segi;
	static const QRegExp rUnderScore = QRegExp("([a-z])_([a-z])");

	QString tmp = field;
	tmp.replace(rUnderScore,"\\1 \\2");
	tmp.replace(rUpperCase,"\\1 \\2");
	if(tmp.indexOf(rFirstLetter) == 0)
	{
		QChar x = tmp.at(0);
		tmp.remove(0,1);
		tmp.prepend(QString(x).toUpper());
	}

	tmp.replace(rLetterNumber,"\\1 #\\2");

	return tmp;
}



//////////////////////////////////////////////////////////////////////////////

LiveLayer::LiveLayer(QObject *parent)
	: QObject(parent)
	, m_secondarySourceActive(false)
	, m_animationsDisabled(false)
	, m_scene(0)
	, m_hideOnShow(0)
	, m_showOnShow(0)
	, m_lockVisibleSetter(false)
	, m_layerId(-1)
	, m_layerIdLoaded(false)
	, m_lockLayerPropertyUpdates(false)
	, m_isUserControllable(false)
{
	m_props["rect"] = QRectF();
	m_props["zIndex"] = -1.;
	m_props["opacity"] = 1.0;
// 	m_props["showFullScreen"] = true;
	m_props["alignment"] = 0;
	m_props["insetTopLeft"] = QPointF(0,0);
	m_props["insetBottomRight"] = QPointF(0,0);
	m_props["alignedSizeScale"] = 1.0;
	m_props["rotation"] = QVector3D(0,0,0);
	
// 	m_props["topPercent"] = 0.;
// 	m_props["leftPercent"] = 0.;
// 	m_props["bottomPercent"] = 1.;
// 	m_props["rightPercent"] = 1.;
	
	
	m_props["fadeIn"] = 1;
	m_props["fadeInLength"] = 300;

	m_props["fadeOut"] = 1;
	m_props["fadeOutLength"] = 300;

	m_props["showAnimationType"] = GLDrawable::AnimNone;
	m_props["showAnimationLength"] = 2500;
	m_props["showAnimationCurve"] = QEasingCurve::OutElastic;

	m_props["hideAnimationType"] = GLDrawable::AnimNone;
	m_props["hideAnimationLength"] = 300;
	m_props["hideAnimationCurve"] = QEasingCurve::Linear;
	
	
	setAnimatePropFlags(QStringList()
		<< "zIndex" 
		<< "aspectRatioMode"
		<< "fadeIn" 
		<< "fadeInLength"
		<< "fadeOut"
		<< "fadeOutLength"
		<< "showAnimationType"
		<< "showAnimationLength"
		<< "showAnimationCurve"
		<< "hideAnimationType"
		<< "hideAnimationLength"
		<< "hideAnimationCurve",
		false);
}

LiveLayer::~LiveLayer()
{
	QList<GLDrawable *> drawables = m_drawables.values();
	m_drawables.clear();
	
	qDeleteAll(drawables);
	drawables.clear();
	
	drawables = m_secondaryDrawables.values();
	m_secondaryDrawables.clear();
	
	qDeleteAll(m_secondaryDrawables);
	m_secondaryDrawables.clear();
	
	m_scene = 0;
	m_hideOnShow = 0;
	m_showOnShow = 0;	
}

int LiveLayer::id()
{
	if(m_layerId < 0)
	{
		QSettings set;
		m_layerId = set.value("livelayer/id-counter",0).toInt() + 1;
		set.setValue("livelayer/id-counter",m_layerId);
	}
	
	return m_layerId;
}

// Returns a GLDrawable for the specified GLWidget. If none exists,
// then calls createDrawable() internally, followed by initDrawable()
GLDrawable* LiveLayer::drawable(GLWidget *widget, bool secondary)
{
	GLDrawable *drawable = 0;
	QHash<GLWidget*, GLDrawable*> cache = secondary ? m_secondaryDrawables : m_drawables;
	if(cache.contains(widget))
	{
// 		qDebug() << "LiveLayer::drawable: widget:"<<widget<<", cache hit";

		drawable = cache[widget];
	}
	else
	{
		drawable = createDrawable(widget,secondary);

		bool wasEmpty = m_drawables.isEmpty();
		if(secondary)
			m_secondaryDrawables[widget] = drawable;
		else
			m_drawables[widget] = drawable;
		
		if(wasEmpty)
		{
 			//qDebug() << "LiveLayer::drawable: widget:"<<widget<<", cache miss, first drawable";
			initDrawable(drawable, true);
		}
		else
		{
 			//qDebug() << "LiveLayer::drawable: widget:"<<widget<<", cache miss, copy from first";
			initDrawable(drawable, false);
		}

		if(widget->property("isEditorWidget").toBool())
		{
			drawable->setAnimationsEnabled(false);
			drawable->show();
		}
	}
	
	// GLW_PROP_NUM_SCENES defined in LiveScene.h
	int numLayers = widget->property(GLW_PROP_NUM_SCENES).toInt();
	drawable->setZIndexModifier(numLayers);
	
	return drawable;
}

QWidget * LiveLayer::generatePropertyEditor(QObject *object, const char *property, const char *slot, PropertyEditorOptions opts, const char *changeSignal)
{
	if(changeSignal == NULL &&
		dynamic_cast<LiveLayer*>(object))
		changeSignal = SIGNAL(layerPropertyChanged(const QString&, const QVariant&, const QVariant&));
		
	QWidget *base = new QWidget();
	QHBoxLayout *hbox = new QHBoxLayout(base);
	hbox->setContentsMargins(0,0,0,0);

	QVariant prop = object->property(property);

	if(opts.value.isValid())
		prop = opts.value;

	if(opts.type == QVariant::Invalid)
		opts.type = prop.type();

	//qDebug() << "generatePropertyEditor: prop:"<<property<<", opts.type:"<<opts.type<<", variant:"<<(opts.value.isValid() ? opts.value : prop);

	if(opts.type == QVariant::Int)
	{
		QSpinBox *spin = new QSpinBox(base);
		if(!opts.suffix.isEmpty())
			spin->setSuffix(opts.suffix);
		spin->setMinimum((int)opts.min);
		spin->setMaximum((int)opts.max);

		if(prop.type() == QVariant::Double && opts.doubleIsPercentage)
			spin->setValue((int)(prop.toDouble()*100.0));
		else
			spin->setValue(prop.toInt());

		QObject::connect(spin, SIGNAL(valueChanged(int)), object, slot);
		if(changeSignal)
			new PropertyChangeListener(object, changeSignal, spin, SLOT(setValue(int)), spin->value(), property);
			 
		hbox->addWidget(spin);

		if(!opts.noSlider)
		{
			QSlider *slider;
			slider = new QSlider(base);
			slider->setOrientation(Qt::Horizontal);
			slider->setMinimum((int)opts.min);
			slider->setMaximum((int)opts.max);
			slider->setSingleStep(opts.step);
			slider->setPageStep(opts.step*2);

			if(prop.type() == QVariant::Double && opts.doubleIsPercentage)
				slider->setValue((int)(prop.toDouble()*100.0));
			else
				slider->setValue(prop.toInt());

			QObject::connect(spin, SIGNAL(valueChanged(int)), slider, SLOT(setValue(int)));
			QObject::connect(slider, SIGNAL(valueChanged(int)), spin, SLOT(setValue(int)));
			hbox->addWidget(slider);
		}
		
		QPushButton *undoBtn = new QPushButton(QPixmap("../data/stock-undo.png"), "");
		ObjectValueSetter *setter = new ObjectValueSetter(spin, SLOT(setValue(int)), spin->value());
		connect(undoBtn, SIGNAL(clicked()), setter, SLOT(executeSetValue()));
		hbox->addWidget(undoBtn);
		
		if(opts.defaultValue.isValid())
		{
			QPushButton *resetBtn = new QPushButton(QPixmap("../data/stock-close.png"), "");
			ObjectValueSetter *setter = new ObjectValueSetter(spin, SLOT(setValue(int)), opts.defaultValue);
			connect(resetBtn, SIGNAL(clicked()), setter, SLOT(executeSetValue()));
			hbox->addWidget(resetBtn);
		}
		
		
	}
	else
	if(opts.type == QVariant::Bool)
	{
		QCheckBox *box = new QCheckBox();
		delete base;

		if(opts.text.isEmpty())
			opts.text = guessTitle(property);

		box->setText(opts.text);

		box->setChecked( prop.toBool() );
		connect(box, SIGNAL(toggled(bool)), object, slot);
		
		if(changeSignal)
			new PropertyChangeListener(object, changeSignal, box, SLOT(setChecked(bool)), prop, property);

		return box;
	}
	else
	if(opts.type == QVariant::String)
	{
		QLineEdit *box = new QLineEdit();
		
		box->setText( prop.toString() );
		QObject::connect(box, SIGNAL(textChanged(const QString&)), object, slot);
		
		if(changeSignal)
			new PropertyChangeListener(object, changeSignal, box, SLOT(setText(const QString&)), prop, property);
		
		if(opts.stringIsFile)
		{
			
			QCompleter *completer = new QCompleter(box);
			QDirModel *dirModel = new QDirModel(completer);
			completer->setModel(dirModel);
			//completer->setMaxVisibleItems(10);
			completer->setCompletionMode(QCompleter::PopupCompletion);
			completer->setCaseSensitivity(Qt::CaseInsensitive);
			completer->setWrapAround(true);
			box->setCompleter(completer);
			
			hbox->addWidget(box);
			
			QPushButton *browseButton = new QPushButton(QPixmap("../data/stock-open.png"), "");
			BrowseDialogLauncher *setter = new BrowseDialogLauncher(box, SLOT(setText(const QString&)), box->text());
			connect(browseButton, SIGNAL(clicked()), setter, SLOT(browse()));
			
			if(!opts.fileTypeFilter.isEmpty())
				setter->setFilter(opts.fileTypeFilter);
			
			hbox->addWidget(browseButton);
			
			return base;
		}
		else
		{
			delete base;
		}

		return box;
	}
	else
	if(opts.type == QVariant::SizeF)
	{
		SizeEditorWidget *editor = new SizeEditorWidget();
		delete base;

		QSizeF size = prop.toSizeF();
		editor->setValue(size);

		connect(editor, SIGNAL(valueChanged(const QSizeF&)), object, slot);
		if(changeSignal)
			new PropertyChangeListener(object, changeSignal, editor, SLOT(setValue(const QSizeF&)), prop, property);

		return editor;
	}
	else
	if(opts.type == QVariant::PointF)
	{
		PointEditorWidget *editor = new PointEditorWidget();
		delete base;

		QPointF point = prop.toPointF();
		editor->setValue(point);

		connect(editor, SIGNAL(valueChanged(const QPointF&)), object, slot);
		if(changeSignal)
			new PropertyChangeListener(object, changeSignal, editor, SLOT(setValue(const QPointF&)), prop, property);

		return editor;
	}
	else
	if(opts.type == QVariant::Double)
	{
		DoubleEditorWidget *editor = new DoubleEditorWidget();
		delete base;

		editor->setShowSlider(!opts.noSlider);

		editor->setValue(prop.toDouble());

		connect(editor, SIGNAL(valueChanged(double)), object, slot);
		if(changeSignal)
			new PropertyChangeListener(object, changeSignal, editor, SLOT(setValue(double)), prop, property);

		return editor;
	}
	else
	{
		qDebug() << "LiveLayer::generatePropertyEditor(): No editor for type: "<<opts.type;
	}


	return base;
}



QWidget * LiveLayer::createLayerPropertyEditors()
{
	QWidget * base = new QWidget();
	QVBoxLayout *blay = new QVBoxLayout(base);
	blay->setContentsMargins(0,0,0,0);


	PropertyEditorOptions opts;

	opts.reset();
	
	{
		ExpandableWidget *group = new ExpandableWidget("Layer Information",base);
		blay->addWidget(group);
	
		QWidget *cont = new QWidget;
		QFormLayout *lay = new QFormLayout(cont);
		lay->setContentsMargins(3,3,3,3);
		
		group->setWidget(cont);
		
		lay->addRow(tr("&Layer Name:"), generatePropertyEditor(this, "layerName", SLOT(setLayerName(const QString&)), opts));
		lay->addRow(generatePropertyEditor(this, "userControllable", SLOT(setUserControllable(bool)), opts));
	}
	
	ExpandableWidget *groupGeom = new ExpandableWidget("Position and Display",base);
	blay->addWidget(groupGeom);

	QWidget *groupGeomContainer = new QWidget;
	QFormLayout *formLayout = new QFormLayout(groupGeomContainer);
	formLayout->setContentsMargins(3,3,3,3);
	//formLayout->setContentsMargins(0,0,0,0);

	m_geomLayout = formLayout;

	groupGeom->setWidget(groupGeomContainer);
	
	

	QStringList showAsList = QStringList()
		<< "Absolute Position"//0
		<< "--------------------"//1
		<< "Centered on Screen"//2
		<< "Aligned Top Left"//3
		<< "Aligned Top Center"//4
		<< "Aligned Top Right"//5
		<< "Aligned Center Right"//6
		<< "Aligned Center Left"//7
		<< "Aligned Bottom Left"//8
		<< "Aligned Bottom Center"//9
		<< "Aligned Bottom Right";//10

	QComboBox *showAsBox = new QComboBox();
	showAsBox->addItems(showAsList);
	//showAsBox->setCurrentIndex(

	int idx = 0;
// 	if(showFullScreen())
// 	{
// 		idx = 0;
// 	}
// 	else
// 	{
		Qt::Alignment align = alignment();

		if((align & Qt::AlignAbsolute) == Qt::AlignAbsolute)
		{
			idx = 0;
		}
		else
		if((align & Qt::AlignHCenter) == Qt::AlignHCenter &&
		   (align & Qt::AlignVCenter) == Qt::AlignVCenter)
		{
			idx = 2;
		}
		else
		if((align & Qt::AlignTop)  == Qt::AlignTop &&
		   (align & Qt::AlignLeft) == Qt::AlignLeft)
		{
			idx = 3;
		}
		else
		if((align & Qt::AlignTop)  == Qt::AlignTop &&
		   (align & Qt::AlignHCenter) == Qt::AlignHCenter)
		{
			idx = 4;
		}
		else
		if((align & Qt::AlignTop)  == Qt::AlignTop &&
		   (align & Qt::AlignRight) == Qt::AlignRight)
		{
			idx = 5;
		}
		else
		if((align & Qt::AlignVCenter) == Qt::AlignVCenter &&
		   (align & Qt::AlignRight) == Qt::AlignRight)
		{
			idx = 6;
		}
		else
		if((align & Qt::AlignVCenter) == Qt::AlignVCenter &&
		   (align & Qt::AlignLeft) == Qt::AlignLeft)
		{
			idx = 7;
		}
		else
		if((align & Qt::AlignBottom) == Qt::AlignBottom &&
		   (align & Qt::AlignLeft) == Qt::AlignLeft)
		{
			idx = 8;
		}
		else
		if((align & Qt::AlignBottom) == Qt::AlignBottom &&
		   (align & Qt::AlignHCenter) == Qt::AlignHCenter)
		{
			idx = 9;
		}
		else
		if((align & Qt::AlignBottom) == Qt::AlignBottom &&
		   (align & Qt::AlignRight) == Qt::AlignRight)
		{
			idx = 10;
		}
// 	}
	if(idx >=0 && idx <= 10)
		showAsBox->setCurrentIndex(idx);

	connect(showAsBox, SIGNAL(activated(const QString&)), this, SLOT(setShowAsType(const QString&)));

	formLayout->addRow(tr("&Show As:"), showAsBox);

	opts.reset();
	opts.type = QVariant::Int;
	opts.min = -1000;
	opts.max =  1000;
	opts.step = 10;
	opts.defaultValue = 0;

	opts.value = insetTopLeft().x();
	formLayout->addRow(tr("&Inset Left:"), m_propWidget["insetLeft"] = generatePropertyEditor(this, "insetLeft", SLOT(setLeftInset(int)), opts));

	opts.value = insetTopLeft().y();
	formLayout->addRow(tr("&Inset Top:"), m_propWidget["insetTop"] = generatePropertyEditor(this, "insetTop", SLOT(setTopInset(int)), opts));

	opts.value = insetBottomRight().x();
	formLayout->addRow(tr("&Inset Right:"), m_propWidget["insetRight"] = generatePropertyEditor(this, "insetRight", SLOT(setRightInset(int)), opts));

	opts.value = insetBottomRight().y();
	formLayout->addRow(tr("&Inset Bottom:"), m_propWidget["insetBottom"] = generatePropertyEditor(this, "insetBottom", SLOT(setBottomInset(int)), opts));

	opts.reset();
	opts.suffix = "%";
	opts.min = 0;
	opts.max = 5000;
	opts.step = 10;
	opts.defaultValue = 100;
	opts.type = QVariant::Int;
	opts.doubleIsPercentage = true;
	formLayout->addRow(tr("&Scale Size:"), m_propWidget["sizeScale"] = generatePropertyEditor(this, "alignedSizeScale", SLOT(setAlignedSizeScale(int)), opts));
	
// 	opts.reset();
// 	opts.suffix = "%";
// 	opts.min = -100;
// 	opts.max =  200;
// 	opts.step = 10;
// 	opts.type = QVariant::Int;
// 	opts.doubleIsPercentage = true;
// 	
// 	opts.defaultValue = 0;
// 	formLayout->addRow(tr("&Top:"), 	m_propWidget["topPercent"]	= generatePropertyEditor(this, "topPercent", SLOT(setTopPercent(int)), opts));
// 	formLayout->addRow(tr("&Left:"), 	m_propWidget["leftPercent"]	= generatePropertyEditor(this, "leftPercent", SLOT(setLeftPercent(int)), opts));
// 	
// 	opts.defaultValue = 100;
// 	formLayout->addRow(tr("&Bottom:"), 	m_propWidget["bottomPercent"]	= generatePropertyEditor(this, "bottomPercent", SLOT(setBottomPercent(int)), opts));
// 	formLayout->addRow(tr("&Right:"), 	m_propWidget["rightPercent"]	= generatePropertyEditor(this, "rightPercent", SLOT(setRightPercent(int)), opts));

	formLayout->addRow(m_propWidget["absPosition"] = new PositionWidget(this));
	

	opts.reset();
	opts.noSlider = true;
	opts.type = QVariant::Int;
	opts.defaultValue = 0;
	formLayout->addRow(tr("&Z Value:"), generatePropertyEditor(this, "zIndex", SLOT(setZIndex(int)), opts));
	
	opts.reset();
	opts.suffix = " deg";
	opts.min = -360.0;
	opts.max =  360.0;
	opts.step = 10;
	opts.defaultValue = 0;
	opts.type = QVariant::Int;
	opts.value = rotation().x();
	formLayout->addRow(tr("&X Rotation:"), generatePropertyEditor(this, "xRotation", SLOT(setXRotation(int)), opts));
	
	opts.value = rotation().y();
	formLayout->addRow(tr("&Y Rotation:"), generatePropertyEditor(this, "yRotation", SLOT(setYRotation(int)), opts));
	
	opts.value = rotation().z();
	formLayout->addRow(tr("&Z Rotation:"), generatePropertyEditor(this, "zRotation", SLOT(setZRotation(int)), opts));

	opts.reset();
	opts.suffix = "%";
	opts.min = 0;
	opts.max = 100;
	opts.defaultValue = 100;
	opts.type = QVariant::Int;
	opts.doubleIsPercentage = true;
	formLayout->addRow(tr("&Opacity:"), generatePropertyEditor(this, "opacity", SLOT(setOpacity(int)), opts));

	// Add layers to a map to sort by zindex
	if(m_scene)
	{
		QMap<double,LiveLayer*> layerMap;
		QList<LiveLayer*> layerList = m_scene->layerList();
		foreach(LiveLayer *layer, layerList)
			if(layer != this)
				layerMap[layer->zIndex()] = layer;
			
		m_sortedLayerList = layerMap.values();
		
		QStringList layerNames;
		layerNames << "(None)";
		int hideIdx=0, showIdx=0, count=1;
		foreach(LiveLayer *layer, m_sortedLayerList)
		{
			layerNames << QString("%1 (%2)").arg(layer->instanceName()).arg(layer->typeName());
			if(layer == m_hideOnShow)
				hideIdx = count;
			if(layer == m_showOnShow)
				showIdx = count;
			count ++;
		}
	
		//qDebug() << "LiveLayer::createLayerPropertyEditors(): hideIdx:"<<hideIdx<<", showIdx:"<<showIdx;
		
		QComboBox *hideOnShowBox = new QComboBox();
		hideOnShowBox->addItems(layerNames);
		hideOnShowBox->setCurrentIndex(hideIdx);
		hideOnShowBox->setEnabled(layerNames.size() > 1);
		connect(hideOnShowBox, SIGNAL(activated(int)), this, SLOT(setHideOnShow(int)));
	
		QComboBox *showOnShowBox = new QComboBox();
		showOnShowBox->addItems(layerNames);
		showOnShowBox->setCurrentIndex(showIdx);
		showOnShowBox->setEnabled(layerNames.size() > 1);
		connect(showOnShowBox, SIGNAL(activated(int)), this, SLOT(setShowOnShow(int)));
	
		formLayout->addRow(tr("&Hide on Show:"), hideOnShowBox);
		formLayout->addRow(tr("&Show on Show:"), showOnShowBox);
	}
	
	//groupGeom->setExpanded(false);
	groupGeom->setExpandedIfNoDefault(false);

	/////////////////////////////////////////

	ExpandableWidget *groupAnim = new ExpandableWidget("Show/Hide Effects",base);
	blay->addWidget(groupAnim);

	QWidget *groupAnimContainer = new QWidget;
	QGridLayout *animLayout = new QGridLayout(groupAnimContainer);
	animLayout->setContentsMargins(3,3,3,3);

	groupAnim->setWidget(groupAnimContainer);

	opts.reset();
	opts.suffix = " ms";
	opts.min = 10;
	opts.max = 8000;
	opts.defaultValue = 300;

	int row = 0;
	animLayout->addWidget(generatePropertyEditor(this, "fadeIn", SLOT(setFadeIn(bool)), opts), row, 0);
	animLayout->addWidget(generatePropertyEditor(this, "fadeInLength", SLOT(setFadeInLength(int)), opts), row, 1);

	row++;
	animLayout->addWidget(generatePropertyEditor(this, "fadeOut", SLOT(setFadeOut(bool)), opts), row, 0);
	animLayout->addWidget(generatePropertyEditor(this, "fadeOutLength", SLOT(setFadeOutLength(int)), opts), row, 1);

	opts.reset();

	groupAnim->setExpandedIfNoDefault(false);

	/////////////////////////////////////////

	ExpandableWidget *groupAnimAdvanced = new ExpandableWidget("Advanced Effects",base);
	blay->addWidget(groupAnimAdvanced);

	QWidget *groupAnimAdvancedContainer = new QWidget;
	QFormLayout *animAdvancedLayout = new QFormLayout(groupAnimAdvancedContainer);
	animAdvancedLayout->setContentsMargins(3,3,3,3);

	groupAnimAdvanced->setWidget(groupAnimAdvancedContainer);

	opts.reset();
	opts.suffix = " ms";
	opts.min = 10;
	opts.max = 8000;
	
	QStringList animTypes = QStringList()
		<< "(None)"
		<< "Zoom In/Out"
		<< "Slide From/To Top"
		<< "Slide From/To Bottom"
		<< "Slide From/To Left"
		<< "Slide From/To Right";

	QComboBox *showBox = new QComboBox();
	showBox->addItems(animTypes);
	idx = (int)showAnimationType();
	showBox->setCurrentIndex(!idx?idx:idx-1);
	connect(showBox, SIGNAL(activated(int)), this, SLOT(setShowAnim(int)));

	QComboBox *hideBox = new QComboBox();
	hideBox->addItems(animTypes);
	idx = (int)hideAnimationType();
	hideBox->setCurrentIndex(!idx?idx:idx-1);
	connect(hideBox, SIGNAL(activated(int)), this, SLOT(setHideAnim(int)));

	animAdvancedLayout->addRow(tr("&Show Animation:"), showBox);
	opts.defaultValue = 2500;
	animAdvancedLayout->addRow(tr("&Show Anim Length:"), generatePropertyEditor(this, "showAnimationLength", SLOT(setShowAnimationLength(int)), opts));
	
	
	PropertyEditorOptions opts2;
	opts2.type = QVariant::Bool;
	opts2.value = showAnimationCurve() == QEasingCurve::OutElastic ? true : false;
	animAdvancedLayout->addRow(tr("Bounce When Shown:"), generatePropertyEditor(this, "", SLOT(setBounceWhenShown(bool)), opts2));
	
	animAdvancedLayout->addRow(tr("&Hide Animation:"), hideBox);
	opts.defaultValue = 300;
	animAdvancedLayout->addRow(tr("&Hide Anim Length:"), generatePropertyEditor(this, "hideAnimationLength", SLOT(setHideAnimationLength(int)), opts));

	groupAnimAdvanced->setExpandedIfNoDefault(false);

	row++;
	animLayout->addWidget(groupAnimAdvanced, row, 0,1, 2);

	/////////////////////////////////////////

	setShowAsType(showAsBox->itemText(showAsBox->currentIndex()));


	return base;
}

void LiveLayer::setBounceWhenShown(bool flag)
{
	setShowAnimationCurve(flag ? QEasingCurve::OutElastic : QEasingCurve::Linear);
}

void LiveLayer::setShowAnim(int x)
{
	setShowAnimationType(x == 0 ? x : x + 1);
}

void LiveLayer::setHideAnim(int x)
{
	setHideAnimationType(x == 0 ? x : x + 1);
}

void LiveLayer::setShowAsType(const QString& text)
{
// 	<< "Full Screen"
// 	<< "--------------------"
// 	<< "Absolute Position"
// 	<< "--------------------"
// 	<< "Centered on Screen"
// 	<< "Aligned Top Left"
// 	<< "Aligned Top Center"
// 	<< "Aligned Top Right"
// 	<< "Aligned Center Right"
// 	<< "Aligned Center Left"
// 	<< "Aligned Bottom Left"
// 	<< "Aligned Bottom Center"
// 	<< "Aligned Bottom Right"
	setVisibleGeometryFields();

	//qDebug() << "LiveLayer::setShowAsType(): text:"<<text;
	if(text.indexOf("---") >= 0)
		return;

// 	if(text.indexOf("Full Screen") >= 0)
// 	{
// 		setVisibleGeometryFields(QStringList());
// 
// 	}
// 	else
// 	{
// 		setShowFullScreen(false);

		if(text.indexOf("Absolute") >= 0)
		{
			setAlignment(Qt::AlignAbsolute);
			setVisibleGeometryFields(QStringList() << "absPosition");
		}
		else
		if(text.indexOf("Centered") >= 0)
		{
			setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
			setVisibleGeometryFields(QStringList() << "sizeScale");
		}
		else
		if(text.indexOf("Top Left") >= 0)
		{
			setAlignment(Qt::AlignTop | Qt::AlignLeft);
			setVisibleGeometryFields(QStringList() << "insetTop" << "insetLeft" << "sizeScale");
		}
		else
		if(text.indexOf("Top Center") >= 0)
		{
			setAlignment(Qt::AlignTop | Qt::AlignHCenter);
			setVisibleGeometryFields(QStringList() << "insetTop" << "sizeScale");
		}
		else
		if(text.indexOf("Top Right") >= 0)
		{
			setAlignment(Qt::AlignTop | Qt::AlignRight);
			setVisibleGeometryFields(QStringList() << "insetTop" << "insetRight" << "sizeScale");
		}
		else
		if(text.indexOf("Center Right") >= 0)
		{
			setAlignment(Qt::AlignVCenter | Qt::AlignRight);
			setVisibleGeometryFields(QStringList() << "insetRight" << "sizeScale");
		}
		else
		if(text.indexOf("Center Left") >= 0)
		{
			setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
			setVisibleGeometryFields(QStringList() << "insetLeft" << "sizeScale");
		}
		else
		if(text.indexOf("Bottom Left") >= 0)
		{
			setAlignment(Qt::AlignBottom | Qt::AlignLeft);
			setVisibleGeometryFields(QStringList() << "insetBottom" << "insetLeft" << "sizeScale");
		}
		else
		if(text.indexOf("Bottom Center") >= 0)
		{
			setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
			setVisibleGeometryFields(QStringList() << "insetBottom" << "sizeScale");
		}
		else
		if(text.indexOf("Bottom Right") >= 0)
		{
			setAlignment(Qt::AlignBottom | Qt::AlignRight);
			setVisibleGeometryFields(QStringList() << "insetBottom" << "insetRight" << "sizeScale");
		}
		else
		{
			qDebug() << "LiveLayer::setShowAsType(): Unknown type:"<<text;
		}
	//}
	
// 	if(!m_drawables.isEmpty())
// 		m_props["rect"] = m_drawables[m_drawables.keys().first()]->rect();
}

void LiveLayer::setVisibleGeometryFields(QStringList list)
{
	QHash<QString,bool> found;
	foreach(QString field, list)
	{
		QWidget *widget = m_propWidget[field];
		if(!widget)
		{
			qDebug() << "LiveLayer::setVisibleGeometryFields: Cannot find requested field:"<<field;
		}
		else
		{
			widget->setVisible(true);

			QWidget *label = m_geomLayout->labelForField(widget);
			if(label)
				label->setVisible(true);

			found[field] = true;
		}
	}

	foreach(QString key, m_propWidget.keys())
	{
		if(!found.contains(key))
		{
			QWidget *widget = m_propWidget[key];
			widget->setVisible(false);

			QWidget *label = m_geomLayout->labelForField(widget);
			if(label)
				label->setVisible(false);
		}
	}
}

void LiveLayer::setX(int value)
{
	QRectF r = rect();
	r = QRectF(value, r.y(), r.width(), r.height());
	setRect(r);
}

void LiveLayer::setY(int value)
{
	QRectF r = rect();
	r = QRectF(r.x(), value, r.width(), r.height());
	setRect(r);
}

void LiveLayer::setX2(int value)
{
	QRectF r = rect();
	r = QRectF(r.x(), r.y(), value-r.x(), r.height());
	setRect(r);
}

void LiveLayer::setY2(int value)
{
	QRectF r = rect();
	r = QRectF(r.x(), r.y(), r.width(), value-r.y());
	setRect(r);
}

void LiveLayer::setWidth(int value)
{
	QRectF r = rect();
	r = QRectF(r.x(), r.y(), value, r.height());
	setRect(r);
}

void LiveLayer::setHeight(int value)
{
	QRectF r = rect();
	r = QRectF(r.x(), r.y(), r.width(), value);
	setRect(r);
}

void LiveLayer::setVisible(bool flag)
{
	if(m_lockVisibleSetter)
		return;
	m_lockVisibleSetter = true;
	
	m_isVisible = flag;
	
	emit isVisible(flag);
		
	
	//qDebug() << "LiveLayer::setVisible: "<<this<<", flag:"<<flag;
	
	foreach(GLWidget *glw, m_glWidgets)
	{
		bool editor = glw->property("isEditorWidget").toBool();
		if(editor)
			continue;
		
		if(m_secondarySourceActive && m_secondaryDrawables.contains(glw))
		{
			m_secondaryDrawables[glw]->setVisible(flag);
			if(m_drawables[glw]->isVisible())
				m_drawables[glw]->setVisible(false);
		}
		else
		{
			m_drawables[glw]->setVisible(flag);
			if(m_secondaryDrawables.contains(glw) &&
				m_secondaryDrawables[glw]->isVisible())
				m_secondaryDrawables[glw]->setVisible(false);
		}
	}
	
	if(m_hideOnShow)
		if(m_hideOnShow->isVisible() == flag)
			m_hideOnShow->setVisible(!flag);

	if(m_showOnShow)
		if(m_showOnShow->isVisible() != flag)
			m_showOnShow->setVisible(flag);
		
	m_lockVisibleSetter = false;
}


void LiveLayer::setRect(const QRectF& rect) 	{ setLayerProperty("rect", rect); }
void LiveLayer::setZIndex(double z)	 	{ setLayerProperty("zIndex", z);  }
void LiveLayer::setZIndex(int z)	 	{ setLayerProperty("zIndex", z);  } // for sig slot compat
void LiveLayer::setOpacity(double o)		{ setLayerProperty("opacity", o); }
void LiveLayer::setOpacity(int o)		{ setLayerProperty("opacity", ((double)o)/100.0); }
void LiveLayer::setTransparency(int o)		{ setLayerProperty("opacity", (100.0-(double)o)/100.0); }

void LiveLayer::lockLayerPropertyUpdates(bool flag) { m_lockLayerPropertyUpdates = flag; } 

// Internally, tries to set the named property on all the drawables if it has such a property
// If no prop exists on the drawable, then tries to set the prop on the layer object
// Either way, sets the prop in m_props and emits layerPropertyChanged()
// This can be overridden to catch property changes internally, since the property editors
// in MainWindow use this slot to update the props. Most of the time, this will Do The Right Thing
// for basic properties, but for some custom ones you might have to intercept it.
void LiveLayer::setLayerProperty(const QString& propertyId, const QVariant& value)
{
	//if(!m_props.contains(propertyId))
	//	return;
	if(m_lockLayerPropertyUpdates)
	{
		//if(propertyId == "rect")
		//	qDebug() << "LiveLayer::setLayerProperty: [LOCKED] id:"<<propertyId<<", value:"<<value<<" [LOCKED]";
		//m_props[propertyId] = value;
		return;
	}
		
	// Prevent recursions that may be triggered by a property setter in turn calling setLayerProperty(), 
	// which would just loop back and call that property setter again for that property - recursion.
	if(m_propSetLock[propertyId])
	{
		//if(propertyId == "rect")
		//	qDebug() << "LiveLayer::setLayerProperty: [PROP_SET_LOCK] id:"<<propertyId<<", value:"<<value<<" [LOCKED]";
		return;
	}
		
	m_propSetLock[propertyId] = true;

 	//if(propertyId != "rect")
 	//if(propertyId == "rect")
 	//	qDebug() << "LiveLayer::setLayerProperty: id:"<<propertyId<<", value:"<<value;

	QVariant oldValue = m_props[propertyId];
	m_props[propertyId] = value;

	if(value != oldValue)
		layerPropertyWasChanged(propertyId, value, oldValue);

	if(m_drawables.isEmpty())
	{
		//if(propertyId == "rect")
		//	qDebug() << "LiveLayer::setLayerProperty: id:"<<propertyId<<", drawables empty, unlocking and returning";
		m_propSetLock[propertyId] = false;
		return;
	}

	if(propertyId.indexOf("fadeIn")    > -1 ||
	   propertyId.indexOf("fadeOut")   > -1 ||
	   propertyId.indexOf("Animation") > -1)
	{
		foreach(GLDrawable *item, m_drawables.values())
		{
			applyAnimationProperties(item);
		}
		
		foreach(GLDrawable *item, m_secondaryDrawables.values())
		{
			applyAnimationProperties(item);
		}
	}
	else
	{
		GLDrawable *drawable = m_drawables[m_drawables.keys().first()];
	
		if(drawable->metaObject()->indexOfProperty(qPrintable(propertyId)) >= 0)
		{
			//if(propertyId == "rect")
			//	qDebug() << "LiveLayer::setLayerProperty: id:"<<propertyId<<", value:"<<value<<", applying to drawables";
			applyDrawableProperty(propertyId, value);
		}
		else
		if(metaObject()->indexOfProperty(qPrintable(propertyId)) >= 0)
		{
			//if(propertyId == "rect")
			//	qDebug() << "LiveLayer::setLayerProperty: id:"<<propertyId<<", applying to self";
			// could cause recursion if the property setter calls this method again, hence the m_propSetLock[] usage
			setProperty(qPrintable(propertyId), value);
		}
		else
		{
			qDebug() << "LiveLayer::setLayerProperty: id:"<<propertyId<<", not found in any meta object!";
		}
	}
	
	m_propSetLock[propertyId] = false;
}


void LiveLayer::applyDrawableProperty(const QString& propertyId, const QVariant& value)
{
// 	m_animationsDisabled = true;
	foreach(GLDrawable *item, m_drawables.values())
	{
		applyDrawablePropertyInternal(item, propertyId, value);
	}
	
	foreach(GLDrawable *item, m_secondaryDrawables.values())
	{
		applyDrawablePropertyInternal(item, propertyId, value);
	}
}
	
void LiveLayer::applyDrawablePropertyInternal(GLDrawable *drawable, const QString& propertyId, const QVariant& value)
{
	if(m_animationsDisabled || !canAnimateProperty(propertyId))
	{
		//qDebug() << "LiveLayer::applyDrawableProperty:"<<drawable<<propertyId<<"="<<value<<", not animating";
		drawable->setProperty(qPrintable(propertyId), value);
	}
	else
	{
		if(propertyId == "alignment")
		{
			//qDebug() << "LiveLayer::applyDrawableProperty:"<<drawable<<propertyId<<"="<<value<<", animating alignment";
			drawable->setAlignment((Qt::Alignment)value.toInt(), true, m_animParam.length, m_animParam.curve);
		}
		else
		{
			//qDebug() << "LiveLayer::applyDrawableProperty:"<<drawable<<propertyId<<"="<<value<<", animating!!";
			QPropertyAnimation *animation = new QPropertyAnimation(drawable, propertyId.toAscii());
			animation->setDuration(m_animParam.length);
			animation->setEasingCurve(m_animParam.curve);
			animation->setEndValue(value);
			animation->start(QAbstractAnimation::DeleteWhenStopped);
		}
	}
}

// void LiveLayer::setShowFullScreen(bool value)
// {
// 	setTopPercent(0.);
// 	setLeftPercent(0.);
// 	setBottomPercent(1.);
// 	setRightPercent(1.);
// }

void LiveLayer::applyAnimationProperties(GLDrawable *drawable)
{
	drawable->resetAllAnimations();

	if(m_props["fadeIn"].toBool())
		drawable->addShowAnimation(GLDrawable::AnimFade,m_props["fadeInLength"].toInt());

	if(m_props["fadeOut"].toBool())
		drawable->addHideAnimation(GLDrawable::AnimFade,m_props["fadeOutLength"].toInt());

	GLDrawable::AnimationType type;

	type = (GLDrawable::AnimationType)m_props["showAnimationType"].toInt();
	if(type != GLDrawable::AnimNone)
		drawable->addShowAnimation(type,m_props["showAnimationLength"].toInt()).curve = (QEasingCurve::Type)m_props["showAnimationCurve"].toInt();

	type = (GLDrawable::AnimationType)m_props["hideAnimationType"].toInt();
	if(type != GLDrawable::AnimNone)
		drawable->addHideAnimation(type,m_props["hideAnimationLength"].toInt()).curve = (QEasingCurve::Type)m_props["hideAnimationCurve"].toInt();
}

void LiveLayer::setInstanceName(const QString& name)
{
	QString oldName = m_instanceName;
	m_instanceName = name;
	setObjectName(qPrintable(m_instanceName));
	emit instanceNameChanged(name);
	layerPropertyWasChanged("instanceName",name,oldName);
}

void LiveLayer::setUserControllable(bool flag)
{
	bool oldFlag = m_isUserControllable;
	m_isUserControllable = flag;
	emit userControllableChanged(flag);
	layerPropertyWasChanged("isUserControllable",flag,oldFlag);
}

void LiveLayer::setLayerName(const QString& name)
{
	QString oldName = m_layerName;
	m_layerName = name;
	emit layerNameChanged(name);
	layerPropertyWasChanged("layerName",name,oldName);
}

// just emits layerPropertyChanged
void LiveLayer::layerPropertyWasChanged(const QString& propertyId, const QVariant& value, const QVariant& oldValue)
{
	//qDebug() << "LiveLayer::layerPropertyWasChanged: "<<this<<": property:"<<propertyId<<", value:"<<value<<", was:"<<oldValue;
	emit layerPropertyChanged(propertyId, value, oldValue);
}

// The core of the layer - create a new drawable instance for the specified context.
// drawable() will call initDrawable() on it to set it up as needed
GLDrawable *LiveLayer::createDrawable(GLWidget */*widget*/, bool /*isSecondary*/)
{
 	qDebug() << "LiveLayer::createDrawable: Nothing created.";
	return 0;
}

// If its the first drawable, setup with defaults and load m_props[] with appros values
// If not first drawable, load drawable with values from m_props[]
void LiveLayer::initDrawable(GLDrawable *drawable, bool /*isFirstDrawable*/)
{
	if(m_scene)
	{
		connect(m_scene, SIGNAL(canvasSizeChanged(const QSizeF&)), drawable, SLOT(setCanvasSize(const QSizeF)));
		drawable->setCanvasSize(m_scene->canvasSize());
	}
	
 	//qDebug() << "LiveLayer::initDrawable: drawable:"<<drawable;
	bool animEnabled = setAnimEnabled(false);
	
	QStringList generalProps = QStringList()
			<< "zIndex"
			<< "opacity"
// 			<< "showFullScreen"
			<< "alignment"
			<< "insetTopLeft"
			<< "insetBottomRight"
			<< "alignedSizeScale"
			<< "rotation"
			<< "rect"
			/*<< "topPercent"
			<< "leftPercent"
			<< "bottomPercent"
			<< "rightPercent"*/;
			
		
	//qDebug() << "LiveLayer::initDrawable: drawable:"<<drawable<<", props list:"<<generalProps;
	applyLayerPropertiesToObject(drawable, generalProps);
	applyAnimationProperties(drawable);
	
	setAnimEnabled(animEnabled);

	//fqDebug() << "LiveLayer::initDrawable: "<<this<<", drawable: "<<drawable<<", now setting visible:"<<m_isVisible<<", rect:"<<drawable->rect();
	//drawable->setVisible(m_isVisible);
	setVisible(m_isVisible);
	
}


// Helper function - given a list of property names, load the props using QObject::property() from the drawable into m_props
void LiveLayer::loadLayerPropertiesFromObject(const QObject *object, const QStringList& list)
{
	foreach(QString key, list)
	{
		m_props[key] = object->property(qPrintable(key));
	}
}

// Helper function - attempts to apply the list of props in 'list' (or all props in m_props if 'list' is empty) to drawable
// 'Attempts' means that if drawable->metaObject()->indexOfProperty() returns <0, then setProperty is not called
void LiveLayer::applyLayerPropertiesToObject(QObject *object, QStringList list)
{
	//qDebug() << "LiveLayer::applyLayerPropertiesToObject(): "<<object<<", list:"<<list;
	if(list.isEmpty())
		foreach(QString key, m_props.keys())
			list << key;

	foreach(QString key, list)
	{
		if(m_props.contains(key))
		{
			if(object->metaObject()->indexOfProperty(qPrintable(key)) >= 0)
			{
				//if(key == "alignment")
				//	qDebug() << "LiveLayer::applyLayerPropertiesToObject(): "<<object<<", prop:"<<qPrintable(key)<<", setting to "<<m_props[key];
				object->setProperty(qPrintable(key), m_props[key]);
			}
			else
			{
				qDebug() << "LiveLayer::applyLayerPropertiesToObject(): "<<object<<", prop:"<<key<<", cannot find index of prop on object, not setting";
			}
		}
		else
		{
			qDebug() << "LiveLayer::applyLayerPropertiesToObject(): "<<object<<", prop:"<<key<<", m_props doesnt contain it, not setting";
		}
	}
}


void LiveLayer::fromByteArray(QByteArray& array)
{
	bool animEnabled = setAnimEnabled(false);
	
	QDataStream stream(&array, QIODevice::ReadOnly);
	QVariantMap map;
	stream >> map;
	
	//qDebug() << "LiveScene::fromByteArray(): "<<map;
	if(map.isEmpty())
	{
		qDebug() << "Error: LiveLayer::fromByteArray(): Map is empty, unable to load scene.";
		return;
	}
	
	loadPropsFromMap(map);
	
	setAnimEnabled(animEnabled);
	
	
}

void LiveLayer::loadPropsFromMap(const QVariantMap& map, bool onlyApplyIfChanged)
{
	bool vis = false;
	
	// So we dont have to engineer our own method of tracking
	// properties, just assume all inherited objects delcare the relevant
	// properties using Q_PROPERTY macro
	const QMetaObject *metaobject = metaObject();
	int count = metaobject->propertyCount();
	for (int i=0; i<count; ++i)
	{
		QMetaProperty metaproperty = metaobject->property(i);
		const char *name = metaproperty.name();
		QVariant value = map[name];
		
		//if(QString(name) == "rect")
		//	qDebug() << "LiveLayer::loadPropsFromMap():"<<this<<": i:"<<i<<", count:"<<count<<", prop:"<<name<<", value:"<<value;
		
		// Hold setting visiblility flag till last so that way any properties that affect
		// animations are set BEFORE animations start!
		if(QString(name) == "isVisible")
		{
			vis = value.toBool();
		}
		else
		if(QString(name) == "id")
		{
			// m_layerId is only set ONCE by this method, overwriting any ID assigned at creation time
			if(!m_layerIdLoaded && value.isValid())
			{
				m_layerIdLoaded = true;
				m_layerId = value.toInt();
			}
		}
		else
		{
			
			if(value.isValid())
			{
				if(onlyApplyIfChanged)
				{
					if(property(name) != value)
					{
 						//qDebug() << "LiveLayer::loadPropsFromMap():"<<this<<": [onlyApplyIfChanged] i:"<<i<<", count:"<<count<<", prop:"<<name<<", value:"<<value;
						setProperty(name,value);
					}
				}
				else
				{
					//if(QString(name) == "alignment")
					//	qDebug() << "LiveLayer::loadPropsFromMap():"<<this<<": i:"<<i<<", count:"<<count<<", prop:"<<name<<", value:"<<value<<" (calling set prop)";
						
					setProperty(name,value);
					//m_props[name] = value;
				}
			}
			else
				qDebug() << "LiveLayer::loadPropsFromMap: Unable to load property for "<<name<<", got invalid property from map";
		}
	}
	
	//qDebug() << "LiveLayer::fromByteArray():"<<this<<": *** Setting visibility to "<<vis;
	if(!onlyApplyIfChanged || isVisible() != vis)
		setVisible(vis);
}

QByteArray LiveLayer::toByteArray()
{
	QByteArray array;
	QDataStream stream(&array, QIODevice::WriteOnly);
	stream << propsToMap();
	return array;
}

QVariantMap LiveLayer::propsToMap()
{
	QVariantMap map;
	// So we dont have to engineer our own method of tracking
	// properties, just assume all inherited objects delcare the relevant
	// properties using Q_PROPERTY macro
	const QMetaObject *metaobject = metaObject();
	int count = metaobject->propertyCount();
	for (int i=0; i<count; ++i)
	{
		QMetaProperty metaproperty = metaobject->property(i);
		const char *name = metaproperty.name();
		QVariant value = property(name);
		
		//if(name == "aspectRatioMode")
		//	qDebug() << "LiveLayer::toByteArray():"<<this<<instanceName()<<": prop:"<<name<<", value:"<<value;
			
		map[name] = value;
	}
	return map;
}

bool LiveLayer::canAnimateProperty(const QString& propId)
{
	if(m_animateProps.contains(propId))
		return m_animateProps[propId];
	return true;
}

void LiveLayer::setAnimatePropFlags(const QStringList& list, bool flag)
{
	foreach(QString propId, list)
		m_animateProps[propId] = flag;
}

void LiveLayer::setShowOnShowLayerId(int value)
{
	setLayerProperty("showOnShowLayerId", value);
	if(m_scene)
		setShowOnShow(m_scene->layerFromId(value));
}

void LiveLayer::setHideOnShowLayerId(int value)
{
	setLayerProperty("hideOnShowLayerId", value);
	if(m_scene)
		setHideOnShow(m_scene->layerFromId(value));
}
	
void LiveLayer::setHideOnShow(LiveLayer *layer)
{
	m_hideOnShow = layer;
	setLayerProperty("hideOnShowLayerId", layer ? layer->id() : 0);
}

void LiveLayer::setHideOnShow(int x)
{
	if(x == 0)
	{
		setHideOnShow((LiveLayer*)0);
		return;
	}
	
	if(x<1 || x>m_sortedLayerList.size())
		return;
	setHideOnShow(m_sortedLayerList[x-1]);
}

void LiveLayer::setShowOnShow(LiveLayer *layer)
{
	m_showOnShow = layer;
	setLayerProperty("showOnShowLayerId", layer ? layer->id() : 0);
}

void LiveLayer::setShowOnShow(int x)
{
	if(x == 0)
	{
		setShowOnShow((LiveLayer*)0);
		return;
	}
	
	
	if(x<1 || x>m_sortedLayerList.size())
		return;
	setShowOnShow(m_sortedLayerList[x-1]);
}

void LiveLayer::setScene(LiveScene *scene)
{
	m_scene = scene;
	
	if(hideOnShowLayerId() > 0 &&
	  !hideOnShow())
	  {
	  	LiveLayer *layer = m_scene->layerFromId(hideOnShowLayerId());
	  	if(layer)
	  		setHideOnShow(layer);
	  }
		
	if(showOnShowLayerId() > 0 &&
	  !showOnShow())
	  {
	  	LiveLayer *layer = m_scene->layerFromId(showOnShowLayerId());
	  	if(layer)
			setShowOnShow(layer);
	  }
	  
	//QList<GLDrawable *> drawables = ;
	
	if(m_scene)
	{	
		foreach(GLDrawable *drawable, m_drawables.values())
		{
			connect(m_scene, SIGNAL(canvasSizeChanged(const QSizeF&)), drawable, SLOT(setCanvasSize(const QSizeF)));
			drawable->setCanvasSize(m_scene->canvasSize());
		}
		
		foreach(GLDrawable *drawable, m_secondaryDrawables.values())
		{
			connect(m_scene, SIGNAL(canvasSizeChanged(const QSizeF&)), drawable, SLOT(setCanvasSize(const QSizeF)));
			drawable->setCanvasSize(m_scene->canvasSize());
		}
	}
}

void LiveLayer::attachGLWidget(GLWidget *glw)
{
	if(!glw)
		return;
		
	//qDebug() << "LiveLayer::attachGLWidget: "<<this<<", mark1";
	m_glWidgets.append(glw);

	glw->addDrawable(drawable(glw));
	//qDebug() << "LiveLayer::attachGLWidget: "<<this<<", mark2";
	
	if(requiresSecondaryDrawable())
		glw->addDrawable(drawable(glw, true));
		
	//qDebug() << "LiveLayer::attachGLWidget: "<<this<<", mark3";
}

void LiveLayer::detachGLWidget(GLWidget *glw)
{
	if(!glw)
		return;

	glw->removeDrawable(drawable(glw));
	
	if(m_secondaryDrawables.contains(glw))
		glw->removeDrawable(drawable(glw,true));

	m_glWidgets.removeAll(glw);
}

bool LiveLayer::setAnimEnabled(bool flag)
{
	bool old = !m_animationsDisabled;
	m_animationsDisabled = !flag;
	return old;
}

void LiveLayer::setAnimParam(const LiveLayer::AnimParam &p)
{
	m_animParam = p;
}
