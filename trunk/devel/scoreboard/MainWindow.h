#ifndef MainWindow_H
#define MainWindow_H

#include <QtGui>
class GLDrawable;

class MainWindow : public QWidget
{
	Q_OBJECT
public:
	MainWindow();
	
protected:

protected slots:
	void timeout();
	
private:
// 	GLImageDrawable *m_imgDrw;
	QTimer m_timer;
	QImage m_image;
	QTime m_time;
	QList<GLDrawable*> m_items;

};



#endif
