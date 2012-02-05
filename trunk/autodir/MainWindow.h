#ifndef MainWindow_H
#define MainWindow_H

#include <QtGui>

class AnalysisFilter;
class PlayerConnection;
class PointTrackingFilter;
class VideoSwitcherJsonServer;

class MainWindow : public QWidget
{
	Q_OBJECT
public:
	MainWindow();
	
private slots:
	void motionRatingChanged(int);
	void widgetClicked();
	void showCon(const QString&);
	void showJsonCon(const QString&, int);
	void setupFilters();
	
protected:
	void resizeEvent(QResizeEvent*);
	QList<int> m_ratings;
	//QList<AnalysisFilter*> m_filters;
	QList<PointTrackingFilter*> m_filters;
	QStringList m_cons;
	PlayerConnection *m_player;
	int m_lastHighNum;
	int m_ignoreCountdown;
	VideoSwitcherJsonServer *m_jsonServer;
	QHBoxLayout *m_hbox;
	int m_numInputs;
	QString m_host;
	QList<double> m_weights;
};

#include "../3rdparty/qjson/serializer.h"
#include "../http/HttpServer.h"

class VideoSwitcherJsonServer : public HttpServer
{
	Q_OBJECT
public:
	VideoSwitcherJsonServer(quint16 port, QObject *parent=0);
	
	// Ideally, you set this right after creating the object
	void setInputList(QStringList inputs) { m_inputList=inputs; };
	
signals:
	void showCon(const QString&, int speedMs=-1);
	void setFadeSpeed(int ms);
	
protected:	
	void dispatch(QTcpSocket *socket, const QStringList &pathElements, const QStringMap &query);
	
private:
	void sendReply(QTcpSocket *socket, QVariantList list);
	QVariant stringToVariant(QString string, QString type);
	
	QJson::Serializer * m_jsonOut;
	
	QStringList m_inputList;

};

#endif
