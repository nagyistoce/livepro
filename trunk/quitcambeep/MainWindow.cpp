#include "MainWindow.h"

#include "VideoWidget.h"
#include "VideoReceiver.h"
#include "PointTrackingFilter.h"

MainWindow::MainWindow()
	: QWidget()
	, m_isWatching(false)
	, m_beepCounter(0)
	, m_labelChanged(false)
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
	
	setWindowTitle("Quiet Camera Monitor");
	
	m_videoWidget = new VideoWidget();
	vbox->addWidget(m_videoWidget);
	
	m_trackingFilter = new PointTrackingFilter();
	m_videoWidget->setVideoSource(m_trackingFilter);
	
	//connect(m_trackingFilter, SIGNAL(historyAvgZero()), this, SLOT(historyAvgZero()));
	connect(m_trackingFilter, SIGNAL(historyAvg(int)), this, SLOT(historyAvg(int)));
	
	
	QHBoxLayout *bottomBox = new QHBoxLayout();
	
	m_watchButton = new QPushButton("Monitor for Quiet");
	connect(m_watchButton, SIGNAL(clicked()), this, SLOT(watchButtonClicked()));
	bottomBox->addWidget(m_watchButton);
	
	m_watchLabel = new QLabel("Not Monitoring");
	bottomBox->addWidget(m_watchLabel);
	
	vbox->addLayout(bottomBox);
	
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
		
	VideoReceiver *rx = VideoReceiver::getReceiver(host,port);
	m_trackingFilter->setVideoSource(rx);
	//m_videoWidget->setVideoSource(rx);
	qDebug() << "Connected to "<<host<<port<<", rx:"<<rx;
}

void MainWindow::textChanged(QString)
{
	m_connectBtn->setEnabled(true);
	m_connectBtn->setText("Connect");
}

void MainWindow::watchButtonClicked()
{
	m_isWatching = !m_isWatching;
	m_watchLabel->setText(m_isWatching ? "<font color=green>Monitoring ...</font>" : "Not Monitoring");
	m_watchButton->setText(m_isWatching ? "Stop Monitoring" : "Monitor for Quiet");
}

void MainWindow::historyAvgZero()
{
	if(m_isWatching)
	{
		if(m_beepCounter -- < 0)
		{
			m_beepCounter = 10;
			system("aplay alert.wav &");
			qDebug() << QTime::currentTime()<<"Beep";
			
			m_watchLabel->setText(m_isWatching ? "<font color=red><b>BEEP BEEP</b></font>" : "Not Monitoring");
			m_labelChanged = true;
		}
	}
}

void MainWindow::historyAvg(int zero)
{
	//qDebug() << "History: "<<zero;
	if(zero <= 5)
	{
		qDebug() << "History: "<<zero;
		historyAvgZero();
	}
	else
	if(m_labelChanged)
	{
		m_watchLabel->setText(m_isWatching ? "<font color=green>Monitoring ...</font>" : "Not Monitoring");
		m_labelChanged = false;
	}
}

