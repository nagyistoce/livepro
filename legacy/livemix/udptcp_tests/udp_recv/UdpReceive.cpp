#include "UdpReceive.h"

#define DEBUG 1

TestLabel::TestLabel()
	: QLabel() {}
	
void TestLabel::imageReady(const QImage& image, int)
{
	setPixmap(QPixmap::fromImage(image));
}

UdpReceive::UdpReceive()
{
	m_socket = new QUdpSocket(this);
	connect(m_socket, SIGNAL(readyRead()),
		this, SLOT(readPendingDatagrams()));

}

UdpReceive::~UdpReceive()
{}

	
void UdpReceive::init(const QHostAddress& address, quint16 port)
{
	m_socket->bind(address,port);
	m_address = address;
	m_port = port;
	
	m_packetCounter = 0;
	m_byteCounter = 0;
	m_buffer.clear();	
}	

void UdpReceive::readPendingDatagrams()
{
	qDebug() << "UdpReceive::readPendingDatagrams()";
		
	while (m_socket->hasPendingDatagrams()) 
	{
		QByteArray datagram;
		datagram.resize(m_socket->pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;
	
		m_socket->readDatagram(datagram.data(), datagram.size(),
					&sender, &senderPort);
	
		processDatagram(datagram);
	}
}

void UdpReceive::processDatagram(const QByteArray& datagram)
{
	if(datagram.startsWith(HEADER_PACKET))
	{
		const char *data = datagram.constData();
		char tmp[100];
		sscanf(data,"%s %d %d %d",(char*)&tmp,&m_byteCount,&m_numPackets,&m_timestamp);
		m_packetCounter = 0;
		m_byteCounter = 0;
		m_buffer.clear();
		
		#ifdef DEBUG
		qDebug() << "UdpReceive::processDatagram(): Header data: byteCount:"<<m_byteCount<<", numPackets:"<<m_numPackets<<", timestamp:"<<m_timestamp<<", raw data:"<<data;
		#endif
	}
	else
	{
		m_buffer.append(datagram);
		m_byteCounter += datagram.size();
		m_packetCounter ++;
		
		#ifdef DEBUG
		qDebug() << "UdpReceive::processDatagram(): Body data: bytes:"<<m_byteCounter<<"/"<<m_byteCount<<", packets:"<<m_packetCounter<<"/"<<m_numPackets;
		#endif
		
		if(m_packetCounter >= m_numPackets || m_byteCounter >= m_byteCount)
			processBuffer();
	}
}

void UdpReceive::processBuffer()
{
	QImage image = QImage::fromData(m_buffer);
	#ifdef DEBUG
	qDebug() << "UdpReceive::processBuffer(): latency: "<<msecTo(m_timestamp);
	#endif
	emit imageReady(image,m_timestamp);
}

int UdpReceive::msecTo(int timestamp)
{
	QTime time = QTime::currentTime();
	int currentTimestamp = time.hour() + time.minute() + time.second() + time.msec();
	return currentTimestamp - timestamp;
}
