#include "FaceDetectFilter.h"

#ifdef OPENCV_ENABLED
#include "EyeCounter.h"
#endif

FaceDetectFilter::FaceDetectFilter(QObject *parent)
	: VideoFilter(parent)
{
#ifdef OPENCV_ENABLED
	m_counter = new EyeCounter(this);
#else
	qDebug() << "FaceDetectFilter: Not compiled with OpenCV support enabled (qmake CONFIG+=opencv ...) - FaceDetectFilter will NOT do any detection.";
#endif
}

FaceDetectFilter::~FaceDetectFilter()
{
}

void FaceDetectFilter::processFrame()
{
	QImage image = frameImage();	
	QImage histo = highlightFaces(image);
	
	enqueue(new VideoFrame(histo,m_frame->holdTime()));
}

QImage FaceDetectFilter::highlightFaces(QImage img)
{
#ifndef OPENCV_ENABLED
	return img.copy();
#else
	QList<EyeCounterResult> results = m_counter->detectEyes(img,true); // true = faces, no eyes
	
	QList<QRect> faces;
	
	QImage imageCopy = img.copy();
	QPainter p(&imageCopy);
	qDebug() << "FaceDetectFilter::highlightFaces: Result size:"<<results.size();
	foreach(EyeCounterResult r, results)
	{
		p.setPen(Qt::red);
		p.drawRect(r.face);
		qDebug() << "FaceDetectFilter::highlightFaces: Face at:"<<r.face;
		
		faces << r.face;
	}
		
	if(faces.size() > 0)
		emit facesFound(faces);
	
	return imageCopy;
#endif
}
