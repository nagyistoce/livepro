#ifndef MainWindow_H
#define MainWindow_H

#include <QtGui>

#include <QtNetwork>

namespace QJson
{
	class Parser;
}


class MainWindow : public QWidget
{
	Q_OBJECT
public:
	MainWindow();
	~MainWindow();
	
protected slots:
	void vidWidgetClicked();
	void connectToServer();
	
	void handleNetworkData(QNetworkReply *networkReply);
	void textChanged(const QString&);
	
protected:
	void resizeEvent(QResizeEvent*);
	
	void processInputEnumReply(const QByteArray&);
	void processExamineSceneReply(const QByteArray&);
	
	void loadUrl(const QString &location);

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
	
};



#endif
