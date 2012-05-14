#include "MainWindow.h"
#include <QApplication>

int main(int argc, char **argv)
{
        qDebug() << "Starting app...";
	QApplication app(argc, argv);
        qDebug() << "Loading MainWindow...";
	
	MainWindow *mw = new MainWindow;
	mw->show();
	
	return app.exec();
}
