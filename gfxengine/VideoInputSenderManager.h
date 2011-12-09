#ifndef VideoInputSenderManager_H
#define VideoInputSenderManager_H

class VideoSender;
#include <QObject>
#include <QHash>

class VideoInputSenderManager : public QObject
{
	Q_OBJECT
	
public:
	VideoInputSenderManager(QObject *parent = 0);
	~VideoInputSenderManager();
	
	QStringList videoConnections(bool justNetString=false);
	
	bool sendingEnabled() { return m_sendingEnabled; }
	
public slots:
	void setSendingEnabled(bool);

private:
	bool m_sendingEnabled;
	
	QHash<QString, VideoSender *> m_videoSenders;
	
	static int m_videoSenderPortAllocator;
	
	
};

#endif 


