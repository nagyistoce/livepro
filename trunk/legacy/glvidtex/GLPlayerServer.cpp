#include "GLPlayerServer.h"

// #include "model/SlideGroup.h"
// #include "model/Slide.h"

#include <QNetworkInterface>
#include <QMutexLocker>

// #include "model/Document.h"
// #include "MainWindow.h"

GLPlayerServer::GLPlayerServer(QObject *parent)
	: QTcpServer(parent)
{
// 	qRegisterMetaType<GLPlayerServer::Command>("GLPlayerServer::Command");
	qRegisterMetaType<QVariant>("QVariant");

}

QString GLPlayerServer::myAddress()
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

void GLPlayerServer::incomingConnection(int socketDescriptor)
{
	GLPlayerServerThread *thread = new GLPlayerServerThread(socketDescriptor);
	//connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
	connect(this, SIGNAL(commandReady(QVariantMap)), thread, SLOT(sendMap(QVariantMap))); //, Qt::QueuedConnection);
	connect(thread, SIGNAL(receivedMap(QVariantMap)), this, SLOT(notifyMap(QVariantMap))); //, Qt::QueuedConnection);

	thread->run();
	qDebug() << "GLPlayerServer: Client Connected, Socket Descriptor:"<<socketDescriptor;
}

void GLPlayerServer::sendMap(QVariantMap map)
{
	//qDebug() << "GLPlayerServer::sendMap: "<<map;
	emit commandReady(map);
}

void GLPlayerServer::notifyMap(QVariantMap map)
{
	emit receivedMap(map);
}

/** Thread **/

GLPlayerServerThread::GLPlayerServerThread(int socketDescriptor, QObject *parent)
    //: QThread(parent)
    : QObject(parent)
    , m_socketDescriptor(socketDescriptor)
    , m_blockSize(0)
{
}

GLPlayerServerThread::~GLPlayerServerThread()
{
	m_socket->disconnectFromHost();
	//m_socket->waitForDisconnected();

	delete m_socket;
}

void GLPlayerServerThread::run()
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

void GLPlayerServerThread::sendMap(QVariantMap map)
{
	//qDebug() << "GLPlayerServerThread::sendMap: "<<map;
	
	QByteArray array;
	QDataStream stream(&array, QIODevice::WriteOnly);
	stream << map;

	char data[256];
	sprintf(data, "%d\n", array.size());
	m_socket->write((const char*)&data,strlen((const char*)data));
	//qDebug() << "GLPlayerServerThread::sendMap: header block size: "<<strlen((const char*)data)<<", header data:"<<data;

	m_socket->write(array);
}




void GLPlayerServerThread::dataReady()
{
	if (m_blockSize == 0) 
	{
		char data[256];
		int bytes = m_socket->readLine((char*)&data,256);
		
		if(bytes == -1)
			qDebug() << "GLPlayerClient::dataReady: Could not read line from socket";
		else
			sscanf((const char*)&data,"%d",&m_blockSize);
		//qDebug() << "Read:["<<data<<"], size:"<<m_blockSize;
		//log(QString("[DEBUG] GLPlayerClient::dataReady(): blockSize: %1 (%2)").arg(m_blockSize).arg(m_socket->bytesAvailable()));
	}
	
	if (m_socket->bytesAvailable() < m_blockSize)
		return;
	
	m_dataBlock = m_socket->read(m_blockSize);
	m_blockSize = 0;
	
	if(m_dataBlock.size() > 0)
	{
		//qDebug() << "Data ("<<m_dataBlock.size()<<"/"<<m_blockSize<<"): "<<m_dataBlock;
		//log(QString("[DEBUG] GLPlayerClient::dataReady(): dataBlock: \n%1").arg(QString(m_dataBlock)));

		processBlock();
	}
	
	
	if(m_socket->bytesAvailable())
	{
		QTimer::singleShot(0, this, SLOT(dataReady()));
	}
}

void GLPlayerServerThread::processBlock()
{
	bool ok;
	QDataStream stream(&m_dataBlock, QIODevice::ReadOnly);
	QVariantMap map;
	stream >> map;
	
	emit receivedMap(map);
/*
	GLPlayerServer::Command cmd = (GLPlayerServer::Command)map["cmd"].toInt();
	//qDebug() << "GLPlayerClient::processBlock: cmd#:"<<cmd;

	processCommand(cmd,map["v1"],map["v2"],map["v3"]);*/
}
