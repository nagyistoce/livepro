#include "VideoInputSenderManager.h"
#include "CameraThread.h"
#include "VideoSender.h"
#include <QNetworkInterface>

VideoInputSenderManager::VideoInputSenderManager(QObject *parent)
	: QObject(parent)
{
	QStringList devs = CameraThread::enumerateDevices();
	foreach(QString dev, devs)
	{
                //qDebug() << "VideoInputSenderManager::ctor: Creating thread for device: "<<dev;
		VideoSender *sender = new VideoSender();
                CameraThread *source = CameraThread::threadForCamera(dev);
                source->setFps(30);
		source->registerConsumer(sender);
		
                // The 'raw frames' mode uses V4L to get the frames instead of LibAV
		// Of course, V4L isn't supported on windows, so we don't enable raw frames on windows.
		// In the future, I suppose I could find code to use the appros Windows API to
		// connect to the capture card - but I just don't have a need for that level of performance
		// on windows right now, so I'll put it off until I need it or someone really wants it.
		// For now, the high-performance capture use is on Linux (for me), so that's where I'll focus. 
		#ifndef Q_OS_WIN
		source->enableRawFrames(true);
		#endif
		
                sender->setVideoSource(source);
                m_videoSenders[dev] = sender;
	}

        //qDebug() << "VideoInputSenderManager::ctor: Done constructing";
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
		QStringList devs = CameraThread::enumerateDevices();
		foreach(QString dev, devs) //m_videoSenders.keys())
		{
			VideoSender *sender = m_videoSenders[dev];
			int port = sender->start(); // auto-allocate a port by leaving the port argument empty
			
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
	
	QString ipAddress = VideoSender::ipAddress();
	
	QStringList list;
	
	QStringList devs = CameraThread::enumerateDevices();
	foreach(QString dev, devs) //m_videoSenders.keys())
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
