#include "TrackingFilter.h"


//#define CV_NO_BACKWARD_COMPATIBILITY

#include "cv.h"
#include "highgui.h"

#include <stdio.h>
#include <ctype.h>


//#define RESET_RATIO 0.75
#define RESET_RATIO 0.25


QImage TrackingFilter::trackPoints(QImage img)
{
	IplImage* frame = 0;
	int i, k;
	
	if(!m_timeInit)
	{
		m_timeInit = true;
		m_timeValue.start();
		m_timeFrameCounter = 0;
	}
	m_timeFrameCounter++;

	if(img.format() != QImage::Format_RGB888)
		img = img.convertToFormat(QImage::Format_RGB888);
		
	if(img.width() < 600)
		img = img.scaled(640,480);
		
	frame = cvCreateImageHeader( cvSize(img.width(), img.height()), IPL_DEPTH_8U, 3);
	frame->imageData = (char*) img.bits();

	if( !m_iplImage )
	{
		/* allocate all the buffers */
		m_iplImage = cvCreateImage( cvGetSize(frame), 8, 3 );
		m_iplImage->origin = frame->origin;
		
		m_greyImg	= cvCreateImage( cvGetSize(frame), 8, 1 );
		m_prevGreyImg	= cvCreateImage( cvGetSize(frame), 8, 1 );
		m_pyramid	= cvCreateImage( cvGetSize(frame), 8, 1 );
		m_prevPyramid	= cvCreateImage( cvGetSize(frame), 8, 1 );
		
		m_currPoints = (CvPoint2D32f*)cvAlloc( MAX_COUNT * sizeof(CvPoint2D32f));
		m_prevPoints = (CvPoint2D32f*)cvAlloc( MAX_COUNT * sizeof(CvPoint2D32f));
		m_cvStatus = (char*)cvAlloc(MAX_COUNT);
		m_cvFlags = 0;
	}

	cvCopy( frame, m_iplImage, 0 );
	cvCvtColor( m_iplImage, m_greyImg, CV_BGR2GRAY );
	
 	//QList<QPoint> pointsList;
	
	
	int win_size = 10;
	if( m_needInit )
	{
		/* automatic initialization */
		IplImage* eig  = cvCreateImage( cvGetSize(m_greyImg), 32, 1 );
		IplImage* temp = cvCreateImage( cvGetSize(m_greyImg), 32, 1 );
		double quality = 0.01;
		double min_distance = 10;
		
		m_validCount = MAX_COUNT;
		cvGoodFeaturesToTrack( m_greyImg, eig, temp, m_currPoints, &m_validCount,
					quality, min_distance, 0, 3, 0, 0.04 );
					
		cvFindCornerSubPix( m_greyImg, m_currPoints, m_validCount,
			cvSize(win_size,win_size), cvSize(-1,-1),
			cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03));
			
		cvReleaseImage( &eig );
		cvReleaseImage( &temp );
		
		for(i=0; i<MAX_COUNT; i++)
			m_pointInfo[i].reset();
		
		m_foundCount = m_validCount;
	}
	else if( m_validCount > 0 )
	{
		cvCalcOpticalFlowPyrLK( m_prevGreyImg, m_greyImg, m_prevPyramid, m_pyramid, 
			m_prevPoints, m_currPoints, m_validCount, cvSize(win_size,win_size), 3, m_cvStatus, 0,
			cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03), m_cvFlags );
			
		m_cvFlags |= CV_LKFLOW_PYR_A_READY;
		
		for( i = k = 0; i < m_validCount; i++ )
		{
			if( !m_cvStatus[i] )
			{
				m_pointInfo[i].valid = false;
				continue;
			}
		
			k++;
			
			//pointsList << QPoint(points[1][i].x, points[1][i].y);
		}
		m_validCount = k;
	}

	
	QImage imageCopy = img.copy();
	QPainter p(&imageCopy);
		
	int elapsed = m_timeValue.elapsed();
	double fps = ((double)elapsed) / ((double)m_timeFrameCounter) / 2.0;
	
	// Calculate movement vectors (distances between last positions)
	for (i=0; i<m_foundCount; i++) 
	{
		float dX, dY;
		float distSquared;
		if (m_pointInfo[i].valid)	// Make sure its an existing vector 
		{	
			dX = m_currPoints[i].x - m_prevPoints[i].x;
			dY = m_currPoints[i].y - m_prevPoints[i].y;
			
			// Make sure its not massive
			distSquared = dX*dX + dY*dY;
			
			float avg = m_pointInfo[i].storeValue(fabs(distSquared));
			
			float distDiff = fabs(m_pointInfo[i].lastValuesAvg - avg);
			bool hasChanged = distDiff > 0.001;
			m_pointInfo[i].distanceMovedChangeCount += hasChanged ? 1 : -1;
			if(m_pointInfo[i].distanceMovedChangeCount > fps)
				m_pointInfo[i].distanceMovedChangeCount --;
			if(m_pointInfo[i].distanceMovedChangeCount < 0)
				m_pointInfo[i].distanceMovedChangeCount = 0;
			m_pointInfo[i].distanceChangeFrequency = m_pointInfo[i].distanceMovedChangeCount / fps;
			
			m_pointInfo[i].storeChangeValue(m_pointInfo[i].distanceMovedChangeCount);
		}
	}
	
	
	QFont font = p.font();
	if(!m_cleanOutput)
		p.setFont(QFont("", 8));
		
	float moveSum = 0;
	int moveCount = 0;
	for (i=0; i<m_foundCount; i++) 
	{
		if (m_pointInfo[i].valid)	// Make sure its an existing vector 
		{	
			QPoint point((int)m_currPoints[i].x, (int)m_currPoints[i].y);
			
			//if (decays[i] == 1.0 || (counter > 0.1 && /*bucket >= 0 && */bucket < bucketCutoff))
			if(m_pointInfo[i].valuesAvg > m_userTuningMinMove &&
			(m_userTuningUseChangeValues ? m_pointInfo[i].changeValuesAvg > m_userTuningMinChange : true)) 
			{
				if(!m_cleanOutput)
				{
			
					p.setBrush(Qt::green);
					p.drawEllipse(QRect(point - QPoint(5,5), QSize(10,10)));
					//p.drawText(point + QPoint(-5,5), QString("%1").arg(i));
				}
				
				moveSum += m_pointInfo[i].valuesAvg;
				moveCount ++;

			}
// 			else
// 			{
// 				p.setBrush(Qt::gray);
// 				p.drawEllipse(QRect(point - QPoint(3,3), QSize(6,6)));
// 			}

		}
	}
	if(!m_cleanOutput)
		p.setFont(font);
	
	
	int moveAvg = (int)(moveSum / (!moveCount?1:moveCount));
	
	//qDebug() << "moveAvg:"<<moveAvg;
	m_history << moveAvg;
	if(m_history.size() > imageCopy.size().width())
		m_history.takeFirst();
	
	
	m_historyWindowTotal -= m_historyWindow[m_historyWindowIndex];
	m_historyWindowTotal += moveAvg;
	m_historyWindow[m_historyWindowIndex] = moveAvg;
	m_historyWindowIndex ++;
	if(m_historyWindowIndex >= m_historyWindowSize)
		m_historyWindowIndex = 0;
		 
	m_historyWindowAverage = m_historyWindowTotal / m_historyWindowSize;
	if(m_historyWindowAverage < 0)
		m_historyWindowAverage = 1;
	
	if(!m_cleanOutput)
	{
		int historyMax = 0;
		foreach(int val, m_history)
			if(val > historyMax)
				historyMax = val;
		
		if(historyMax<=0)
			historyMax = 1;
			
		//if(historyMax > 10000)
		//historyMax = 50000;
		//historyMax = 200;
		
		historyMax /= 2;
		
		//qDebug() << "historyMax:"<<historyMax<<", moveAvg:"<<moveAvg;
		
		int lineMaxHeight = (int)((double)imageCopy.size().height() * .25);
		int margin = 0;
		int lineZeroY = imageCopy.size().height() - margin;
		int lineLastY = lineZeroY;
		
		int lastLineHeight = 0;
		
		int textInc = 25;
		
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
				//int val = ratio*100;
				int val = value;
				p.setPen(QColor(0,0,0));//, (int)((double)(i/textInc) / (double)(imageCopy.size().width()/textInc) * 255.f)));
				p.drawText( QPoint( i+2, lineTop+1 ), QString("%1").arg((int)(val)) );
				p.setPen(QColor(255,255,255));//, (int)((double)(i/textInc) / (double)(imageCopy.size().width()/textInc) * 255.f)));
				p.drawText( QPoint( i+1, lineTop ), QString("%1").arg((int)(val)) );
			}
			
			//qDebug() << "value:"<<value<<", lineHeight:"<<lineHeight<<", lineMaxHeight:"<<lineMaxHeight<<", margin:"<<margin<<", lineTop:"<<lineTop;
			
			lineLastY = lineTop;
			//lastLineHeight = lineHeight;
			lastLineHeight = (int)(ratio*100);
		}
	
		p.setFont(QFont("", 24, QFont::Bold));
		p.setPen(Qt::black);
		p.drawText(imageCopy.rect().topRight() + QPoint( -149, 31 ), QString().sprintf("%06d", m_historyWindowAverage)); //moveAvg));
		p.setPen(Qt::white);
		p.drawText(imageCopy.rect().topRight() + QPoint( -150, 30 ), QString().sprintf("%06d", m_historyWindowAverage)); //moveAvg));
		p.setFont(font);
	} 
	
	if(m_historyWindowAverage <= 0)
	{
		//if(!m_cleanOutput)
			//qDebug() << QTime::currentTime().toString() << " - HistoryAverage reached ZERO";
		emit historyAvgZero();
	}
	
	emit historyAvg(m_historyWindowAverage);
	emit averageMovement(moveAvg);
	
	
	CV_SWAP( m_prevGreyImg, m_greyImg, m_swapTemp );
	CV_SWAP( m_prevPyramid, m_pyramid, m_swapTemp );
	CV_SWAP( m_prevPoints, m_currPoints, m_swapPoints );
	m_needInit = 0;
	
	if(m_validCount <= m_foundCount * m_resetRatio)
	{
		//qDebug() << "Need to init!";
		//if(!m_cleanOutput)
		//	qDebug() << "count less than "<<(m_foundCount * RESET_RATIO)<<", resetting.";
		m_needInit = 1;
	}
	else
	{
		//qDebug() << "Count:"<<count<<", reset point:"<<(m_foundCount * RESET_RATIO);
	}
	
	cvReleaseImage(&frame);
		
	
	//return pointsList;
	return imageCopy;
}


TrackingFilter::TrackingFilter(QObject *parent)
	: VideoFilter(parent)
	, m_cleanOutput(false)
{
	
	m_iplImage = 0;
	m_greyImg = 0;
	m_prevGreyImg = 0;
	m_pyramid = 0;
	m_prevPyramid = 0;
	m_swapTemp = 0;
	
	m_currPoints = 0;
	m_prevPoints = 0;
	m_swapPoints = 0;
	m_cvStatus = 0;
	m_validCount = 0;
	m_needInit = 0;
	m_cvFlags = 0;
	m_foundCount = 0;
	
	m_timeInit = false;
	m_timeFrameCounter = 0;
	
	m_userTuningMinMove   = TRACKING_DEFAULT_MIN_MOVE; // default 0.075
	m_userTuningMinChange = TRACKING_DEFAULT_MIN_CHANGE; // default 0.1
	m_userTuningUseChangeValues = true;
	
	for(int i=0; i<HISTORY_MAX_WINDOW_SIZE; i++)
		m_historyWindow[i] = 0;
	
	m_historyWindowTotal = 0;
	m_historyWindowSize = HISTORY_MAX_WINDOW_SIZE / 10;
	m_historyWindowAverage = 0;
	m_historyWindowIndex = 0;
	
	m_resetRatio = RESET_RATIO;
}

TrackingFilter::~TrackingFilter()
{
}

void TrackingFilter::processFrame()
{
	QImage image = frameImage();	
	QImage histo = trackPoints(image);
	
	enqueue(new VideoFrame(histo,m_frame->holdTime()));
}

void TrackingFilter::tune(float minMove, bool useChange, int minChange, int numValues, int numChangeValues, int historyWindowSize, float resetRatio)
{
	for(int i=0; i<MAX_COUNT; i++)
	{
		if(numValues > 0)
			m_pointInfo[i].userTuningNumValues = numValues;
		if(numChangeValues > 0)
			m_pointInfo[i].userTuningNumChangeValues = numChangeValues;
	}
	
	m_userTuningMinMove   = minMove   >= 0 ? minMove   : TRACKING_DEFAULT_MIN_MOVE;
	m_userTuningMinChange = minChange >= 0 ? minChange : TRACKING_DEFAULT_MIN_CHANGE;
	m_userTuningUseChangeValues = useChange;
	
	m_historyWindowSize = historyWindowSize > 0 ? 
		(historyWindowSize >= HISTORY_MAX_WINDOW_SIZE ? HISTORY_MAX_WINDOW_SIZE : historyWindowSize)
		:
		HISTORY_MAX_WINDOW_SIZE / 10;
		
	m_resetRatio = resetRatio > 0.f && resetRatio < 1.f ? resetRatio : RESET_RATIO;
}
