#ifndef GLVideoMjpegDrawable_h
#define GLVideoMjpegDrawable_h

#include "GLVideoDrawable.h"

class MjpegThread;

class GLVideoMjpegDrawable : public GLVideoDrawable
{
	Q_OBJECT
	
	Q_PROPERTY(QString url READ url WRITE setUrl USER true);
	
public:
	GLVideoMjpegDrawable(QString url="", QObject *parent=0);
	~GLVideoMjpegDrawable();
	
	QString url() { return m_url; }
	
public slots:
	bool setUrl(const QString&);
	
private:
	QString m_url;
	MjpegThread * m_thread;
};

#endif
