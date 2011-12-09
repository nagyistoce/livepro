#ifndef GLVideoReceiverDrawable_h
#define GLVideoReceiverDrawable_h

#include "GLVideoDrawable.h"

class VideoReceiver;
class GLVideoReceiverDrawable : public GLVideoDrawable
{
	Q_OBJECT
	
	Q_PROPERTY(QString host READ host WRITE setHost);
	Q_PROPERTY(int port READ port WRITE setPort);
	
public:
	GLVideoReceiverDrawable(QString host="localhost", int port=7755, QObject *parent=0);
	
	QString host() { return m_host; }
	int port() { return m_port; }
	
public slots:
	void setHost(const QString&);
	void setPort(int);
	
private:
	QString m_host;
	int m_port;
	
	VideoReceiver *m_rx;
};

#endif
