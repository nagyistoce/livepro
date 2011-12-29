#ifndef MainWindow_H
#define MainWindow_H

#include <QtGui>

class SwitchMonWidget;

class MainWindow : public QWidget
{
	Q_OBJECT
public:
	MainWindow();
	~MainWindow();

protected:
	void resizeEvent(QResizeEvent*);

private:	
	SwitchMonWidget *m_switchMon;

};

#endif
