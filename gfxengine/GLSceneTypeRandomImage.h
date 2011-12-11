#ifndef GLSceneTypeRandomImage_H
#define GLSceneTypeRandomImage_H

#include "GLSceneGroupType.h"

#include <QNetworkReply>

/** \class GLSceneTypeRandomImage
	Shows a random image from the given directory each time the scene is displayed.
	
	Parameters:
		- Folder - Folder of images to show
		- UpdateTime - Time in minutes to wait between checking the folder for updates
		- ChangeTime - If the scene doesn't change, this is how long to wait till showing the next image.
		- NotRandom - Show the list of photos in order instead of randomly.
		- AutoRotate - Automatically rotate based on EXIF data
		- IgnoreAR - Ignore aspect ratio of the file and stretch to fill the field  
	 	
	Fields Required:
		- Image
			- Image
			- This will be the image itself
			- Required
		- Comments
			- Text
			- The comments field extracted from the EXIF data in the image file
			- Optional
		- Filename
			- Text
			- This class will attempt to parse the filename and extract a photograph number from it.
			- Example: Photograph # 1128
			- Optional
		- DateTime
			- Text
			- Example: Photographed Monday, June 10, 2010 
			- Optional
*/			


class GLSceneTypeRandomImage : public GLSceneType
{
	Q_OBJECT
	
	Q_PROPERTY(QString folder READ folder WRITE setFolder);
	Q_PROPERTY(int updateTime READ updateTime WRITE setUpdateTime);
	Q_PROPERTY(int changeTime READ changeTime WRITE setChangeTime);
	Q_PROPERTY(bool notRandom READ notRandom WRITE setNotRandom);
	Q_PROPERTY(int autoRotate READ autoRotate WRITE setAutoRotate);
	Q_PROPERTY(int ignoreAR READ ignoreAR WRITE setIgnoreAR);	
	
public:
	GLSceneTypeRandomImage(QObject *parent=0);
	virtual ~GLSceneTypeRandomImage() {};
	
	virtual QString id()		{ return "16369407-a6c4-44b3-affe-fb6eca51630e"; }
	virtual QString title()		{ return "Random Image"; }
	virtual QString description()	{ return "Displays a random image from a given directory."; }
	
	/** Returns the current location parameter value.
		\sa setLocation() */
	QString folder() { return m_params["folder"].toString(); }
	
	/** Returns the current update time parameter value.
		\sa setUpdateTime() */
	int updateTime() { return m_params["updateTime"].toInt(); }
	
	/** Returns the current change time parameter value. 
		\sa setChangeTime() */ 
	int changeTime() { return m_params["changeTime"].toInt(); }
	
	/** Returns the current 'not random' parameter value. 
		\sa setNotRandom() */
	bool notRandom() { return m_params["notRandom"].toBool(); }
	
	/** Returns the current 'auto rotate' parameter value. 
		\sa setAutoRotate() */
	bool autoRotate() { return m_params["autoRotate"].toBool(); }
	
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
	
	/** Set the time to wait between changing images to \a seconds */
	void setChangeTime(int seconds) { setParam("changeTime", seconds); }
	
	/** Enable/disable random image choosing */
	void setNotRandom(bool notRandom) { setParam("notRandom", notRandom); }
	
	/** Enable/disable auto rotate*/
	void setAutoRotate(bool autoRotate) { setParam("autoRotate", autoRotate); }
	
	/** Enable/disable ignore AR */
	void setIgnoreAR(bool ignoreAR) { setParam("ignoreAR", ignoreAR); }
		
	/** Reload the list of files from the given folder */
	void reloadData();
	
	/** Show the next image in the list (or choose a random image, depending on the value of the 'notRandom' parameter.) */
	void showNextImage();
	
protected:
	/** Calls showNextImage() as soon as a scene is attached */
	virtual void sceneAttached(GLScene *);
	
private slots:
	void readFolder(const QString &folder);
	
private:
	class ImageItem
	{
	public:
		QString imageFile;
		QString comments;
		QString parsedFilename;
		QString datetime;
	};
	
	QList<ImageItem> m_images;
	
	QTimer m_reloadTimer;
	QTimer m_changeTimer;
	
	int m_currentIndex;
};

#endif
