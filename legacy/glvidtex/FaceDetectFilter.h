#ifndef FaceDetectFilter_H
#define FaceDetectFilter_H

#include <QtGui>
#include "VideoFilter.h"

class EyeCounter;

class FaceDetectFilter : public VideoFilter
{
	Q_OBJECT
public:
	FaceDetectFilter(QObject *parent=0);
	~FaceDetectFilter();
	
signals:
	void facesFound(QList<QRect>);
	
protected:
	virtual void processFrame();
	
	QImage highlightFaces(QImage);
	
	EyeCounter *m_counter;


};

#endif
