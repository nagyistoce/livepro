#ifndef PlayerCommandLineInterface_H
#define PlayerCommandLineInterface_H

#include <QtGui>

class PlayerConnection;
class GLDrawable;

class PlayerCommandLineInterface : public QObject
{
	Q_OBJECT
	
public:
	PlayerCommandLineInterface();
	~PlayerCommandLineInterface();	
	
	void printHelp();
	void processCmdLine();
	void startConnection();
	
	class Command
	{
	public:
		Command() {}
		virtual ~Command() {}
		virtual bool execute(PlayerCommandLineInterface*) = 0;
		virtual QString name() = 0;
		virtual QString help() = 0;
		virtual bool dataReceived() { return true; }
		virtual void printHelp() { qDebug() << "Usage:\n" << qPrintable(help()) << "\n"; }
	};
	
	
	void quit();
	QString shiftArgs() { return m_rawArgs.isEmpty() ? "" : m_rawArgs.takeFirst(); }
	PlayerConnection *con() { return m_con; }

private slots:
	void loginSuccess();
	void loginFailure();
	void playerError(const QString&);
	void propQueryResponse(GLDrawable *drawable, QString propertyName, const QVariant& value);
	void dataReceived();

private:
	
	PlayerConnection *m_con;
	QStringList m_rawArgs;
	
	QList<Command*> m_commands;
	
	Command *m_pendingData;
	
	QString m_binaryName;
};

#endif
