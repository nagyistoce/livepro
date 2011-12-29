#ifndef TESTSERVER_H
#define TESTSERVER_H

#include "HttpServer.h"

class TestServer : public HttpServer
{
	Q_OBJECT
public:
	TestServer(quint16 port, QObject* parent = 0);
	
protected:
	void dispatch(QTcpSocket *socket, const QStringList &pathElements, const QStringMap &query);

};

#endif

