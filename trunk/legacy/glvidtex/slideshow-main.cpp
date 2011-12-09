#include <QApplication>

#include "SlideShowWindow.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	
	qApp->setApplicationName("GLSlideShow");
	qApp->setOrganizationName("Josiah Bryan");
	qApp->setOrganizationDomain("mybryanlife.com");
	
	SlideShowWindow *win = new SlideShowWindow();
	win->show();

	return app.exec();
}

