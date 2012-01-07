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
protected slots:
protected:
private:
	class VideoWidget *m_videoWidget;
	class CameraThread *m_source;
	class FaceDetectFilter *m_filter;
};



#endif
