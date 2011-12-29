#include "VariantMapServer.h"

// #include "model/SlideGroup.h"
// #include "model/Slide.h"

#include <QNetworkInterface>
#include <QMutexLocker>

// #include "model/Document.h"
// #include "MainWindow.h"

VariantMapServer::VariantMapServer(QObject *parent)
	: QTcpServer(parent)
{
// 	qRegisterMetaType<VariantMapServer::Command>("VariantMapServer::Command");
	qRegisterMetaType<QVariant>("QVariant");

}

QString VariantMapServer::myAddress()
{
	QString ipAddress;

	QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
	// use the first non-localhost IPv4 address
	for (int i = 0; i < ipAddressesList.size(); ++i)
	{
		if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
		    ipAddressesList.at(i).toIPv4Address())
			ipAddress = ipAddressesList.at(i).toString();
	}

	// if we did not find one, use IPv4 localhost
	if (ipAddress.isEmpty())
		ipAddress = QHostAddress(QHostAddress::LocalHost).toString();

	return QString("dviz://%1:%2/").arg(ipAddress).arg(serverPort());
}

void VariantMapServer::incomingConnection(int socketDescriptor)
{
	VariantMapServerThread *thread = new VariantMapServerThread(socketDescriptor);
	//connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
	connect(this, SIGNAL(commandReady(QVariantMap)), thread, SLOT(sendMap(QVariantMap))); //, Qt::QueuedConnection);
	connect(thread, SIGNAL(receivedMap(QVariantMap)), this, SLOT(notifyMap(QVariantMap))); //, Qt::QueuedConnection);

	thread->run();
	qDebug() << "VariantMapServer: Client Connected, Socket Descriptor:"<<socketDescriptor;
}

void VariantMapServer::sendMap(QVariantMap map)
{
	//qDebug() << "VariantMapServer::sendMap: "<<map;
	emit commandReady(map);
}

void VariantMapServer::notifyMap(QVariantMap map)
{
	emit receivedMap(map);
}

/** Thread **/

VariantMapServerThread::VariantMapServerThread(int socketDescriptor, QObject *parent)
    //: QThread(parent)
    : QObject(parent)
    , m_socketDescriptor(socketDescriptor)
    , m_blockSize(0)
{
}

VariantMapServerThread::~VariantMapServerThread()
{
	m_socket->disconnectFromHost();
	//m_socket->waitForDisconnected();

	delete m_socket;
}

void VariantMapServerThread::run()
{
	m_socket = new QTcpSocket();

	if (!m_socket->setSocketDescriptor(m_socketDescriptor))
	{
		emit error(m_socket->error());
		return;
	}

	//connect(m_socket, SIGNAL(readyRead()), 		this, SLOT(socketDataReady()));
	connect(m_socket, SIGNAL(readyRead()), 		this, SLOT(dataReady()));
	connect(m_socket, SIGNAL(disconnected()), 	this, SLOT(deleteLater()));
	//connect(m_socket, SIGNAL(connected()), 		this, SLOT(socketConnected()));
	//connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
}

void VariantMapServerThread::sendMap(QVariantMap map)
{
	//qDebug() << "VariantMapServerThread::sendMap: "<<map;
	
	QByteArray array;
	QDataStream stream(&array, QIODevice::WriteOnly);
	stream << map;

	char data[256];
	sprintf(data, "%d\n", array.size());
	m_socket->write((const char*)&data,strlen((const char*)data));
	//qDebug() << "VariantMapServerThread::sendMap: header block size: "<<strlen((const char*)data)<<", header data:"<<data;

	m_socket->write(array);
}




void VariantMapServerThread::dataReady()
{
	if (m_blockSize == 0) 
	{
		char data[256];
		int bytes = m_socket->readLine((char*)&data,256);
		
		if(bytes == -1)
			qDebug() << "VariantMapClient::dataReady: Could not read line from socket";
		else
			sscanf((const char*)&data,"%d",&m_blockSize);
		//qDebug() << "Read:["<<data<<"], size:"<<m_blockSize;
		//log(QString("[DEBUG] VariantMapClient::dataReady(): blockSize: %1 (%2)").arg(m_blockSize).arg(m_socket->bytesAvailable()));
	}
	
	if (m_socket->bytesAvailable() < m_blockSize)
		return;
	
	m_dataBlock = m_socket->read(m_blockSize);
	m_blockSize = 0;
	
	if(m_dataBlock.size() > 0)
	{
		//qDebug() << "Data ("<<m_dataBlock.size()<<"/"<<m_blockSize<<"): "<<m_dataBlock;
		//log(QString("[DEBUG] VariantMapClient::dataReady(): dataBlock: \n%1").arg(QString(m_dataBlock)));

		processBlock();
	}
	
	
	if(m_socket->bytesAvailable())
	{
		QTimer::singleShot(0, this, SLOT(dataReady()));
	}
}

void VariantMapServerThread::processBlock()
{
	bool ok;
	QDataStream stream(&m_dataBlock, QIODevice::ReadOnly);
	QVariantMap map;
	stream >> map;
	
	emit receivedMap(map);
/*
	VariantMapServer::Command cmd = (VariantMapServer::Command)map["cmd"].toInt();
	//qDebug() << "VariantMapClient::processBlock: cmd#:"<<cmd;

	processCommand(cmd,map["v1"],map["v2"],map["v3"]);*/
}
