#include <QApplication>

#include "PlayerCommandLineInterface.h"

int main(int argc, char *argv[])
{

	QApplication app(argc, argv);
	
	qApp->setApplicationName("GLPlayerCommand");
	qApp->setOrganizationName("Josiah Bryan");
	qApp->setOrganizationDomain("mybryanlife.com");
	
	PlayerCommandLineInterface proc;// = new StreamEncoderProcess();
	//Q_UNUSED(proc);
	
	//QTimer::singleShot(15000, &app, SLOT(quit())); 
	
	return app.exec();
}

