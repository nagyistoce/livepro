#include "VideoReceiver.h"
#include "VideoSenderCommands.h"

#include <QCoreApplication>
#include <QTime>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>

#define DEBUG

QMap<QString,VideoReceiver *> VideoReceiver::m_threadMap;
QMutex VideoReceiver::m_threadCacheMutex;

QString VideoReceiver::cacheKey(const QString& host, int port)
{
	return QString("%1:%2").arg(host).arg(port);
}

QString VideoReceiver::cacheKey()
{
	return cacheKey(m_host,m_port);
}

VideoReceiver * VideoReceiver::getReceiver(const QString& host, int port)
{
	if(host.isEmpty())
		return 0;

	QMutexLocker lock(&m_threadCacheMutex);

	QString key = cacheKey(host,port);
	
	if(m_threadMap.contains(key))
	{
		VideoReceiver *v = m_threadMap[key];
		#ifdef DEBUG
 		qDebug() << "VideoReceiver::getReceiver(): "<<v<<": "<<key<<": [CACHE HIT] +";
 		#endif
		return v;
	}
	else
	{
		VideoReceiver *v = new VideoReceiver();
		if(v->connectTo(host,port))
		{
			m_threadMap[key] = v;
			#ifdef DEBUG
			qDebug() << "VideoReceiver::getReceiver(): "<<v<<": "<<key<<": [CACHE MISS] -";
			#endif
	//  		v->initCamera();
			//v->start(); //QThread::HighPriority);
			//usleep(750 * 1000); // give it half a sec or so to init
			
			return v;
		}
		else
		{
			v->deleteLater();
			qDebug() << "VideoReceiver::getReceiver(): "<<key<<": Unable to connect to requested host, removing connection.";
			return 0;
		}
	}
}

VideoReceiver::VideoReceiver(QObject *parent) 
	: VideoSource(parent)
	, m_socket(0)
	, m_boundary("")
	, m_firstBlock(true)
	, m_dataBlock("")
	, m_autoResize(-1,-1)
	, m_autoReconnect(true)
	, m_byteCount(-1)
	, m_connected(false)
	
{
// #ifdef MJPEG_TEST
// 	m_label = new QLabel();
// 	m_label->setWindowTitle("VideoReceiver Test");
// 	m_label->setGeometry(QRect(0,0,320,240));
// 	m_label->show();
// #endif
	setIsBuffered(false);
}
VideoReceiver::~VideoReceiver()
{
	#ifdef DEBUG
	//qDebug() << "VideoReceiver::~VideoReceiver(): "<<this;
	#endif
	
	QMutexLocker lock(&m_threadCacheMutex);
	m_threadMap.remove(cacheKey());

	if(m_socket)
		exit();
		
	quit();
	wait();
}

  
bool VideoReceiver::connectTo(const QString& host, int port, QString url, const QString& user, const QString& pass)
{
	if(url.isEmpty())
		url = "/";
		
	m_host = host;
	m_port = port;
	m_url = url;
	m_user = user;
	m_pass = pass;
	
	m_connected = false;
	
	if(m_socket)
	{
		//qDebug() << "VideoReceiver::connectTo: Discarding old socket:"<<m_socket;
		disconnect(m_socket, 0, this, 0);
		m_dataBlock.clear();
		
		m_socket->abort();
		m_socket->disconnectFromHost();
		//m_socket->waitForDisconnected();
		m_socket->deleteLater();
// 		delete m_socket;
		m_socket = 0;
	}
		
	m_socket = new QTcpSocket(this);
	
	connect(m_socket, SIGNAL(readyRead()),    this,   SLOT(dataReady()));
	connect(m_socket, SIGNAL(disconnected()), this,   SLOT(lostConnection()));
	connect(m_socket, SIGNAL(disconnected()), this, SIGNAL(socketDisconnected()));
	connect(m_socket, SIGNAL(connected()),    this, SIGNAL(socketConnected()));
	connect(m_socket, SIGNAL(connected()),    this,   SLOT(connectionReady()));
	connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SIGNAL(socketError(QAbstractSocket::SocketError)));
	connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(lostConnection(QAbstractSocket::SocketError)));
	
	m_socket->connectToHost(host,port);
	m_socket->setReadBufferSize(1024 * 1024);
	
	//qDebug() << "VideoReceiver::connectTo: Connecting to"<<host<<"with socket:"<<m_socket;
	
	
	m_time.start();
	m_debugFps = false;
	m_frameCount = 0;
	m_latencyAccum = 0;
	
	return true;
}

void VideoReceiver::connectionReady()
{
	//qDebug() << "Connected";
	m_connected = true;
	
	emit connected();
}

void VideoReceiver::log(const QString& str)
{
	qDebug() << "VideoReceiver::log(): "<<str;
}

void VideoReceiver::lostConnection()
{
	if(m_autoReconnect)
	{
		//qDebug() << "VideoReceiver::lostSonnection: Lost server, attempting to reconnect in 1sec";
		
		enqueue(new VideoFrame(QImage("dot.gif"),1000/30));
		QTimer::singleShot(5000,this,SLOT(reconnect()));
	}
	else
	{
		exit();
	}
}

void VideoReceiver::lostConnection(QAbstractSocket::SocketError error)
{
	//qDebug() << "VideoReceiver::lostConnection("<<error<<"):" << m_socket->errorString();
	
	if(error == QAbstractSocket::ConnectionRefusedError)
		lostConnection();
}


void VideoReceiver::reconnect()
{
	//log(QString("Attempting to reconnect to %1:%2%3").arg(m_host).arg(m_port).arg(m_url));
	if(m_port > 0)
		connectTo(m_host,m_port,m_url);
}


void VideoReceiver::sendCommand(QVariantMap map)
{
	if(!m_connected)
		return;
		
	//QByteArray json = m_stringy->serialize(map);
	QByteArray array;
	QDataStream stream(&array, QIODevice::WriteOnly);
	stream << map;

	//qDebug() << "GLPlayerServerThread: Send Map:"<<map<<", JSON:"<<json;

	char data[256];
	sprintf(data, "%d\n", array.size());
	m_socket->write((const char*)&data,strlen((const char*)data));
	//qDebug() << "block size: "<<strlen((const char*)data)<<", data:"<<data;

	m_socket->write(array);
}

void VideoReceiver::sendCommand(QVariantList reply)
{
	QVariantMap map;
	if(reply.size() % 2 != 0)
	{
		qDebug() << "VideoReceiver::sendCommand: [WARNING]: Odd number of elelements in reply: "<<reply;
	}

	for(int i=0; i<reply.size(); i+=2)
	{
		if(i+1 >= reply.size())
			continue;

		QString key = reply[i].toString();
		QVariant value = reply[i+1];

		map[key] = value;
	}


	//qDebug() << "VideoReceiver::sendCommand: [DEBUG] port:"<<m_port<<", map:"<<map;

	sendCommand(map);
}

void VideoReceiver::setHue(int x)
{
	sendCommand(QVariantList() 
		<< "cmd"   << Video_SetHue
		<< "value" << x);
}

void VideoReceiver::setSaturation(int x)
{
	sendCommand(QVariantList() 
		<< "cmd"   << Video_SetSaturation
		<< "value" << x);
}

void VideoReceiver::setContrast(int x)
{
	sendCommand(QVariantList() 
		<< "cmd"   << Video_SetContrast
		<< "value" << x);
}

void VideoReceiver::setBrightness(int x)
{
	sendCommand(QVariantList() 
		<< "cmd"   << Video_SetBright
		<< "value" << x);
}

void VideoReceiver::setFPS(int x)
{
	sendCommand(QVariantList() 
		<< "cmd" << Video_SetFPS
		<< "fps" << x);
}

void VideoReceiver::setSize(int w, int h)
{
	sendCommand(QVariantList() 
		<< "cmd" << Video_SetSize
		<< "w"   << w
		<< "h"   << h);
}


void VideoReceiver::queryHue()
{
	sendCommand(QVariantList() 
		<< "cmd"   << Video_GetHue);
}

void VideoReceiver::querySaturation()
{
	sendCommand(QVariantList() 
		<< "cmd"   << Video_GetSaturation);
}

void VideoReceiver::queryContrast()
{
	sendCommand(QVariantList() 
		<< "cmd"   << Video_GetContrast);
}

void VideoReceiver::queryBrightness()
{
	sendCommand(QVariantList() 
		<< "cmd"   << Video_GetBright);
}

void VideoReceiver::queryFPS()
{
	sendCommand(QVariantList() 
		<< "cmd" << Video_GetFPS);
}

void VideoReceiver::querySize()
{
	sendCommand(QVariantList() 
		<< "cmd" << Video_GetSize);
}

void VideoReceiver::dataReady()
{
	if(!m_connected)
	{
		m_dataBlock.clear();
		return;
	}
	QByteArray bytes = m_socket->readAll();
	//qDebug() << "VideoReceiver::dataReady(): Reading from socket:"<<m_socket<<", read:"<<bytes.size()<<" bytes"; 
	if(bytes.size() > 0)
	{
		m_dataBlock.append(bytes);
//		qDebug() << "dataReady(), read bytes:"<<bytes.count()<<", buffer size:"<<m_dataBlock.size();
		
		processBlock();
	}
}

void VideoReceiver::processBlock()
{
	if(!m_connected)
		return;
		
	#define HEADER_SIZE 256
	
	// First thing server sends is a single 256-byte header containing the initial frame byte count
	// Byte count and frame size CAN change in-stream as needed.
	if(m_byteCount < 0)
	{
		if(m_dataBlock.size() >= HEADER_SIZE)
		{
			QByteArray header = m_dataBlock.left(HEADER_SIZE);
			m_dataBlock.remove(0,HEADER_SIZE);
			
			const char *headerData = header.constData();
			sscanf(headerData,"%d",&m_byteCount);
		}
	}
	
	if(m_byteCount >= 0)
	{
		int frameSize = m_byteCount+HEADER_SIZE;
		
		while(m_dataBlock.size() >= frameSize)
		{
			QByteArray block = m_dataBlock.left(frameSize);
			m_dataBlock.remove(0,frameSize);
			
			QByteArray header = block.left(HEADER_SIZE);
			block.remove(0,HEADER_SIZE);
			
			const char *headerData = header.constData();
			
								
			int byteTmp,imgX,imgY,pixelFormatId,imageFormatId,bufferType,timestamp,holdTime,origX,origY;
			
			sscanf(headerData,
					"%d " // byteCount
					"%d " // w
					"%d " // h
					"%d " // pixelFormat
					"%d " // image.format
					"%d " // bufferType
					"%d " // timestamp
					"%d " // holdTime
					"%d " // original width
					"%d", // original height
					&byteTmp, 
					&imgX,
					&imgY,
					&pixelFormatId,
					&imageFormatId,
					&bufferType,
					&timestamp,
					&holdTime,
					&origX,
					&origY);
					
			if(imgX < 0 && imgY < 0 && holdTime < 0)
			{
				// data frame from VideoSender in response to a query request (Video_Get* command)
				
				if(byteTmp != m_byteCount)
				{
					m_byteCount = byteTmp;
					frameSize = m_byteCount + HEADER_SIZE;
					//qDebug() << "VideoReceiver::processBlock: Frame size changed: "<<frameSize;
				}
				
				
				/// TODO: The parsing code here wont work for data replies. Need to rework to parse header, get byte count, THEN get data packet from header
			}
			else
			// No need to create and emit frames if noone is listeneing for frames!
			if(!m_consumerList.isEmpty())
			{
				//qDebug() << "raw header scan: byteTmp:"<<byteTmp<<", size:"<<imgX<<"x"<<imgY;
				
				//qDebug() << "VideoReceiver::processBlock: raw header data:"<<headerData;
				if(byteTmp > 1024*1024*1024 ||
					imgX > 1900 || imgX < 0 ||
					imgY > 1900 || imgY < 0)
				{
					qDebug() << "VideoReceiver::processBlock: Frame too large (bytes > 1GB or invalid W/H)";
					m_dataBlock.clear();
					return;
				}
				
				if(byteTmp != m_byteCount)
				{
					m_byteCount = byteTmp;
					frameSize = m_byteCount + HEADER_SIZE;
					//qDebug() << "VideoReceiver::processBlock: Frame size changed: "<<frameSize;
				}
				//qDebug() << "VideoReceiver::processBlock: header data:"<<headerData;
				//QImage frame = QImage::fromData(block);
			
				//QImage frame = QImage::fromData(block);
				//QImage frame((const uchar*)block.constData(), imgX, imgY, (QImage::Format)formatId); 
				if(pixelFormatId == 0)
					pixelFormatId = (int)QVideoFrame::Format_RGB32;
					
				//VideoFrame frame;
				VideoFrame *frame = new VideoFrame();
				#ifdef DEBUG_VIDEOFRAME_POINTERS
				qDebug() << "VideoReceiver::processBlock(): Created new frame:"<<frame;
				#endif
				
				frame->setHoldTime    (holdTime);
				frame->setCaptureTime (timestampToQTime(timestamp));
				frame->setPixelFormat ((QVideoFrame::PixelFormat)pixelFormatId);
				frame->setBufferType  ((VideoFrame::BufferType)bufferType);
				
				//qDebug() << "final pixelformat:"<<pixelFormatId;
				
				
				if(frame->bufferType() == VideoFrame::BUFFER_IMAGE)
				{
					frame->setImage(QImage((const uchar*)block.constData(), imgX, imgY, (QImage::Format)imageFormatId).copy());
					
					// Disabled upscaling for now because:
					// 1. It was taking approx 20ms to upscale 320x240->1024x758
					// 2. Why upscale here?? If user wants a larger image, he can make the thing drawing this frame larger,
					//    which will scale the frame at that point - which probably will be on the GPU anyway
					// Therefore, given the 20ms (or, even 10ms if it was) and the fact that the GPU can do scaling anyway,
					// no need to waste valuable time upscaling here for little to no benefit at all. 
// 					if(origX != imgX || origY != imgY)
// 					{
// 						QTime x;
// 						x.start();
// 						frame->setImage(frame->image().scaled(origX,origY));
// 						qDebug() << "VideoReceiver::processBlock: Upscaled frame from "<<imgX<<"x"<<imgY<<" to "<<origX<<"x"<<imgY<<" in "<<x.elapsed()<<"ms";
// 						
// 					} 
// 					else
// 					{
						//frame->setImage(frame->image().copy()); // disconnect from the QByteArray by calling copy() to make an internal copy of the buffer
//					}
				}
				else
				{
					//QByteArray array;
					//qDebug() << "m_byteCount:"<<m_byteCount<<", size:"<<imgX<<"x"<<imgY;
					uchar *pointer = frame->allocPointer(m_byteCount);
					memcpy(pointer, (const char*)block.constData(), m_byteCount);
					//array.append((const char*)block.constData(), m_byteCount);
					//frame->setByteArray(array);
				}
				
// 				if(origX != imgX || origY != imgY)
// 					frame->setSize(QSize(origX,origY));
// 				else
					frame->setSize(QSize(imgX,imgY));
	
				#ifdef DEBUG_VIDEOFRAME_POINTERS
				qDebug() << "VideoReceiver::processBlock(): Enqueing new frame:"<<frame;
				#endif
				
				enqueue(frame);
				/*
				//480,480, QImage::Format_RGB32);
				
				if(!frame.isNull())
				{
					//qDebug() << "processBlock(): New image received, original size:"<<frame.size()<<", bytes:"<<block.length()<<", format:"<<formatId;
					
					if(m_autoResize.width()>0 && m_autoResize.height()>0 && 
					m_autoResize != frame.size())
						frame = frame.scaled(m_autoResize);
				
					
					//		qDebug() << "processBlock(): Emitting new image, size:"<<frame.size();
					emit newImage(frame);
				}	*/
				
				
				#ifdef MJPEG_TEST
	// 			QPixmap pix = QPixmap::fromImage(frame);
	// 			m_label->setPixmap(pix);
	// 			m_label->resize(pix.width(),pix.height());
	// 			//qDebug() << "processBlock(): latency: "<<;
				#endif
				
				int msecLatency = msecTo(timestamp);
				m_latencyAccum += msecLatency;
				
				if (m_debugFps && !(m_frameCount % 100)) 
				{
					QString framesPerSecond;
					framesPerSecond.setNum(m_frameCount /(m_time.elapsed() / 1000.0), 'f', 2);
					
					QString latencyPerFrame;
					latencyPerFrame.setNum((((double)m_latencyAccum) / ((double)m_frameCount)), 'f', 3);
					
					if(m_debugFps && framesPerSecond!="0.00")
						qDebug() << "VideoReceiver: Receive FPS: " << qPrintable(framesPerSecond) << qPrintable(QString(", Receive Latency: %1 ms").arg(latencyPerFrame));
			
					m_time.start();
					m_frameCount = 0;
					m_latencyAccum = 0;
					
					//lastFrameTime = time.elapsed();
				}
				m_frameCount++;
			}
		}
	}
}


QTime VideoReceiver::timestampToQTime(int ts)
{
	int hour = ts / 1000 / 60 / 60;
	int msTmp = ts - (hour * 60 * 60 * 1000);
	int min = msTmp / 1000 / 60;
	msTmp -= min * 60 * 1000;
	int sec = msTmp / 1000;
	msTmp -= sec * 1000;
	
	return QTime(hour, min, sec, msTmp);
}


int VideoReceiver::msecTo(int timestamp)
{
	QTime time = QTime::currentTime();
	//int currentTimestamp = time.hour() + time.minute() + time.second() + time.msec();
	int currentTimestamp = 
			time.hour()   * 60 * 60 * 1000 +
			time.minute() * 60 * 1000      + 
			time.second() * 1000           +
			time.msec();
	return currentTimestamp - timestamp;
}

void VideoReceiver::exit()
{
	if(m_socket)
	{
		qDebug() << "VideoReceiver::exit: Discarding old socket:"<<m_socket;
		disconnect(m_socket,0,this,0);
		m_dataBlock.clear();
		
		//qDebug() << "VideoReceiver::exit: Quiting video receivier";
		m_socket->abort();
		m_socket->disconnectFromHost();
		//m_socket->waitForDisconnected();
		m_socket->deleteLater();
		//delete m_socket;
		m_socket = 0;
	}
}


