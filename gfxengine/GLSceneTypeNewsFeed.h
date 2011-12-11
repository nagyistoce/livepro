#ifndef GLSceneTypeNewsFeed_H
#define GLSceneTypeNewsFeed_H

#include "GLSceneGroupType.h"
#include "RssParser.h"

#include <QNetworkReply>

/** \class GLSceneTypeNewsFeed
	A simple news feed based on the Google News Data API. Shows one news item every time the slide is displayed, looping through the list it downloads from google.
	
	Parameters:
		- UpdateTime - Time in minutes to wait between reloading the weather data from the server
	 
	Fields Required:
		- Title
			- Text
			- Example: Workers continue Egypt strikes
			- Required
		- Text
			- Text
			- Example: Doctors and lawyers among thousands of workers joining strike as anti-Mubarak demonstrations enter 17th day. Egyptian labour unions have held nationwide strikes for a second day, adding momentum to the pro-democracy demonstrations in Cairo and other ..
			- Required
		- Source
			- Text
			- Example: Aljazeera.net
			- Optional
		- Date
			- Text
			- Example: 13 minutes ago
			- Optional
		- QRCode
			- Image
			- Displays a QR Code with a link to the news item 
			- Optional
*/			


class GLSceneTypeNewsFeed : public GLSceneType
{
	Q_OBJECT
	
	Q_PROPERTY(QString newsUrl READ newsUrl WRITE setNewsUrl);
	Q_PROPERTY(int updateTime READ updateTime WRITE setUpdateTime);
	
public:
	GLSceneTypeNewsFeed(QObject *parent=0);
	virtual ~GLSceneTypeNewsFeed() {};
	
	virtual QString id()		{ return "0aa1083e-1aea-46c0-a77e-c8c5d8576f6a"; }
	virtual QString title()		{ return "News Feed"; }
	virtual QString description()	{ return "Displays a news feed from the Google News Data API"; }
	
	/** Returns the current News Feed URL 
		\sa setNewsUrl() */
	QString newsUrl() { return m_params["newsUrl"].toString(); }
	
	/** Returns the current update time parameter value.
		\sa setUpdateTime() */
	int updateTime() { return m_params["updateTime"].toInt(); }
	
public slots:
	virtual void setLiveStatus (bool flag=true);
	
	/** Overridden to intercept changes to the 'updateTime' parameter. */
	virtual void setParam(QString param, QVariant value);
	
	/** Sets the news feeed URL to \a url. If you specify the special value "google", 
		it will use the internal parser to download "http://www.google.com/ig/api?news".
		Otherwise, it will pass the given URL to RssParser and assume its an RSS feed. */
	void setNewsUrl(QString url) { setParam("newsUrl", url); }
	
	/** Set the time to wait between updates to \a minutes */
	void setUpdateTime(int minutes) { setParam("updateTime", minutes); }
		
	/** Reload the data from Google */
	void reloadData();
	
private slots:
	void requestData(const QString &location);
	void handleNetworkData(QNetworkReply *networkReply);
	void parseData(const QString &data);
	
	void showNextItem();
	
	void itemsAvailable(QList<RssParser::RssItem>);

private:
	QTimer m_reloadTimer;
	
	QList<RssParser::RssItem> m_news;
	int m_currentIndex;

	RssParser *m_parser;
	bool m_requestLocked;
	QString m_lastLocation;
};

#endif
