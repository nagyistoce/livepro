#ifndef GLPlayerSchedule_H
#define GLPlayerSchedule_H

#include <QObject>
#include <QDateTime>

class GLScheduleItemRef : public QObject
{
	Q_OBJECT
	Q_PROPERTY(int drawableId READ drawableId WRITE setDrawableId);
	Q_PROPERTY(QVariant userProperty READ userProperty WRITE setUserProperty);
	Q_PROPERTY(bool isVisible READ isVisible WRITE setIsVisible);
	
public:
	GLScheduleItemRef();
	
	int drawableId() { return m_drawableId; }
	QVariant userProperty() { return m_userProperty; }
	bool isVisible() { return m_isVisible; }
	
	QByteArray toByteArray();
	void fromByteArray(QByteArray&);

public slots:
	void setDrawableId(int d) { m_drawableId=d; }
	void setUserProperty(QVariant v) { m_userProperty=v; }
	void setIsVisible(bool v) { m_isVisible=v; }
	
private:
	int m_drawableId;
	QVariant m_userProperty;
	bool m_isVisible;
};

class GLScheduleEntry : public QObject
{
	Q_OBJECT
	
	Q_PROPERTY(QString sceneFile READ sceneFile WRITE setSceneFile)
	Q_PROPERTY(int sceneId READ sceneId WRITE setSceneId)
	Q_PROPERTY(int layoutId READ layoutId WRITE setLayoutId)
	// 0 = implicit, 1 = manual
	Q_PROPERTY(int durationType READ durationType WRITE setDurationType)
	// duration specified in seconds 
	Q_PROPERTY(double duration READ duration WRITE setDuration)
	// 0 = implicit, 1 = manual
	Q_PROPERTY(int dateTimeType READ dateTimeType WRITE setDateTimeType)
	Q_PROPERTY(QDateTime dateTime READ dateTime WRITE setDateTime)
	
public:
	GLScheduleEntry();

	QString sceneFile() { return m_sceneFile; }
	int sceneId() { return m_sceneId; }
	int layoutId() { return m_layoutId; }
	int durationType() { return m_durationType; }
	double duration() { return m_duration; }
	int dateTimeType() { return m_dateTimeType; }
	QDateTime dateTime() { return m_dateTime; }

		
	QList<GLScheduleItemRef*> itemRefs() { return m_items; }
	void addItemRef(GLScheduleItemRef* x) { m_items << x; }
	void removeItemRef(GLScheduleItemRef* x) { m_items.removeAll(x); }


	QByteArray toByteArray();
	void fromByteArray(QByteArray&);

public slots:
	void setSceneFile(const QString& f) { m_sceneFile=f; }
	void setSceneId(int x) { m_sceneId=x; }
	void setLayoutId(int x) { m_layoutId=x; }
	void setDurationType(int x) { m_durationType=x; }
	void setDuration(double x) { m_duration=x; }
	void setDateTimetype(int x) { m_dateTimeType=x; }
	void setDateTime(const QDateTime& x) { m_dateTime=x; }

private:
	QString m_sceneFile;
	int m_sceneId;
	int m_layoutId;
	int m_durationtype;
	double m_duration;
	int m_dateTimetype;
	QDateTime m_dateTime;
	
	QList<GLScheduleItemRef*> m_items;
	
};

class GLSchedule : public QObject
{
	Q_OBJECT
	
	Q_PROPERTY(int scheduleId READ scheduleId);
	Q_PROPERTY(QString scheduleName READ scheduleName WRITE setScheduleName);
public:
	GLSchedule(QObject *parent=0);
	GLSchedule(QByteArray&, QObject *parent=0);
	~GLSchedule();
	
	int scheduleId();
	QString scheduleName() { return m_scheduleName; }
	
	QByteArray toByteArray();
	void fromByteArray(QByteArray&);
	
	QList<GLScheduleEntry*> entryList() { return m_entries; }
	void addEntry(GLScheduleEntry*);
	void removeEntry(GLScheduleEntry*);
	
	//GLScheduleEntry * lookupEntry(int id);

public slots:
	void setScheduleName(const QString& name);
	
signals:
	void entryAdded(GLScheduleEntry*);
	void entryRemoved(GLScheduleEntry*);
	
	void scheduleNameChanged(const QString&);
	
protected:
	int m_scheduleId;
	QString m_scheduleName;
	
	QList<GLScheduleEntry*> m_entries;
	//QHash<int,GLScheduleEntry*> m_entryIdLookup;
};


#endif
