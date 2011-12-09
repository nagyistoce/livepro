#include "VideoInputSenderManager.h"
#include "../livemix/CameraThread.h"
#include "VideoSender.h"
#include <QNetworkInterface>

int VideoInputSenderManager::m_videoSenderPortAllocator = 7755;

VideoInputSenderManager::VideoInputSenderManager(QObject *parent)
	: QObject(parent)
{
	QStringList devs = CameraThread::enumerateDevices();
	foreach(QString dev, devs)
	{
		VideoSender *sender = new VideoSender();
		CameraThread *source = CameraThread::threadForCamera(dev);
		source->setFps(30);
		source->registerConsumer(sender);
		#ifndef Q_OS_WINDOWS
		source->enableRawFrames(true);
		#endif
		sender->setVideoSource(source);
		
		m_videoSenders[dev] = sender;
	}
}

VideoInputSenderManager::~VideoInputSenderManager()
{
	foreach(QString dev, m_videoSenders.keys())
	{
		VideoSender *sender = m_videoSenders[dev];
		CameraThread *camera = CameraThread::threadForCamera(dev);
		camera->release(sender);
		
		sender->close();
		m_videoSenders.remove(dev);
		sender->deleteLater();
	}
}

void VideoInputSenderManager::setSendingEnabled(bool flag)
{
	m_sendingEnabled = flag;
	
	//return;

	if(m_sendingEnabled)
	{
		foreach(QString dev, m_videoSenders.keys())
		{
			VideoSender *sender = m_videoSenders[dev];
			bool done = false;
			int port = -1;
			while(!done)
			{
				port = m_videoSenderPortAllocator ++;
				if(sender->listen(QHostAddress::Any,port))
				{
					done = true;
				}
			}
			
			qDebug() << "VideoInputSenderManager::setSendingEnabled: Started video sender for "<<dev<<" on port "<<port;
		}
	}
	else
	{
		foreach(QString dev, m_videoSenders.keys())
		{
			VideoSender *sender = m_videoSenders[dev];
			sender->close();
			
			qDebug() << "VideoInputSenderManager::setSendingEnabled: Stopped transmitter for "<<dev;
		}
	}
}

QStringList VideoInputSenderManager::videoConnections(bool justNetString)
{
	if(!m_sendingEnabled)
	{
		qDebug() << "VideoInputSenderManager::videoConnections: Sending not enabled, returning empty list.";
		return QStringList();
	}
	
	QString ipAddress;

	QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
	// use the first non-localhost IPv4 address
	for (int i = 0; i < ipAddressesList.size(); ++i)
	{
		if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
		    ipAddressesList.at(i).toIPv4Address())
		{
			QString tmp = ipAddressesList.at(i).toString();
			if(!tmp.startsWith("192.168.122."))
				ipAddress = tmp;
		}
	}

	// if we did not find one, use IPv4 localhost
	if (ipAddress.isEmpty())
		ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
	
	QStringList list;
	
	foreach(QString dev, m_videoSenders.keys())
	{
		VideoSender *sender = m_videoSenders[dev];
// 		qDebug() << "VideoInputSenderManager::setSendingEnabled: Stopped transmitter for "<<dev<<";

		CameraThread *camera = CameraThread::threadForCamera(dev);
		
		QString con;
		if(justNetString)
		{
			con = QString("%1:%2")
				.arg(ipAddress)
				.arg(sender->serverPort());
		}
		else
		{
			QStringList inputs = camera->inputs();
			QString input = inputs.isEmpty() ? "" : inputs.at(camera->input());
			
			con = QString("dev=%1,input=%2,net=%3:%4")
				.arg(dev)
				.arg(input)
				.arg(ipAddress)
				.arg(sender->serverPort());
		}
		
		qDebug() << "VideoInputSenderManager::videoConnections: "<<dev<<" Added con: "<<con;
		
		list << con;
	}
	
	qDebug() << "VideoInputSenderManager::videoConnections: List: "<<list; 

	return list;
}
