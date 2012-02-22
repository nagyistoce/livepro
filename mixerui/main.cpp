#include <QApplication>
#include <QCleanlooksStyle>
#include <QCDEStyle>
#include "DirectorWindow.h"

int main(int argc, char *argv[])
{

	QApplication app(argc, argv);
	
	qApp->setApplicationName("GLDirector");
	qApp->setOrganizationName("Josiah Bryan");
	qApp->setOrganizationDomain("mybryanlife.com");
	
	DirectorWindow *director = new DirectorWindow();
	director->show();

	return app.exec();
}

