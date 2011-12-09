#ifndef DVizSharedMemoryThread_h
#define DVizSharedMemoryThread_h

#include <QImage>
#include <QTimer>
#include <QMutex>
#include <QSharedMemory>

#include "VideoSource.h"

class DVizSharedMemoryThread : public VideoSource
{
	Q_OBJECT

private:
	DVizSharedMemoryThread(const QString& device, QObject *parent=0);
	static QMap<QString,DVizSharedMemoryThread *> m_threadMap;
	
public:
	~DVizSharedMemoryThread();

	static DVizSharedMemoryThread * threadForKey(const QString& key);
	
	void setFps(int fps=30);
	int fps() { return m_fps; }

signals:
	void frameReady();

private slots:
	void readFrame();

protected:
	void run();
	
	void destroySource();
	
private:
	int m_fps;
	QString m_key;
	QSharedMemory m_sharedMemory;
	
	int m_timeAccum;
	int m_frameCount;
	QTime m_readTime;
	
	QTimer m_readTimer;
	
	int m_v4lOutputDev;
	
	static QMutex threadCacheMutex;
};


#endif //DVizSharedMemoryThread_h

