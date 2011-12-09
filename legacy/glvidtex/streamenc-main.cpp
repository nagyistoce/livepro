#include <QApplication>

#include "StreamEncoderProcess.h"

int main(int argc, char *argv[])
{

	QApplication app(argc, argv);
	
	qApp->setApplicationName("GLStreamEncoder");
	qApp->setOrganizationName("Josiah Bryan");
	qApp->setOrganizationDomain("mybryanlife.com");
	
	StreamEncoderProcess proc;// = new StreamEncoderProcess();
	//Q_UNUSED(proc);
	
	//QTimer::singleShot(15000, &app, SLOT(quit())); 
	
	return app.exec();
}

