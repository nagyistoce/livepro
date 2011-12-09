#include "UdpSend.h"

#include <QApplication>
int main(int argc, char **argv)
{
	QApplication app(argc,argv);
	
	UdpSend sender;
	sender.init(QHostAddress::LocalHost, 7755);
	QImage image("../huesat640.jpg");
	while(true)
	{
		sender.sendImage(image,QTime::currentTime());
		usleep(2 * 1000 * 1000);
	}
}
