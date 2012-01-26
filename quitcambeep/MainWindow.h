#ifndef MainWindow_H
#define MainWindow_H

#include <QtGui>

class PointTrackingFilter;
class VideoWidget;
class MainWindow : public QWidget
{
	Q_OBJECT
public:
	MainWindow();
	
protected slots:
	void historyAvgZero();
	void historyAvg(int);
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
	
	VideoWidget *m_videoWidget;
	bool m_labelChanged;
};



#endif
