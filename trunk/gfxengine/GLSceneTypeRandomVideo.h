#ifndef GLSceneTypeRandomVideo_H
#define GLSceneTypeRandomVideo_H

#include "GLSceneGroupType.h"

#include <QNetworkReply>

/** \class GLSceneTypeRandomVideo
	Shows a random image from the given directory each time the scene is displayed. This scenetype will automatically update the duration of the scene to the video length.
	
	Parameters:
		- Folder - Folder of videos to show
		- UpdateTime - Time in minutes to wait between checking the folder for updates
		- NotRandom - Show the list of photos in order instead of randomly.
		- IgnoreAR - Ignore aspect ratio of the file and stretch to fill the field  
	 	
	Fields Required:
		- Video
			- Required
*/


class GLSceneTypeRandomVideo : public GLSceneType
{
	Q_OBJECT
	
	Q_PROPERTY(QString folder READ folder WRITE setFolder);
	Q_PROPERTY(int updateTime READ updateTime WRITE setUpdateTime);
	Q_PROPERTY(bool notRandom READ notRandom WRITE setNotRandom);
	Q_PROPERTY(int ignoreAR READ ignoreAR WRITE setIgnoreAR);	
	
public:
	GLSceneTypeRandomVideo(QObject *parent=0);
	virtual ~GLSceneTypeRandomVideo() {};
	
	virtual QString id()		{ return "92adaf7b-7fbb-42c7-8024-7e357cb2fa8b"; }
	virtual QString title()		{ return "Random Video"; }
	virtual QString description()	{ return "Displays a random video from a given directory."; }
	virtual bool doesAutomateSceneDuration() { return true; }
	
	/** Returns the current location parameter value.
		\sa setLocation() */
	QString folder() { return m_params["folder"].toString(); }
	
	/** Returns the current update time parameter value.
		\sa setUpdateTime() */
	int updateTime() { return m_params["updateTime"].toInt(); }
	
	/** Returns the current 'not random' parameter value. 
		\sa setNotRandom() */
	bool notRandom() { return m_params["notRandom"].toBool(); }
	
	/** Returns the current 'ignore AR' parameter value. 
		\sa setIgnoreAR() */
	bool ignoreAR() { return m_params["ignoreAR"].toBool(); }
	
	
public slots:
	virtual void setLiveStatus (bool flag=true);
	
	/** Overridden to intercept changes to the 'location' parameter. */
	virtual void setParam(QString param, QVariant value);
	
	/** Set the current folder to \a folder. */
	void setFolder(const QString& local) { setParam("folder", local); }
	
	/** Set the time to wait between updates to \a minutes */
	void setUpdateTime(int minutes) { setParam("updateTime", minutes); }
	
	/** Enable/disable random image choosing */
	void setNotRandom(bool notRandom) { setParam("notRandom", notRandom); }
	
	/** Enable/disable ignore AR */
	void setIgnoreAR(bool ignoreAR) { setParam("ignoreAR", ignoreAR); }
		
	/** Reload the list of files from the given folder */
	void reloadData();
	
	/** Show the next video in the list (or choose a random video, depending on the value of the 'notRandom' parameter.) */
	void showNextVideo();
	
protected:
	/** Calls showNextImage() as soon as a scene is attached */
	virtual void sceneAttached(GLScene *);
	
private slots:
	void readFolder(const QString &folder);
	void durationChanged(double d);
	void statusChanged(int);
	
private:
	QList<QString> m_videos;
	
	QTimer m_reloadTimer;
	
	int m_currentIndex;
};

#endif
