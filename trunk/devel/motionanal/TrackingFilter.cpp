#include "TrackingFilter.h"


//#define CV_NO_BACKWARD_COMPATIBILITY

#include "cv.h"
#include "highgui.h"
#include <stdio.h>
#include <ctype.h>


IplImage *image = 0, *grey = 0, *prev_grey = 0, *pyramid = 0, *prev_pyramid = 0, *swap_temp;

int win_size = 10;
const int MAX_COUNT = 500;
CvPoint2D32f* points[2] = {0,0}, *swap_points;
char* status = 0;
int count = 0;
int need_to_init = 0;
int flags = 0;
int originalCount = 0;

int frameCounter = 0;

QTime timeValue;
bool timeInit = false;
int timeFrameCounter = 0;

//#define RESET_RATIO 0.75
 #define RESET_RATIO 0.75

#define PointType_Static 0
#define PointType_Live   1
#define PointType_Wierd  2

// 4 seconds of data at 30 frames a sec
#define POINT_INFO_MAX_VALUES 120
// Default to just a few frames over 30fps
#define POINT_INFO_DEFAULT_NUM_VALUES 32

class PointInfo 
{
public:
	PointInfo() { reset(); }
	
	void reset() 
	{
		type = PointType_Static; // mover
		deltaCounter = 0;
		valid = true;
		for(int i=0; i<POINT_INFO_MAX_VALUES; i++)
			values[i] = 0;
		valuesIdx = 0;
		valuesTotal = 0;
		numValuesUsed = POINT_INFO_DEFAULT_NUM_VALUES;
		valuesAvg = 0;
		lastValuesAvg = 0;
		distanceMovedChangeCount = 0;
	};
	
	inline float storeValue(float value)
	{
		// Store last average for other uses
		lastValuesAvg = valuesAvg;
		
		valuesTotal -= values[valuesIdx];
		valuesTotal += value;
		values[valuesIdx] = value;
		
		valuesIdx ++;
		int max = MIN(numValuesUsed, POINT_INFO_MAX_VALUES);
		if(valuesIdx >= max)
			valuesIdx = 0;
		
		valuesAvg = valuesTotal / (float)max;
		return valuesAvg;
	}
	
	inline float storeChangeValue(float value)
	{
		// Store last average for other uses
		//lastValuesAvg = valuesAvg;
		
		changeValuesTotal -= changeValues[changeValuesIdx];
		changeValuesTotal += value;
		changeValues[changeValuesIdx] = value;
		
		changeValuesIdx ++;
		int max = 3;//MIN(numValuesUsed, POINT_INFO_MAX_VALUES);
		if(changeValuesIdx >= max)
			changeValuesIdx = 0;
		
		changeValuesAvg = changeValuesTotal / (float)max;
		return changeValuesAvg;
	}
	
	float type;
	float deltaCounter;
	bool valid;
	float values[POINT_INFO_MAX_VALUES];
	float valuesTotal;
	float valuesAvg;
	float lastValuesAvg;
	int valuesIdx;
	int numValuesUsed;
	int distanceMovedChangeCount;
	double distanceChangeFrequency;
	
	float changeValues[POINT_INFO_MAX_VALUES];
	float changeValuesTotal;
	float changeValuesAvg;
	int changeValuesIdx; 
};

PointInfo pointInfo[MAX_COUNT];


QList<int> m_history;
	
/// Round up to next higher power of 2 (return x if it's already a power
/// of 2).
/// From: http://stackoverflow.com/questions/364985/algorithm-for-finding-the-smallest-power-of-two-thats-greater-or-equal-to-a-giv
inline int pow2roundup (int x)
{
    if (x < 0)
        return 0;
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x+1;
}




QImage trackPoints(QImage img)
{
	IplImage* frame = 0;
	int i, k, c;
	
	if(!timeInit)
	{
		timeInit = true;
		timeValue.start();
		timeFrameCounter = 0;
	}
	timeFrameCounter++;

	if(img.format() != QImage::Format_RGB888)
		img = img.convertToFormat(QImage::Format_RGB888);
		
	if(img.width() < 600)
		img = img.scaled(640,480);
		
	frame = cvCreateImageHeader( cvSize(img.width(), img.height()), IPL_DEPTH_8U, 3);
	frame->imageData = (char*) img.bits();

	if( !image )
	{
		/* allocate all the buffers */
		image = cvCreateImage( cvGetSize(frame), 8, 3 );
		image->origin = frame->origin;
		grey = cvCreateImage( cvGetSize(frame), 8, 1 );
		prev_grey = cvCreateImage( cvGetSize(frame), 8, 1 );
		pyramid = cvCreateImage( cvGetSize(frame), 8, 1 );
		prev_pyramid = cvCreateImage( cvGetSize(frame), 8, 1 );
		points[0] = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(points[0][0]));
		points[1] = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(points[0][0]));
		status = (char*)cvAlloc(MAX_COUNT);
		flags = 0;
	}

	cvCopy( frame, image, 0 );
	cvCvtColor( image, grey, CV_BGR2GRAY );
	
 	QList<QPoint> pointsList;
	
	if( need_to_init )
	{
		/* automatic initialization */
		IplImage* eig  = cvCreateImage( cvGetSize(grey), 32, 1 );
		IplImage* temp = cvCreateImage( cvGetSize(grey), 32, 1 );
		double quality = 0.01;
		double min_distance = 10;
	
		count = MAX_COUNT;
		cvGoodFeaturesToTrack( grey, eig, temp, points[1], &count,
					quality, min_distance, 0, 3, 0, 0.04 );
					
		cvFindCornerSubPix( grey, points[1], count,
			cvSize(win_size,win_size), cvSize(-1,-1),
			cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03));
			
		cvReleaseImage( &eig );
		cvReleaseImage( &temp );
		
		for(i=0; i<MAX_COUNT; i++)
		{
			pointInfo[i].reset();
		}
		
		originalCount = count;
		frameCounter = 0;

	}
	else if( count > 0 )
	{
		frameCounter ++;
		
		cvCalcOpticalFlowPyrLK( prev_grey, grey, prev_pyramid, pyramid, 
			points[0], points[1], count, cvSize(win_size,win_size), 3, status, 0,
			cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03), flags );
			
		flags |= CV_LKFLOW_PYR_A_READY;
		
		for( i = k = 0; i < count; i++ )
		{
			if( !status[i] )
			{
				pointInfo[i].valid = false;
				continue;
			}
		
			k++;
			
			pointsList << QPoint(points[1][i].x, points[1][i].y);
		}
		count = k;
	}

	
	
	//QList<QPoint> points = trackPoints(img);
	
	QImage imageCopy = img.copy();
	QPainter p(&imageCopy);
		
	//int min = 25;
	int bucketCutoff = 25; /// Magic number
	
	//double rateCutoff = 1.0; /// Magic number
	//int fps = 30; /// Just a guess
	
	int elapsed = timeValue.elapsed();
	double fps = ((double)elapsed) / ((double)timeFrameCounter) / 2.0;
	//qDebug() << "fps:"<<fps<<", elapsed:"<<elapsed<<", timeFrameCounter:"<<timeFrameCounter; 
	
	int distatnceChangeOkCount = 0;
	
	// Calculate movement vectors (distances between last positions)
	for (i=0; i<originalCount; i++) 
	{
		float dX, dY;
		float distSquared;
		if (status[i] && pointInfo[i].valid)	// Make sure its an existing vector 
		{	
			dX = points[1][i].x - points[0][i].x;
			dY = points[1][i].y - points[0][i].y;
			
			// Make sure its not massive
			distSquared = dX*dX + dY*dY;
			
			float avg = pointInfo[i].storeValue(abs(distSquared));
			
			float distDiff = abs(pointInfo[i].lastValuesAvg - avg);
			bool hasChanged = distDiff > 0.001;
			pointInfo[i].distanceMovedChangeCount += hasChanged ? 1 : -1;
			if(pointInfo[i].distanceMovedChangeCount > fps)
				pointInfo[i].distanceMovedChangeCount --;
			if(pointInfo[i].distanceMovedChangeCount < 0)
				pointInfo[i].distanceMovedChangeCount = 0;
			pointInfo[i].distanceChangeFrequency = pointInfo[i].distanceMovedChangeCount / fps;
			
			pointInfo[i].storeChangeValue(pointInfo[i].distanceMovedChangeCount);
		}
	}
	
	
	QFont font = p.font();
	p.setFont(QFont("", 8));
	
	float moveSum = 0;
	int moveCount = 0;
	for (i=0; i<originalCount; i++) 
	{
		if (status[i] && pointInfo[i].valid)	// Make sure its an existing vector 
		{	
			QPoint point(points[1][i].x, points[1][i].y);
			
			//if (decays[i] == 1.0 || (counter > 0.1 && /*bucket >= 0 && */bucket < bucketCutoff))
			if(pointInfo[i].valuesAvg > 0.075 //)
			&&
			   pointInfo[i].changeValuesAvg > 0.1) 
			{
				// app-spec code
				//if(/*decays[i] == 1.0 || */distanceChangeCount[i])
				{
					p.setBrush(Qt::green);
					p.drawEllipse(QRect(point - QPoint(5,5), QSize(10,10)));
					//p.drawText(point + QPoint(-5,5), QString("%1").arg(i));
					
// 					moveSum += counter;
// 					moveCount ++;
				}
				
				moveSum += pointInfo[i].valuesAvg;
				moveCount ++;

			}
			
			//qDebug() << i << moveSum; //pointInfo[i].valuesAvg;
			
// 			else
// 			{
// 				p.setBrush(Qt::gray);
// 				p.drawEllipse(QRect(point - QPoint(3,3), QSize(6,6)));
// 			}

		}
	}
 	p.setFont(font);
 	
 	
 	
 	
 	
	
	int moveAvg = moveSum / (!moveCount?1:moveCount);
	
	//qDebug() << "moveAvg:"<<moveAvg;
	m_history << moveAvg;
	if(m_history.size() > imageCopy.size().width())
		m_history.takeFirst();
	
	int historyMax = 0;
	foreach(int val, m_history)
		if(val > historyMax)
			historyMax = val;
	
	if(historyMax<=0)
		historyMax = 1;
		
	//if(historyMax > 10000)
	//historyMax = 50000;
	//historyMax = 50;
	
	historyMax /= 2;
	
	//qDebug() << "historyMax:"<<historyMax<<", moveAvg:"<<moveAvg;
	
	int lineMaxHeight = imageCopy.size().height() * .25;
	int margin = 0;
	int lineZeroY = imageCopy.size().height() - margin;
	int lineX = margin;
	int lineLastY = lineZeroY;
	
	int lastLineHeight = 0;
	
	int textInc = 10;
	
	p.setPen(Qt::red);
	for(i=0; i<m_history.size(); i++)
	{
		p.setPen(Qt::red);
		
		int value = m_history[i];
		double ratio = ((double)value/(double)historyMax);
		int lineHeight = (int)(
					ratio *  
					(lineMaxHeight - (margin*2))
				);
			
		int lineTop = lineZeroY - lineHeight;
		
		//p.setPen(QColor(255,0,0, (int)((double)i / (double)(imageCopy.size().width()) * 255.f)));
		p.setPen(Qt::white);
			
		p.drawLine(i+1,lineLastY+1,i+2,lineTop+1);
		
		p.setPen(Qt::red);
			
		p.drawLine(i,lineLastY,i+1,lineTop);
		
		if(i % textInc == 0)
		{
			p.setPen(QColor(0,0,0));//, (int)((double)(i/textInc) / (double)(imageCopy.size().width()/textInc) * 255.f)));
			p.drawText( QPoint( i+2, lineTop+1 ), QString("%1").arg((int)(ratio*100)) );
			p.setPen(QColor(255,255,255));//, (int)((double)(i/textInc) / (double)(imageCopy.size().width()/textInc) * 255.f)));
			p.drawText( QPoint( i+1, lineTop ), QString("%1").arg((int)(ratio*100)) );
		}
		
		//qDebug() << "value:"<<value<<", lineHeight:"<<lineHeight<<", lineMaxHeight:"<<lineMaxHeight<<", margin:"<<margin<<", lineTop:"<<lineTop;
		
		lineLastY = lineTop;
		//lastLineHeight = lineHeight;
		lastLineHeight = ratio*100;
	}





	
	
	
	CV_SWAP( prev_grey, grey, swap_temp );
	CV_SWAP( prev_pyramid, pyramid, swap_temp );
	CV_SWAP( points[0], points[1], swap_points );
	need_to_init = 0;
	//cvShowImage( "LkDemo", image );
		
	if(count <= originalCount * RESET_RATIO)
	{
		//qDebug() << "Need to init!";
		qDebug() << "count less than "<<(originalCount * RESET_RATIO)<<", resetting.";
		need_to_init = 1;
	}
	else
	{
		//qDebug() << "Count:"<<count<<", reset point:"<<(originalCount * RESET_RATIO);
	}
	
	cvReleaseImage(&frame);
		
	
	//return pointsList;
	return imageCopy;
}


TrackingFilter::TrackingFilter(QObject *parent)
	: VideoFilter(parent)
{
}

TrackingFilter::~TrackingFilter()
{
}

void TrackingFilter::processFrame()
{
	QImage image = frameImage();	
	QImage histo = highlightFaces(image);
	
	enqueue(new VideoFrame(histo,m_frame->holdTime()));
}

QImage TrackingFilter::highlightFaces(QImage img)
{
#if 0
	return img.copy();
#else
	return trackPoints(img);
#endif
}




