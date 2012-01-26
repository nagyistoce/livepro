#ifndef MainWindow_H
#define MainWindow_H

#include <QtGui>

class AnalysisFilter;
class PlayerConnection;
class PointTrackingFilter;

class MainWindow : public QWidget
{
	Q_OBJECT
public:
	MainWindow();
	
private slots:
	void motionRatingChanged(int);
	void widgetClicked();

protected:
	void resizeEvent(QResizeEvent*);
	QList<int> m_ratings;
	//QList<AnalysisFilter*> m_filters;
	QList<PointTrackingFilter*> m_filters;
	QStringList m_cons;
	PlayerConnection *m_player;
	int m_lastHighNum;
	int m_ignoreCountdown;
};



#endif
