#ifndef ImageServer_H
#define ImageServer_H

#include <QTcpServer>
#include <QThread>
#include <QTcpSocket>
#include <QImageWriter>
#include <QTimer>
#include <QImage>

class SimpleV4L2;
#include "../../VideoFrame.h"


class ImageServer : public QTcpServer
{
	Q_OBJECT
	
public:
	ImageServer(QObject *parent = 0);
	
	void setAdaptiveWriteEnabled(bool flag) { m_adaptiveWriteEnabled = flag; }
	bool adaptiveWriteEnabled() { return m_adaptiveWriteEnabled; }
	
	void setProvider(QObject *provider, const char * signalName);
	
	void runTestImage(const QImage&, int fps=10);
	
// 	QString myAddress();

protected:
	void incomingConnection(int socketDescriptor);
	
private slots:
	void emitTestImage();
	
signals:
	void testImage(const QImage&, const QTime&);

private:
	QObject * m_imageProvider;
	const char * m_signalName;
	bool m_adaptiveWriteEnabled;
	
	QImage m_testImage;
	QTimer m_testTimer;
	
	SimpleV4L2 *m_v4l2;
	VideoFrame m_frame;
	
};


class QImage;
class ImageServerThread : public QThread
{
    Q_OBJECT

public:
	ImageServerThread(int socketDescriptor, bool adaptiveWriteEnabled, QObject *parent = 0);
	~ImageServerThread();
	
	void run();

signals:
	void error(QTcpSocket::SocketError socketError);

public slots:
	void imageReady(const QImage&, const QTime&);

private:
	void writeHeaders();
	
	int m_socketDescriptor;
	QTcpSocket * m_socket;
	
	QByteArray m_boundary;
	QImageWriter m_writer;
	bool m_adaptiveWriteEnabled;
	bool m_httpJpegServerMode;
	bool m_sentFirstHeader;
};


#endif //ImageServer_H

