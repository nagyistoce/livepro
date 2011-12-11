#ifndef GLVideoInputDrawable_h
#define GLVideoInputDrawable_h

#include "GLVideoDrawable.h"

#ifdef Q_OS_WIN
	#define DEFAULT_INPUT "vfwcap://0"
#else
	#define DEFAULT_INPUT "/dev/video0"
#endif

class CameraThread;
class VideoReceiver;

class GLVideoInputDrawable : public GLVideoDrawable
{
	Q_OBJECT
	
	// 'videoConnection' is like a DB connection string - options encoded into a single string.
	// Example: dev=/dev/video0,input=S-Video0,net=10.0.1.70:8877
	Q_PROPERTY(QString videoConnection READ videoConnection WRITE setVideoConnection USER true);
	
	Q_PROPERTY(QString videoInput READ videoInput WRITE setVideoInput);
	Q_PROPERTY(QString cardInput READ cardInput WRITE setCardInput);
	Q_PROPERTY(bool deinterlace READ deinterlace WRITE setDeinterlace);
	Q_PROPERTY(QString networkSource READ networkSource WRITE setNetworkSource);
	
	// Dont need this saved as a property - automatically decided
	//Q_PROPERTY(bool useNetworkSource READ useNetworkSource WRITE setUseNetworkSource);
	
public:
	GLVideoInputDrawable(QString file=DEFAULT_INPUT, QObject *parent=0);
	~GLVideoInputDrawable();
	
	QString videoConnection() { return m_videoConnection; }
	
	QString videoInput() { return m_videoInput; }
	QString cardInput();
	bool deinterlace();
	
	QString networkSource() { return m_networkSource; }
	
	bool useNetworkSource() { return m_useNetworkSource; }
	
	QStringList cardInputs();
	
public slots:
	void setVideoConnection(const QString&);
	bool setVideoInput(const QString&);
	bool setCardInput(const QString&);
	void setDeinterlace(bool flag=true);
	void setNetworkSource(const QString&);
	
	void setUseNetworkSource(bool);
	
private slots:
	void testXfade();
	
private:
	QString m_videoConnection;
	QString m_videoInput;
	QPointer<CameraThread> m_source;
	QString m_networkSource;
	VideoReceiver *m_rx;
	bool m_useNetworkSource;
	QHash<QString,bool> m_localHasError;
	QHash<QString,bool> m_isLocal;
};

#endif
