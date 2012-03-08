#include <QtGui>
#include "ControlWindow.h"
#include ".build/ui_ControlWindow.h"

#include "OutputWindow.h"

ControlWindow::ControlWindow(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::ControlWindow)
{
	ui->setupUi(this);
	setupUi();
}

ControlWindow::~ControlWindow()
{
	delete ui;
}

void ControlWindow::setupUi()
{
	setWindowTitle("StageDisplay Control");
	
	connect(ui->sourceBox, SIGNAL(textChanged(QString)), this, SLOT(setVideoSource(QString)));
	connect(ui->connectBtn, SIGNAL(clicked()), this, SLOT(connectSource()));
	
	connect(ui->outX, SIGNAL(valueChanged(int)), this, SLOT(setOutputX(int)));
	connect(ui->outY, SIGNAL(valueChanged(int)), this, SLOT(setOutputY(int)));
	connect(ui->outW, SIGNAL(valueChanged(int)), this, SLOT(setOutputW(int)));
	connect(ui->outH, SIGNAL(valueChanged(int)), this, SLOT(setOutputH(int)));
	
	connect(ui->timerEnabled, SIGNAL(toggled(bool)), 		this, SLOT(setTimerEnabled(bool)));
	connect(ui->currentTime, SIGNAL(valueChanged(int)), 		this, SLOT(setCurrentTime(int)));
	connect(ui->timerBtn, SIGNAL(clicked()), 			this, SLOT(timerBtn()));
	connect(ui->resetBtn, SIGNAL(clicked()), 			this, SLOT(resetTimerBtn()));
	connect(ui->timerFontSize, SIGNAL(valueChanged(int)), 		this, SLOT(timerFontSizeChanged(int)));
	connect(ui->timerDrawBg, SIGNAL(toggled(bool)), 		this, SLOT(timerDrawBgChanged(bool)));
	connect(ui->timerPosition, SIGNAL(currentIndexChanged(int)),	this, SLOT(timerPositionChanged(int)));
	
	connect(ui->message, SIGNAL(textChanged(QString)),		this, SLOT(messageChanged(QString)));
	connect(ui->showMsgBtn, SIGNAL(clicked()), 			this, SLOT(showMsgBtn()));
	connect(ui->hideMsgBtn, SIGNAL(clicked()), 			this, SLOT(hideMsgBtn()));
	connect(ui->flashMsgBtn, SIGNAL(toggled(bool)), 		this, SLOT(flashMsgBtnToggled(bool)));
	connect(ui->flashSpeed, SIGNAL(valueChanged(int)), 		this, SLOT(flashSpeedChanged(int)));
	connect(ui->msgFontSize, SIGNAL(valueChanged(int)), 		this, SLOT(msgFontSizeChanged(int)));
	connect(ui->msgDrawBg, SIGNAL(toggled(bool)), 			this, SLOT(msgDrawBgChanged(bool)));
	connect(ui->msgPosition, SIGNAL(currentIndexChanged(int)), 	this, SLOT(msgPositionChanged(int)));
}

void ControlWindow::setOutputWindow(OutputWindow *win)
{
	m_ow = win;
	connect(win, SIGNAL(counterValueChanged(int)), this, SLOT(currentTimeChanged(int)));
	
	ui->sourceBox->setText(m_ow->inputSource());
	
	QRect geom = m_ow->windowGeom();
	ui->outX->setValue(geom.x());
	ui->outY->setValue(geom.y());
	ui->outW->setValue(geom.width());
	ui->outH->setValue(geom.height());
	
	ui->timerEnabled->setChecked(m_ow->isCounterVisible());
	ui->currentTime->setValue(m_ow->counterValue());
	
	// Not implemented yet
	//ui->timerFontSize->setValue(m_ow->counterFontSize());
	//ui->timerDrawBg->setChecked(m_ow->drawCounterBg());
	//ui->timerPosition->setValue(m_ow->timerPosition());
	
	ui->message->setText(m_ow->overlayText());
	ui->flashSpeed->setValue(m_ow->blinkSpeed());
	
	// Not implemented yet
	//ui->msgFontSize->setValue(m_ow->overlayFontSize());
	//ui->msgDrawBg->setChecked(m_ow->drawOverlayBg());
	//ui->msgPosition->setValue(m_ow-o>verlayPosition());
}

void ControlWindow::setVideoSource(QString s)
{
	m_source = s;
}

void ControlWindow::connectSource()
{
	m_ow->setInputSource(m_source);
}

void ControlWindow::setOutputX(int z)
{
	QRect geom = m_ow->windowGeom();
	geom = QRect(z, geom.y(), geom.width(), geom.height());
	m_ow->setWindowGeom(geom);
}

void ControlWindow::setOutputY(int z)
{
	QRect geom = m_ow->windowGeom();
	geom = QRect(geom.x(), z, geom.width(), geom.height());
	m_ow->setWindowGeom(geom);
}

void ControlWindow::setOutputW(int z)
{
	QRect geom = m_ow->windowGeom();
	geom = QRect(geom.x(), geom.y(), z, geom.height());
	m_ow->setWindowGeom(geom);
}

void ControlWindow::setOutputH(int z)
{
	QRect geom = m_ow->windowGeom();
	geom = QRect(geom.x(), geom.y(), geom.width(), z);
	m_ow->setWindowGeom(geom);
}


void ControlWindow::setTimerEnabled(bool flag)
{
	m_ow->setCounterVisible(flag);
}

void ControlWindow::setCurrentTime(int time)
{
	m_ow->setCounterValue(time * 60);
}

void ControlWindow::currentTimeChanged(int time)
{
	if(ui->currentTime->value() != (int)(time/60))
		ui->currentTime->setValue(time/60);
}

void ControlWindow::timerBtn()
{
	m_ow->setCounterActive(!m_ow->isCounterActive());
	ui->timerBtn->setText(m_ow->isCounterActive() ? "Stop Timer" : "Start Timer");
}

void ControlWindow::resetTimerBtn()
{
	setCurrentTime(0);
}

void ControlWindow::timerFontSizeChanged(int)
{
}

void ControlWindow::timerDrawBgChanged(bool)
{
}

void ControlWindow::timerPositionChanged(int)
{
}

void ControlWindow::messageChanged(QString txt)
{
	m_ow->setOverlayText(txt);
}

void ControlWindow::showMsgBtn()
{
	m_ow->setOverlayVisible(true);
}

void ControlWindow::hideMsgBtn()
{
	m_ow->setOverlayVisible(false);
}

void ControlWindow::flashMsgBtnToggled(bool flag)
{
	m_ow->setBlinkOverlay(flag, ui->flashSpeed->value());
}

void ControlWindow::flashSpeedChanged(int speed)
{
	m_ow->setBlinkOverlay(ui->flashMsgBtn->isChecked(), speed);
}

void ControlWindow::msgFontSizeChanged(int)
{
}

void ControlWindow::msgDrawBgChanged(bool)
{
}

void ControlWindow::msgPositionChanged(int)
{
}


void ControlWindow::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
	switch (e->type())
	{
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void ControlWindow::closeEvent(QCloseEvent *)
{
	m_ow->close();
}
