#include "TrackingFilter.h"


#define CV_NO_BACKWARD_COMPATIBILITY

#include "cv.h"
#include "highgui.h"
#include <stdio.h>
#include <ctype.h>

IplImage *image = 0, *grey = 0, *prev_grey = 0, *pyramid = 0, *prev_pyramid = 0, *swap_temp;

int win_size = 10;
const int MAX_COUNT = 500;
CvPoint2D32f* points[2] = {0,0}, *swap_points;
bool valid[MAX_COUNT];
int deltaCounters[MAX_COUNT];
char* status = 0;
int count = 0;
int need_to_init = 0;
int night_mode = 0;
int flags = 0;
CvPoint pt;
int originalCount = 0;
int deltaSum = 0;

int changeRateCounters[MAX_COUNT];
int ampCounters[MAX_COUNT];
int frameCounter = 0;

//#define RESET_RATIO 0.75
#define RESET_RATIO 0.60
#define DELTA_COUNTER_DECAY 0.9

#define NUM_BUCKETS 2000
int buckets[NUM_BUCKETS];

QList<int> m_history;
	
QImage trackPoints(QImage img)
{
	IplImage* frame = 0;
	int i, k, c;

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
	
// 	if( night_mode )
// 		cvZero( image );

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
			valid[i] = true;
			deltaCounters[i] = 0;
			changeRateCounters[i] = 0;
			ampCounters[i] = 0;
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
				valid[i] = false;
				continue;
			}
		
			k++;
			
			//points[1][k++] = points[1][i];
			//cvCircle( image, cvPointFrom32f(points[1][i]), 3, CV_RGB(0,255,0), -1, 8,0);
			pointsList << QPoint(points[1][i].x, points[1][i].y);
		}
		count = k;
	}

	
	
	//QList<QPoint> points = trackPoints(img);
	
	QImage imageCopy = img.copy();
	QPainter p(&imageCopy);
	//qDebug() << "TrackingFilter::highlightFaces: Result size:"<<results.size();
// 	if(results.isEmpty())
// 		qDebug() << "No faces";
// 	else
 	
//  	if(!results.isEmpty())
// 		qDebug() << "Found "<<results.size()<<" faces";
		
	//int min = 25;
	int bucketCutoff = 10; /// Magic number
	
	deltaSum = 0;
	
	double rateCutoff = 1.0; /// Magic number
	int fps = 30; /// Just a guess
	
	// Calculate movement vectors (distances between last positions)
	for (i=0; i<originalCount; i++) 
	{
		float dX, dY;
		float distSquared;
		//int indexX, indexY;
		//int newCount;
		//CvScalar lineColor;
		if (status[i] && valid[i])	// Make sure its an existing vector 
		{	
			dX = points[1][i].x - points[0][i].x;
			dY = points[1][i].y - points[0][i].y;
			
			// Make sure its not massive
			distSquared = dX*dX + dY*dY;
			
			
			/// All this changeRate and ampCounters stuff is an attempt
			// to filter out points that are just oscilating back and forth very fast - so far it works maybe 10% of the time.
			// Need to do better!
			
			if(frameCounter > fps)
			{
				changeRateCounters[i] --;
				if(changeRateCounters[i]<0)
					changeRateCounters[i]= 0;
				
				ampCounters[i] -= ampCounters[i]/fps;
			}

			if(distSquared > 1)
			{
				changeRateCounters[i] ++;
				ampCounters[i] += distSquared; 
			}
			
			double ampAvg = (double)ampCounters[i] / (double)fps;
			
			double avgChangeRate = (double)changeRateCounters[i] / (double)fps; 
			//if(avgChangeRate > rateCutoff ||
			//   ampAvg > 100)
			//if(ampAvg < 4 && avgChangeRate > 0)/// MORE magic numbers 
			//	qDebug() << "Avg change rate for "<<i<<": "<<avgChangeRate<<", "<<changeRateCounters[i]<<ampAvg;
			
			
			// Decay previous value
			deltaCounters[i] *= DELTA_COUNTER_DECAY; // 0.9
			// Add distance change
			if(avgChangeRate <= rateCutoff ||
			   (ampAvg < 100 && ampAvg > 4)) /// MORE magic numbers
				deltaCounters[i] += abs(distSquared) * DELTA_COUNTER_DECAY;
			
			if(deltaCounters[i] < 0)
				deltaCounters[i] = 0;
				
			
			//deltaCounters[i] = abs(distSquared);
			
			// Sum the total distance changed 
			deltaSum += deltaCounters[i];
			
//			QPoint point(points[1][i].x, points[1][i].y);
			
// 			if (deltaCounters[i] > min*min) // MAX_LOCAL_MOVEMENT*MAX_LOCAL_MOVEMENT) 
// 			{
// 				// app-spec code
// 				p.setBrush(Qt::green);
// 				p.drawEllipse(QRect(point - QPoint(5,5), QSize(10,10)));
// 			}
// 			else
// 			{
// 				p.setBrush(Qt::gray);
// 				p.drawEllipse(QRect(point - QPoint(3,3), QSize(6,6)));
// 			}
		}
	}
	
	// Reset the buckets
	for (i=0; i<NUM_BUCKETS; i++)
		buckets[i] = 0;
	
	//qDebug() << "Delta sum:"<<deltaSum;
	int bucketValueWidth = deltaSum / NUM_BUCKETS;
	int bucketMax = 0; 
	
	if(bucketValueWidth == 0)
		bucketValueWidth = 1;
	
	for (i=0; i<originalCount; i++)
	{
		int bucket = deltaCounters[i] / bucketValueWidth;
		if(bucket < 0 || bucket >= NUM_BUCKETS-1)
			continue;
			
		buckets[bucket] ++;
		
		if(buckets[bucket] > bucketMax)
			bucketMax = buckets[bucket]; 
	}
	
	int moveSum = 0;
	int moveCount = 0;
	for (i=0; i<originalCount; i++) 
	{
		if (status[i] && valid[i])	// Make sure its an existing vector 
		{	
			int counter = deltaCounters[i];
			int bucket = counter / bucketValueWidth;
			
			QPoint point(points[1][i].x, points[1][i].y);
			
			if (bucket > 0 && bucket < bucketCutoff) 
			{
				// app-spec code
				p.setBrush(Qt::green);
				p.drawEllipse(QRect(point - QPoint(5,5), QSize(10,10)));
				
				moveSum += counter;
				moveCount ++;
			}
			else
			{
				p.setBrush(Qt::gray);
				p.drawEllipse(QRect(point - QPoint(3,3), QSize(6,6)));
			}
		}
	}
	
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
	historyMax = 10000;
	
	//qDebug() << "historyMax:"<<historyMax;
	
	int lineMaxHeight = imageCopy.size().height() * .25;
	int margin = 0;
	int lineZeroY = imageCopy.size().height() - margin;
	int lineX = margin;
	int lineLastY = lineZeroY;
	
	p.setPen(Qt::red);
	for(i=0; i<m_history.size(); i++)
	{
		int value = m_history[i];
		int lineHeight = (int)(
					((double)value/(double)historyMax) * 
					(lineMaxHeight - (margin*2))
				);
			
		int lineTop = lineZeroY - lineHeight;
		p.drawLine(i,lineLastY,i+1,lineTop);
		
		//qDebug() << "value:"<<value<<", lineHeight:"<<lineHeight<<", lineMaxHeight:"<<lineMaxHeight<<", margin:"<<margin<<", lineTop:"<<lineTop;
		
		lineLastY = lineTop;
	}
	
	
	int bucketHeight = 10;
	int bucketLeft   = 10;
	int bucketGraphStartY = 10;
	int bucketMaxWidth = 100;
	
	bucketMax = 200;

	for (i=1; i<bucketCutoff; i++)
	{
		int bucketValue = buckets[i];
		if(bucketValue > bucketMax)
			bucketValue = bucketMax;
			
		//qDebug() << i << (i * bucketValueWidth) << ((i+1) * bucketValueWidth) << bucketValue;
		
		//if(bucketValue <= 0)
		//	continue;
			
		//p.setBrush(Qt::green);
		p.setPen(QColor(0,80,0).rgba());
		
		int top = bucketGraphStartY + i*bucketHeight;
		int width = (((double)bucketValue) / ((double)bucketMax)) * bucketMaxWidth;
		p.fillRect(QRect(bucketLeft, top, width, bucketHeight), Qt::green);
		
		p.setPen(Qt::black);
		p.drawText( bucketLeft + 6, top + bucketHeight + 1, QString("%4: %1: %2% / %3").arg( i * bucketValueWidth ).arg(width).arg(bucketValue).arg(i) );
		p.setPen(Qt::white);
		p.drawText( bucketLeft + 5, top + bucketHeight, QString("%4: %1: %2% / %3").arg( i * bucketValueWidth ).arg(width).arg(bucketValue).arg(i) );
	}
	
	int includeCount = 0;
	for(i=1; i<bucketCutoff; i++)
		includeCount += buckets[i];
		
	int ignoredCount = 0;
	for(i=bucketCutoff; i<NUM_BUCKETS; i++)
		ignoredCount += buckets[i];
	
	int top = bucketGraphStartY + bucketCutoff*bucketHeight;
	p.setPen(Qt::black);
	p.drawText( bucketLeft + 6, top + bucketHeight + 1, QString("Ignored: %1").arg( ignoredCount ) );
	p.setPen(Qt::white);
	p.drawText( bucketLeft + 5, top + bucketHeight, QString("Ignored: %1").arg( ignoredCount ) );
	
	top = bucketGraphStartY + 0*bucketHeight;
	p.setPen(Qt::black);
	p.drawText( bucketLeft + 6, top + bucketHeight + 1, QString("Counted>0: %1").arg( includeCount ) );
	p.setPen(Qt::white);
	p.drawText( bucketLeft + 5, top + bucketHeight, QString("Counted>0: %1").arg( includeCount ) );
	
	
	qDebug() << "------";
	
	
	
// 	foreach(QPoint point, pointsList)
// 	{
// 		p.setBrush(Qt::green);
// 		p.drawEllipse(QRect(point - QPoint(3,3), QSize(6,6)));
// 	}
		
	
	
	
	
	
	
	
	
	
	
	
	
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




