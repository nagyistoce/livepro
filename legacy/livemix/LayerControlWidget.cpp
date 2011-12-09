#include <QtGui>

#include "LayerControlWidget.h"
#include "LiveLayer.h"

LayerControlWidget::LayerControlWidget(LiveLayer *layer)
        : QFrame()
        , m_layer(layer)
{
	setupUI();
}

LayerControlWidget::~LayerControlWidget()
{}

void LayerControlWidget::mouseReleaseEvent(QMouseEvent*)
{
	emit clicked();
}

void LayerControlWidget::setIsCurrentWidget(bool flag)
{
	m_isCurrentWidget = flag;
	m_editButton->setStyleSheet(flag ? "background: yellow" : "");
	m_editButton->setChecked(flag);
}

void LayerControlWidget::drawableVisibilityChanged(bool flag)
{
	m_liveButton->setStyleSheet(flag ? "background: red; color: white; font-weight: bold" : "");
	m_liveButton->setChecked(flag);
}

void LayerControlWidget::setupUI()
{
	setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
	setLineWidth(2);

	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->setContentsMargins(2,2,2,2);

	m_editButton = new QPushButton(QPixmap("../data/stock-edit.png"),"");
	m_editButton->setCheckable(true);
	connect(m_editButton, SIGNAL(clicked()), this, SIGNAL(clicked()));
	layout->addWidget(m_editButton);


	m_nameLabel = new QLabel;
	layout->addWidget(m_nameLabel);
	connect(m_layer, SIGNAL(instanceNameChanged(const QString&)), this, SLOT(instanceNameChanged(const QString&)));
	instanceNameChanged(m_layer->instanceName());
	layout->addStretch(1);

	m_opacitySlider = new QSlider(this);
	m_opacitySlider->setOrientation(Qt::Horizontal);
	m_opacitySlider->setMinimum(0);
	m_opacitySlider->setMaximum(100);
	m_opacitySlider->setValue(100);
	connect(m_opacitySlider, SIGNAL(valueChanged(int)), this, SLOT(opacitySliderChanged(int)));
	layout->addWidget(m_opacitySlider);

	QSpinBox *box = new QSpinBox(this);
	box->setMinimum(0);
	box->setMaximum(100);
	box->setValue(100);
	box->setSuffix("%");
	connect(m_opacitySlider, SIGNAL(valueChanged(int)), box, SLOT(setValue(int)));
	connect(box, SIGNAL(valueChanged(int)), m_opacitySlider, SLOT(setValue(int)));
	layout->addWidget(box);

	//layout->addStretch(1);

	m_liveButton = new QPushButton("Live");
	m_liveButton->setCheckable(true);
	connect(m_liveButton, SIGNAL(toggled(bool)), m_layer, SLOT(setVisible(bool)));
	connect(m_layer, SIGNAL(isVisible(bool)), this, SLOT(drawableVisibilityChanged(bool)));
	if(m_layer->isVisible())
		drawableVisibilityChanged(true);
	layout->addWidget(m_liveButton);

}

void LayerControlWidget::instanceNameChanged(const QString& name)
{
	m_nameLabel->setText(QString("<b>%1</b><br>%2").arg(name).arg(m_layer->typeName()));
}

void LayerControlWidget::opacitySliderChanged(int value)
{
	m_layer->setOpacity((double)value/100.0);
}

