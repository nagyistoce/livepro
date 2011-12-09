#ifndef GVTestWindow_H
#define GVTestWindow_H

#include <QtGui>
#include "VideoFrame.h"

class BoxItem : public QObject, public QGraphicsItem
{
	Q_OBJECT
	Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity);
public:
	
	QRectF m_contentsRect;
	QPen pen;
	QBrush brush;

	BoxItem(QGraphicsScene * scene, QGraphicsItem * parent, QString camera = "/dev/video0");
	~BoxItem();
	QRectF boundingRect() const;
	
	void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget);
	bool isVisible() { return m_isVisible; }
	
public slots:
	void show();
	void hide();
	
signals:
	void isVisible(bool);
	
private slots:
	void readFrame();
	
private:
	class SimpleV4L2 *m_v4l2;
	QTimer m_readTimer;
	VideoFrame m_frame;
	QTime m_time;
	int m_frameCount;
	int m_latencyAccum;
	bool m_debugFps;
	bool m_isVisible;
	
};




#endif
