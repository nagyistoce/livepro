#ifndef SwitchMonWidget_H
#define SwitchMonWidget_H

#include <QtGui>

#include <QtNetwork>

class MouthyTest : public QWidget
{
	Q_OBJECT
public:
	MouthyTest(QWidget *parent=0);
	~MouthyTest() {}
	
public slots:
	void beep();
		
protected slots:
protected:
private:
	class VideoWidget *m_videoWidget;
	class CameraThread *m_source;
	class TrackingFilter *m_filter;
	int m_beepCounter;
};



#endif
