#ifndef VideoSender_H
#define VideoSender_H

#include <QTcpServer>
#include <QThread>
#include <QTcpSocket>
#include <QImageWriter>
#include <QTimer>
#include <QImage>
#include <QMutex>

class SimpleV4L2;
#include "VideoFrame.h"
#include "VideoSource.h"



class VideoSender : public QTcpServer
{
	Q_OBJECT
	
public:
	VideoSender(QObject *parent = 0);
	~VideoSender();
	
	void setAdaptiveWriteEnabled(bool flag) { m_adaptiveWriteEnabled = flag; }
	bool adaptiveWriteEnabled() { return m_adaptiveWriteEnabled; }
	
	VideoSource *videoSource() { return m_source; }
// 	VideoFramePtr frame();
// 	VideoFramePtr scaledFrame();
	QSharedPointer<uchar> dataPtr() { return m_dataPtr; }
	int byteCount() { return m_byteCount; }
	QImage::Format imageFormat() { return m_imageFormat; }
	QSize imageSize() { return m_imageSize; }
	QSize origSize() { return m_origSize; }
	QVideoFrame::PixelFormat pixelFormat() { return m_pixelFormat; }
	int holdTime() { return m_holdTime; }
	QTime captureTime() { return m_captureTime; }
	
	void sendLock() { m_sendMutex.lock(); }
	void sendUnlock() { m_sendMutex.unlock(); }
	
	void setVideoSource(VideoSource *source);
 	
 	static QString ipAddress();

	int transmitFps() { return m_transmitFps; }
	QSize transmitSize() { return m_transmitSize; }
	
	int start(int port=-1); // -1 = auto-allocate
		
signals: 
	void receivedFrame();
	void signalStatusChanged(bool);
	void customSignal(QString key, QVariant value);
	
public slots:
	void disconnectVideoSource();
	void setTransmitFps(int fps=-1); // auto fps based on source if -1
	void setTransmitSize(const QSize& size=QSize()); // null size means auto size based on input
	void setTransmitSize(int x, int y) { setTransmitSize(QSize(x,y)); }
	
	void sendCustomSignal(QString key, QVariant value);
	
	void transmitImage(QImage image);

private slots:
	void frameReady();
	void processFrame();
	
protected:
	void incomingConnection(int socketDescriptor);
	
private:
	bool m_adaptiveWriteEnabled;
	VideoSource *m_source;
	VideoFramePtr m_frame;
// 	VideoFramePtr m_scaledFrame;
	QSharedPointer<uchar> m_dataPtr;
	int m_byteCount;
	QImage::Format m_imageFormat;
	QSize m_imageSize;
	QSize m_origSize;
	QVideoFrame::PixelFormat m_pixelFormat;
	int m_holdTime;
	QTime m_captureTime;
	QSize m_transmitSize;
	int m_transmitFps;
	QTimer m_fpsTimer;
	QMutex m_frameMutex;
	QMutex m_sendMutex;
	
	bool m_consumerRegistered;
	
	static int m_videoSenderPortAllocator;
};


class QImage;
class VideoSenderThread : public QThread
{
    Q_OBJECT

public:
	VideoSenderThread(int socketDescriptor, bool adaptiveWriteEnabled, QObject *parent = 0);
	~VideoSenderThread();
	
	void run();
	void setSender(VideoSender*);

signals:
	void error(QTcpSocket::SocketError socketError);

public slots:
	void frameReady();
	void signalStatusChanged(bool);
	void customSignal(QString key, QVariant value);
	
protected slots:
	void dataReady();
	
protected:
	void processBlock();
	void sendMap(QVariantMap map);
	void sendReply(QVariantList reply);

private:
	int m_socketDescriptor;
	QTcpSocket * m_socket;
	
	bool m_adaptiveWriteEnabled;
	bool m_sentFirstHeader;
	VideoSender *m_sender;

	int m_blockSize;
	QByteArray m_dataBlock;

};


#endif //VideoSender_H

