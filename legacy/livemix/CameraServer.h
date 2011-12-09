#ifndef CameraServer_H
#define CameraServer_H

#include <QTcpServer>
#include <QThread>
#include <QTcpSocket>
#include <QImageWriter>
#include <QBuffer>
#include <QImage>

class CameraServer : public QTcpServer
{
	Q_OBJECT
	
public:
	CameraServer(QObject *parent = 0);
	
	void setAdaptiveWriteEnabled(bool flag) { m_adaptiveWriteEnabled = flag; }
	bool adaptiveWriteEnabled() { return m_adaptiveWriteEnabled; }
	
	void setProvider(QObject *provider, const char * signalName);
	
// 	QString myAddress();

protected:
	void incomingConnection(int socketDescriptor);

private:
	QObject * m_imageProvider;
	const char * m_signalName;
	bool m_adaptiveWriteEnabled;

};


class QImage;
class CameraServerThread : public QThread
{
    Q_OBJECT

public:
	CameraServerThread(int socketDescriptor, bool adaptiveWriteEnabled, QObject *parent = 0);
	~CameraServerThread();
	
	void run();

signals:
	void error(QTcpSocket::SocketError socketError);

public slots:
	void imageReady(QImage);

private:
	int m_socketDescriptor;
	QTcpSocket * m_socket;
	
	QByteArray m_array;
	QBuffer m_buffer;
	QImageWriter m_writer;
	bool m_adaptiveWriteEnabled;
	
};


#endif //CameraServer_H

