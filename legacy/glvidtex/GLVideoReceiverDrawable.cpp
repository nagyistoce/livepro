#include "GLVideoReceiverDrawable.h"

#include "VideoReceiver.h"

GLVideoReceiverDrawable::GLVideoReceiverDrawable(QString host, int port, QObject *parent)
	: GLVideoDrawable(parent)
	, m_host("localhost")
	, m_port(7755)
	, m_rx(0)
{
	setHost(host);
	setPort(port);
	
	
}
	
void GLVideoReceiverDrawable::setHost(const QString& host)
{
	if(m_rx)
		m_rx->release(this);
	
	m_rx = VideoReceiver::getReceiver(host,m_port);
	m_rx->registerConsumer(this);
	setVideoSource(m_rx);
	
	m_host = host;
}

void GLVideoReceiverDrawable::setPort(int port)
{
	if(m_rx)
		m_rx->release(this);
	
	m_rx = VideoReceiver::getReceiver(m_host,port);
	m_rx->registerConsumer(this);
	setVideoSource(m_rx);
	
	m_port = port;
}

