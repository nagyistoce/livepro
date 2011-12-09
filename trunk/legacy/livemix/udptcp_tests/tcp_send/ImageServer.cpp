#include "ImageServer.h"

#include <QNetworkInterface>
#include <QTime>

#include "../../SimpleV4L2.h"

ImageServer::ImageServer(QObject *parent)
	: QTcpServer(parent)
	, m_imageProvider(0)
	, m_signalName(0)
	, m_adaptiveWriteEnabled(true)
	, m_v4l2(0)
{
	connect(&m_testTimer, SIGNAL(timeout()), this, SLOT(emitTestImage()));
}
	
void ImageServer::setProvider(QObject *provider, const char * signalName)
{
	m_imageProvider = provider;
	m_signalName    = signalName;
}

void ImageServer::incomingConnection(int socketDescriptor)
{
	ImageServerThread *thread = new ImageServerThread(socketDescriptor, m_adaptiveWriteEnabled);
	connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
	connect(m_imageProvider, m_signalName, thread, SLOT(imageReady(const QImage&, const QTime&)), Qt::QueuedConnection);
	thread->moveToThread(thread);
	thread->start();
	qDebug() << "ImageServer: Client Connected, Socket Descriptor:"<<socketDescriptor;
}

void ImageServer::emitTestImage()
{
	
	
	if(m_v4l2)
	{
		VideoFrame f = m_v4l2->readFrame();
		if(f.isValid())
			m_frame = f;
		
		//qglClearColor(Qt::black);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
		QImage image((const uchar*)m_frame.byteArray.constData(),m_frame.size.width(),m_frame.size.height(),QImage::Format_RGB32);
		//emit testImage(image.scaled(160,120), m_frame.captureTime);
		emit testImage(image, QTime::currentTime()); //m_frame.captureTime);
	}
	else
	{
		emit testImage(m_testImage, QTime::currentTime());	
	}
		
}

void ImageServer::runTestImage(const QImage& image, int fps)
{
	qDebug() << "ImageServer::runTestImage: Running"<<image.byteCount()<<"byte image at"<<fps<<"fps";
	m_testImage = image;
	m_testTimer.setInterval(1000 / fps);
	m_testTimer.start();
	setProvider(this, SIGNAL(testImage(const QImage&, const QTime&)));
	
	
		m_v4l2 = new SimpleV4L2();
		QString cameraFile = "/dev/video0";
		if(m_v4l2->openDevice(qPrintable(cameraFile)))
		{
			setObjectName(qPrintable(cameraFile));
// 			// Do the code here so that we dont have to open the device twice
// 			if(!setInput("Composite"))
// 				if(!setInput("Composite1"))
// 					setInput(0);

			m_v4l2->initDevice();
			m_v4l2->startCapturing();
// 			m_inited = true;
			
// 			qDebug() << "CameraThread::initCamera(): finish2";

// 			return 1;
		}
}

/** Thread **/
#define BOUNDARY "ImageServerThread-uuid-0eda9b03a8314df4840c97113e3fe160"
#include <QImageWriter>
#include <QImage>

ImageServerThread::ImageServerThread(int socketDescriptor, bool adaptiveWriteEnabled, QObject *parent)
    : QThread(parent)
    , m_socketDescriptor(socketDescriptor)
    , m_adaptiveWriteEnabled(adaptiveWriteEnabled)
    , m_httpJpegServerMode(false)
    , m_sentFirstHeader(false)
{
	
}

ImageServerThread::~ImageServerThread()
{
	m_socket->abort();
	delete m_socket;
}

void ImageServerThread::run()
{
	m_socket = new QTcpSocket();
	
	if (!m_socket->setSocketDescriptor(m_socketDescriptor)) 
	{
		emit error(m_socket->error());
		return;
	}
	
	writeHeaders();
	
	m_writer.setDevice(m_socket);
	m_writer.setFormat("jpg");
	//m_writer.setQuality(80);
	
	// enter event loop
	exec();
	
	// when imageReady() signal arrives, write data with header to socket
}

void ImageServerThread::writeHeaders()
{
	if(m_httpJpegServerMode)
	{
		m_socket->write("HTTP/1.0 200 OK\r\n");
		m_socket->write("Server: DViz/ImageServer - Josiah Bryan <josiahbryan@gmail.com>\r\n");
		m_socket->write("Content-Type: multipart/x-mixed-replace;boundary=" BOUNDARY "\r\n");
		m_socket->write("\r\n");
		m_socket->write("--" BOUNDARY "\r\n");
	}
}

void ImageServerThread::imageReady(const QImage& image, const QTime& time)
{
	
	//QTime time = QTime::currentTime();
	int timestamp = time.hour()   * 60 * 60 * 1000 +
			time.minute() * 60 * 1000      + 
			time.second() * 1000           +
			time.msec();
	

	//QImage image = *tmp;
	static int frameCounter = 0;
 	frameCounter++;
//  	qDebug() << "ImageServerThread: [START] Writing Frame#:"<<frameCounter;
	
	if(m_adaptiveWriteEnabled && m_socket->bytesToWrite() > 0)
	{
		qDebug() << "ImageServerThread::imageReady():"<<m_socket->bytesToWrite()<<"bytes pending write on socket, not sending image"<<frameCounter;
	}
	else
	{
		if(m_httpJpegServerMode)
		{
			QImage copy = image;
			if(copy.format() != QImage::Format_RGB32)
				copy = copy.convertToFormat(QImage::Format_RGB32);
			
			if(m_socket->state() == QAbstractSocket::ConnectedState)
			{
				m_socket->write("Content-type: image/jpeg\r\n\r\n");
			}
			
			if(!m_writer.write(copy))
			{
				qDebug() << "ImageWriter reported error:"<<m_writer.errorString();
				quit();
			}
			
			m_socket->write("--" BOUNDARY "\r\n");
		}
		else
		{
			QImage copy = image;
			//if(copy.format() != QImage::Format_RGB32)
			//	copy = copy.convertToFormat(QImage::Format_RGB32);
			
			int byteCount = copy.byteCount();
			
// 			char headerData[512];
// 			sprintf((char*)&headerData,"//ImageServer// %d %d\n",byteCount,timestamp);
// 			m_socket->write((const char*)&headerData);
// 			qDebug() << "header data:"<<headerData;
			
			if(!m_sentFirstHeader)
			{
				m_sentFirstHeader = true;
				char headerData[256];
				memset(&headerData, 0, 256);
				// X Y Format Timestamp
				sprintf((char*)&headerData,"%d",copy.byteCount());
				//qDebug() << "header data:"<<headerData;
				
				m_socket->write((const char*)&headerData,256);
			}
			
			char headerData[256];
			memset(&headerData, 0, 256);
			// X Y Format Timestamp
 			sprintf((char*)&headerData,"%d %d %d %d %d",copy.byteCount(),copy.width(),copy.height(),(int)copy.format(),timestamp);
 			//qDebug() << "header data:"<<headerData;
			
			m_socket->write((const char*)&headerData,256);
 			
			const uchar *bits = (const uchar*)copy.bits();
			m_socket->write((const char*)bits,byteCount);
			m_socket->flush();
			
			QTime time2 = QTime::currentTime();
			int timestamp2 = time.hour() + time.minute() + time.second() + time.msec();
			//qDebug() << "write time: "<<(timestamp2-timestamp);
			
		}
	}

}

