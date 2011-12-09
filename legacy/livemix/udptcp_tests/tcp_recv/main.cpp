#include "ImageReceiver.h"

#include <QApplication>
int main(int argc, char **argv)
{
	QApplication app(argc,argv);
	
	ImageReceiver rec;
	if(!rec.connectTo("localhost", 7755))
	{
		qDebug() << "Unable to connect: "<<rec.errorString();
	}
	
	return app.exec();
}
