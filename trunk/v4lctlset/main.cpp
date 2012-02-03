#include "SimpleV4L2.h"
#include <QCoreApplication>
#include <QDebug>

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	
	if(argc < 2)
	{
		qDebug() << "Usage: "<<argv[0]<<" <videoDevice> [list|set] [controlId] [value]";
		exit(-1);
	}
	
	QString device = argv[1];
	QString cmd    = argc >= 3 ? argv[2] : "list";
	 
	if(cmd == "set" && argc < 5)
	{
		qDebug() << "Usage: "<<argv[0]<< device <<"set <controlId> <value>";
		exit(-1);
	}
	
// 	MainWindow *mw = new MainWindow;
// 	mw->show();
	SimpleV4L2 *dev = new SimpleV4L2();
	if(!dev->openDevice(qPrintable(device)))
	{
		qDebug() << "Error opening "<<device<<", exiting.";
		delete dev;
		exit(-1);
	}
	
	if(cmd == "list")
	{
		
		QList<SimpleV4L2::ControlData> controls = dev->enumerateControls();
		
		foreach(SimpleV4L2::ControlData data, controls)
		{
			qDebug() << data.id << data.name << data.value;
		}
	}
	else
	if(cmd == "set")
	{
		QString idStr = argv[3];
		int id = idStr.toInt();
		
		QString valueStr = argv[4];
		int value = valueStr.toInt();
		
		SimpleV4L2::ControlData setData;
		setData.id    = id;
		setData.value = value;
		if(dev->setControl(setData))
		{
			qDebug() << "Set control "<<id<<" to "<<value;
		}
		else
		{
			qDebug() << "Error setting control "<< id <<" to "<< value<<", exiting.";
			delete dev;
			exit(-1);
		}
	}
	else
	{
		qDebug() << "Unknown command "<<cmd;
	}
	
	delete dev;
	exit(0);
	
	//return app.exec();
}
