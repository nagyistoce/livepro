#include <QFileInfo>
#include "LiveVideoLoopLayer.h"
#include "ExpandableWidget.h"
#include "VideoThread.h"
#include "../glvidtex/GLWidget.h"
#include "../glvidtex/GLVideoDrawable.h"

LiveVideoLoopLayer::LiveVideoLoopLayer(QObject *parent)
	: LiveVideoLayer(parent)
	, m_video(0)
{
}

LiveVideoLoopLayer::~LiveVideoLoopLayer()
{
	setVideoSource(0); // doesnt change m_video
	if(m_video)
	{
		delete m_video;
		m_video = 0;
	}
}

GLDrawable *LiveVideoLoopLayer::createDrawable(GLWidget *widget)
{
	// We overrride createDrawable here just for future expansiosn sake
	return LiveVideoLayer::createDrawable(widget);
}

void LiveVideoLoopLayer::initDrawable(GLDrawable *drawable, bool isFirst)
{
	//qDebug() << "LiveVideoLoopLayer::setupDrawable: drawable:"<<drawable<<", copyFrom:"<<copyFrom;
	LiveVideoLayer::initDrawable(drawable, isFirst);

	if(isFirst)
	{
		// nothing to do 
		// Maybe in future set a static video source with a blue image?
	}
	
	if(m_video)
	{
		qDebug() << "LiveVideoLoopLayer::initDrawable: setting video:"<<m_video;
		setVideo(m_video);
	}
	else
	{
		qDebug() << "LiveVideoLoopLayer::initDrawable: m_video is null";
	}
}

void LiveVideoLoopLayer::setVideo(VideoThread *vid)
{
	qDebug() << "LiveVideoLoopLayer::setVideo: "<<vid;
	
// 	if(vid == m_video)
// 	{
// 		qDebug() << "LiveVideoLoopLayer::setVideo: Not setting, vid == m_video";
// 		return;
// 	}
		
	setVideoSource(vid);
	m_video = vid;
	setInstanceName(guessTitle(QFileInfo(vid->videoFile()).baseName()));
}

QWidget * LiveVideoLoopLayer::createLayerPropertyEditors()
{
	QWidget * base = new QWidget();
	QVBoxLayout *blay = new QVBoxLayout(base);
	blay->setContentsMargins(0,0,0,0);
	
	ExpandableWidget *groupContent = new ExpandableWidget("Video File",base);
	blay->addWidget(groupContent);
	
	QWidget *groupContentContainer = new QWidget;
	QFormLayout *formLayout = new QFormLayout(groupContentContainer);
	formLayout->setContentsMargins(3,3,3,3);
	
	groupContent->setWidget(groupContentContainer);
	
	PropertyEditorOptions opts;
	opts.stringIsFile = true;
	opts.fileTypeFilter = tr("Video Files (*.wmv *.mpeg *.mpg *.avi *.wmv *.flv *.mov *.mp4 *.m4a *.3gp *.3g2 *.mj2 *.mjpeg *.ipod *.m4v *.gsm *.swf *.dv *.dvd *.asf *.mtv *.roq *.aac *.ac3 *.aiff *.alaw *.iif);;Any File (*.*)");
	
	formLayout->addRow(tr("&File:"), generatePropertyEditor(this, "file", SLOT(setFile(const QString&)), opts));
	
 	
 	groupContent->setExpandedIfNoDefault(true);
	
	/////////////////////////////////////////
	
	QWidget *basics =  LiveVideoLayer::createLayerPropertyEditors();
	blay->addWidget(basics);
	
	return base;
}

void LiveVideoLoopLayer::setFile(const QString& file)
{
	if(m_video && m_video->videoFile() == file)
		return;
		 
	QFileInfo info(file);
	
	if(!info.exists())
	{
		qDebug() << "LiveVideoLoopLayer::setFile: File does not exist: "<<file;
		return;
	}
	
	if(m_video)
	{
		m_video->stop();
		m_video->quit();
		m_video->deleteLater();
		m_video = 0;
	}
	
	qDebug() << "LiveVideoLoopLayer::setFile: Loading file: "<<file;
	VideoThread *video = new VideoThread();
	video->setVideo(file);
	video->start();
	setVideo(video);
	
	m_props["file"] = file;
	
	setInstanceName(guessTitle(info.fileName()));
}

void LiveVideoLoopLayer::setLayerProperty(const QString& key, const QVariant& value)
{
	if(key == "file")
	{
		setFile(value.toString());
	}
	else
	if(m_video)
	{
		const char *keyStr = qPrintable(key);
		if(m_video->metaObject()->indexOfProperty(keyStr)>=0)
		{
			m_video->setProperty(keyStr, value);
		}
		
	}
	
	LiveLayer::setLayerProperty(key,value);
}
