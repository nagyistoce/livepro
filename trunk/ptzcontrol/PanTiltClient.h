#ifndef PanTiltClient_H
#define PanTiltClient_H

#include <QDialog>
#include <QTcpSocket>

class QTcpSocket;

class PanTiltClient : public QObject
{
	Q_OBJECT

public:
	PanTiltClient(bool verbose=false, QObject *parent = 0);
	~PanTiltClient();
	
	bool start(const QString& host="", int port=3729);
	void exit();
	
	bool isConnected() { return m_connected; }
	
	int lastPan() { return m_lastPan; }
	int lastTilt() { return m_lastTilt; }
	int lastZoom() { return m_lastZoom; }

signals:
	void accelDataRx(int x, int y, int z);
	
	void socketDisconnected();
	void socketError(QAbstractSocket::SocketError);
	void error(QString);	
	void socketConnected();
	void connected();
	void connectionStatusChanged(bool isConnected);

public slots:
	void setPan(int);
	void setTilt(int);
	void setZoom(int);
	
	void sendRaw(const QString& raw);

private slots:
	void dataReady();
	void processBlock();
	void lostConnection();
	void lostConnection(QAbstractSocket::SocketError);
	void reconnect();
	void connectionReady();

	void emitError(QAbstractSocket::SocketError socketError);

private:
	QTcpSocket *m_socket;
	QByteArray m_dataBlock;
	bool m_connected;
	QString m_host;
	int m_port;
	bool m_verbose;
	bool m_autoReconnect;
	int m_lastPan;
	int m_lastTilt;
	int m_lastZoom;

};

#endif
