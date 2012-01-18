#include "TrackingFilter.h"


#define CV_NO_BACKWARD_COMPATIBILITY

#include "cv.h"
#include "highgui.h"
#include <stdio.h>
#include <ctype.h>

#include "Fourier.h"

IplImage *image = 0, *grey = 0, *prev_grey = 0, *pyramid = 0, *prev_pyramid = 0, *swap_temp;

int win_size = 10;
const int MAX_COUNT = 500;
CvPoint2D32f* points[2] = {0,0}, *swap_points;
bool valid[MAX_COUNT];
float deltaCounters[MAX_COUNT];
char* status = 0;
int count = 0;
int need_to_init = 0;
int night_mode = 0;
int flags = 0;
CvPoint pt;
int originalCount = 0;
float deltaSum = 0;

int changeRateCounters[MAX_COUNT];
int ampCounters[MAX_COUNT];
int frameCounter = 0;

QTime timeValue;
bool timeInit = false;
int timeFrameCounter = 0;

#define OSC_NUM_READINGS 10
int oscTrackingValues[MAX_COUNT][OSC_NUM_READINGS];
int oscTrackingCounters[MAX_COUNT][2]; // total, index
float lastDistance[MAX_COUNT];
int distanceChangeCount[MAX_COUNT];
float decays[MAX_COUNT];

//#define RESET_RATIO 0.75
#define RESET_RATIO 0.10
#define DELTA_COUNTER_DECAY 0.999

#define NUM_BUCKETS 2000
int buckets[NUM_BUCKETS];

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
	
	if(timeInit)
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
			for(k=0; k<OSC_NUM_READINGS; k++)
				oscTrackingValues[i][k] = 0;
			oscTrackingCounters[i][0] = 0; // total
			oscTrackingCounters[i][1] = 0; // index
			oscTrackingCounters[i][2] = 0; // last dist
			lastDistance[i] = 0;
			distanceChangeCount[i] = 0;
			decays[i] = DELTA_COUNTER_DECAY;


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
	int bucketCutoff = 25; /// Magic number
	
	deltaSum = 0;
	
	//double rateCutoff = 1.0; /// Magic number
	//int fps = 30; /// Just a guess
	
	int elapsed = timeValue.elapsed();
	double fps = 15; //((double)elapsed) / ((double)timeFrameCounter);
	
	int distatnceChangeOkCount = 0;
	
	// Calculate movement vectors (distances between last positions)
	for (i=0; i<originalCount; i++) 
	{
		float dX, dY;
		float distSquared;
		//int indexX, indexY;
		//int newCount;
		//CvScalar lineColor;
		//qDebug() << i << ", status: "<< status[i] << ", valid: "<<valid[i];
		if (status[i] && valid[i])	// Make sure its an existing vector 
		{	
			dX = points[1][i].x - points[0][i].x;
			dY = points[1][i].y - points[0][i].y;
			
			// Make sure its not massive
			distSquared = dX*dX + dY*dY;
			deltaCounters[i] *= decays[i]; //DELTA_COUNTER_DECAY; // 0.9
			
			
			// New attempt at osilation filtering
// 			int oscIdx   = oscTrackingCounters[i][1];
// 			int oscTotal = oscTrackingCounters[i][0];
			
// 			// subtract the last reading 
// 			oscTotal -= oscTrackingValues[i][oscIdx];
// 			
// 			// set the current reading
// 			oscTrackingValues[i][oscIdx] = distSquared;
			
			// update total
			float distDiff = abs(lastDistance[i] - distSquared);
			bool changed  = distDiff >= 0.0001;
			distanceChangeCount[i] += changed ? 1:-1;
			if(frameCounter > fps)
				distanceChangeCount[i] --;
			if(distanceChangeCount[i] < 0)
				distanceChangeCount[i] = 0;

			
			double changeFrequency = ((double)distanceChangeCount[i]) / 30.;
			
// 			// wrap idx to beginning if needed
// 			oscIdx ++;
// 			if(oscIdx >= OSC_NUM_READINGS)
// 				oscIdx = 0;
				
			// store totals and index
// 			oscTrackingCounters[i][0] = oscTotal;
// 			oscTrackingCounters[i][1] = oscIdx;
			//qDebug() << i << QString().sprintf("%.04f x %.04f , %.018f (%d,%d)",abs(dX),abs(dY),distSquared,points[1][i].x,(int)points[1][i].y);
			//qDebug() << i << changeFrequency;
			//qDebug() << i << QString().sprintf("%.05f", (changeFrequency < 0.00001 ? 0.f : changeFrequency)) << distanceChangeCount[i] << distDiff << changed;
			
			lastDistance[i] = distSquared;
			
			//qDebug() << i << QString().sprintf("%.03f", (distSquared < 0.001 ? 0.f : distSquared));
			//if(dX <= 0.0000000000001)
			//	qDebug() << i<<" ZERO";
			
			/// All this changeRate and ampCounters stuff is an attempt
			// to filter out points that are just oscilating back and forth very fast - so far it works maybe 10% of the time.
			// Need to do better!
			/*
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
			// Add distance change
			if(avgChangeRate <= rateCutoff ||
			   (ampAvg < 100 && ampAvg > 4)) /// MORE magic numbers
			*/
			//if(distanceChangeCount[i] > 0)
			{
				deltaCounters[i] += abs(distSquared);// * DELTA_COUNTER_DECAY;
				distatnceChangeOkCount ++;
			}
			
			if(deltaCounters[i] < 0)
				deltaCounters[i] = 0;
				
			
			//deltaCounters[i] = abs(distSquared);
			
			// Sum the total distance changed 
			//if(distanceChangeCount[i] > 0)
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
	
	//count = distatnceChangeOkCount;
	
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
		int bucket = (int)(deltaCounters[i] / bucketValueWidth);
		if(bucket < 0 || bucket >= NUM_BUCKETS-1)
			continue;
			
		buckets[bucket] ++;
		
		if(buckets[bucket] > bucketMax)
			bucketMax = buckets[bucket]; 
	}
	
	QFont font = p.font();
	p.setFont(QFont("", 8));
	
	int moveSum = 0;
	int moveCount = 0;
	for (i=0; i<originalCount; i++) 
	{
		if (status[i] && valid[i])	// Make sure its an existing vector 
		{	
			float counter = deltaCounters[i];
			int bucket = counter / bucketValueWidth;
			
			QPoint point(points[1][i].x, points[1][i].y);
			
			if (decays[i] == 1.0 || (counter > 0.1 && /*bucket >= 0 && */bucket < bucketCutoff)) 
			{
				// app-spec code
				if(/*decays[i] == 1.0 || */distanceChangeCount[i])
				{
					p.setBrush(Qt::green);
					p.drawEllipse(QRect(point - QPoint(5,5), QSize(10,10)));
					//p.drawText(point + QPoint(-5,5), QString("%1").arg(i));
					
					moveSum += counter;
					moveCount ++;
				}
				
				
			}
// 			else
// 			{
// 				p.setBrush(Qt::gray);
// 				p.drawEllipse(QRect(point - QPoint(3,3), QSize(6,6)));
// 			}
		}
	}
	p.setFont(font);
	if(frameCounter > fps)
	{
		for (i=0; i<originalCount; i++) 
		{
			if (status[i] && valid[i])	// Make sure its an existing vector 
			{	
				float counter = deltaCounters[i];
				int bucket = counter / bucketValueWidth;
				
				if (counter > 0.1 && /*bucket >= 0 && */bucket < bucketCutoff) 
				{
					// app-spec code
					if(distanceChangeCount[i])
					{
						decays[i] = 1;
					}
				}
			}
		}
	}
	
	if(frameCounter > fps * 4)
	{
		qDebug() << "moveCount:"<<moveCount;
		count = moveCount;
	}
	else
		qDebug() << "waiting for "<<frameCounter<<" > "<<(fps*4);
	
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
	historyMax = 50000;
	
	//qDebug() << "historyMax:"<<historyMax;
	
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
		
		p.setPen(QColor(255,0,0, (int)((double)i / (double)(imageCopy.size().width()) * 255.f)));
			
		p.drawLine(i,lineLastY,i+1,lineTop);
		
		if(i % textInc == 0)
		{
			p.setPen(QColor(0,0,0, (int)((double)(i/textInc) / (double)(imageCopy.size().width()/textInc) * 255.f)));
			p.drawText( QPoint( i+2, lineTop+1 ), QString("%1").arg((int)(ratio*100)) );
			p.setPen(QColor(255,255,255, (int)((double)(i/textInc) / (double)(imageCopy.size().width()/textInc) * 255.f)));
			p.drawText( QPoint( i+1, lineTop ), QString("%1").arg((int)(ratio*100)) );
		}
		
		//qDebug() << "value:"<<value<<", lineHeight:"<<lineHeight<<", lineMaxHeight:"<<lineMaxHeight<<", margin:"<<margin<<", lineTop:"<<lineTop;
		
		lineLastY = lineTop;
		//lastLineHeight = lineHeight;
		lastLineHeight = ratio*100;
	}
	/*
	QFont font = p.font();
	p.setFont(QFont("", 24, QFont::Bold));
	p.setPen(Qt::black);
	p.drawText(imageCopy.rect().bottomRight() - QPoint(44, 4), QString("%1").arg(lastLineHeight));
	p.setPen(Qt::white);
	p.drawText(imageCopy.rect().bottomRight() - QPoint(45, 5), QString("%1").arg(lastLineHeight));
	p.setFont(font);*/
	
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
	
	
	//qDebug() << "------";
	
	
	
// 	foreach(QPoint point, pointsList)
// 	{
// 		p.setBrush(Qt::green);
// 		p.drawEllipse(QRect(point - QPoint(3,3), QSize(6,6)));
// 	}
		
	
	
	
	
/*	//Creates a CFourier object
	//The CFourier class has the FFT function and a couple of usefull variables
	Fourier fft;

	//sample rate of the signal (must be 2^n)
	long sample_rate=8192;
	
	if(fps <= 0 || fps > 1000)
		fps = 30;
	
	sample_rate = pow2roundup((int)fps);
	
	//number of samples you want to send for processing in the fft (any)
	//for example 100 samples
	long captured_samples = m_history.size();
	if(captured_samples <= 0)
		captured_samples = 1;
// 	//frequency of the signal (has to be smaller than sample_rate/2)
// 	//for example 46
 	int frequency=250;

	float data[5000];

	//example of a sin signal(you can try to add some more signals to see the
	//fourier change
	
	int histAvg = 0;
	if(m_history.size() > 0)
	{
		foreach(int val, m_history)
			histAvg += val;
		histAvg /= m_history.size();
	}

 	for(int i=0; i<captured_samples; i++)
 	{
 		double temp = (double)(2*fft.pi*frequency*((float)i/sample_rate));		
 		//data[i] = ((double)30*sin(temp));
		data[i] = i < m_history.size() ? (float)(m_history.at(i) - histAvg) : 0;
 		//qDebug() << i << data[i];	
 	}
 	qDebug() << "executeFFT: captured_samples:"<<captured_samples<<", sample_rate:"<<sample_rate;
	
	//aply the FFT to the signal
	fft.executeFFT(data,captured_samples,sample_rate,1);
	
	//do the drawing of the signal
	//the fft object has two usefull variables
	//one is the fft.fundamental_frequency which contains the value of
	//the fundamental frequency
	//the second one is the fft.vector which contains the values of the
	//Fourier signal we will use to do the drawing

	//dcMem.PatBlt(0, 0,rcClient.right, rcClient.bottom, WHITENESS);
	p.fillRect(imageCopy.rect(), QColor(255,255,255,127));
	p.setPen(Qt::black);
		
	p.drawText(5,20, QString("Fundamental Frequency: %1").arg(fft.fundamental_frequency));
	int x,y, x_temp, y_temp;
	QRect rect = imageCopy.rect();
	for(x=0 ; x<rect.width(); x++)
	{
		//this temp variables are used to ajust the sign to the wiindow
		//if you want to see more of the signal ajust these fields
		x_temp = ( (x*(sample_rate/2)) / rect.width() );
		y_temp = (int)(
			( rect.height() * ( pow( fft.vector[2*x_temp] , 2) + pow( fft.vector[2*x_temp+1] , 2 ) ) )
			/
			( (double)pow( fft.vector[2*fft.fundamental_frequency] , 2 ) + pow( fft.vector[2*fft.fundamental_frequency+1] , 2 ) )
		);
		
		p.drawLine(x, rect.bottom(), x, rect.bottom() - y_temp);
	}
*/
	
	
	
	
	
	
	
	CV_SWAP( prev_grey, grey, swap_temp );
	CV_SWAP( prev_pyramid, pyramid, swap_temp );
	CV_SWAP( points[0], points[1], swap_points );
	need_to_init = 0;
	//cvShowImage( "LkDemo", image );
		
	if(count <= 1) //originalCount * RESET_RATIO)
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




