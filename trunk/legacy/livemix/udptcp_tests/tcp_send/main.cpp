#include "ImageServer.h"

#include <QApplication>
int main(int argc, char **argv)
{
	QApplication app(argc,argv);
	
	ImageServer server;
	
	int listenPort = 7755;
	if (!server.listen(QHostAddress::Any,listenPort))
	{
		qDebug() << "ImageServer could not start on port"<<listenPort<<": "<<server.errorString();
		return -1;
	}
	
	QImage image("../huesat640.jpg");
	server.runTestImage(image, 35); 
	
	return app.exec();
}
