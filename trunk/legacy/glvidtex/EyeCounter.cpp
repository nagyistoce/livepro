#include "EyeCounter.h"

#include <QtGui>
#include <QDebug>

#include "opencv/cv.h"
#include "opencv/highgui.h"

void detectEyes(IplImage *img);
IplImage* QImage2IplImage(QImage qimg);
QImage IplImage2QImage(IplImage *iplImg);

EyeCounter::EyeCounter(QObject *parent, QString faceXml, QString eyeXml)
	: QObject(parent)
{
	QFileInfo info(faceXml);
	if(!info.exists())
		qDebug() << "File:"<<faceXml<<" doesnt exist!";
	/* load the face classifier */
	m_cvCascadeFace = (CvHaarClassifierCascade*)cvLoad(qPrintable(faceXml), 0, 0, 0);
	
	/* load the eye classifier */
	m_cvCascadeEyes = (CvHaarClassifierCascade*)cvLoad(qPrintable(eyeXml), 0, 0, 0);
	
	/* setup memory storage, needed by the object detector */
	m_cvStorage = cvCreateMemStorage(0);
}

EyeCounter::~EyeCounter()
{
// 	delete m_cvCascadeFace;
// 	delete m_cvCascadeEyes;
// 	delete m_cvStorage;
}

QList<EyeCounterResult> EyeCounter::detectEyes(QImage image, bool includeFacesWithNoEyes)
{
	//qDebug() << "EyeCounter::detectEyes: image size:"<<image.size();
	IplImage *cvImg;
	//cvImg = QImage2IplImage(image); //.scaled(1024,1024));
	
	if(image.format() != QImage::Format_ARGB32)
		image = image.convertToFormat(QImage::Format_ARGB32);
		
	cvImg = cvCreateImageHeader( cvSize(image.width(), image.width()), IPL_DEPTH_8U, 4);
	cvImg->imageData = (char*) image.bits();
	
// 	uchar* newdata = (uchar*) malloc(sizeof(uchar) * image.byteCount());
// 	memcpy(newdata, image.bits(), image.byteCount());
// 	imgHeader->imageData = (char*)newdata;
	
	/* always check */
	//assert(cascade_f && cascade_e && storage && img);
	
	int i;

	/* detect faces */
	CvSeq *faces = cvHaarDetectObjects(
			cvImg, 
			m_cvCascadeFace, 
			m_cvStorage,
			1.1, 3, 0, 
			cvSize( 40, 40 ) );

	QList<EyeCounterResult> faceList;
	
	for(int faceNum = 0; faceNum < faces->total; faceNum ++)
	{

		/* draw a rectangle */
		CvRect *faceRect = (CvRect*)cvGetSeqElem(faces, faceNum);
					
	// 	cvRectangle(cvImg,
	// 		cvPoint(r->x, r->y),
	// 		cvPoint(r->x + r->width, r->y + r->height),
	// 		CV_RGB(255, 0, 0), 1, 8, 0);

		/* reset buffer for the next object detection */
		cvClearMemStorage(m_cvStorage);

	
		/* Set the Region of Interest: estimate the eyes' position */
		int roi_x = faceRect->x, 
		    roi_y = (int)(faceRect->y + (faceRect->height/5.5));
		cvSetImageROI(cvImg, 
			cvRect(roi_x, roi_y, 
				faceRect->width, (int)(faceRect->height/3.0)));

		/* detect eyes */
		CvSeq* eyes = cvHaarDetectObjects( 
			cvImg, 
			m_cvCascadeEyes, 
			m_cvStorage,
			1.15, 3, 0, 
			cvSize(25, 15));
	
		QList<QRect> eyeRects;
	
		/* draw a rectangle for each eye found */
		for( i = 0; i < (eyes ? eyes->total : 0); i++ ) 
		{
			CvRect *r = (CvRect*)cvGetSeqElem( eyes, i );
			eyeRects << QRect(r->x + roi_x, r->y + roi_y, r->width, r->height);
	// 		cvRectangle(img, 
	// 			cvPoint(r->x, r->y), 
	// 			cvPoint(r->x + r->width, r->y + r->height),
	// 			CV_RGB(255, 0, 0), 1, 8, 0);
		}
		
		if(!includeFacesWithNoEyes && eyeRects.isEmpty())
			continue;
		 
		 EyeCounterResult person;
		 person.face = QRect(faceRect->x, faceRect->y, faceRect->width, faceRect->height);
		 
		 // Just guesing that the first and second rect is left/right eye!
		 if(eyeRects.size() > 0)
		 	person.leftEye = eyeRects.at(0);
		 if(eyeRects.size() > 1)
		 	person.rightEye = eyeRects.at(1);
		 	
		 person.allEyes = eyeRects;
		 	
		 faceList << person;
		 	
		 cvResetImageROI(cvImg);
	}
		
	cvReleaseImage(&cvImg);
	return faceList;
}



IplImage* QImage2IplImage(QImage qimg)
{
	if(qimg.format() != QImage::Format_ARGB32)
		qimg = qimg.convertToFormat(QImage::Format_ARGB32);
		
	IplImage *imgHeader = cvCreateImageHeader( cvSize(qimg.width(), qimg.width()), IPL_DEPTH_8U, 4);
	imgHeader->imageData = (char*) qimg.bits();
	
	uchar* newdata = (uchar*) malloc(sizeof(uchar) * qimg.byteCount());
	memcpy(newdata, qimg.bits(), qimg.byteCount());
	imgHeader->imageData = (char*)newdata;
	
	// NB: Caller is responsible for deleting imageData!
	return imgHeader;
}

QImage IplImage2QImage(IplImage *iplImg)
{
	int h        = iplImg->height;
	int w        = iplImg->width;
	int step     = iplImg->widthStep;
	char *data   = iplImg->imageData;
	int channels = iplImg->nChannels;
	
	QImage qimg(w, h, QImage::Format_ARGB32);
	for (int y = 0; y < h; y++, data += step)
	{
		for (int x = 0; x < w; x++)
		{
			char r=0, g=0, b=0, a=0;
			if (channels == 1)
			{
				r = data[x * channels];
				g = data[x * channels];
				b = data[x * channels];
			}
			else if (channels == 3 || channels == 4)
			{
				r = data[x * channels + 2];
				g = data[x * channels + 1];
				b = data[x * channels];
			}
			
			if (channels == 4)
			{
				a = data[x * channels + 3];
				qimg.setPixel(x, y, qRgba(r, g, b, a));
			}
			else
			{
				qimg.setPixel(x, y, qRgb(r, g, b));
			}
		}
	}
	return qimg;

}

