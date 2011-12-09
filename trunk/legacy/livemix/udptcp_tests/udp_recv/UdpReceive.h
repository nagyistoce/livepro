#ifndef UdpReceive_H
#define UdpReceive_H

#include <QUdpSocket>
#include <QtGui>

// Max 512 bytes per http://doc.qt.nokia.com/4.6/qudpsocket.html#writeDatagram
#define MAX_DATAGRAM_SIZE 512

#define HEADER_PACKET "0x0UdpSend0x0"

#define FOOTER_PACKET "1x1UdpSend1x1"

class TestLabel : public QLabel
{
	Q_OBJECT
public:
	TestLabel();
public slots:
	void imageReady(const QImage&, int timestamp);

};

class UdpReceive : public QObject
{
	Q_OBJECT
public:
	UdpReceive();
	~UdpReceive();
	
	void init(const QHostAddress& address, quint16 port);
	
	int msecTo(int timestamp);

signals:
	void imageReady(const QImage& image, int timestamp);
	
private slots:
	void readPendingDatagrams();
	void processBuffer();
	void processDatagram(const QByteArray&);

private:
	QUdpSocket *m_socket;
	QHostAddress m_address;
	quint16 m_port;
	QByteArray m_buffer;
	
	int m_timestamp;
	int m_byteCount;
	int m_numPackets;
	int m_packetCounter;
	int m_byteCounter;
	

};

#endif
