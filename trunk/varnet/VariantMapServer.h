#ifndef VariantMapServer_H
#define VariantMapServer_H

#include <QTcpServer>
#include <QThread>
#include <QTcpSocket>
#include <QVariant>


class OutputInstance;
class SlideGroup;
class Slide;

class VariantMapServer : public QTcpServer
{
	Q_OBJECT

public:
	VariantMapServer(QObject *parent = 0);

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
Q_DECLARE_METATYPE(VariantMapServer::Command);*/


#include <QMutex>
#include <QTimer>

class VariantMapServerThread : public QObject //QThread
{
    Q_OBJECT

public:
	VariantMapServerThread(int socketDescriptor, QObject *parent = 0);
	~VariantMapServerThread();

	void run();

signals:
	void error(QTcpSocket::SocketError socketError);
	void receivedMap(QVariantMap);
	
public slots:
	//void queueCommand(VariantMapServer::Command,QVariant,QVariant,QVariant);
	void sendMap(QVariantMap);

private slots:
	//void processCommandBuffer();
	//void sendCommand(VariantMapServer::Command,QVariant,QVariant,QVariant);
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

