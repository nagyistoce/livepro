#include <QApplication>

#include "VideoInputColorBalancer.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	
	qApp->setApplicationName("GLInputBalance");
	qApp->setOrganizationName("Josiah Bryan");
	qApp->setOrganizationDomain("mybryanlife.com");
	
	VideoInputColorBalancer *win = new VideoInputColorBalancer();
	//win->adjustSize();
	win->show();

	return app.exec();
}

