#ifndef RectSelectVideoWidget_H
#define RectSelectVideoWidget_H

#include "VideoWidget.h"


class RectSelectVideoWidget : public VideoWidget
{
	Q_OBJECT
public:
	RectSelectVideoWidget(QWidget *parent=0);
	
	bool rectSelectEnabled() { return m_rectSelectEnabled; }

public slots:
	void setRectSelectEnabled(bool flag);
	
signals:
	void rectSelected(QRect);	
	
protected:
	void paintEvent(QPaintEvent*);
	
	void mousePressEvent(QMouseEvent*);
	void mouseMoveEvent(QMouseEvent*);
	void mouseReleaseEvent(QMouseEvent*);
	
	
protected:
	bool m_rectSelectEnabled;
	
	QRect m_rect;
	bool m_mouseIsDown;
	
};

#endif
