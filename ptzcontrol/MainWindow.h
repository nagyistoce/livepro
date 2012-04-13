#ifndef MainWindow_H
#define MainWindow_H

#include <QtGui>

class VideoSource;

class MainWindow : public QWidget
{
	Q_OBJECT
public:
	MainWindow();
	
private slots:
	void pointClicked(QPoint);
	
protected:
	VideoSource *m_src;
};

#endif
