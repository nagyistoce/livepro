#ifndef GLVideoFileDrawable_h
#define GLVideoFileDrawable_h

#include "GLVideoDrawable.h"

#ifdef HAS_QT_VIDEO_SOURCE
#include "QtVideoSource.h"
#endif


class GLVideoFileDrawable : public GLVideoDrawable
{
	Q_OBJECT
	
	Q_PROPERTY(QString videoFile READ videoFile WRITE setVideoFile USER true);
	
	Q_PROPERTY(int volume READ volume WRITE setVolume);
	Q_PROPERTY(bool muted READ isMuted WRITE setMuted);
	Q_PROPERTY(int status READ status WRITE setStatus);
	Q_PROPERTY(quint64 position READ position WRITE setPosition);
	
	Q_PROPERTY(double videoLength READ videoLength);
	
public:
	GLVideoFileDrawable(QString file="", QObject *parent=0);
	
	QString videoFile() { return m_videoFile; }
	
	double videoLength();
	int volume();
	bool isMuted();
	int status();
	quint64 position();
	
	// GLDrawable:: - overridden to remote the 'status' prop
	virtual void loadPropsFromMap(const QVariantMap&, bool onlyApplyIfChanged = false);
	
signals:
	void videoFileChanged(const QString&);
	void positionChanged(qint64 position);
	void durationChanged(double duration); // in seconds
	void statusChanged(int);
	
public slots:
	bool setVideoFile(const QString&);
	
	void setVolume(int);
	void setMuted(bool);
	void setStatus(int);
	void setPosition(qint64);
	
private slots:
	void testXfade();
	void deleteSource(VideoSource *source);
#ifdef HAS_QT_VIDEO_SOURCE
	void setDuration ( qint64 duration );
	void error ( QMediaPlayer::Error error );
	//void mediaStatusChanged ( QMediaPlayer::MediaStatus status )
	//void positionChanged ( qint64 position )
	//void seekableChanged ( bool seekable )
	void stateChanged ( QMediaPlayer::State state );
	
	void firstFrameReceived();
#endif 

protected:
	// GLVideoDrawable::
	virtual void setLiveStatus(bool);
	
private:
	QString m_videoFile;
	double m_videoLength;
#ifdef HAS_QT_VIDEO_SOURCE	
	QtVideoSource * m_qtSource;
#endif

};

#endif
