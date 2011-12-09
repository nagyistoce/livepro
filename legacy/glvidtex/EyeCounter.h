#ifndef EyeCounter_H
#define EyeCounter_H

#include <QtGui>

class CvHaarClassifierCascade;
class CvMemStorage;

class EyeCounterResult
{
public:
	EyeCounterResult() {}
	
	QRect face;
	QRect leftEye;
	QRect rightEye;
	
	QList<QRect> allEyes;
};

class EyeCounter : public QObject
{
	Q_OBJECT
public:
	EyeCounter(QObject *parent=0, QString faceXml="haarcascade_frontalface_alt.xml", QString eyeXml="haarcascade_eye.xml");
	~EyeCounter();
	
	QList<EyeCounterResult> detectEyes(QImage image, bool includeFacesWithNoEyes=false);
private:
	CvHaarClassifierCascade *m_cvCascadeFace;
	CvHaarClassifierCascade *m_cvCascadeEyes;
	CvMemStorage		*m_cvStorage;
 
	
};


#endif
