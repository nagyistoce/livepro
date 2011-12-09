#ifndef GLImageHttpDrawable_h
#define GLImageHttpDrawable_h

#include "GLImageDrawable.h"

#include <QtNetwork>

class GLImageHttpDrawable : public GLImageDrawable
{
	Q_OBJECT
	
	Q_PROPERTY(QString url READ url WRITE setUrl);
	Q_PROPERTY(int updateTime READ updateTime WRITE setUpdateTime);
	Q_PROPERTY(bool pollDviz READ pollDviz WRITE setPollDviz);
	
public:
	GLImageHttpDrawable(QObject *parent=0);
	virtual ~GLImageHttpDrawable();
	
	
// 	virtual void loadPropsFromMap(const QVariantMap&, bool onlyApplyIfChanged = false);
// 	virtual QVariantMap propsToMap();
	QString url() { return m_url; }
	bool pollDviz() { return m_pollDviz; }
	int updateTime() { return m_updateTime; }

public slots:
	//virtual bool setImageFile(const QString&);
	void setUrl(const QString& url);
	void setPollDviz(bool flag);
	void setUpdateTime(int ms);
	
protected:
	virtual bool canReleaseImage() { return false; }
	
	// Overridden in order to start/stop polling if live/not live
	virtual void setLiveStatus(bool flag); 
	
private slots:
	void initDvizPoll();
	void initImagePoll();
	
	void loadUrl(const QString& url);
	void handleNetworkData(QNetworkReply *networkReply);
	
private:
	QString m_url;
	bool m_pollDviz;
	int m_updateTime;
	
	QTimer m_pollDvizTimer;
	QTimer m_pollImageTimer;
	
	// state
	bool m_isDataPoll;
	
	// for polling DViz
	int m_slideId;
	QString m_slideName;
	
};

#endif
