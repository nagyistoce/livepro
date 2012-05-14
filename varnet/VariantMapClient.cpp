//#include "OutputInstance.h"
#include "VariantMapClient.h"

#include <QColor>
#include <QVariant>
#include <QFileInfo>
#include <QMessageBox>

/*#include "model/Slide.h"
#include "model/SlideGroup.h"
#include "songdb/SongSlideGroup.h"*/


#include "VariantMapServer.h" // VariantMapServer::Command enum is needed

VariantMapClient::VariantMapClient(QObject *parent)
	: QObject(parent)
	, m_socket(0)
// 	, m_log(0)
	//, m_inst(0)
	, m_blockSize(0)
	, m_dataBlock("")
{
}
VariantMapClient::~VariantMapClient()
{
}

// void VariantMapClient::setLogger(MainWindow *log)
// {
// 	m_log = log;
// }

// void VariantMapClient::setInstance(OutputInstance *inst)
// {
// 	m_inst = inst;
// }

bool VariantMapClient::connectTo(const QString& host, int port)
{
	if(m_socket)
		m_socket->abort();

	m_socket = new QTcpSocket(this);
	connect(m_socket, SIGNAL(readyRead()), this, SLOT(dataReady()));
        connect(m_socket, SIGNAL(disconnected()), this, SIGNAL(exit()));
        connect(m_socket, SIGNAL(disconnected()), this, SIGNAL(socketDisconnected()));
	connect(m_socket, SIGNAL(connected()), this, SIGNAL(socketConnected()));
	connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SIGNAL(socketError(QAbstractSocket::SocketError)));

	m_blockSize = 0;
	m_socket->connectToHost(host,port);

	return true;
}

void VariantMapClient::log(const QString& str)
{
	qDebug() << "VariantMapClient::log(): "<<str;
// 	if(m_log)
// 		m_log->log(str);
}


void VariantMapClient::sendMap(const QVariantMap& map)
{
	//QByteArray json = m_stringy->serialize(map);
	QByteArray array;
	QDataStream stream(&array, QIODevice::WriteOnly);
	stream << map;

	//qDebug() << "VariantMapServerThread: Send Map:"<<map<<", JSON:"<<json;

	char data[256];
	sprintf(data, "%d\n", array.size());
	m_socket->write((const char*)&data,strlen((const char*)data));
	//qDebug() << "block size: "<<strlen((const char*)data)<<", data:"<<data;

	m_socket->write(array);
}

void VariantMapClient::dataReady()
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
		//qDebug() << "VariantMapClient::dataReady(): blockSize: "<<m_blockSize<<", bytes avail: "<<m_socket->bytesAvailable();
	}

	if (m_socket->bytesAvailable() < m_blockSize)
		return;

	m_dataBlock = m_socket->read(m_blockSize);
	m_blockSize = 0;

	if(m_dataBlock.size() > 0)
	{
		//qDebug() << "Data ("<<m_dataBlock.size()<<"/"<<m_blockSize<<"): "<<m_dataBlock;
		//qDebug() << "VariantMapClient::dataReady(): dataBlock: " << m_dataBlock;

		processBlock();
	}

	if(m_socket &&
	   m_socket->bytesAvailable())
	{
		QTimer::singleShot(0, this, SLOT(dataReady()));
	}
}

void VariantMapClient::processBlock()
{
	QDataStream stream(&m_dataBlock, QIODevice::ReadOnly);
	QVariantMap map;
	stream >> map;

	//qDebug() << "VariantMapClient::processBlock: map: "<<map;

	emit receivedMap(map);
/*
	VariantMapServer::Command cmd = (VariantMapServer::Command)map["cmd"].toInt();
	//qDebug() << "VariantMapClient::processBlock: cmd#:"<<cmd;

	processCommand(cmd,map["v1"],map["v2"],map["v3"]);*/
}

// void VariantMapClient::processCommand(VariantMapServer::Command cmd, QVariant a, QVariant b, QVariant c)
// {
// 	if(!m_inst)
// 		return;
//
// 	switch(cmd)
// 	{
// 		case VariantMapServer::SetSlideGroup:
// 			log("[INFO] Downloading new Slide Group from Server ...");
// 			cmdSetSlideGroup(a,b.toInt());
// 			break;
// 		case VariantMapServer::SetSlide:
// 			log(QString("[INFO] Changing to Slide # %1").arg(a.toInt()));
// 			m_inst->setSlide(a.toInt());
// 			break;
// 		case VariantMapServer::AddFilter:
// 			log(QString("[INFO] Added Filter # %1").arg(a.toInt()));
// 			cmdAddfilter(a.toInt());
// 			break;
// 		case VariantMapServer::DelFilter:
// 			log(QString("[INFO] Removed Filter # %1").arg(a.toInt()));
// 			cmdDelFilter(a.toInt());
// 			break;
// // 		case VariantMapServer::FadeClear:
// // 			log(QString("[INFO] Background-Only (\"Clear\") Frame %1").arg(a.toBool() ? "On":"Off"));
// // 			m_inst->fadeClearFrame(a.toBool());
// // 			break;
// 		case VariantMapServer::FadeBlack:
// 			log(QString("[INFO] Black Frame %1").arg(a.toBool() ? "On":"Off"));
// 			m_inst->fadeBlackFrame(a.toBool());
// 			break;
// 		case VariantMapServer::SetBackgroundColor:
// 			m_inst->setBackground(QColor(a.toString()));
// 			break;
// 		case VariantMapServer::SetOverlaySlide:
// 			log(QString("[INFO] Downloading new Overlay Slide from Server..."));
// 			cmdSetOverlaySlide(a);
// 			break;
// 		case VariantMapServer::SetLiveBackground:
// 			cmdSetLiveBackground(a.toString(),b.toBool());
// 			break;
// 		case VariantMapServer::SetTextResize:
// 			log(QString("[INFO] Set Text Resize: %1").arg(a.toBool()));
// 			m_inst->setAutoResizeTextEnabled(a.toBool());
// 			break;
// 		case VariantMapServer::SetFadeSpeed:
// 			log(QString("[INFO] Set Fade Speed to %1 ms").arg(a.toInt()));
// 			m_inst->setFadeSpeed(a.toInt());
// 			break;
// 		case VariantMapServer::SetFadeQuality:
// 			log(QString("[INFO] Set Fade Quality to %1 fps").arg(a.toInt()));
// 			m_inst->setFadeQuality(a.toInt());
// 			break;
// 		case VariantMapServer::SetAspectRatio:
// 			log(QString("[INFO] Set Aspect Ratio %1").arg(a.toDouble()));
// 			emit aspectRatioChanged(a.toDouble());
// 			break;
// 		case VariantMapServer::SetSlideObject:
// 			log(QString("[INFO] Downloading new out-of-group Slide from Server..."));
// 			cmdSetSlideObject(a);
// 			break;
// 		default:
// 			qDebug() << "Command Not Handled: "<<(int)cmd;
// 			log(QString("[DEBUG] Unknown Command: '%1'").arg(cmd));
// 	};
// }
//
// void VariantMapClient::cmdSetSlideGroup(const QVariant& var, int start)
// {
// 	QByteArray ba = var.toByteArray();
// 	SlideGroup * group = SlideGroup::fromByteArray(ba);
// 	if(group)
// 	{
// 		if(group->groupNumber()<0)
// 			group->setGroupNumber(1);
//
// 		log(QString("[INFO] Slide Group # %1 (\"%2\") Downloaded, Showing on Live Output").arg(group->groupNumber()).arg(group->groupTitle()));
//
// 		m_inst->setSlideGroup(group,start);
//
// 		//log(QString("[DEBUG] Slide Group Display routines completed."));
//
// 	}
// }
//
// void VariantMapClient::cmdAddfilter(int id)
// {
// 	AbstractItemFilter * filter = AbstractItemFilter::filterById(id);
// 	if(filter)
// 		m_inst->addFilter(filter);
// 	else
// 		log(QString("[ERROR] Add Filter: Server requested filter# %1 which is not installed in this viewer. This may mean that the server is newer than the client. You may want to check for an update to the DViz viewer on the DViz website.").arg(id));
// }
//
// void VariantMapClient::cmdDelFilter(int id)
// {
// 	AbstractItemFilter * filter = AbstractItemFilter::filterById(id);
// 	if(filter)
// 		m_inst->removeFilter(filter);
// 	else
// 		log(QString("[ERROR] Remove Filter: Server requested filter# %1 which is not installed in this viewer. This may mean that the server is newer than the client. You may want to check for an update to the DViz viewer on the DViz website.").arg(id));
// }
//
// void VariantMapClient::cmdSetOverlaySlide(const QVariant& var)
// {
// 	QByteArray ba = var.toByteArray();
// 	Slide * slide = new Slide();
// 	slide->fromByteArray(ba);
//
// 	log(QString("[INFO] Overlay slide downloaded, showing on Live Output"));
// 	m_inst->setOverlaySlide(slide);
// }
//
// void VariantMapClient::cmdSetSlideObject(const QVariant& var)
// {
// 	QByteArray ba = var.toByteArray();
// 	Slide * slide = new Slide();
// 	slide->fromByteArray(ba);
//
// 	log(QString("[INFO] New out-of-group slide downloaded, showing on Live Output"));
// 	m_inst->setSlide(slide);
// }
//
// void VariantMapClient::cmdSetLiveBackground(const QString& file, bool flag)
// {
// 	m_inst->setLiveBackground(QFileInfo(file),flag);
// }
//

void VariantMapClient::exit()
{
	if(m_socket)
	{
		//qDebug() << "VariantMapClient::exit(): Closing and deleting socket.";
		m_socket->abort();
		m_socket->disconnectFromHost();
		//m_socket->waitForDisconnected();
		m_socket->deleteLater();
		m_socket = 0;
	}
}

