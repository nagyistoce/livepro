#ifndef MainWindow_H
#define MainWindow_H

#include <QtGui>

class ScreenVideoSource;
class VideoSender;
class MainWindow : public QWidget
{
	Q_OBJECT
public:
	MainWindow();
	

protected:
	friend class RectChangeWidget;
	QRect currentCapRect(int);

protected slots:
	void changeRectSlot();
	void setCapRect(int num, QRect);
	
protected:
	void resizeEvent(QResizeEvent*);

	QList<ScreenVideoSource *> m_caps;
	QList<VideoSender *> m_senders;
};



#endif
