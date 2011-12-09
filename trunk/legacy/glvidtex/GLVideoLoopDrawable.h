#ifndef GLVideoLoopDrawable_h
#define GLVideoLoopDrawable_h

#include "GLVideoDrawable.h"

class VideoThread;

class GLVideoLoopDrawable : public GLVideoDrawable
{
	Q_OBJECT
	
	Q_PROPERTY(QString videoFile READ videoFile WRITE setVideoFile USER true);
	
public:
	GLVideoLoopDrawable(QString file="", QObject *parent=0);
	
	QString videoFile() { return m_videoFile; }
	double videoLength() { return m_videoLength; } // in seconds
	
signals:
	void videoFileChanged(const QString&);
	
public slots:
	bool setVideoFile(const QString&);
	
private slots:
	void testXfade();
	
	void deleteSource(VideoSource*);
	

protected:
	// GLVideoDrawable::
	virtual void setLiveStatus(bool);
	
private:
	QString m_videoFile;
	double m_videoLength;
	VideoThread *m_videoThread;
};

#endif
