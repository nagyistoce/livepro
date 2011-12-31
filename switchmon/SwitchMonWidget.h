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
	
public slots:	
	void setFadeSpeed(int);
	void setCutFlag(bool);

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
	void clearViewerLayout();
	void createViewers();
	
	enum LastRequestType {
		T_UNKNOWN = 0,
		T_EnumInputs,
		T_ExamineScene,
		T_SetProperty,
		T_ShowCon,
		T_NUM_TYPES
	} ;
		
	QNetworkAccessManager *m_ua;
	QList<QString> m_inputDevices;
	
	LastRequestType m_lastReqType;
	
	// Various layout items
	QVBoxLayout *m_vbox;
	QLayout *m_viewerLayout;
	QLineEdit *m_serverBox;
	QPushButton *m_connectBtn;
	
	// for re-creating viewers after changing size of window (from Horizontal to Vertical, etc)
	QVariantList m_inputList;
	
	// not really used - set, but not used
	bool m_isConnected;
	
	// used in sending URL commands
	QString m_host;
	int m_drawableId;
		
	// Parser for responses from server 
	QJson::Parser *m_parser;
	
	// Used to resend last 'switch' command if needed
	QString m_lastCon;
	bool m_resendLastCon;
	
	// API Version the server advertises
	double m_serverApiVer;
	
	// For re-adding when window orientation changes
	QHBoxLayout *m_bottomRow;
	
	// fade speed
	QSpinBox *m_fadeSpeedBox;
	
	// cut or fade?
	bool m_cutFlag;
	
	// saved so we can reset the border color when we switch to a new widget
	QPointer<QObject> m_lastLiveWidget;
	// used to re-set border color after orientation changed
	QString m_lastLiveCon;
	
	// store ptr so we can change text on btn face
	QPushButton *m_cutBtn;
};



#endif
