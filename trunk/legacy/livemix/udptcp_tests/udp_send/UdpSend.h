#ifndef UdpSend_H
#define UdpSend_H

#include <QUdpSocket>
#include <QtGui>

// Max 512 bytes per http://doc.qt.nokia.com/4.6/qudpsocket.html#writeDatagram
#define MAX_DATAGRAM_SIZE (63 * 1024)

#define HEADER_PACKET "0x0UdpSend0x0"

#define FOOTER_PACKET "1x1UdpSend1x1"

class UdpSend : public QObject
{
	Q_OBJECT
public:
	UdpSend();
	~UdpSend();
	
	void init(const QHostAddress& address, quint16 port);
	
public slots:
	void sendImage(const QImage& image, const QTime& time);

private:
	QUdpSocket *m_socket;
	QHostAddress m_address;
	quint16 m_port;

};

#endif

