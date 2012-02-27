#ifndef OutputWindow_H
#define OutputWindow_H
#include <QtGui>
#include <QtNetwork>

#include "VideoReceiver.h"
#include "GLVideoDrawable.h"

class OutputWindow : public QGraphicsView
{
	Q_OBJECT
public:
	OutputWindow();
	~OutputWindow() {}
	
// 	virtual void loadPropsFromMap(const QVariantMap&, bool onlyApplyIfChanged = false);
// 	virtual QVariantMap propsToMap();
	QString url() { return m_url; }
	bool pollDviz() { return m_pollDviz; }
	int updateTime() { return m_updateTime; }
	
// 	enum InputSource {
// 		SrcDviz=0,
// 		SrcVideoSender,
// 		SrcCapture, // not impl yet...
// 	};
// 
// 	// If source==SrcCapture, host is the capture device, port is ignored
// 	void setInputSource(InputSource source, QString host, int port=-1);
	
	bool isCounterActive(bool);
	
	QGraphicsSimpleTextItem *counterTextItem() { return m_counterText; }
	QGraphicsSimpleTextItem *overlayTextItem() { return m_overlayText; }
	
	Qt::Alignment counterAlignment() { return m_counterAlignment; }
	Qt::Alignment overlayAlignment() { return m_overlayAlignment; }
	
	bool blinkOverlay() { return m_blinkOverlay; }
	
	bool isOverlayVisible() { return m_overlayVisible; }
	bool isCounterVisible() { return m_counterVisible; }
	
public slots:
	//virtual bool setImageFile(const QString&);
	void setUrl(const QString& url);
	void setPollDviz(bool flag);
	void setUpdateTime(int ms);
	
	void setImage(QImage);
	
	void setCounterActive(bool);
	//void resetCounter();
	
	void setCounterAlignment(Qt::Alignment);
	void setOverlayAlignment(Qt::Alignment);
	
	void setBlinkOverlay(bool blink, int speed = 333);
	
	void setCounterVisible(bool flag);
	void setOverlayVisible(bool flag);
	
	void setOverlayText(QString text);
	
// 	void setCounterFont(QFont font) { m_counterText->setFont(font); }
// 	void setCounterColor(QColor color) { m_counterText->setBrush(color); }
	
private slots:
	void initDvizPoll();
	void initImagePoll();
	
	void loadUrl(const QString& url);
	void handleNetworkData(QNetworkReply *networkReply);
	
	//void toggleCounter();
	void counterTick();
	
	//void setPollDvizEnabled(bool flag);
	
	void blinkOverlaySlot();

private:
	QGraphicsPixmapItem *m_pixmap;
	
	QGraphicsSimpleTextItem *m_counterText;
	QGraphicsRectItem *m_counterBgRect;
	
	QGraphicsSimpleTextItem *m_overlayText;
	QGraphicsRectItem *m_overlayBgRect;
	
	Qt::Alignment m_counterAlignment;
	Qt::Alignment m_overlayAlignment;

	QString m_url;
	bool m_pollDviz;
	int m_updateTime;
	
	QTimer m_pollDvizTimer;
	QTimer m_pollImageTimer;
	
	// state
	bool m_isDvizPollEnabled;
	bool m_isDataPoll;
	
	// for polling DViz
	int m_slideId;
	QString m_slideName;

	// Timer
	QTimer m_counterTimer;
	//QPushButton *m_startStopButton;
	int m_countValue;
	
	// For the SrcVideoSender source...
	VideoReceiver *m_rx;
	GLVideoDrawable *m_drw;
	
	QTimer m_blinkOverlayTimer;
	bool m_blinkOverlay;
	
	bool m_overlayVisible;
	bool m_counterVisible;
	
};



#endif
