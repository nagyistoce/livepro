#ifndef GLPlayerServer_H
#define GLPlayerServer_H

#include <QTcpServer>
#include <QThread>
#include <QTcpSocket>
#include <QVariant>


class OutputInstance;
class SlideGroup;
class Slide;

class GLPlayerServer : public QTcpServer
{
	Q_OBJECT

public:
	GLPlayerServer(QObject *parent = 0);

	QString myAddress();

public slots:
	void sendMap(QVariantMap);

signals:
	void receivedMap(QVariantMap);
/* internal */
	void commandReady(QVariantMap);

private slots:
	void notifyMap(QVariantMap);
	
protected:
	void incomingConnection(int socketDescriptor);

// 	OutputInstance *m_inst;
};
/*
Q_DECLARE_METATYPE(GLPlayerServer::Command);*/


#include <QMutex>
#include <QTimer>

class GLPlayerServerThread : public QObject //QThread
{
    Q_OBJECT

public:
	GLPlayerServerThread(int socketDescriptor, QObject *parent = 0);
	~GLPlayerServerThread();

	void run();

signals:
	void error(QTcpSocket::SocketError socketError);
	void receivedMap(QVariantMap);
	
public slots:
	//void queueCommand(GLPlayerServer::Command,QVariant,QVariant,QVariant);
	void sendMap(QVariantMap);

private slots:
	//void processCommandBuffer();
	//void sendCommand(GLPlayerServer::Command,QVariant,QVariant,QVariant);
	//void socketError(QAbstractSocket::SocketError);
	//void socketConnected();
	//void socketDisconnected();
	//void socketDataReady();
	
	void dataReady();
	void processBlock();

private:
// 	void sendMap(const QVariantMap &);

	int m_socketDescriptor;
	QTcpSocket * m_socket;

	int m_blockSize;
	QByteArray m_dataBlock;
};


#endif //JpegServer_H

