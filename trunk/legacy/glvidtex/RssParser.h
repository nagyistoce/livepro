#ifndef RssParser_H
#define RssParser_H

#include <QtNetwork>

class RssParser : public QObject
{
	Q_OBJECT
	
	Q_PROPERTY(QString url READ url WRITE setUrl);
	Q_PROPERTY(int updateTime READ updateTime WRITE setUpdateTime);
	
public:
	RssParser(QString url="", QObject *parent=0);
	
	class RssItem
	{
	public:
		QString title;
		QString text;
		QString source;
		QString url;
		QString date;
	};
	
	QString url() { return m_url; }
	int updateTime() { return m_updateTime; }

	QList<RssItem> items() { return m_items; }

signals:
	void itemsAvailable(QList<RssParser::RssItem>);

	
public slots:
	/** Set the time to wait between updates to \a minutes */
	void setUpdateTime(int minutes);
		
	/** Reload the data from server*/
	void reloadData();
	
	void setUrl(const QString&);

private slots:
	void loadUrl(const QString &url);
	void handleNetworkData(QNetworkReply *networkReply);
	void parseData(const QString &data);

private:
	QTimer m_reloadTimer;
		
	QString m_url;
	int m_updateTime;
	QList<RssItem> m_items;

};


#endif
