#include "MdiCameraWidget.h"

#include <QMessageBox>
#include <QSettings>
#include <QPushButton>
#include <QMenu>
#include <QCDEStyle>

#include "VideoWidget.h"

MdiCameraWidget::MdiCameraWidget(QWidget *parent)
	: MdiVideoChild(parent)
{
	QHBoxLayout *hbox = new QHBoxLayout();
	m_layout->addLayout(hbox);
	
	m_deviceBox = new QComboBox(this);
	
	hbox->addWidget(m_deviceBox);

	m_cameras = CameraThread::enumerateDevices();
	if(!m_cameras.size())
	{
		QMessageBox::critical(this,tr("No Cameras Found"),tr("Sorry, but no camera devices were found attached to this computer."));
		setWindowTitle("No Camera");
		return;
	}

	QStringList items;
	int counter = 1;
	foreach(QString dev, m_cameras)
		items << QString("Camera # %1").arg(counter++);
	
	m_deviceBox->addItems(items);
	
	connect(m_deviceBox, SIGNAL(currentIndexChanged(int)), this, SLOT(deviceBoxChanged(int)));
	
	// load last deinterlace setting
	QSettings settings;
	m_deinterlace = (bool)settings.value("mdicamerawidget/deinterlace",false).toInt();
	
	// setup the popup config menu
	QPushButton *configBtn = new QPushButton(QPixmap("../data/stock-preferences.png"),"");
	configBtn->setToolTip("Options");
	
	// Use CDE style to minimize the space used by the button
	// (Could use a custom stylesheet I suppose - later.)
	configBtn->setStyle(new QCDEStyle());
	//configBtn->setStyleSheet("border:1px solid black; padding:0; width: 1em; margin:0");
	
	QAction * action;
	
	// Deinterlace option
	action = m_configMenu->addAction("Deinterlace Video");
	action->setCheckable(true);
	action->setChecked(deinterlace());
	connect(action, SIGNAL(toggled(bool)), this, SLOT(setDeinterlace(bool)));
	
	configBtn->setMenu(m_configMenu);
	hbox->addWidget(configBtn);
	
	//videoWidget()->setFps(30);
	
	// Start the camera
	deviceBoxChanged(0);
}

void MdiCameraWidget::deviceBoxChanged(int idx)
{
	if(idx < 0 || idx >= m_cameras.size())
		return;
		
	QString camera = m_cameras[idx];
	
	m_thread = CameraThread::threadForCamera(camera);
	
	if(!m_thread)
	{
		QMessageBox::critical(this,"Missing Camera",QString("Sorry, cannot connect to %1!").arg(camera));
		setWindowTitle("Camera Error");
	}
	
	// Enable raw VL42 access on linux - note that VideoWidget must be able to handle raw frames
	//m_thread->enableRawFrames(true);
	
	// Default clip rect to compensate for oft-seen video capture 'bugs' (esp. on hauppauge/bttv)
	m_videoWidget->setSourceRectAdjust(11,0,-6,-3);
	
	m_thread->setDeinterlace(m_deinterlace);
	
	setWindowTitle(camera);
	
	setVideoSource(m_thread);	
}

void MdiCameraWidget::setDeinterlace(bool flag)
{
	m_deinterlace = flag;
	
	QSettings settings;
	settings.setValue("mdicamerawidget/deinterlace",(int)m_deinterlace);
	
	if(m_thread)
		m_thread->setDeinterlace(m_deinterlace);
}

