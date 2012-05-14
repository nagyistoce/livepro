#ifndef VariantMapClient_H
#define VariantMapClient_H


#include <QObject>
#include <QByteArray>
#include <QTcpSocket>
#include <QPointer>

//#include "VariantMapServer.h"

class VariantMapClient : public QObject
{
	Q_OBJECT
public:
	VariantMapClient(QObject *parent = 0);
	~VariantMapClient();
	
// 	void setLogger(MainWindow*);
	//void setInstance(OutputInstance*);
	bool connectTo(const QString& host, int port);
	QString errorString(){ return m_socket->errorString(); }
	
	bool waitForData(int msec) { return m_socket ? m_socket->waitForReadyRead(msec) : false;  }
	bool waitForWrite(int msec) { return m_socket ? m_socket->waitForBytesWritten(msec) : false;  }

public slots:
	void sendMap(const QVariantMap& map);
        void exit();

signals:
	//void aspectRatioChanged(double);
	void socketDisconnected();
	void socketError(QAbstractSocket::SocketError);
	void socketConnected();
	
	void receivedMap(QVariantMap);
	
private slots:
	void dataReady();
	void processBlock();
	//void processCommand(OutputServer::Command, QVariant, QVariant, QVariant);
	
	
	
protected:
// 	void cmdSetSlideGroup(const QVariant &,int);
// 	void cmdAddfilter(int);
// 	void cmdDelFilter(int);
// 	void cmdSetOverlaySlide(const QVariant &);
// 	void cmdSetLiveBackground(const QString &, bool);
// 	void cmdSetSlideObject(const QVariant &);

private:
	void log(const QString&);
        QPointer<QTcpSocket> m_socket;
	
	int m_blockSize;
	QByteArray m_dataBlock;
};

#endif
