#ifndef GLSpinnerDrawable_H
#define GLSpinnerDrawable_H

#include "GLImageDrawable.h"

class GLSpinnerDrawable : public GLImageDrawable
{
	Q_OBJECT
	
	Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor USER true);
	Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor);
	Q_PROPERTY(double borderWidth READ borderWidth WRITE setBorderWidth);
	
	Q_PROPERTY(double cycleDuration READ cycleDuration WRITE setCycleDuration);
	Q_PROPERTY(bool loopAtEnd READ loopAtEnd WRITE setLoopAtEnd);

public:
	GLSpinnerDrawable(QObject *parent=0);
	
	QColor fillColor() { return m_fillColor; }
	QColor borderColor() { return m_borderColor; }
	double borderWidth() { return m_borderWidth; }
	
	double cycleDuration() { return m_cycleDuration; }
	bool loopAtEnd() { return m_loopAtEnd; }
	
signals:
	void cycleFinished();
	
public slots:
	void setFillColor(QColor c);
	void setBorderColor(QColor c);
	void setBorderWidth(double d);
	
	void setCycleDuration(double);
	void setLoopAtEnd(bool);
	
	void start();
	
protected slots:
	void tick();
	
protected:
	void renderImage(double progress);
	
	// For compat with QGraphicsItem
	virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget);
	
protected:
	QColor m_fillColor;
	QColor m_borderColor;
	double m_borderWidth;
	
	double m_cycleDuration;
	bool m_loopAtEnd;
	
	QTimer m_animTimer;
	QTime m_animClock;
	bool m_animClockStarted;
	
	bool m_sceneTickLock;
};

#endif

