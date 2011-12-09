#include "GLVideoLoopDrawable.h"


#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

#include "../livemix/VideoThread.h"
#include "GLEditorGraphicsScene.h"

GLVideoLoopDrawable::GLVideoLoopDrawable(QString file, QObject *parent)
	: GLVideoDrawable(parent)
	, m_videoLength(-1)
{
	if(!file.isEmpty())
		setVideoFile(file);
	
	connect(this, SIGNAL(sourceDiscarded(VideoSource*)), this, SLOT(deleteSource(VideoSource*)));
	
	//QTimer::singleShot(1500, this, SLOT(testXfade()));
}
	
void GLVideoLoopDrawable::testXfade()
{
	qDebug() << "GLVideoLoopDrawable::testXfade(): loading file #2";
	setVideoFile("/data/appcluster/dviz-old/dviz-r62-b2/src/data/Seasons_Loop_2_SD.mpg");
}
	
bool GLVideoLoopDrawable::setVideoFile(const QString& file)
{
	qDebug() << "GLVideoLoopDrawable::setVideoFile(): file:"<<file;
	
	QFileInfo fileInfo(file);
	if(!fileInfo.exists())
	{
		qDebug() << "GLVideoLoopDrawable::setVideoFile: "<<file<<" does not exist!";
		return false;
	}
	
	if(m_videoFile == file)
		return true;
	
	m_videoFile = file;
	
	m_videoThread = new VideoThread();
	m_videoThread->setVideo(file);
	
	// Assuming duration in seconds
	m_videoLength = m_videoThread->duration(); // / 1000.;
		
	//source->setVideo("../samples/BlueFish/EssentialsVol05_Abstract_Media/HD/Countdowns/Abstract_Countdown_3_HD.mp4");
	//source->setVideo("../samples/BlueFish/EssentialsVol05_Abstract_Media/SD/Countdowns/Abstract_Countdown_3_SD.mpg");
	
	m_videoThread->start(true); // true = start paused
	
	setVideoSource(m_videoThread);
	setObjectName(qPrintable(file));
	
	emit videoFileChanged(file);
	
	//qDebug() << "GLVideoLoopDrawable::setVideoFile: Created video thread:"<<source;
	
	return true;
	
}

void GLVideoLoopDrawable::deleteSource(VideoSource *source)
{
	VideoThread *vt = dynamic_cast<VideoThread*>(source);
	if(vt)
	{
		qDebug() << "GLVideoLoopDrawable::deleteSource: Deleting video thread:" <<vt;
		delete vt;
		vt = 0;
		source = 0;
	}
	else
	{
		qDebug() << "GLVideoLoopDrawable::deleteSource: Source not deleted because its not a 'VideoThread':" <<source;
	}
}


void GLVideoLoopDrawable::setLiveStatus(bool flag)
{
	GLVideoDrawable::setLiveStatus(flag);

	qDebug() << "GLVideoLoopDrawable::setLiveStatus: new status:"<<flag;
	
	if(flag)
	{
		GLEditorGraphicsScene *scenePtr = dynamic_cast<GLEditorGraphicsScene*>(scene());
		if(!scenePtr)
		{
			if(m_videoThread)
			{
				m_videoThread->play();
			}
			else
			{
				qDebug() << "GLVideoFileDrawable::setLiveStatus: Can't fade in volume - no video source.";
			}
		}
		else
		{
			qDebug() << "GLVideoFileDrawable::setLiveStatus: Not going live due to the fact we're in the editor scene. Also, limiting frame rate to 10fps for CPU usage's sake.";
			setFpsLimit(10.);
		}
	}
	else
	{
		if(m_videoThread)
		{
			QTimer::singleShot(xfadeLength(), m_videoThread, SLOT(pause())); 
		}
		else
		{
			qDebug() << "GLVideoFileDrawable::setLiveStatus: Can't fade out volume - no video source.";
		}
	}
}
