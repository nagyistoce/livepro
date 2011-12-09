#include "UdpReceive.h"

#include <QApplication>
int main(int argc, char **argv)
{
	QApplication app(argc,argv);
	
	UdpReceive rec;
	rec.init(QHostAddress::LocalHost, 7755);
	
	TestLabel *label = new TestLabel;
	label->setWindowTitle("UDP Receive");
	
	QObject::connect(&rec, SIGNAL(imageReady(const QImage&, int)), label, SLOT(imageReady(const QImage&,int)));
	
	label->show();
	
	return app.exec();
}
