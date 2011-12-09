#include "GLVideoMjpegDrawable.h"

#include "../livemix/MjpegThread.h"

GLVideoMjpegDrawable::GLVideoMjpegDrawable(QString file, QObject *parent)
	: GLVideoDrawable(parent)
{
	m_thread = new MjpegThread();
	setVideoSource(m_thread);
	
	if(!file.isEmpty())
		setUrl(file);
	
}

GLVideoMjpegDrawable::~GLVideoMjpegDrawable()
{
	if(m_thread)
	{
		m_thread->exit();
		delete m_thread;
		m_thread = 0;
	}
}

bool GLVideoMjpegDrawable::setUrl(const QString& file)
{
	qDebug() << "GLVideoMjpegDrawable::setUrl(): url:"<<file;
	
	m_url = file;
	
	
	//QUrl url("http://cameras:8082");
	QUrl url(file);
	//QUrl url("http://cameras:9041");
	if(!url.isValid())
	{
		qDebug() << "GLVideoMjpegDrawable: URL:"<<url<<", Error: Sorry, the URL you entered is not a properly-formatted URL. Please try again.";
		return false;
	}
	
	if(!m_thread->connectTo(url.host(), url.port(), url.path(), url.userName(), url.password()))
	{
		qDebug() << "GLVideoMjpegDrawable: URL:"<<url<<", Connection Problem, could not connect to the URL given!";
		return false;
	}
	
	return true;
	
}
