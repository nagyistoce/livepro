#include "CameraServer.h"

#include <QNetworkInterface>

CameraServer::CameraServer(QObject *parent)
	: QTcpServer(parent)
	, m_imageProvider(0)
	, m_signalName(0)
	, m_adaptiveWriteEnabled(true)
{
}
	
void CameraServer::setProvider(QObject *provider, const char * signalName)
{
	m_imageProvider = provider;
	m_signalName    = signalName;
}

void CameraServer::incomingConnection(int socketDescriptor)
{
	CameraServerThread *thread = new CameraServerThread(socketDescriptor, m_adaptiveWriteEnabled);
	connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
	connect(m_imageProvider, m_signalName, thread, SLOT(imageReady(QImage)), Qt::QueuedConnection);
	thread->start();
	qDebug() << "CameraServer: Client Connected, Socket Descriptor:"<<socketDescriptor;
}

/** Thread **/
#define BOUNDARY "CameraServerThread-uuid-0eda9b03a8314df4840c97113e3fe160"
#include <QImageWriter>
#include <QImage>

CameraServerThread::CameraServerThread(int socketDescriptor, bool adaptiveWriteEnabled, QObject *parent)
    : QThread(parent)
    , m_socketDescriptor(socketDescriptor)
    , m_adaptiveWriteEnabled(adaptiveWriteEnabled)
{
	
}

CameraServerThread::~CameraServerThread()
{
	m_socket->abort();
	delete m_socket;
}

void CameraServerThread::run()
{
	m_socket = new QTcpSocket();
	
	if (!m_socket->setSocketDescriptor(m_socketDescriptor)) 
	{
		emit error(m_socket->error());
		return;
	}
	
	connect(m_socket, SIGNAL(disconnected()), 	this, SLOT(quit()));
	
	//m_stream = new QDataStream(&m_buffer, QIODevice::WriteOnly);
	m_buffer.setBuffer(&m_array);
	m_buffer.open(QIODevice::WriteOnly);

	
	m_writer.setDevice(&m_buffer);
	m_writer.setFormat("jpg");
	//m_writer.setQuality(80);
	
	// enter event loop
	exec();
	
	// when imageReady() signal arrives, write data with header to socket
}

void CameraServerThread::imageReady(QImage image)
{
	static int frameCounter=0;
	frameCounter ++;
	if(m_adaptiveWriteEnabled && m_socket->bytesToWrite() > 100)
	{
		qDebug() << "CameraServerThread::imageReady():"<<m_socket->bytesToWrite()<<"bytes pending write on socket, not sending image"<<frameCounter;
	}
	else
	{
		if(image.format() != QImage::Format_RGB32)
			image = image.convertToFormat(QImage::Format_RGB32);
		
		if(m_socket->state() == QAbstractSocket::ConnectedState)
		{
			//m_socket->write("Content-type: image/jpeg\r\n\r\n");
		}
		
		if(!m_writer.write(image))
		{
			qDebug() << "ImageWriter reported error:"<<m_writer.errorString();
			quit();
		}
		
		char data[256];
		sprintf(data, "%d\n", m_array.size());
		m_socket->write((const char*)&data,strlen((const char*)data));
		//qDebug() << "block size: "<<strlen((const char*)data)<<", data:"<<data;
		qDebug() << "frame size:"<<m_array.size();
	
		m_socket->write(m_array);
		m_array.clear();
		m_buffer.seek(0);
	}

}

