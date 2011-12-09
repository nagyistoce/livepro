#ifndef LiveSchedule_H
#define LiveSchedule_H

#include <QtGui>
#include <QString>

class LiveLayer;

class LiveSchedule : public QObject
{
	Q_OBJECT
public:
	LiveSchedule(const QString& file="");
	~LiveSchedule();
	 
	QString filename() { return m_filename; }
	
	QList<LiveScheduleEntry*> entries() { return m_entries; }
	
	// For loading/saving 
	QByteArray toByteArray();
	void fromByteArray(QByteArray&);
	
public slots:
	// sets filename
	void writeFile(const QString& filename="");
	
	void addEntry(LiveScheduleEntry*);
	void addLayout(LiveScene*);
	
	void removeEntry(LiveScheduleEntry*);
	void removeEntry(int);

protected:
	QList<LiveScheduleEntry*> m_entries;
	QString m_filename;
};



#endif
