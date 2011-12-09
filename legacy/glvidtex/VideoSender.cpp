#include "VideoSender.h"
#include "VideoSenderCommands.h"

// for setting hue, color, etc
#include "../livemix/CameraThread.h"

#include <QNetworkInterface>
#include <QTime>
#include <QProcess>

VideoSender::VideoSender(QObject *parent)
	: QTcpServer(parent)
	, m_adaptiveWriteEnabled(true)
	, m_source(0)
	, m_transmitSize(240,180)
	, m_transmitFps(10)
	//, m_transmitSize(0,0)
	//, m_scaledFrame(0)
	//, m_frame(0)
	, m_consumerRegistered(false)
{
	connect(&m_fpsTimer, SIGNAL(timeout()), this, SLOT(processFrame()));
	setTransmitFps(m_transmitFps);
}

VideoSender::~VideoSender()
{
	setVideoSource(0);
	m_dataPtr.clear(); // allow the pointer being held to be deleted
	
	// Valgrind reported that the pointer in m_dataPtr didnt get deleted at close of program:
	
// 	==22564== 326,400 bytes in 3 blocks are possibly lost in loss record 5,903 of 5,912
// 	==22564==    at 0x4005903: malloc (vg_replace_malloc.c:195)
// 	==22564==    by 0x80A8BB5: VideoSender::processFrame() (VideoSender.cpp:151)
// 	==22564==    by 0x8266001: VideoSender::qt_metacall(QMetaObject::Call, int, void**) (moc_VideoSender.cpp:95)
// 	==22564==    by 0x604CADA: QMetaObject::metacall(QObject*, QMetaObject::Call, int, void**) (qmetaobject.cpp:237)
// 	==22564==    by 0x605A5A6: QMetaObject::activate(QObject*, QMetaObject const*, int, void**) (qobject.cpp:3285)
// 	==22564==    by 0x60B1176: QTimer::timeout() (moc_qtimer.cpp:134)
// 	==22564==    by 0x606353D: QTimer::timerEvent(QTimerEvent*) (qtimer.cpp:271)
// 	==22564==    by 0x60580AE: QObject::event(QEvent*) (qobject.cpp:1204)
// 	==22564==    by 0x553ABCE: QApplicationPrivate::notify_helper(QObject*, QEvent*) (qapplication.cpp:4300)
// 	==22564==    by 0x553E98D: QApplication::notify(QObject*, QEvent*) (qapplication.cpp:3704)
// 	==22564==    by 0x604650A: QCoreApplication::notifyInternal(QObject*, QEvent*) (qcoreapplication.cpp:704)
// 	==22564==    by 0x60762C2: QTimerInfoList::activateTimers() (qcoreapplication.h:215)

}

void VideoSender::setTransmitSize(const QSize& size)
{
	m_transmitSize = size;
}

void VideoSender::setTransmitFps(int fps)
{
	m_transmitFps = fps;
	if(fps <= 0)
		m_fpsTimer.stop();
	else
	{
		//qDebug() << "VideoSender::setTransmitFps(): fps:"<<fps;
		m_fpsTimer.setInterval(1000 / fps);
		m_fpsTimer.start();
	}
}

void VideoSender::processFrame()
{
	//qDebug() << "VideoSender::processFrame(): "<<this<<" mark";
	//sendLock();
	QMutexLocker lock(&m_sendMutex);
	
	if(m_frame && m_frame->isValid())
	{
		m_origSize = m_frame->size();
		#ifdef DEBUG_VIDEOFRAME_POINTERS
		qDebug() << "VideoSender::processFrame(): Mark1: m_frame:"<<m_frame;
		#endif
		
// 		m_frame->incRef();
		if(m_transmitSize.isEmpty())
			m_transmitSize = m_origSize;
			
		// Use 16bit format for transmission because:
		//  320x240x16bits / 8bits/byte = 153,600 bytes
		//  320x240x32bits / 8bits/byte = 307,200 bytes
		//  Half as much bandwidth required to transmit the same image - at the expense of 1ms on the sending side.

		//qDebug() << "VideoSender::processFrame: Downscaling video for transmission to "<<m_transmitSize;
		// To scale the video frame, first we must convert it to a QImage if its not already an image.
		// If we're lucky, it already is. Otherwise, we have to jump thru hoops to convert the byte 
		// array to a QImage then scale it.
		QImage scaledImage;
		if(!m_frame->image().isNull())
		{
			scaledImage = m_transmitSize == m_origSize ? 
				m_frame->image() : 
				m_frame->image().scaled(m_transmitSize);
			
			scaledImage = scaledImage.convertToFormat(QImage::Format_RGB16);
		}
		else
		{
			#ifdef DEBUG_VIDEOFRAME_POINTERS
			qDebug() << "VideoSender::processFrame(): Scaling data from frame:"<<m_frame<<", pointer:"<<m_frame->pointer();
			#endif
			const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(m_frame->pixelFormat());
			if(imageFormat != QImage::Format_Invalid)
			{
				QImage image(m_frame->pointer(),
					m_frame->size().width(),
					m_frame->size().height(),
					m_frame->size().width() *
						(imageFormat == QImage::Format_RGB16  ||
						imageFormat == QImage::Format_RGB555 ||
						imageFormat == QImage::Format_RGB444 ||
						imageFormat == QImage::Format_ARGB4444_Premultiplied ? 2 :
						imageFormat == QImage::Format_RGB888 ||
						imageFormat == QImage::Format_RGB666 ||
						imageFormat == QImage::Format_ARGB6666_Premultiplied ? 3 :
						4),
					imageFormat);
					
				//QTime t; t.start();
				scaledImage = m_transmitSize == m_origSize ? 
					image.convertToFormat(QImage::Format_RGB16) : // call convertToFormat instead of copy() because conversion does an implicit copy 
					image.scaled(m_transmitSize).convertToFormat(QImage::Format_RGB16); // do convertToFormat() after scaled() because less bytes to convert
					
				//qDebug() << "Downscaled image from "<<image.byteCount()<<"bytes to "<<scaledImage.byteCount()<<"bytes, orig ptr len:"<<m_frame->pointerLength()<<", orig ptr:"<<m_frame->pointer();
				//convertToFormat(QImage::Format_RGB16).
				//qDebug() << "VideoSender::processFrame: [QImage] downscale and 16bit conversion took"<<t.elapsed()<<"ms";
			}
			else
			{
				//qDebug() << "VideoSender::processFrame: Unable to convert pixel format to image format, cannot scale frame. Pixel Format:"<<m_frame->pixelFormat();
				return;
			}
		}
		
		#ifdef DEBUG_VIDEOFRAME_POINTERS
		qDebug() << "VideoSender::processFrame(): Mark2: frame:"<<m_frame;
		#endif
		
		// Now that we've got the image out of the original frame and scaled it, we have to construct a new
		// video frame to transmit on the wire from the scaledImage (assuming the sccaledImage is valid.)
		// We attempt to transmit in its native format without converting it if we can to save local CPU power.
		if(!scaledImage.isNull())
		{
			m_captureTime = m_frame->captureTime();

			QImage::Format format = scaledImage.format();
			m_pixelFormat = 
				format == QImage::Format_ARGB32 ? QVideoFrame::Format_ARGB32 :
				format == QImage::Format_RGB32  ? QVideoFrame::Format_RGB32  :
				format == QImage::Format_RGB888 ? QVideoFrame::Format_RGB24  :
				format == QImage::Format_RGB16  ? QVideoFrame::Format_RGB565 :
				format == QImage::Format_RGB555 ? QVideoFrame::Format_RGB555 :
				//format == QImage::Format_ARGB32_Premultiplied ? QVideoFrame::Format_ARGB32_Premultiplied :
				// GLVideoDrawable doesn't support premultiplied - so the format conversion below will convert it to ARGB32 automatically
				QVideoFrame::Format_Invalid;
				
			if(m_pixelFormat == QVideoFrame::Format_Invalid)
			{
				qDebug() << "VideoFrame: image was not in an acceptable format, converting to ARGB32 automatically.";
				scaledImage = scaledImage.convertToFormat(QImage::Format_ARGB32);
				m_pixelFormat = QVideoFrame::Format_ARGB32;
			}
			
			uchar *ptr = (uchar*)malloc(sizeof(uchar) * scaledImage.byteCount());
			const uchar *src = (const uchar*)scaledImage.bits();
			memcpy(ptr, src, scaledImage.byteCount());
			
			m_dataPtr = QSharedPointer<uchar>(ptr);
			m_byteCount = scaledImage.byteCount();
			m_imageFormat = scaledImage.format();
			m_imageSize = scaledImage.size();
			
			m_holdTime = m_transmitFps <= 0 ? m_frame->holdTime() : 1000/m_transmitFps;
			
			#ifdef DEBUG_VIDEOFRAME_POINTERS
			qDebug() << "VideoSender::processFrame(): Mark5: frame:"<<m_frame;
			#endif
		}
	}
	
	//sendUnlock();
	
	#ifdef DEBUG_VIDEOFRAME_POINTERS
	qDebug() << "VideoSender::processFrame(): Mark6: m_frame:"<<m_frame;
	#endif
	
	//qDebug() << "VideoSender::processFrame(): "<<this<<" mark end";
	emit receivedFrame();
}
	
void VideoSender::setVideoSource(VideoSource *source)
{
	if(m_source == source)
		return;
		
	if(m_source)
		disconnectVideoSource();
	
	m_source = source;
	if(m_source)
	{	
		connect(m_source, SIGNAL(frameReady()), this, SLOT(frameReady()));
		connect(m_source, SIGNAL(destroyed()), this, SLOT(disconnectVideoSource()));
		
		//qDebug() << "GLVideoDrawable::setVideoSource(): "<<objectName()<<" m_source:"<<m_source;
		//setVideoFormat(m_source->videoFormat());
		m_consumerRegistered = false;
		
		frameReady();
	}
	else
	{
		qDebug() << "VideoSender::setVideoSource(): "<<this<<" Source is NULL";
	}

}

void VideoSender::disconnectVideoSource()
{
	if(!m_source)
		return;
	m_source->release(this);
	disconnect(m_source, 0, this, 0);
	m_source = 0;
}


void VideoSender::frameReady()
{
	if(!m_source)
		return;
	
	VideoFramePtr frame = m_source->frame();
	if(!frame || !frame->isValid())
	{
		//qDebug() << "VideoSender::frameReady(): Invalid frame or no frame";
		return;
	}
	
	m_frame = frame;
	
	if(m_transmitFps <= 0)
		processFrame();
}


void VideoSender::incomingConnection(int socketDescriptor)
{
	VideoSenderThread *thread = new VideoSenderThread(socketDescriptor, m_adaptiveWriteEnabled);
	connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
	connect(this, SIGNAL(receivedFrame()), thread, SLOT(frameReady()), Qt::QueuedConnection);
	thread->moveToThread(thread);
	thread->setSender(this);
	thread->start();
	
	if(!m_consumerRegistered)
	{
		m_consumerRegistered = true;
		m_source->registerConsumer(this);
	}
		
		
	//qDebug() << "VideoSender: "<<this<<" Client Connected, Socket Descriptor:"<<socketDescriptor;
}


/** Thread **/

VideoSenderThread::VideoSenderThread(int socketDescriptor, bool adaptiveWriteEnabled, QObject *parent)
    : QThread(parent)
    , m_socketDescriptor(socketDescriptor)
    , m_adaptiveWriteEnabled(adaptiveWriteEnabled)
    , m_sentFirstHeader(false)
    , m_blockSize(0)
{
	//connect(m_sender, SIGNAL(destroyed()),    this, SLOT(quit()));
}

VideoSenderThread::~VideoSenderThread()
{
	m_sender = 0;
	
	m_socket->abort();
	delete m_socket;
	m_socket = 0;
}

void VideoSenderThread::setSender(VideoSender *s)
{
	m_sender = s;
}

void VideoSenderThread::run()
{
	m_socket = new QTcpSocket();
	connect(m_socket, SIGNAL(disconnected()), this, SLOT(deleteLater()));
	connect(m_socket, SIGNAL(readyRead()), 	  this, SLOT(dataReady()));
	
	if (!m_socket->setSocketDescriptor(m_socketDescriptor)) 
	{
		emit error(m_socket->error());
		return;
	}
	
	qDebug() << "VideoSenderThread: Connection from "<<m_socket->peerAddress().toString(); //, Socket Descriptor:"<<socketDescriptor;
	
	
	// enter event loop
	exec();
	
	// when frameReady() signal arrives, write data with header to socket
}


void VideoSenderThread::frameReady()
{
	if(!m_socket)
		return;
		
	if(!m_sender)
		return;
		
	
	//QTime time = QTime::currentTime();
	//QImage image = *tmp;
	static int frameCounter = 0;
 	frameCounter++;
//  	qDebug() << "VideoSenderThread: [START] Writing Frame#:"<<frameCounter;
	
	if(m_adaptiveWriteEnabled && m_socket->bytesToWrite() > 0)
	{
		//qDebug() << "VideoSenderThread::frameReady():"<<m_socket->bytesToWrite()<<"bytes pending write on socket, not sending image"<<frameCounter;
	}
	else
	{
		m_sender->sendLock();
		
		QSize originalSize = m_sender->origSize(); //scaledSize.isEmpty() ? (xmitFrame ? xmitFrame->size() : QSize()) : (origFrame ? origFrame->size() : QSize());
		QSharedPointer<uchar> dataPtr = m_sender->dataPtr();
		if(dataPtr)
		{
					
			QTime time = m_sender->captureTime();
			int timestamp = time.hour()   * 60 * 60 * 1000 +
					time.minute() * 60 * 1000      + 
					time.second() * 1000           +
					time.msec();
			
			int byteCount = m_sender->byteCount();
			
			#define HEADER_SIZE 256
			
			if(!m_sentFirstHeader)
			{
				m_sentFirstHeader = true;
				char headerData[HEADER_SIZE];
				memset(&headerData, 0, HEADER_SIZE);
				sprintf((char*)&headerData,"%d",byteCount);
				//qDebug() << "header data:"<<headerData;
				
				m_socket->write((const char*)&headerData,HEADER_SIZE);
			}
			
			if(byteCount > 0)
			{
				QSize imageSize = m_sender->imageSize();
				
				char headerData[HEADER_SIZE];
				memset(&headerData, 0, HEADER_SIZE);
				
				sprintf((char*)&headerData,
							"%d " // byteCount
							"%d " // w
							"%d " // h
							"%d " // pixelFormat
							"%d " // image.format
							"%d " // bufferType
							"%d " // timestamp
							"%d " // holdTime
							"%d " // original size X
							"%d", // original size Y
							byteCount, 
							imageSize.width(), 
							imageSize.height(),
							(int)m_sender->pixelFormat(),
							(int)m_sender->imageFormat(),
							(int)VideoFrame::BUFFER_IMAGE,
							timestamp, 
							m_sender->holdTime(),
							originalSize.width(), 
							originalSize.height());
				//qDebug() << "VideoSenderThread::frameReady: "<<this<<" header data:"<<headerData;
				
				m_socket->write((const char*)&headerData,HEADER_SIZE);
				m_socket->write((const char*)dataPtr.data(),byteCount);
			}
	
			m_socket->flush();
			
		}
		
		m_sender->sendUnlock();
	}

}

void VideoSenderThread::sendMap(QVariantMap map)
{
	//qDebug() << "VideoSenderThread::sendMap: "<<map;
	
	QByteArray array;
	QDataStream stream(&array, QIODevice::WriteOnly);
	stream << map;
	
	int byteCount = array.size();
	if(byteCount > 0)
	{
		char headerData[HEADER_SIZE];
		memset(&headerData, 0, HEADER_SIZE);
		
		sprintf((char*)&headerData,
					"%d " // byteCount
					"%d " // w
					"%d " // h
					"%d " // pixelFormat
					"%d " // image.format
					"%d " // bufferType
					"%d " // timestamp
					"%d " // holdTime
					"%d " // original size X
					"%d", // original size Y
					byteCount, 
					-1, -1, -1,
					-1, -1, -1, 
					-1, -1, -1);
		//qDebug() << "VideoSenderThread::frameReady: header data:"<<headerData;
		
		m_socket->write((const char*)&headerData,HEADER_SIZE);
		m_socket->write(array);
	}

	m_socket->flush();
}

void VideoSenderThread::sendReply(QVariantList reply)
{
	QVariantMap map;
	if(reply.size() % 2 != 0)
	{
		qDebug() << "VideoSenderThread::sendReply: [WARNING]: Odd number of elelements in reply: "<<reply;
	}
	
	for(int i=0; i<reply.size(); i+=2)
	{
		if(i+1 >= reply.size())
			continue;
		
		QString key = reply[i].toString();
		QVariant value = reply[i+1];
		
		map[key] = value;
	}
	
	
	//qDebug() << "VideoSenderThread::sendReply: [DEBUG] map:"<<map;
	sendMap(map);
}

void VideoSenderThread::dataReady()
{
	if (m_blockSize == 0) 
	{
		char data[256];
		int bytes = m_socket->readLine((char*)&data,256);
		
		if(bytes == -1)
			qDebug() << "VideoSenderThread::dataReady: Could not read line from socket";
		else
			sscanf((const char*)&data,"%d",&m_blockSize);
		//qDebug() << "VideoSenderThread::dataReady: Read:["<<data<<"], size:"<<m_blockSize;
		//log(QString("[DEBUG] GLPlayerClient::dataReady(): blockSize: %1 (%2)").arg(m_blockSize).arg(m_socket->bytesAvailable()));
	}
	
	if (m_socket->bytesAvailable() < m_blockSize)
	{
		//qDebug() << "VideoSenderThread::dataReady: Bytes avail:"<<m_socket->bytesAvailable()<<", block size:"<<m_blockSize<<", waiting for more data";
		return;
	}
	
	m_dataBlock = m_socket->read(m_blockSize);
	m_blockSize = 0;
	
	if(m_dataBlock.size() > 0)
	{
		//qDebug() << "Data ("<<m_dataBlock.size()<<"/"<<m_blockSize<<"): "<<m_dataBlock;
		//log(QString("[DEBUG] GLPlayerClient::dataReady(): dataBlock: \n%1").arg(QString(m_dataBlock)));

		processBlock();
	}
	else
	{
		//qDebug() << "VideoSenderThread::dataReady: Didnt read any data from m_socket->read()";
	}
	
	
	if(m_socket->bytesAvailable())
	{
		QTimer::singleShot(0, this, SLOT(dataReady()));
	}
}

void VideoSenderThread::processBlock()
{
// 	bool ok;
	QDataStream stream(&m_dataBlock, QIODevice::ReadOnly);
	QVariantMap map;
	stream >> map;
	
	if(!m_sender)
	{
		qDebug() << "VideoSenderThread::processBlock: m_sender went away, can't process";
		return;
	}
	
	QString cmd = map["cmd"].toString();
	//qDebug() << "VideoSenderThread::processBlock: map:"<<map;
	
	if(cmd == Video_SetHue ||
	   cmd == Video_SetSaturation ||
	   cmd == Video_SetBright ||
	   cmd == Video_SetContrast)
	{
		//qDebug() << "VideoSenderThread::processBlock: Color command:"<<cmd;
		VideoSource *source = m_sender->videoSource();
		CameraThread *camera = dynamic_cast<CameraThread*>(source);
		if(!camera)
		{
			// error
			qDebug() << "VideoSenderThread::processBlock: "<<cmd<<": Video source is not a video input class ('CameraThread'), unable to determine system device to adjust."; 
			return;
		}
		
		/// TODO: The setting of BCHS should be done inside CamereaThread instead of here!
		
		QString colorCmd = cmd == Video_SetHue		? "hue" :
				   cmd == Video_SetSaturation	? "color" :
				   cmd == Video_SetBright	? "bright" :
				   cmd == Video_SetContrast	? "contrast" : "";
		
		if(colorCmd.isEmpty())
		{
			// error
			qDebug() << "VideoSenderThread::processBlock: "<<cmd<<": Unknown color command.";
			return;
		}
			
		QString device = camera->inputName();
		int value = map["value"].toInt();
		
		if(value > 100)
			value = 100;
		if(value < 0)
			value = 0;
		
		QString shellCommand = QString("v4lctl -c %1 %2 %3%").arg(device).arg(colorCmd).arg(value);
			
		qDebug() << "VideoSenderThread::processBlock: "<<cmd<<": Executing shell command: "<<shellCommand;
			
		system(qPrintable(shellCommand));
	}
	else
	if(cmd == Video_GetHue ||
	   cmd == Video_GetSaturation ||
	   cmd == Video_GetBright ||
	   cmd == Video_GetContrast)
	{
		//qDebug() << "VideoSenderThread::processBlock: Color command:"<<cmd;
		VideoSource *source = m_sender->videoSource();
		CameraThread *camera = dynamic_cast<CameraThread*>(source);
		if(!camera)
		{
			// error
			qDebug() << "VideoSenderThread::processBlock: "<<cmd<<": Video source is not a video input class ('CameraThread'), unable to determine system device to adjust."; 
			return;
		}
		
		/// TODO: The Getting of BCHS should be done inside CamereaThread instead of here!
		
		QString colorCmd = cmd == Video_GetHue		? "hue" :
				   cmd == Video_GetSaturation	? "color" :
				   cmd == Video_GetBright	? "bright" :
				   cmd == Video_GetContrast	? "contrast" : "";
		
		if(colorCmd.isEmpty())
		{
			// error
			qDebug() << "VideoSenderThread::processBlock: "<<cmd<<": Unknown color command.";
			return;
		}
			
		QString device = camera->inputName();
		
		QString program = "v4lctl";
		QStringList args = QStringList() << "-c" << device << "list";
		QProcess proc;
		proc.start(program, args);
		proc.waitForFinished();
		QByteArray rawData = proc.readAllStandardOutput();
		QString output(rawData);
		QStringList outputLines = output.split("\n");
		
		//qDebug() << "VideoSenderThread::processBlock: "<<cmd<<": args:"<<args<<", raw output:"<<output; 
		
		int colorValue = -1;
		foreach(QString line, outputLines)
		{
			qDebug() << "VideoSenderThread::processBlock: "<<cmd<<": "<<colorCmd<<": checking line:"<<line;
			QStringList values = line.split("|");
			if(values.length() >= 5)
			{
				QString key = values.at(0).trimmed();
				qDebug() << "VideoSenderThread::processBlock: "<<cmd<<": "<<colorCmd<<": \t checking key:"<<key;
				if(key == colorCmd)
				{
					int value = values.at(2).trimmed().toInt();
					
					QString comment = values.at(4).trimmed();
					QStringList commentParts = comment.split(">");
					int rangeTop = commentParts.last().trimmed().toInt();
					
					colorValue = (int)( (((double)value) / ((double)rangeTop)) * 100 );
					
					qDebug() << "VideoSenderThread::processBlock: "<<cmd<<": "<<colorCmd<<": \t \t found key, value:"<<value<<", rangeTop:"<<rangeTop<<", colorValue:"<<colorValue;
				}
			}
		}
		
		sendReply(QVariantList() 
			<< "cmd"	<< cmd
			<< "value"	<< colorValue);
	}
	else
	if(cmd == Video_SetFPS)
	{
		int fps = map["fps"].toInt();
		if(fps < 1)
			fps = 1;
		if(fps > 60)
			fps = 60;
		qDebug() << "VideoSenderThread::processBlock: "<<cmd<<": Setting fps:"<<fps;
		
		m_sender->setTransmitFps(fps);
	}
	else
	if(cmd == Video_GetFPS)
	{
		int fps = m_sender->transmitFps();
		qDebug() << "VideoSenderThread::processBlock: "<<cmd<<": Getting fps:"<<fps;
		
		sendReply(QVariantList() << "cmd" << cmd << "value" << fps);
	}
	else
	if(cmd == Video_SetSize)
	{
		int w = map["w"].toInt();
		int h = map["h"].toInt();
		
		QSize originalSize = m_sender->origSize();
		if(w > originalSize.width())
			w = originalSize.width();
		if(h > originalSize.height())
			h = originalSize.height();
			
		if(w < 16)
			w = 16;
		if(h < 16)
			h = 16;
			
		originalSize.scale(w,h,Qt::KeepAspectRatio);
		qDebug() << "VideoSenderThread::processBlock: "<<cmd<<": Setting size:"<<originalSize;
		
		m_sender->setTransmitSize(originalSize);
	}
	else
	if(cmd == Video_GetSize)
	{
		QSize size = m_sender->transmitSize();
		qDebug() << "VideoSenderThread::processBlock: "<<cmd<<": Getting size:"<<size;
		
		sendReply(QVariantList() << "cmd" << cmd << "w" << size.width() << "h" << size.height());
	}
	else
	{
		// Unknown Command
		qDebug() << "VideoSenderThread::processBlock: "<<cmd<<": Unknown command.";
	}
}

