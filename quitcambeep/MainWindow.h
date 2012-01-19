#ifndef MainWindow_H
#define MainWindow_H

#include <QtGui>

class PointTrackingFilter;
class MainWindow : public QWidget
{
	Q_OBJECT
public:
	MainWindow();
	
protected slots:
	void historyAvgZero();
	void textChanged(QString);
	void connectToServer();
	void watchButtonClicked();

protected:
	bool m_isWatching;
	int m_beepCounter;
	PointTrackingFilter *m_trackingFilter;

	QLineEdit *m_serverBox;
	QPushButton *m_connectBtn;
	
	QPushButton *m_watchButton;
	QLabel *m_watchLabel;
};



#endif
