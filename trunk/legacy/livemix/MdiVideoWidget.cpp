
#include "MdiVideoWidget.h"

#include <QCompleter>
#include <QDirModel>
#include <QFileDialog>
#include <QPushButton>

#include "VideoThread.h"
#include "VideoWidget.h"

#include <QCDEStyle>

MdiVideoWidget::MdiVideoWidget(QWidget *parent)
	: MdiVideoChild(parent)
{
	// MdiVideoChild adds the default VideoWidget and titler box, 
	// so here we just need to add our video browser box
	
	QHBoxLayout *layout = new QHBoxLayout();
	
	// Crate the line edit
	m_fileInput = new QLineEdit(this);
	connect(m_fileInput, SIGNAL(returnPressed()), this, SLOT(returnPressed()));
		
	layout->addWidget(m_fileInput);
	
	// Add the filename completor to the line edit
	QCompleter *completer = new QCompleter(m_fileInput);
	QDirModel *dirModel = new QDirModel(completer);
	completer->setModel(dirModel);
	completer->setCompletionMode(QCompleter::PopupCompletion);
	completer->setCaseSensitivity(Qt::CaseInsensitive);
	completer->setWrapAround(true);
	m_fileInput->setCompleter(completer);
	
	// Create the 'browse' button at the end of the line edit
	QPushButton *btn = new QPushButton("...",this);
	connect(btn, SIGNAL(clicked()), this, SLOT(browseButtonClicked()));
	layout->addWidget(btn);
	
	// Use CDE style to minimize the space used by the button
	// (Could use a custom stylesheet I suppose - later.)
	btn->setStyle(new QCDEStyle());
	//btn->setStyleSheet("border:1px solid black; padding:0; width: 1em; margin:0");
	
	// Add it to the default layout
	m_layout->addLayout(layout);
	
	// Retain the default width if possible
	resize(200,sizeHint().height());
	
	setWindowTitle("No Video");
	
	// For debugging/development, load a default video known to be 
	// in ../data and included subversion (as of r571 anyway)
	setVideoFile("../data/Seasons_Loop_3_SD.mpg");
}
	
void MdiVideoWidget::setVideoFile(const QString& file)
{
	if(m_videoFile == file)
		return;
		
	if(m_fileInput->text() != file)
		m_fileInput->setText(file);
	
	if(m_videoSource)
	{
		m_videoSource->quit();
		m_videoSource->deleteLater();
	}
	
	VideoThread *videoSource = new VideoThread();
	videoSource->setVideo(file);
	videoSource->start();
	//videoSource->pause();
	
	setVideoSource(videoSource);
	
	setWindowTitle(QFileInfo(file).baseName());
}

void MdiVideoWidget::browseButtonClicked()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Select Video"), m_fileInput->text(), tr("Video Files (*.wmv *.mpeg *.mpg *.avi *.wmv *.flv *.mov *.mp4 *.m4a *.3gp *.3g2 *.mj2 *.mjpeg *.ipod *.m4v *.gsm *.swf *.dv *.dvd *.asf *.mtv *.roq *.aac *.ac3 *.aiff *.alaw *.iif);;Any File (*.*)"));
	if(fileName != "")
	{
		setVideoFile(fileName);
// 		QString abs = fileName.startsWith("http://") ? fileName : QFileInfo(fileName).absolutePath();
// 		m_commonUi->videoFilenameBox->setText(abs);
// 		AppSettings::setPreviousPath("videos",abs);
	}
}

void MdiVideoWidget::returnPressed()
{
	setVideoFile(m_fileInput->text());
}
