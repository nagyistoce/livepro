#ifndef TrackingFilter_H
#define TrackingFilter_H

#include <QtGui>
#include "VideoFilter.h"

class EyeCounter;

class TrackingFilter : public VideoFilter
{
	Q_OBJECT
public:
	TrackingFilter(QObject *parent=0);
	~TrackingFilter();
	
signals:
	void facesFound(QList<QRect>);
	
protected:
	virtual void processFrame();
	
	QImage highlightFaces(QImage);
	
	EyeCounter *m_counter;
	
	


};

#endif
