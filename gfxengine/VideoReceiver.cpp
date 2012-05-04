#include "VideoReceiver.h"
#include "VideoSenderCommands.h"

#include <QCoreApplication>
#include <QTime>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>

//#define DEBUG

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

VideoReceiver * VideoReceiver::getReceiver(const QString& hostTmp, int port)
{
	QString host = hostTmp;
	
	if(host.isEmpty())
		return 0;
	
	// Assume they passed a single QString with the format of "Host:Port"
	if(host.indexOf(":") > 0 && port < 0)
	{
		QStringList list = host.split(":");
		host = list[0];
		port = list[1].toInt();
	}

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
	, m_hasReceivedHintsFromServer(false)
	, m_connected(false)
	
{
// #ifdef MJPEG_TEST
// 	m_label = new QLabel();
// 	m_label->setWindowTitle("VideoReceiver Test");
// 	m_label->setGeometry(QRect(0,0,320,240));
// 	m_label->show();
// #endif
	setIsBuffered(false);
	
	//QImage blueImage(1,1, QImage::Format_ARGB32);
	//blueImage.fill(Qt::transparent);
	
	// For some reason, GLVideoDrawable WONT WORK if I sent the QImage (above) first - it HAS to use the dot.gif!!!! Grrr.
	enqueue(new VideoFrame(QImage("../data/icons/dot.gif"),1000/30));
}
VideoReceiver::~VideoReceiver()
{
	#ifdef DEBUG
	qDebug() << "VideoReceiver::~VideoReceiver(): "<<this;
	#endif
	
	if(m_socket)
		exit();
		
	quit();
	wait();
}


void VideoReceiver::destroySource()
{
	QMutexLocker lock(&m_threadCacheMutex);
	m_threadMap.remove(cacheKey());
	
	VideoSource::destroySource();
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
		#ifdef DEBUG
		qDebug() << "VideoReceiver::connectTo: Discarding old socket:"<<m_socket;
		#endif
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
	m_socket->setReadBufferSize(1024 * 1024 * 5);
	
	#ifdef DEBUG
	qDebug() << "VideoReceiver::connectTo: Connecting to"<<host<<":"<<port<<" with socket:"<<m_socket;
	#endif
	
	
	m_time.start();
	m_debugFps = false;
	m_frameCount = 0;
	m_latencyAccum = 0;
	
	return true;
}

void VideoReceiver::connectionReady()
{
	#ifdef DEBUG
	qDebug() << "VideoReceiver::connectionReady(): Connected";
	#endif
	m_connected = true;
	
	emit connected();
	
	// Proactively request video hints
	queryVideoHints();
}

void VideoReceiver::log(const QString& str)
{
	qDebug() << "VideoReceiver::log(): "<<str;
}

void VideoReceiver::lostConnection()
{
	if(m_autoReconnect)
	{
		#ifdef DEBUG
		qDebug() << "VideoReceiver::lostSonnection: Lost server, attempting to reconnect in 5 sec";
		#endif
		
		// GLVideoDrawable MUSt use dot.gif, not a transparent image as below
		enqueue(new VideoFrame(QImage("../data/icons/dot.gif"),1000/30));
		//QImage blueImage(1,1, QImage::Format_ARGB32);
	        //blueImage.fill(Qt::transparent);
		//enqueue(new VideoFrame(blueImage,1000/30));

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
		#ifdef DEBUG
		qDebug() << "VideoReceiver::sendCommand: [WARNING]: Odd number of elelements in reply: "<<reply;
		#endif
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

void VideoReceiver::setCardInput(const QString& input)
{
	sendCommand(QVariantList() 
		<< "cmd" << Video_SetCardInput
		<< "input" << input);
}

void VideoReceiver::setVideoHints(QVariantMap hints)
{
	m_videoHints = hints;
	sendCommand(QVariantList() 
		<< "cmd" << Video_SetVideoHints
		<< "hints" << hints);
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

void VideoReceiver::queryVideoHints()
{
	sendCommand(QVariantList() 
		<< "cmd" << Video_GetVideoHints);
}

void VideoReceiver::dataReady()
{
	#ifdef DEBUG
	qDebug() << "VideoReceiver::dataReady()";
	#endif
	
	if(!m_connected)
	{
		#ifdef DEBUG
		qDebug() << "VideoReceiver::dataReady(): Port: "<<m_port<<": Not connected, clearing data block.";
		#endif
		m_dataBlock.clear();
		return;
	}
	QByteArray bytes = m_socket->readAll();
	#ifdef DEBUG
	qDebug() << "VideoReceiver::dataReady(): Port: "<<m_port<<": Reading from socket:"<<m_socket<<", read:"<<bytes.size()<<" bytes";
	#endif
	 
	if(bytes.size() > 0)
	{
		m_dataBlock.append(bytes);
		#ifdef DEBUG
		qDebug() << "VideoReceiver::dataReady(): Port: "<<m_port<<": Read bytes:"<<bytes.count()<<", buffer size:"<<m_dataBlock.size();
		#endif
		
		processBlock();
	}
}

void VideoReceiver::processBlock()
{
	if(!m_connected)
		return;

	#ifdef DEBUG_TIME
	QTime time;
	time.start();

	QTime t2;
	t2.start();
	#endif

	#define HEADER_SIZE 256
	
	// First thing server sends is a single 256-byte header containing the initial frame byte count
	// Byte count and frame size CAN change in-stream as needed.
// 	if(m_byteCount < 0)
// 	{
// 		if(m_dataBlock.size() >= HEADER_SIZE)
// 		{
// 			QByteArray header = m_dataBlock.left(HEADER_SIZE);
// 			//m_dataBlock.remove(0,HEADER_SIZE);
// 			
// 			const char *headerData = header.constData();
// 			sscanf(headerData,"%d",&m_byteCount);
// 			//qDebug() << "VideoReceiver::processBlock(): First frame on connect, m_byteCount:"<<m_byteCount; 
// 		}
// 	}
	
	bool done = false;
	while(!done)
	{
		// It's possible we entered this function with a m_byteCount>0, 
		// which indicates that we found a header last time, but not enough bytes
		// in the block for the data body, so we returned. But this time,
		// we may have enough bytes - but the header is already taken, so just look
		// for the body if m_byteCount>0;
		if(m_dataBlock.size() < HEADER_SIZE)
		{
			#ifdef DEBUG
			qDebug() << "VideoReceiver::processBlock: Port: "<<m_port<<": Not enough bytes for a header, setting done";
			#endif
			done = true;
			continue;
		}
		if(m_byteCount <= 0)/* &&
		   m_dataBlock.size() >= HEADER_SIZE)*/
		{
			QByteArray header = m_dataBlock.left(HEADER_SIZE);
			m_dataBlock.remove(0,HEADER_SIZE);
			
			const char *headerData = header.constData();
				
			#ifdef DEBUG
			qDebug() << "VideoReceiver::processBlock: Port: "<<m_port<<": header data:"<<headerData;
			#endif
			if(QString(header).isEmpty())
			{
				#ifdef DEBUG
				qDebug() << "VideoReceiver::processBlock: Port: "<<m_port<<": Empty header, setting done";
				#endif
				done = true;
				continue;
			}
		
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
				&m_byteCount, 
				&m_imgX,
				&m_imgY,
				&m_pixelFormatId,
				&m_imageFormatId,
				&m_bufferType,
				&m_timestamp,
				&m_holdTime,
				&m_origX,
				&m_origY);
		}
		
		#ifdef DEBUG
		qDebug() << "VideoReceiver::processBlock: Port: "<<m_port<<": Done reading header, m_byteCount:"<<m_byteCount<<" bytes, m_dataBlock size:"<<m_dataBlock.size();
		#endif
	
		if(m_dataBlock.size() < m_byteCount)
		{
			done = true;
			#ifdef DEBUG
			qDebug() << "VideoReceiver::processBlock: Port: "<<m_port<<": Found header, got byte count: "<<m_byteCount<<", m_dataBlock.size():"<<m_dataBlock.size()<<", but not enough bytes left in block to continue processing.";
			#endif
			continue;
		}
		else
		{
			#ifdef DEBUG_TIME
			qDebug() << "VideoReceiver::processBlock: Port: "<<m_port<<": t2.elapsed:"<<t2.restart()<<", mark1";
			#endif
			
			// If these variables are negative, then assume that indicates a QMap data block
			if(m_imgX < 0 && m_imgY < 0 && m_holdTime < 0)
			{
				// data frame from VideoSender in response to a query request (Video_Get* command)
				// or from a signal received by the VideoSender (such as signalStatusChanged(bool))
				
				QByteArray block = m_dataBlock.left(m_byteCount);

				QDataStream stream(&block, QIODevice::ReadOnly);
				QVariantMap map;
				stream >> map;
				
				#ifdef DEBUG
				qDebug() << "VideoReceiver::processBlock: Port: "<<m_port<<": Received MAP block: "<<map;
				#endif
				
				if(!map.isEmpty())
				    processReceivedMap(map);
			}
			else
			// No need to create and emit frames if noone is listeneing for frames!
			if(m_consumerList.isEmpty())
			{
				#ifdef DEBUG
				qDebug() << "VideoReceiver::processBlock: Port: "<<m_port<<": No consumers, not processingframe";
				#endif
				//block.clear();
				m_dataBlock.remove(0,m_byteCount);

				continue;
			}
			else
			 // consumer list wasn't empty, not a map block
			{
				//qDebug() << "raw header scan: byteTmp:"<<byteTmp<<", size:"<<imgX<<"x"<<imgY;
				
				//qDebug() << "VideoReceiver::processBlock: raw header data:"<<headerData;
				if(m_byteCount > 1024*1024*1024     ||
					m_imgX > 1900 || m_imgX < 0 ||
					m_imgY > 1900 || m_imgY < 0)
				{
					#ifdef DEBUG
					qDebug() << "VideoReceiver::processBlock: Port: "<<m_port<<": Frame too large (bytes > 1GB or invalid W/H): "<<m_byteCount<<m_imgX<<m_imgY;
					#endif
					//m_dataBlock.clear();
					
					//enqueue(new VideoFrame(QImage("../data/icons/dot.gif"),1000/30));
					
					//return;
					m_dataBlock.remove(0,m_byteCount);
					continue;
				}
				
				//QImage frame = QImage::fromData(block);
			
				//QImage frame = QImage::fromData(block);
				//QImage frame((const uchar*)block.constData(), imgX, imgY, (QImage::Format)formatId); 
				if(m_pixelFormatId == 0)
					m_pixelFormatId = (int)QVideoFrame::Format_RGB32;
					
				//VideoFrame frame;
				VideoFrame *frame = new VideoFrame();
				
				#ifdef DEBUG_VIDEOFRAME_POINTERS
				qDebug() << "VideoReceiver::processBlock(): Created new frame:"<<frame;
				#endif
				
				frame->setHoldTime    (m_holdTime);
				frame->setCaptureTime (timestampToQTime(m_timestamp));
				frame->setPixelFormat ((QVideoFrame::PixelFormat)m_pixelFormatId);
				frame->setBufferType  ((VideoFrame::BufferType)m_bufferType);
				
				//qDebug() << "final pixelformat:"<<pixelFormatId;
				
				bool validFrame = true;

				if(frame->bufferType() == VideoFrame::BUFFER_IMAGE)
				{
					QImage::Format imageFormat = (QImage::Format)m_imageFormatId;
					int expectedBytes = m_imgY * m_imgX *
						(imageFormat == QImage::Format_RGB16  ||
						 imageFormat == QImage::Format_RGB555 ||
						 imageFormat == QImage::Format_RGB444 ||
						 imageFormat == QImage::Format_ARGB4444_Premultiplied ? 2 :
						 imageFormat == QImage::Format_RGB888 ||
						 imageFormat == QImage::Format_RGB666 ||
						 imageFormat == QImage::Format_ARGB6666_Premultiplied ? 3 :
						 4);
					
					if(expectedBytes > m_dataBlock.size())
					{
						qDebug() << "Error: not enough bytes in block for frame, skipping: "<<expectedBytes<<" > "<<m_dataBlock.size();
						validFrame = false;
					}
					else
					{
						#ifdef DEBUG_TIME
						qDebug() << "VideoReceiver::processBlock: Port: "<<m_port<<": t2.elapsed:"<<t2.restart()<<", mark2";
						#endif
						
						//QImage origImage((const uchar*)block.constData(), m_imgX, m_imgY, (QImage::Format)m_imageFormatId);
						QImage origImage((const uchar*)m_dataBlock.constData(), m_imgX, m_imgY, (QImage::Format)m_imageFormatId);
						//qDebug() << "origImage.rect:"<<origImage.rect()<<", imgX:"<<imgX<<", imgY:"<<imgY<<", block.size:"<<block.size();
						frame->setImage(origImage.copy());
						
						#ifdef DEBUG_TIME
						qDebug() << "VideoReceiver::processBlock: Port: "<<m_port<<": t2.elapsed:"<<t2.restart()<<", mark3";
						#endif
					}
				}
				else
				{
					//QByteArray array;
					qDebug() << "m_byteCount:"<<m_byteCount<<", size:"<<m_imgX<<"x"<<m_imgY;
					uchar *pointer = frame->allocPointer(m_byteCount);
					//if(pointer && m_byteCount > 0)
						memcpy(pointer, (const char*)m_dataBlock.constData(), m_byteCount);
					//array.append((const char*)block.constData(), m_byteCount);
					//frame->setByteArray(array);
				}
				
// 				if(origX != imgX || origY != imgY)
// 					frame->setSize(QSize(origX,origY));
// 				else
					frame->setSize(QSize(m_imgX,m_imgY));
					
				if(validFrame)
				{
					#ifdef DEBUG
					qDebug() << "VideoReceiver::processBlock: Port: "<<m_port<<": New Frame, frame size:"<<frame->size()<<", consumers size:"<< m_consumerList.size();
					#endif
					//frame->image().save("frametest.jpg");
		
					#ifdef DEBUG_VIDEOFRAME_POINTERS
					qDebug() << "VideoReceiver::processBlock(): Enqueing new frame:"<<frame;
					#endif
					
					enqueue(frame);
				}
				else
				{
					#ifdef DEBUG
					qDebug() << "VideoReceiver::processBlock: Port: "<<m_port<<": validFrame=false, not enquing frame";
					#endif
				}
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
				
				int msecLatency = msecTo(m_timestamp);
				m_latencyAccum += msecLatency;
				#ifdef DEBUG_TIME
				m_debugFps = true;
				#endif
				if (m_debugFps && !(m_frameCount % 10))
				{
					QString framesPerSecond;
					framesPerSecond.setNum(m_frameCount /(m_time.elapsed() / 1000.0), 'f', 2);
					
					QString latencyPerFrame;
					latencyPerFrame.setNum((((double)m_latencyAccum) / ((double)m_frameCount)), 'f', 3);
					
					if(m_debugFps && framesPerSecond!="0.00")
						qDebug() << "VideoReceiver::processBlock: Port: "<<m_port<<": Receive FPS: " << qPrintable(framesPerSecond) << qPrintable(QString(", Receive Latency: %1 ms").arg(latencyPerFrame));
			
					m_time.start();
					m_frameCount = 0;
					m_latencyAccum = 0;
					
					//lastFrameTime = time.elapsed();
				}
				
				m_frameCount++;
				
				#ifdef DEBUG_TIME
				qDebug() << "VideoReceiver::processBlock: Port: "<<m_port<<": t2.elapsed:"<<t2.restart()<<", mark4";
				#endif
				
			} // consumer list wasn't empty, not a map block
		
			m_dataBlock.remove(0,m_byteCount);

			// Reset byte count so that the next time thru, m_dataBlock is assumed to start with a header
			m_byteCount = -1;

			
		} // m_dataBlock.size() >= m_byteCount
	} // while !done

	#ifdef DEBUG_TIME
	qDebug() << "VideoReceiver::processBlock: Port: "<<m_port<<": end of function, processing time: "<<time.elapsed();
	#endif
} // end of func


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
		#ifdef DEBUG
		qDebug() << "VideoReceiver::exit: Discarding old socket:"<<m_socket;
		#endif
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

void VideoReceiver::processReceivedMap(const QVariantMap & map)
{
	QString cmd = map["cmd"].toString();
	//qDebug() << "VideoSenderThread::processBlock: map:"<<map;
	
	if(cmd == Video_GetHue)
	{
		emit currentHue(map["value"].toInt());
	}
	else
	if(cmd == Video_GetSaturation)
	{
		emit currentSaturation(map["value"].toInt());
	}
	else
	if(cmd == Video_GetBright)
	{
		emit currentContrast(map["value"].toInt());
	}
	else
	if(cmd == Video_GetContrast)
	{
		emit currentBrightness(map["value"].toInt());
	}
	else
	if(cmd == Video_GetFPS)
	{
		//"value" << fps
		emit currentFPS(map["value"].toInt());
	}
	else
	if(cmd == Video_GetSize)
	{
		//"w" << size.width() << "h" << size.height()
		emit currentSize(map["w"].toInt(), map["h"].toInt());
	}
	else
	if(cmd == Video_GetVideoHints)
	{
		m_videoHints = map["hints"].toMap();
		m_hasReceivedHintsFromServer = true;
		emit currentVideoHints(m_videoHints);
	}
	else
	if(cmd == Video_SignalStatusChanged)
	{
		//<< "flag"	<< flag
		emit signalStatusChanged(map["flag"].toBool());
	}
	else
	{
		#ifdef DEBUG
		qDebug() << "VideoReceiver::processReceivedMap: Unknown map received, cmd:"<<cmd<<", full map:"<<map;
		#endif
	}
	
}

QVariantMap VideoReceiver::videoHints(bool *hasReceivedFromServer)
{
	if(hasReceivedFromServer)
		*hasReceivedFromServer = m_hasReceivedHintsFromServer;
	return m_videoHints;
}
