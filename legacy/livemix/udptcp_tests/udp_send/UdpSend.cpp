#include "UdpSend.h"

#define DEBUG 1

UdpSend::UdpSend()
{
	m_socket = new QUdpSocket(this);
}

UdpSend::~UdpSend()
{}

	
void UdpSend::init(const QHostAddress& address, quint16 port)
{
	m_address = address;
	m_port = port;
}	
	
void UdpSend::sendImage(const QImage& image, const QTime& time)
{
	// must split image data into chunks of <= MAX_DATAGRAM_SIZE
	
	const uchar *bits = (const uchar*)image.bits();
	int byteCount = image.byteCount();
	int numPackets = byteCount / MAX_DATAGRAM_SIZE;
	
	char headerData[MAX_DATAGRAM_SIZE];
	int timestamp = time.hour() + time.minute() + time.second() + time.msec();
	sprintf((char*)&headerData,"%s %d %d %d\n",HEADER_PACKET,byteCount,numPackets,timestamp);
	m_socket->writeDatagram((const char*)&headerData, strlen((const char*)headerData), m_address, m_port);
	
	#ifdef DEBUG
	qDebug() << "UdpSend::sendImage(): header:"<<headerData;
	int packetNum = 0;
	int bytesWritten = 0;
	#endif
	
	int ptrPos = 0;
	int pktSize = MAX_DATAGRAM_SIZE > byteCount ? byteCount : MAX_DATAGRAM_SIZE;
	while(ptrPos + pktSize < byteCount)
	{
		m_socket->writeDatagram((const char*)bits, pktSize, m_address, m_port); 
		
		pktSize = ptrPos + MAX_DATAGRAM_SIZE <= byteCount ? MAX_DATAGRAM_SIZE : byteCount - ptrPos;
		ptrPos += pktSize;
		bits   += pktSize;
		
		#ifdef DEBUG
		bytesWritten += pktSize;
		qDebug() << "UdpSend::sendImage(): writing packet "<<packetNum<<"/"<<numPackets<<", bytes written:"<<bytesWritten<<"/"<<byteCount<<", pktSize:"<<pktSize;
		packetNum++;
		#endif
		
	}
	
	m_socket->flush();
	
	//m_socket->writeDatagram(FOOTER_PACKET, strlen(FOOTER_PACKET), m_address, m_port);
}

