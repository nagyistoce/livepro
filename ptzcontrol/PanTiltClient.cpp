#include <QtGui>
#include <QtNetwork>

#include "PanTiltClient.h"

PanTiltClient::PanTiltClient(bool verbose, QObject *parent)
	: QObject(parent)
	, m_socket(0)
	, m_connected(false)
	, m_host("")
	, m_port(0)
	, m_verbose(verbose)
	, m_autoReconnect(true)
	, m_lastPan(-1)
	, m_lastTilt(-1)
	, m_lastZoom(-1)
{
}

PanTiltClient::~PanTiltClient()
{
	if(m_socket)
		exit();
		
	//quit();
	//wait();
}



bool PanTiltClient::start(const QString& host, int port)
{
	QString ipAddress = host;
	
	// find out which IP to connect to
	if(ipAddress.isEmpty())
	{
		QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
		
		// use the first non-localhost IPv4 address
		for (int i=0; i < ipAddressesList.size(); ++i) 
		{
			if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
				ipAddressesList.at(i).toIPv4Address()) 
			{
				ipAddress = ipAddressesList.at(i).toString();
				break;
			}
		}
	}
	
	// if we did not find one, use IPv4 localhost
	if (ipAddress.isEmpty())
		ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
	
	m_host = ipAddress;
	m_port = port;

	if(m_socket)
	{
		disconnect(m_socket, 0, this, 0);
		m_dataBlock.clear();
		
		m_socket->abort();
		m_socket->disconnectFromHost();
		//m_socket->waitForDisconnected();
		m_socket->deleteLater();
// 		delete m_socket;
		m_socket = 0;
	}
	
	m_socket = new QTcpSocket(this);
	connect(m_socket, SIGNAL(readyRead()),    this,   SLOT(dataReady()));
	connect(m_socket, SIGNAL(disconnected()), this,   SLOT(lostConnection()));
	connect(m_socket, SIGNAL(disconnected()), this, SIGNAL(socketDisconnected()));
	connect(m_socket, SIGNAL(connected()),    this, SIGNAL(socketConnected()));
	connect(m_socket, SIGNAL(connected()),    this,   SLOT(connectionReady()));
	connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SIGNAL(socketError(QAbstractSocket::SocketError)));
	connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(lostConnection(QAbstractSocket::SocketError)));
	connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(emitError(QAbstractSocket::SocketError)));
	
	m_socket->connectToHost(ipAddress, port);
	
	return true;
}


void PanTiltClient::connectionReady()
{
	//qDebug() << "Connected";
	m_connected = true;
	
	emit connected();
	emit connectionStatusChanged(m_connected);
}


void PanTiltClient::lostConnection()
{
	m_connected = false;
	emit connectionStatusChanged(false);
	
	if(m_autoReconnect)
	{
		if(m_verbose)
			qDebug() << "PanTiltClient::lostSonnection: Lost server, attempting to reconnect in 1sec";
		QTimer::singleShot(1000,this,SLOT(reconnect()));
	}
	else
	{
		exit();
	}
}

void PanTiltClient::lostConnection(QAbstractSocket::SocketError error)
{
	//qDebug() << "PanTiltClient::lostConnection("<<error<<"):" << m_socket->errorString();
	
	if(error == QAbstractSocket::ConnectionRefusedError)
		lostConnection();
}


void PanTiltClient::reconnect()
{
	//log(QString("Attempting to reconnect to %1:%2%3").arg(m_host).arg(m_port).arg(m_url));
	if(m_port > 0)
		start(m_host,m_port);
}


void PanTiltClient::dataReady()
{
	if(!m_connected)
	{
		m_dataBlock.clear();
		return;
	}
	QByteArray bytes = m_socket->readAll();
	//qDebug() << "PanTiltClient::dataReady(): Reading from socket:"<<m_socket<<", read:"<<bytes.size()<<" bytes"; 
	if(bytes.size() > 0)
	{
		m_dataBlock.append(bytes);
//		qDebug() << "dataReady(), read bytes:"<<bytes.count()<<", buffer size:"<<m_dataBlock.size();
		
		processBlock();
	}
}

void PanTiltClient::processBlock()
{
	if(!m_connected)
		return;
	
	QString line = m_dataBlock;
	line = line.replace("\r","");
	if(line.indexOf("\n") > 0)
	{
		QStringList lines = line.split("\n");
		//qDebug() << "PanTiltClient: Received line: "<<line.left(line.length()-1);
		foreach(QString item, lines)
		{
			item = item.replace("\n","");
			if(!item.isEmpty())
			{
				//
				if(item.startsWith("accl:"))
				{
					item = item.replace("accl:","");
					QStringList values = item.split(",");
					//qDebug() << "values:"<<values<<", size:"<<values.size();
					if(values.size() >= 3)
					{
						int x = values[0].toInt(),
						    y = values[1].toInt(),
						    z = values[2].toInt();
						//qDebug() << "Accel: "<<x<<y<<z;
						emit accelDataRx(x,y,z);
					}
				}
				else
					qDebug() << "\t " << item;
			}
		}
		
		m_dataBlock.clear();
	}
	return;
}

void PanTiltClient::setPan(int v)
{
	QString str = QString("%1u").arg(v);
	//qDebug() << "PanTiltClient::setPan(): "<<str;
	m_socket->write(str.toAscii());
	sendRaw(str);
	m_lastPan = v;
}

void PanTiltClient::setTilt(int v)
{
	QString str = QString("%1s").arg(v);
	//qDebug() << "PanTiltClient::setTilt(): "<<str;
	//m_socket->write(str.toAscii());
	sendRaw(str);
	m_lastTilt = v;
}
	
void PanTiltClient::setZoom(int v)
{
	QString str = QString("%1w").arg(v);
	//qDebug() << "PanTiltClient::setZoom(): "<<str;
	//m_socket->write(str.toAscii());
	sendRaw(str);
	m_lastZoom = v;
}

void PanTiltClient::sendRaw(const QString& raw)
{
	qDebug() << "PanTiltClient::sendRaw(): "<<raw;
	m_socket->write(raw.toAscii());
	//m_socket->waitForBytesWritten();
}

void PanTiltClient::exit()
{
	if(m_socket)
	{
		if(m_verbose)
			qDebug() << "PanTiltClient::exit: Discarding old socket:"<<m_socket;
		disconnect(m_socket,0,this,0);
		m_dataBlock.clear();
		
		//qDebug() << "PanTiltClient::exit: Quiting video receivier";
		m_socket->abort();
		m_socket->disconnectFromHost();
		//m_socket->waitForDisconnected();
		m_socket->deleteLater();
		//delete m_socket;
		m_socket = 0;
	}
}


void PanTiltClient::emitError(QAbstractSocket::SocketError socketError)
{
	switch (socketError) 
	{
		case QAbstractSocket::RemoteHostClosedError:
			break;
		case QAbstractSocket::HostNotFoundError:
			emit error(tr("The host was not found. Please check the "
						"host name and port settings."));
			break;
		case QAbstractSocket::ConnectionRefusedError:
			emit error(tr("The connection was refused by the peer. "
						"Make sure the server is running, "
						"and check that the host name and port "
						"settings are correct."));
			break;
		default:
			emit error(tr("The following error occurred: %1.")
						.arg(m_socket->errorString()));
	}
	
}
