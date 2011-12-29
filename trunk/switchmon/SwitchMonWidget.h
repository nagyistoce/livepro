#ifndef SwitchMonWidget_H
#define SwitchMonWidget_H

#include <QtGui>

#include <QtNetwork>

namespace QJson
{
	class Parser;
}


class SwitchMonWidget : public QWidget
{
	Q_OBJECT
public:
	SwitchMonWidget(QWidget *parent=0);
	~SwitchMonWidget();
	
protected slots:
	void vidWidgetClicked();
	void connectToServer();
	
	void handleNetworkData(QNetworkReply *networkReply);
	void textChanged(const QString&);
	
protected:
	void resizeEvent(QResizeEvent*);
	
	void processInputEnumReply(const QByteArray&);
	void processExamineSceneReply(const QByteArray&);
	void processSetPropReply(const QByteArray&);
	
	void loadUrl(const QString &location);
	
	void sendCon(const QString& con);

private:
	enum LastRequestType {
		T_UNKNOWN = 0,
		T_EnumInputs,
		T_ExamineScene,
		T_SetProperty,
		T_NUM_TYPES
	} ;
		
	QNetworkAccessManager *m_ua;
	QList<QString> m_inputDevices;
	
	LastRequestType m_lastReqType;
	
	QHBoxLayout *m_hbox;
	QLineEdit *m_serverBox;
	QPushButton *m_connectBtn;
	
	bool m_isConnected;
	
	QString m_host;
	int m_drawableId;
		
	QJson::Parser *m_parser;
	
	QString m_lastCon;
	bool m_resendLastCon;
	
};



#endif
