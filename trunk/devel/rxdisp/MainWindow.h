#ifndef MainWindow_H
#define MainWindow_H

#include <QtGui>
class GLWidget;
class VideoReceiver;
class GLImageHttpDrawable;

class MainWindow : public QWidget
{
	Q_OBJECT
public:
	MainWindow();
	
protected slots:
	void customSignal(QString key, QVariant value);
	void videoReceiverChanged(VideoReceiver *);

protected:
	GLWidget *m_glw;
	GLImageHttpDrawable *m_drw;
};



#endif
