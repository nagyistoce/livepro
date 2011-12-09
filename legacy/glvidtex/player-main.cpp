#include <QApplication>

#include "GLWidget.h"
#include "GLDrawables.h"
#include "MetaObjectUtil.h"
#include "PlayerWindow.h"

#include "QtGetOpt.h"

int main(int argc, char *argv[])
{

	QApplication app(argc, argv);

	// construct class from command line arguments
	GetOpt opts(argc, argv);

	bool verbose = false;
	opts.addSwitch("verbose", &verbose);

	// add some switches
	QString configFile;
	opts.addOptionalOption('c',"config", &configFile, "player.ini");

	// do the parsing and check for errors
	if (!opts.parse())
	{
			fprintf(stderr,"Usage: %s [--verbose] [-c|--config configfile]\n - If no config file specified, it defaults to 'player.ini'\n", qPrintable(opts.appName()));
			return 1;
	}

	if(configFile.isEmpty())
			configFile = "player.ini";

	qApp->setApplicationName("GLPlayer");
	qApp->setOrganizationName("Josiah Bryan");
	qApp->setOrganizationDomain("mybryanlife.com");

	MetaObjectUtil_Register(GLImageDrawable);
	MetaObjectUtil_Register(GLTextDrawable);
	MetaObjectUtil_Register(GLVideoFileDrawable);
	MetaObjectUtil_Register(GLVideoInputDrawable);
	MetaObjectUtil_Register(GLVideoLoopDrawable);
	MetaObjectUtil_Register(GLVideoReceiverDrawable);


	PlayerWindow *win = new PlayerWindow();
	win->loadConfig(configFile, verbose);
	win->show();

	return app.exec();
}

