
#include "MdiVideoChild.h"

#include "VideoWidget.h"

#include <QSettings>

MdiVideoChild::MdiVideoChild(QWidget *parent)
	: MdiChild(parent)
	, m_layout(new QVBoxLayout(this))
	, m_videoSource(0)
	, m_videoWidget(new VideoWidget())
	, m_configMenu(0)
{
	m_videoWidget->setFps(10);
	connect(m_videoWidget, SIGNAL(clicked()), this, SIGNAL(clicked()));
	setupDefaultGui();
}
	
void MdiVideoChild::setVideoSource(VideoSource* source)
{
	m_videoSource = source;
	m_videoWidget->setVideoSource(source);
}

void MdiVideoChild::closeEvent(QCloseEvent*)
{
	m_videoWidget->disconnectVideoSource();
	deleteLater();
	qDebug() << "MdiVideoChild::closeEvent(): "<<this;
}

void MdiVideoChild::showEvent(QShowEvent*)
{
	//m_videoWidget->setVideoSource(source);
}

void MdiVideoChild::setupDefaultGui()
{
	m_layout->setContentsMargins(3,3,3,3);
	m_layout->addWidget(m_videoWidget);
	
	m_configMenu = new QMenu(this);
	m_configMenu->addSeparator()->setText(tr("FPS"));
	
	QAction * action;
	QSettings settings;
	
	// FPS option
	bool flag = settings.value(QString("%1/fps").arg(metaObject()->className()),false).toInt();
	
	action = m_configMenu->addAction("Show FPS");
	action->setCheckable(true);
	action->setChecked(flag);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(setRenderFps(bool)));
	
	setRenderFps(flag);
	
	
}

void MdiVideoChild::setRenderFps(bool flag)
{
	QSettings settings;
	settings.setValue(QString("%1/fps").arg(metaObject()->className()),flag);
	
	m_videoWidget->setRenderFps(flag);
}


void MdiVideoChild::contextMenuEvent(QContextMenuEvent * event)
{
	if(m_configMenu)
		m_configMenu->popup(event->globalPos());
}
