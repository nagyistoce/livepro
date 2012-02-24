#include <QtGui/QApplication>

#include "OutputWindow.h"
#include "ControlWindow.h"

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	app.setApplicationName("DVizRearView");
	app.setOrganizationName("Josiah Bryan");
	app.setOrganizationDomain("mybryanlife.com");
	
	OutputWindow *mw = new OutputWindow();
	mw->show();
	
	ControlWindow *ctrl = new ControlWindow();
	ctrl->setOutputWindow(mw);
	ctrl->show();
	
	return app.exec();
}

