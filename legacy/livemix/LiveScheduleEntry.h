#ifndef LiveScheduleEntry_H
#define LiveScheduleEntry_H

#include <QtGui>
class LiveLayer;
class LiveScene;

class LiveScheduleEntryPlaylistItem : public QObject
{
	Q_OBJECT
public:
	LiveScheduleEntryPlaylistItem();
	~LiveScheduleEntryPlaylistItem();
	
	QString title() { return m_title; }
	QVariant value() { return m_value; }
	QPixmap pixmap() { return m_pixmap; }
	QTime startTime() { return m_startTime; }
	long duration() { return m_duration; }
	bool isVisible() { return m_isVisible; }
	
	// For loading/saving 
	QByteArray toByteArray();
	void fromByteArray(QByteArray&);

signals:
	void propertyChanged(QString prop, QVariant value);
	void titleChanged(QString);
	void valueChanged(QVariant);
	void pixmapChanged(QPixmap);
	void startTimeChanged(QTime);
	void durationChanged(long);
	void isVisibleChanged(bool);
	
public slots:
	void setTitle(const QString&);
	void setValue(const QVariant&);
	void setPixmap(const QPixmap&);
	void setStartTime(const QTime&);
	void setDuration(long);
	void setIsVisible(bool);

protected:
	QString m_title;
	QVariant m_value;	
	QPixmap m_pixmap;
	QTime m_startTime;
	long m_duration;
	bool m_isVisible;
};

class LiveScheduleEntryPlaylist : public QObject
{
	Q_OBJECT
public:
	LiveScheduleEntryPlaylist(LiveLayer*);
	~LiveScheduleEntryPlaylist();
	
	LiveLayer *layer() { return m_layer; }
	QList<LiveScheduleEntryPlaylistItem*> items() { return m_items; }
	
	// For loading/saving 
	QByteArray toByteArray();
	void fromByteArray(QByteArray&);

signals:
	void itemAdded(LiveScheduleEntryPlaylistItem*);
	void itemRemoved(LiveScheduleEntryPlaylistItem*);
	
public slots:
	void addItem(LiveScheduleEntryPlaylistItem*);
	void removeItem(LiveScheduleEntryPlaylistItem*);
	bool removeItem(int idx);

		
protected:
	LiveLayer *m_layer;
	QList<LiveScheduleEntryPlaylistItem*> m_items;
};

class LiveScheduleEntry : public QObject
{
	Q_OBJECT
public:
	LiveScheduleEntry(LiveScene*);
	~LiveScheduleEntry();
	
	LiveScene *scene() { return m_scene; }
	
	QHash<LiveLayer*, LiveScheduleEntryPlaylist> playlists();
	LiveScheduleEntryPlaylist *playlist(LiveLayer*);
	LiveScheduleEntryPlaylist *playlist(int layerId);
	void setPlaylist(LiveLayer*, LiveScheduleEntryPlaylist*);
	void setPlaylist(int layerId, LiveScheduleEntryPlaylist*);
	
	QString title() { return m_title; }
	QPixmap pixmap() { return m_pixmap; }
	QTime startTime() { return m_startTime; }
	long duration() { return m_duration; }
	
	// For loading/saving 
	QByteArray toByteArray();
	void fromByteArray(QByteArray&);

signals:
	void propertyChanged(QString prop, QVariant value);
	void titleChanged(QString);
	void pixmapChanged(QPixmap);
	void startTimeChanged(QTime);
	void durationChanged(long);
	
public slots:
	void setTitle(const QString&);
	void setPixmap(const QPixmap&);
	void setStartTime(const QTime&);
	void setDuration(long);

protected:
	QString m_title;
	LiveScene *m_scene;
	QPixmap m_pixmap;
	QTime m_startTime;
	long m_duration;
	
};

#endif
