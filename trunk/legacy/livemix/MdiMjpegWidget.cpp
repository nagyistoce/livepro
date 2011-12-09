#include "MdiMjpegWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QUrl>
#include <QMessageBox>
#include <QSettings>

#include "VideoWidget.h"

MdiMjpegWidget::MdiMjpegWidget(QWidget *parent)
	: MdiVideoChild(parent)
{
	m_thread = new MjpegThread();
	setVideoSource(m_thread);
	
	QHBoxLayout *layout = new QHBoxLayout();
	
	m_urlInput = new QLineEdit();
	connect(m_urlInput, SIGNAL(returnPressed()), this, SLOT(urlReturnPressed()));
		
	layout->addWidget(new QLabel("URL:"));
	layout->addWidget(m_urlInput);
	m_layout->addLayout(layout);
	
	setWindowTitle("MJPEG");
	
	videoWidget()->setFps(7);
	
	QSettings settings;
	QString lastUrl = settings.value("mdimjpegwidget/last-url","").toString();
	if(!lastUrl.isEmpty())
	{
		m_urlInput->setText(lastUrl);
		urlReturnPressed();
	}
}

void MdiMjpegWidget::urlReturnPressed()
{
	qDebug() << "MdiMjpeg::urlReturnPressed(): "<<m_urlInput->text(); 
	
	setWindowTitle(m_urlInput->text());
	QUrl url(m_urlInput->text());
	
	QSettings settings;
	settings.setValue("mdimjpegwidget/last-url",m_urlInput->text());
	
	if(!url.isValid())
	{
		QMessageBox::critical(this, "Invalid URL","Sorry, the URL you entered is not a properly-formatted URL. Please try again.");
		return;
	}
	
	if(!m_thread->connectTo(url.host(), url.port(), url.path(), url.userName(), url.password()))
	{
		QMessageBox::critical(this, "Connection Problem","Sorry, could not connect to the URL given!");
		return;
	}
}
