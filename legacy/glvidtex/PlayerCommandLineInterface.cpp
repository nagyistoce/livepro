#include "PlayerCommandLineInterface.h"
#include "PlayerConnection.h"
#include "GLDrawable.h"
#include "GLSceneGroup.h"
#include <QApplication>
/*
class Command
	{
	public:
		virtual void execute() = 0;
		virtual QString name() = 0;
		virtual QString help() = 0;
	};*/

class CmdSetBlack : public PlayerCommandLineInterface::Command 
{
public:
	QString name() { return "setblack"; }
	bool execute(PlayerCommandLineInterface* api)
	{
		QString arg = api->shiftArgs().toLower();
		api->con()->fadeBlack(arg == "on");
		return true;
	};
	QString help() { return 
"  setblack (on|off)\n"
"        Command the player to fade to black (setblack on) or fade from\n"
"        black (setblack off) to the current scene."; }
};

class CmdListVideoInputs : public PlayerCommandLineInterface::Command 
{
	PlayerCommandLineInterface* m_api;
public:
	QString name() { return "listvid"; }
	bool execute(PlayerCommandLineInterface* api)
	{
		m_api = api;
		api->con()->requestVideoInputList();
		return false;
	}
	
	bool dataReceived()
	{
		bool recd = false;
		QStringList inputs = m_api->con()->videoInputs(&recd);
		
		if(!recd)
			return false;
		
		printInputs(inputs);
		
		return true;
	}
	
	void printInputs(QStringList inputs)
	{
		foreach(QString input, inputs)
			qDebug() << qPrintable(input);
	};
	
	QString help() { return 
"  listvid \n"
"        Print a list of video inputs available on the remote player."; }
};

class CmdSetXfadeSpeed : public PlayerCommandLineInterface::Command 
{
public:
	QString name() { return "setxfadespeed"; }
	bool execute(PlayerCommandLineInterface* api)
	{
		QString arg = api->shiftArgs().toLower();
		api->con()->setCrossfadeSpeed(arg.toInt());
		return true;
	};
	QString help() { return 
"  setxfadespeed (milliseconds)\n"
"        Set the crossfade speed in milliseconds."; }
};

class CmdListGroups : public PlayerCommandLineInterface::Command 
{
public:
	QString name() { return "listgroups"; }
	bool execute(PlayerCommandLineInterface* api)
	{
		QString arg = api->shiftArgs();
		GLSceneGroupCollection col;
		if(!col.readFile(arg))
		{
			qDebug() << qPrintable(name()) << ": Error reading collection file"<<arg;
			printHelp();
			return true; 
		}
		
		foreach(GLSceneGroup *group, col.groupList())
			//qDebug() << group->groupId() << "\"" << qPrintable(group->groupName().replace("\"","\\\"")) << "\"";
			qDebug() << group->groupId() << group->groupName();
			
		return true;
	};
	QString help() { return 
"  listgroups (collection file)\n"
"        List groups by name and ID in the given file."; }
};


class CmdListScenes : public PlayerCommandLineInterface::Command 
{
public:
	QString name() { return "listscenes"; }
	bool execute(PlayerCommandLineInterface* api)
	{
		QString arg = api->shiftArgs();
		GLSceneGroupCollection col;
		if(!col.readFile(arg))
		{
			qDebug() << qPrintable(name()) << ": Error reading collection file"<<arg;
			printHelp();
			return true; 
		}
		
		int groupId = api->shiftArgs().toInt();
		GLSceneGroup *group = col.lookupGroup(groupId);
		if(!group)
		{
			qDebug() << qPrintable(name()) << ": Invalid groupId"<<groupId;
			printHelp();
			return true;
		}
		
		foreach(GLScene *scene, group->sceneList())
			qDebug() << scene->sceneId() << scene->sceneName();
			
		return true;
	};
	QString help() { return 
"  listscenes (collection file) (groupid)\n"
"        List scenes by name and ID in the given file and the given \n"
"        groupid. Find the groupid by first using the 'listgroups' \n"
"        command (above.)"; }
};

class CmdSetGroup : public PlayerCommandLineInterface::Command 
{
public:
	QString name() { return "setgroup"; }
	bool execute(PlayerCommandLineInterface* api)
	{
		QString arg = api->shiftArgs();
		GLSceneGroupCollection col;
		if(!col.readFile(arg))
		{
			qDebug() << qPrintable(name()) << ": Error reading collection file"<<arg;
			printHelp();
			return true; 
		}
		
		int groupId = api->shiftArgs().toInt();
		GLSceneGroup *group = col.lookupGroup(groupId);
		if(!group)
		{
			qDebug() << qPrintable(name()) << ": Invalid groupId"<<groupId;
			printHelp();
			return true;
		}
		
		QString sceneIdTmp = api->shiftArgs();
		GLScene *scene = 0;
		if(!sceneIdTmp.isEmpty())
		{
			scene = group->lookupScene(sceneIdTmp.toInt());
			if(!scene)
			{
				qDebug() << qPrintable(name()) << ": Unable to find requested sceneId"<<sceneIdTmp<<", starting with first scene in group.";
			}
		}
		
		api->con()->setGroup(group, scene);
			
		return true;
	};
	QString help() { return 
"  setgroup (collection file) (groupid) [sceneid]\n"
"        Load group 'groupid' from the given file and send it to the\n"
"        player, optionally starting at scene 'sceneid'. If sceneid \n"
"        is not specified or is invalid, the player will start at the\n"
"        first scene in the given group."; }
};


class CmdSetScene : public PlayerCommandLineInterface::Command 
{
public:
	QString name() { return "setscene"; }
	bool execute(PlayerCommandLineInterface* api)
	{
		QString arg = api->shiftArgs();
		GLSceneGroupCollection col;
		if(!col.readFile(arg))
		{
			qDebug() << qPrintable(name()) << ": Error reading collection file"<<arg;
			printHelp();
			return true; 
		}
		
		int groupId = api->shiftArgs().toInt();
		GLSceneGroup *group = col.lookupGroup(groupId);
		if(!group)
		{
			qDebug() << qPrintable(name()) << ": Invalid groupId"<<groupId;
			printHelp();
			return true;
		}
		
		int sceneId = api->shiftArgs().toInt();
		GLScene *scene = group->lookupScene(sceneId);
		if(!group)
		{
			qDebug() << qPrintable(name()) << ": Invalid sceneId"<<sceneId;
			printHelp();
			return true;
		}
		
		api->con()->setScene(scene);
			
		return true;
	};
	QString help() { return 
"  setscene (collection file) (groupid) (sceneId)\n"
"        Send scene 'sceneId' to the player as the live scene."; }
};

class CmdListItems : public PlayerCommandLineInterface::Command 
{
public:
	QString name() { return "listitems"; }
	bool execute(PlayerCommandLineInterface* api)
	{
		QString arg = api->shiftArgs();
		GLSceneGroupCollection col;
		if(!col.readFile(arg))
		{
			qDebug() << qPrintable(name()) << ": Error reading collection file"<<arg;
			printHelp();
			return true; 
		}
		
		int groupId = api->shiftArgs().toInt();
		GLSceneGroup *group = col.lookupGroup(groupId);
		if(!group)
		{
			qDebug() << qPrintable(name()) << ": Invalid groupId"<<groupId;
			printHelp();
			return true;
		}
		
		int sceneId = api->shiftArgs().toInt();
		GLScene *scene = group->lookupScene(sceneId);
		if(!group)
		{
			qDebug() << qPrintable(name()) << ": Invalid sceneId"<<sceneId;
			printHelp();
			return true;
		}
		
		GLDrawableList list = scene->drawableList();
		if(list.isEmpty())
			qDebug() << "Warning: Found "<<list.size()<<"items in scene "<<sceneId;
			
		foreach(GLDrawable *gld, list)
		{
			QString typeName = gld->metaObject()->className();
			typeName = typeName.replace(QRegExp("^GL"),"");
			typeName = typeName.replace(QRegExp("Drawable$"),"");
			qDebug() << gld->id() << qPrintable(typeName) << gld->itemName();
		}
			
		return true;
	};
	QString help() { return 
"  listitems (collection file) (groupid) (sceneId)\n"
"        List items in the scene specified by 'sceneId' in 'groupId' in \n"
"        the given file. Find the groupid by first using the 'listgroups'\n"
"        command (above.)"; }
};

class CmdListItemProps : public PlayerCommandLineInterface::Command 
{
public:
	QString name() { return "listitemprops"; }
	bool execute(PlayerCommandLineInterface* api)
	{
		QString arg = api->shiftArgs();
		GLSceneGroupCollection col;
		if(!col.readFile(arg))
		{
			qDebug() << qPrintable(name()) << ": Error reading collection file"<<arg;
			printHelp();
			return true; 
		}
		
		int groupId = api->shiftArgs().toInt();
		GLSceneGroup *group = col.lookupGroup(groupId);
		if(!group)
		{
			qDebug() << qPrintable(name()) << ": Invalid groupId"<<groupId;
			printHelp();
			return true;
		}
		
		int sceneId = api->shiftArgs().toInt();
		GLScene *scene = group->lookupScene(sceneId);
		if(!group)
		{
			qDebug() << qPrintable(name()) << ": Invalid sceneId"<<sceneId;
			printHelp();
			return true;
		}
		
		int itemId = api->shiftArgs().toInt();
		GLDrawable *gld = scene->lookupDrawable(itemId);
		if(!gld)
		{
			qDebug() << qPrintable(name()) << ": Invalid itemId"<<itemId;
			printHelp();
			return true;
		}
		
		const QMetaObject *metaobject = gld->metaObject();
		int count = metaobject->propertyCount();
		for (int i=0; i<count; ++i)
		{
			QMetaProperty meta = metaobject->property(i);
			const char *name = meta.name();
			QVariant value = gld->property(name);
	
			qDebug() << QString(name) << meta.type() << value.toString();
		}
			
		return true;
	};
	QString help() { return 
"  listitemprops (file) (groupid) (sceneid) (itemid)\n"
"        List properties available on the given item."; }
};

class CmdSetItemProp : public PlayerCommandLineInterface::Command 
{
public:
	QString name() { return "setitemprop"; }
	bool execute(PlayerCommandLineInterface* api)
	{
		QString arg = api->shiftArgs();
		GLSceneGroupCollection col;
		if(!col.readFile(arg))
		{
			qDebug() << qPrintable(name()) << ": Error reading collection file"<<arg;
			printHelp();
			return true; 
		}
		
		int groupId = api->shiftArgs().toInt();
		GLSceneGroup *group = col.lookupGroup(groupId);
		if(!group)
		{
			qDebug() << qPrintable(name()) << ": Invalid groupId"<<groupId;
			printHelp();
			return true;
		}
		
		int sceneId = api->shiftArgs().toInt();
		GLScene *scene = group->lookupScene(sceneId);
		if(!group)
		{
			qDebug() << qPrintable(name()) << ": Invalid sceneId"<<sceneId;
			printHelp();
			return true;
		}
		
		int itemId = api->shiftArgs().toInt();
		GLDrawable *gld = scene->lookupDrawable(itemId);
		if(!gld)
		{
			qDebug() << qPrintable(name()) << ": Invalid itemId"<<itemId;
			printHelp();
			return true;
		}
		
		QString string = api->shiftArgs();
		
		QString type = api->shiftArgs();
		QString prop = api->shiftArgs();
		
		QVariant value = string;
		
		if(!type.isEmpty())
		{
			if(type == "string")
			{
				// Not needed, default type
			}
			else
			if(type == "int")
			{
				value = string.toInt();
			}
			else
			if(type == "double")
			{
				value = string.toDouble();
			}
			else
			if(type == "bool")
			{
				value = string == "true";
			}
			else
			{
				qDebug() << "CmdSetItemProp::execute: Unknown data type:"<<type<<", defaulting to 'string'";
			}
		}
		
		api->con()->setUserProperty(gld, value, prop);
			
		return true;
	};
	QString help() { return 
"  setitemprop (file) (groupid) (sceneid) (itemid) (value) [datatype] [prop]\n"
"        Set a property on the specified item to the given value.\n"
"\n"
"        Datatype is not required, but if you want to specify tye property\n"
"        name, you must give a datatype - default is 'string'. Valid types:\n"
"                string, int, double, bool\n"
"\n"
"        To discover a list of properties, use 'listitemprops' - which may\n"
"        list a number of properties that are not setable due current list\n"
"        of datatypes above. If this is a problem, email someone or patch."; }
};


class CmdSetVisible : public PlayerCommandLineInterface::Command 
{
public:
	QString name() { return "setitemvisible"; }
	bool execute(PlayerCommandLineInterface* api)
	{
		QString arg = api->shiftArgs();
		GLSceneGroupCollection col;
		if(!col.readFile(arg))
		{
			qDebug() << qPrintable(name()) << ": Error reading collection file"<<arg;
			printHelp();
			return true; 
		}
		
		int groupId = api->shiftArgs().toInt();
		GLSceneGroup *group = col.lookupGroup(groupId);
		if(!group)
		{
			qDebug() << qPrintable(name()) << ": Invalid groupId"<<groupId;
			printHelp();
			return true;
		}
		
		int sceneId = api->shiftArgs().toInt();
		GLScene *scene = group->lookupScene(sceneId);
		if(!group)
		{
			qDebug() << qPrintable(name()) << ": Invalid sceneId"<<sceneId;
			printHelp();
			return true;
		}
		
		int itemId = api->shiftArgs().toInt();
		GLDrawable *gld = scene->lookupDrawable(itemId);
		if(!gld)
		{
			qDebug() << qPrintable(name()) << ": Invalid itemId"<<itemId;
			printHelp();
			return true;
		}
		
		bool visible = api->shiftArgs() == "true";
		
		api->con()->setVisibility(gld, visible);
			
		return true;
	};
	QString help() { return 
"  setitemvisible (file) (groupid) (sceneid) (itemid) (true|false)\n"
"        Sets the item visible or hidden."; }
};

PlayerCommandLineInterface::PlayerCommandLineInterface()
{
	m_rawArgs = qApp->arguments();
	
	// first arg is the binary name
	m_binaryName = m_rawArgs.takeFirst();
	
	m_commands << new CmdSetBlack;
	m_commands << new CmdListVideoInputs;
	m_commands << new CmdSetXfadeSpeed;
	m_commands << new CmdListGroups;
	m_commands << new CmdListScenes;
	m_commands << new CmdSetGroup;
	m_commands << new CmdSetScene;
	m_commands << new CmdListItems;
	m_commands << new CmdListItemProps;
	m_commands << new CmdSetItemProp;
	m_commands << new CmdSetVisible;
	
	startConnection();	
}

PlayerCommandLineInterface::~PlayerCommandLineInterface()
{
}

void PlayerCommandLineInterface::startConnection()
{
	if(m_rawArgs.isEmpty())
	{
		qDebug() << "PlayerCommandLineInterface::startConnection: No connection info given on command line!";
		printHelp();
		quit();
	}
	
	QString rawCon = shiftArgs();
	
	QRegExp rx("(?:(\\w+):(\\w+)@)?([A-Za-z0-9\\.-]+)",Qt::CaseInsensitive);
	rx.indexIn(rawCon);
	
	QString user = rx.cap(1);
	QString pass = rx.cap(2);
	QString host = rx.cap(3);
	
	if(user.isEmpty())
		user = "player";
	if(pass.isEmpty())
		pass = "player";
	if(host.isEmpty() || host == "-")
		host = "localhost";
	
	PlayerConnection *con = new PlayerConnection();
	connect(con, SIGNAL(loginSuccess()), this, SLOT(loginSuccess()));
	connect(con, SIGNAL(loginFailure()), this, SLOT(loginFailure()));
	connect(con, SIGNAL(dataReceived()), this, SLOT(dataReceived()));
	connect(con, SIGNAL(playerError(QString)), this, SLOT(playerError(QString)));
	
	con->setUser(user);
	con->setPass(pass);
	con->setHost(host);
	con->setAutoReconnect(false);
	
	qDebug() << "PlayerCommandLineInterface::startConnection: Connecting to "<<host<<" with creds:"<<user<<":"<<pass<<", raw:"<<rawCon;
	
	con->connectPlayer(false); // false = dont send defaults
	
	m_con = con;
}

void PlayerCommandLineInterface::playerError(const QString& error)
{
	qDebug() << "PlayerCommandLineInterface::playerError: "<<error;
	quit();
}

void PlayerCommandLineInterface::loginSuccess()
{
	//qDebug() << "PlayerCommandLineInterface::loginSuccess: Logged in!";
	processCmdLine();
}

void PlayerCommandLineInterface::loginFailure()
{
	qDebug() << "PlayerCommandLineInterface::loginFailure: Failed to login!";
	quit();
}

void PlayerCommandLineInterface::printHelp()
{
	qDebug() << "Usage: "<<qPrintable(m_binaryName)<<" (player) (command) (...)\n"
"  (player) is specified in the form of user:password@host. User and password \n"
"           is optional, and host may be IP or hostname.\n"
"  (command) is one of:\n";
	 
	foreach(PlayerCommandLineInterface::Command *cmdPtr, m_commands)
	{
		qDebug() << qPrintable(cmdPtr->help()) << "\n";
	}
}

void PlayerCommandLineInterface::processCmdLine()
{
	QString cmd = shiftArgs();
	if(cmd.isEmpty())
	{
		qDebug() << "PlayerCommandLineInterface::processCmdLine: No command given after player connection info on command line, aborting!";
		printHelp();
		quit();
	}
	
	if(cmd == "help" || cmd == "?" || cmd == "h")
	{
		printHelp();
		quit();
	}
	
	bool foundCmd = false;
	m_pendingData = 0;
	foreach(PlayerCommandLineInterface::Command *cmdPtr, m_commands)
	{
		if(cmdPtr->name() == cmd)
		{
			if(!cmdPtr->execute(this))
			{
				m_pendingData = cmdPtr;
				return;
			}
			foundCmd = true;
			break;
		}
	}
	
	if(!foundCmd)
	{
		qDebug() << "PlayerCommandLineInterface::processCmdLine: Unknown command:"<<cmd;
		printHelp();
	}
	
	qApp->processEvents();
	m_con->waitForWrite();
	quit();
}

void PlayerCommandLineInterface::dataReceived()
{
	if(m_pendingData)
		if(m_pendingData->dataReceived())
			quit();
}

void PlayerCommandLineInterface::propQueryResponse(GLDrawable */*drawable*/, QString propertyName, const QVariant& value)
{
	/// TODO output in a more appros formats
	qDebug() << "PlayerCommandLineInterface::propQueryResponse: Property name: "<<propertyName<<", Value: "<<value;
}

void PlayerCommandLineInterface::quit()
{
	qApp->quit();
	exit(0);
}
