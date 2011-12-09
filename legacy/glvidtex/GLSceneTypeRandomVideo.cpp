#include "GLSceneTypeRandomVideo.h"
#include "GLVideoFileDrawable.h"

GLSceneTypeRandomVideo::GLSceneTypeRandomVideo(QObject *parent)
	: GLSceneType(parent)
	, m_currentIndex(0)
{
	m_fieldInfoList 
		<< FieldInfo("video", 
			"Video", 
			"The current video", 
			"VideoFile", 
			true)
		;
		
		
	m_paramInfoList
		<< ParameterInfo("folder",
			"Folder",
			"Folder of images to display",
			QVariant::String,
			true,
			SLOT(setFolder(const QString&)))
			
		<< ParameterInfo("updateTime",
			"Update Time",
			"Time in minutes to wait between updates",
			QVariant::Int,
			true,
			SLOT(setUpdateTime(int)))
		
		<< ParameterInfo("notRandom",
			"Not Random",
			"Show images in order, not randomly",
			QVariant::Bool,
			true,
			SLOT(setNotRandom(bool)))
			
		<< ParameterInfo("ignoreAR",
			"Ignore AR",
			"Stretch image to fill field provided",
			QVariant::Bool,
			true,
			SLOT(setIgnoreAR(bool)));
			
	PropertyEditorFactory::PropertyEditorOptions opts;
	
	opts.reset();
	opts.stringIsDir = true;
	m_paramInfoList[0].hints = opts;
	 
	opts.reset();
	opts.min = 1;
	opts.max = 15;
	m_paramInfoList[1].hints = opts;
	
	opts.reset();
	opts.type = QVariant::Bool;
	opts.text = m_paramInfoList[2].title; 
	m_paramInfoList[2].hints = opts;
	
	opts.reset();
	opts.type = QVariant::Bool;
	opts.text = m_paramInfoList[3].title; 
	m_paramInfoList[3].hints = opts;
	
	connect(&m_reloadTimer, SIGNAL(timeout()), this, SLOT(reloadData()));
	
	setParam("updateTime", 1);
	setParam("notRandom",  false);
	setParam("ignoreAR",   false);
}

void GLSceneTypeRandomVideo::setLiveStatus(bool flag)
{
	//qDebug() << "GLSceneTypeRandomVideo::setLiveStats(): "<<flag;
	GLSceneType::setLiveStatus(flag);
	
	if(flag)
	{
		m_reloadTimer.start();
		applyFieldData();
	}
	else
	{
		QTimer::singleShot( 0, this, SLOT(showNextVideo()) );
	}
}

void GLSceneTypeRandomVideo::setParam(QString param, QVariant value)
{
	//qDebug() << "GLSceneTypeRandomVideo::setParam(): "<<param<<": "<<value;
	GLSceneType::setParam(param, value);
	
	if(param == "folder")
		reloadData();
	else
	if(param == "updateTime")
		m_reloadTimer.setInterval(value.toInt() * 60 * 1000);
}

void GLSceneTypeRandomVideo::reloadData()
{
	//qDebug() << "GLSceneTypeRandomVideo::reloadData()";
	readFolder(folder());
}

void GLSceneTypeRandomVideo::readFolder(const QString &folder) 
{
	//qDebug() << "GLSceneTypeRandomVideo::readFolder()";
	QDir dir(folder);
	dir.setNameFilters(QStringList()  
			<< "*.wmv"
			<< "*.mpeg" 
			<< "*.mpg" 
			<< "*.avi" 
			<< "*.flv"
			<< "*.mov" 
			<< "*.mp4" 
			<< "*.m4a" 
			<< "*.3gp" 
			<< "*.3g2" 
			<< "*.mj2" 
			<< "*.mjpeg" 
			<< "*.ipod" 
			<< "*.m4v"
		);
	QFileInfoList list = dir.entryInfoList(QDir::Files, QDir::Name);
	
	m_videos.clear();
	
	//qDebug() << "GLSceneTypeRandomVideo::readFolder(): Found "<<list.size()<<" images in "<<folder;
	
	foreach(QFileInfo info, list)
		m_videos << info.absoluteFilePath();;
	
	if(scene() && !liveStatus())
	{
		if(m_videos.isEmpty())
	   		scene()->setDuration(0);
		else
			showNextVideo();
	}
}

void GLSceneTypeRandomVideo::showNextVideo()
{
	//qDebug() << "GLSceneTypeRandomVideo::showNextVideo()";
	if(m_videos.isEmpty())
	{
		qDebug() << "GLSceneTypeRandomVideo::showNextVideo(): No videos loaded, nothing to show.";
		if(scene())
			scene()->setDuration(0);
		return;
	}
		
	if(!notRandom())
	{
		int high = m_videos.size() - 1;
		if(high <= 0)
			high = 1;
		m_currentIndex = qrand() % (high + 1);
	}
	
	if(m_currentIndex < 0 || m_currentIndex >= m_videos.size())
		m_currentIndex = 0;
	
	QString video = m_videos[m_currentIndex];
	
	setField("video", 	video);
	
	GLVideoFileDrawable *vidfile = dynamic_cast<GLVideoFileDrawable*>(lookupField("video"));
	
	if(vidfile && scene());
	{
		double d = vidfile->videoLength();
		if(d > 1)
		{
			//qDebug() << "GLSceneTypeRandomVideo::showNextVideo(): d:"<<d;
			scene()->setDuration(d);
		}
		else
		{
			//qDebug() << "GLSceneTypeRandomVideo::showNextVideo(): d:"<<d<<", overriding to 30.0";
			scene()->setDuration(30.0);
		}
	}
	
	m_currentIndex++;
}

void GLSceneTypeRandomVideo::durationChanged(double d)
{
	if(scene())
	{
		//qDebug() << "GLSceneTypeRandomVideo::durationChanged: "<<d;
		scene()->setDuration(d);
	}
}

void GLSceneTypeRandomVideo::statusChanged(int status)
{
	if(!status && scene())
	{
		//qDebug() << "GLSceneTypeRandomVideo::statusChanged: "<<status<<", setting duration to zero";
		scene()->setDuration(0);
	}

}

void GLSceneTypeRandomVideo::sceneAttached(GLScene *)
{
	if(m_videos.isEmpty() && !folder().isEmpty())
		reloadData();
		
	GLVideoFileDrawable *vidfile = dynamic_cast<GLVideoFileDrawable*>(lookupField("video"));
	if(vidfile)
	{
		connect(vidfile, SIGNAL(durationChanged(double)), this, SLOT(durationChanged(double)));
		connect(vidfile, SIGNAL(statusChanged(int)), this, SLOT(statusChanged(int)));
	}
	
	//qDebug() << "GLSceneTypeRandomVideo::sceneAttached()";
	showNextVideo();
}

