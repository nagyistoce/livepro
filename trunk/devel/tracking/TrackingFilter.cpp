#include "TrackingFilter.h"


//#define CV_NO_BACKWARD_COMPATIBILITY

// #include "cv.h"
// #include "highgui.h"

#include <stdio.h>
#include <ctype.h>
#include <math.h>

#define RESET_RATIO 0.75
//#define RESET_RATIO 0.25


QImage TrackingFilter::trackPoints(QImage img)
{
	if(img.isNull())
		return QImage();
		
	IplImage* frame = 0;
	int i, k;
	
	if(!m_timeInit)
	{
		m_timeInit = true;
		m_timeValue.restart();
		m_timeFrameCounter = 0;
	}
	m_timeFrameCounter++;

	// For testing...
	//img = img.scaled(240,180);

	// RGB888 is compat with CV 3chan 8U
	if(img.format() != QImage::Format_RGB888)
		img = img.convertToFormat(QImage::Format_RGB888);
		
	// For testing...
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
				m_pointInfo[i].setType(TrackingPointInfo::InvalidPoint);
				//qDebug() << "setType(), point:"<<i<<", setting INVALID";
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
	double fps = ((double)m_timeFrameCounter) /  ((double)elapsed / 1000.0);
	
 	if(m_timeFrameCounter % (int)fps == 0)
 	{
 		if(m_autoTuneToFps)
 		{
 			if(((int)fps) != m_lastAutoTuneFps)
 				tuneToFps((int)fps);
 				
 			m_lastAutoTuneFps = (int)fps;
 		}
 		
 		//qDebug() << "Measured fps:" <<fps;
 	}
 		
	
	QRect newRect;
	
	// Calculate movement vectors (distances between last positions)
	for (i=0; i<m_foundCount; i++) 
	{
		float dX, dY;
		float distSquared;
		//if (m_pointInfo[i].valid)	// Make sure its an existing vector 
		{	
			dX = m_currPoints[i].x - m_prevPoints[i].x;
			dY = m_currPoints[i].y - m_prevPoints[i].y;
			
			// Make sure its not massive
			distSquared = dX*dX + dY*dY;
			
			float avg = m_pointInfo[i].storeValue(fabs(distSquared));
			
			float distDiff = fabs(m_pointInfo[i].lastValuesAvg - avg);
			bool hasChanged = distDiff > m_userTuningMinDistChange;
			
			m_pointInfo[i].distanceMovedChangeCount += hasChanged ? 1 : -1;
			
// 			if(m_pointInfo[i].distanceMovedChangeCount > fps)
// 				m_pointInfo[i].distanceMovedChangeCount --;
				
			if(m_pointInfo[i].distanceMovedChangeCount < 0)
				m_pointInfo[i].distanceMovedChangeCount = 0;
				
			//m_pointInfo[i].distanceChangeFrequency = m_pointInfo[i].distanceMovedChangeCount / fps;
			//if(m_pointInfo[i].changeValuesAvg <= 0.001)
			//	qDebug() << i << ": ZERO changeValuesAvg, fps:"<<fps<<", distanceMovedChangeCount:"<<m_pointInfo[i].distanceMovedChangeCount<<", hasChanged:"<<hasChanged<<", distDiff:"<<distDiff<<", m_userTuningMinDistChange:"<<m_userTuningMinDistChange<<", avg:"<<avg;
			
			m_pointInfo[i].storeChangeValue(m_pointInfo[i].distanceMovedChangeCount);
			
			double changePerFps = (double)m_pointInfo[i].changeValuesAvg / (double)fps;
			//if(m_pointInfo[i].changeValuesAvg <= m_userTuningMinChange ||
			
			
			
			
			float avgDist = m_pointInfo[i].storeValue(distSquared);
			
			
			
			
			
			
			bool changeIsZero = changePerFps < 0.001;
			bool distGtrZero  = fabs(avgDist) > 0.011;
			if(
				// High change rate, low distance
				(changePerFps > 10.0 &&
				 fabs(avgDist) < 10.0) ||
				// approx zero change, some dist
				(changeIsZero &&
				  (distGtrZero || isnan(avgDist)))
				
			)
			{
				m_pointInfo[i].setType(TrackingPointInfo::FrackPoint);
				//qDebug() << "setType(), point:"<<i<<", setting FRACK";
			}
			else
			{
				QPoint point((int)m_currPoints[i].x, (int)m_currPoints[i].y);
					
					
				if(m_pointInfo[i].type == TrackingPointInfo::FrackPoint ||
				   m_pointInfo[i].type == TrackingPointInfo::InvalidPoint)
				{
					m_pointInfo[i].setType(TrackingPointInfo::NonObjectPoint);
				}
				
				if(m_pointInfo[i].type == TrackingPointInfo::NonObjectPoint &&
				   m_trackingRect.contains(point))
				{
					m_pointInfo[i].setType(TrackingPointInfo::ObjectPoint);
				}
				else
				if(m_pointInfo[i].type == TrackingPointInfo::ObjectPoint &&
				  !m_trackingRect.contains(point))
				{
					m_pointInfo[i].setType(TrackingPointInfo::NonObjectPoint);
				}
					
				//qDebug() << "setType(), point:"<<i<<", setting NON OBJECT";
				
			}
			
			if(m_pointInfo[i].type == TrackingPointInfo::ObjectPoint)
			{
				QPoint point((int)m_currPoints[i].x, (int)m_currPoints[i].y);
				QRect pointRect(point - QPoint(5,5), QSize(10,10));
				
				if(newRect.isNull())
					newRect = pointRect;
				else
					newRect = newRect.united(pointRect);
			}
			
			//qDebug() << i << "type:"<<(int)m_pointInfo[i].type<<", count/fps: "<<QString().sprintf("%.03f",changePerFps)<<", avgDist:"<<QString().sprintf("%.03f",avgDist)<<", changeIsZero:"<<changeIsZero<<", distGtrZero:"<<distGtrZero<<", isnan(dist):"<<isnan(avgDist) << fabs(avgDist);
			
// 			if(m_pointInfo[i].valuesAvg > m_userTuningMinMove &&
// 			  (m_userTuningUseChangeValues ? m_pointInfo[i].changeValuesAvg > m_userTuningMinChange : true))
// 			{
// 				/// change type?
// 				
// 			}
		}
	}
	
	
	QFont font = p.font();
	if(!m_cleanOutput)
		p.setFont(QFont("", 9, QFont::Bold));
		
	
	
	float moveSum = 0;
	int moveCount = 0;
	for (i=0; i<m_foundCount; i++) 
	{
		//if (m_pointInfo[i].valid)	// Make sure its an existing vector 
		{	
			QPoint point((int)m_currPoints[i].x, (int)m_currPoints[i].y);
				
			TrackingPointInfo::PointType type = m_pointInfo[i].type;
			
// 			InvalidPoint = 0,
// 			NonObjectPoint,
// 			ObjectPoint,
// 			FrackPoint,
			
			bool valid=false;
			
			//p.setBrush(Qt::green);
			//qDebug() << "Point:"<<i<<", type:"<<(int)type;
						
			if(type == TrackingPointInfo::InvalidPoint)
			{
				p.setBrush(Qt::white);
				//valid=true;
			}
			else
			if(type == TrackingPointInfo::NonObjectPoint)
			{
				p.setBrush(Qt::cyan);
				//valid=true;
			}
			else
			if(type == TrackingPointInfo::ObjectPoint)
			{
				p.setBrush(Qt::red);
				//valid=true;
			}
			else
			if(type == TrackingPointInfo::FrackPoint)
			{
				p.setBrush(Qt::black);
				//valid=true;
			}
			
			//valid = false;
			if(valid)
			{
				QRect pointRect(point - QPoint(5,5), QSize(10,10));
			
				p.setPen(Qt::black);
				p.drawEllipse(pointRect);
				
				p.setPen(Qt::black);
				p.drawText(point + QPoint(-3,5), QString("%1").arg( i ));
				
				p.setPen(Qt::yellow);
				p.drawText(point + QPoint(-4,4), QString("%1").arg( i ));
			}
			
					//p.drawText(point + QPoint(-5,5), QString("%1/%4 | %2/%3").arg( m_pointInfo[i].changeValuesAvg ).arg(m_pointInfo[i].valuesAvg).arg(m_userTuningMinMove).arg(m_userTuningMinChange));
				
				
// 				moveSum += m_pointInfo[i].valuesAvg;
// 				moveCount ++;

// 			}
// 			else
// 			if(m_trackingRect.contains(point))
// 			{
// 				p.setBrush(Qt::red);
// 				p.drawEllipse(QRect(point - QPoint(5,5), QSize(10,10)));
// 			}
// 			else
// 			if(m_drawUnused)
// 			{
// 				p.setBrush(Qt::gray);
// 				p.drawEllipse(QRect(point - QPoint(5,5), QSize(10,10)));
// 				//p.drawText(point + QPoint(-5,5), QString("%1 | %2").arg( m_pointInfo[i].changeValuesAvg ).arg(m_pointInfo[i].valuesAvg));
// 				//p.drawText(point + QPoint(-5,5), QString("%1/%4 | %2/%3").arg( m_pointInfo[i].changeValuesAvg ).arg(m_pointInfo[i].valuesAvg).arg(m_userTuningMinMove).arg(m_userTuningMinChange));
// 			}
// 			
// 			
// 			if(m_trackingRect.contains(point)) // &&
// 			  //(m_userTuningUseChangeValues ? m_pointInfo[i].changeValuesAvg > m_userTuningMinChange : true))
// 			{
// 				QRect prect = QRect(point - QPoint(5,5), QSize(10,10));
// 				if(newRect.isNull())
// 					newRect = prect;
// 				else
// 					newRect = newRect.united(prect); 
// 			}

		}
	}
	if(!m_cleanOutput)
		p.setFont(font);
	
	m_trackingRect = newRect;
	
	if(!m_trackingRect.isNull())
	{
		int penWidth = 2;
		p.setPen(QPen(Qt::red,(double)penWidth));
		p.setBrush(QBrush());
		p.drawRect(m_trackingRect.adjusted(penWidth/2,penWidth/2,-penWidth/2,-penWidth/2));
	}
	else
	{
		m_trackingRect = m_origTrackingRect;
	}
	
	int moveAvg = moveCount == 0 ? 0 : (int)(moveSum / moveCount);

	m_history << moveAvg;
	if(m_history.size() > imageCopy.size().width())
		m_history.takeFirst();
	
	
	//m_historyWindowTotal -= m_historyWindow[m_historyWindowIndex];
	//m_historyWindowTotal += moveAvg;
	m_historyWindow[m_historyWindowIndex] = moveAvg;
	m_historyWindowIndex ++;
	if(m_historyWindowIndex >= m_historyWindowSize)
		m_historyWindowIndex = 0;
		
	m_historyWindowTotal = 0;
	for(i=0; i<m_historyWindowSize; i++)
		m_historyWindowTotal += m_historyWindow[i]; 
		 
	m_historyWindowAverage = m_historyWindowTotal / m_historyWindowSize;
	
	//qDebug() << "moveAvg:"<<moveAvg<<", m_historyWindowAverage:"<<m_historyWindowAverage;
	
	if(m_historyWindowAverage < 0)
	{
// 		if(!m_cleanOutput)
// 			qDebug() << "Capped history window, value:"<<m_historyWindowAverage<<", derived from: "<<m_historyWindowTotal<<",m_historyWindowSize:"<<m_historyWindowSize<<", moveAvg:"<<moveAvg;
		m_historyWindowAverage = 1;
	}
	
// 	if(!m_cleanOutput)
// 	{
// 		int historyMax = 0;
// 		foreach(int val, m_history)
// 			if(val > historyMax)
// 				historyMax = val;
// 		
// 		if(historyMax<=0)
// 			historyMax = 1;
// 			
// 		//if(historyMax > 10000)
// 		//historyMax = 50000;
// 		//historyMax = 200;
// 		
// 		historyMax /= 2;
// 		
// 		//qDebug() << "historyMax:"<<historyMax<<", moveAvg:"<<moveAvg;
// 		
// 		int lineMaxHeight = (int)((double)imageCopy.size().height() * .25);
// 		int margin = 0;
// 		int lineZeroY = imageCopy.size().height() - margin;
// 		int lineLastY = lineZeroY;
// 		
// 		int lastLineHeight = 0;
// 		
// 		int textInc = 25;
// 		
// 		p.setPen(Qt::red);
// 		for(i=0; i<m_history.size(); i++)
// 		{
// 			p.setPen(Qt::red);
// 			
// 			int value = m_history[i];
// 			double ratio = ((double)value/(double)historyMax);
// 			int lineHeight = (int)(
// 						ratio *  
// 						(lineMaxHeight - (margin*2))
// 					);
// 				
// 			int lineTop = lineZeroY - lineHeight;
// 			
// 			//p.setPen(QColor(255,0,0, (int)((double)i / (double)(imageCopy.size().width()) * 255.f)));
// 			p.setPen(Qt::white);
// 				
// 			p.drawLine(i+1,lineLastY+1,i+2,lineTop+1);
// 			
// 			p.setPen(Qt::red);
// 				
// 			p.drawLine(i,lineLastY,i+1,lineTop);
// 			
// 			if(i % textInc == 0)
// 			{
// 				//int val = ratio*100;
// 				int val = value;
// 				p.setPen(QColor(0,0,0));//, (int)((double)(i/textInc) / (double)(imageCopy.size().width()/textInc) * 255.f)));
// 				p.drawText( QPoint( i+2, lineTop+1 ), QString("%1").arg((int)(val)) );
// 				p.setPen(QColor(255,255,255));//, (int)((double)(i/textInc) / (double)(imageCopy.size().width()/textInc) * 255.f)));
// 				p.drawText( QPoint( i+1, lineTop ), QString("%1").arg((int)(val)) );
// 			}
// 			
// 			//qDebug() << "value:"<<value<<", lineHeight:"<<lineHeight<<", lineMaxHeight:"<<lineMaxHeight<<", margin:"<<margin<<", lineTop:"<<lineTop;
// 			
// 			lineLastY = lineTop;
// 			//lastLineHeight = lineHeight;
// 			lastLineHeight = (int)(ratio*100);
// 		}
// 	
// 		p.setFont(QFont("", 24, QFont::Bold));
// 		p.setPen(Qt::black);
// 		p.drawText(imageCopy.rect().topRight() + QPoint( -149, 31 ), QString().sprintf("%06d", m_historyWindowAverage)); //moveAvg));
// 		p.setPen(Qt::white);
// 		p.drawText(imageCopy.rect().topRight() + QPoint( -150, 30 ), QString().sprintf("%06d", m_historyWindowAverage)); //moveAvg));
// 		p.setFont(font);
// 	} 
	
	if(m_historyWindowAverage <= 0)
	{
		//if(!m_cleanOutput)
			//qDebug() << QTime::currentTime().toString() << " - HistoryAverage reached ZERO";
		emit historyAvgZero();
		
		m_zeroMoveFrameCounter ++;
	}
	else
	{
		m_zeroMoveFrameCounter = 0;
	}
	
// 	if(m_zeroMoveFrameCounter > fps * 5)
// 	{
// 		// if five seconds of zero movement have passed, force re-init of points
// 		m_validCount = 0;
// 		
// 		//if(!m_cleanOutput)
// 			//qDebug() << "m_zeroMoveFrameCounter("<<m_zeroMoveFrameCounter<<") > "<<(fps*5)<<", resetting points";
// 	}
	
	emit historyAvg(m_historyWindowAverage);
	emit averageMovement(moveAvg);
	
	
	CV_SWAP( m_prevGreyImg, m_greyImg, m_swapTemp );
	CV_SWAP( m_prevPyramid, m_pyramid, m_swapTemp );
	CV_SWAP( m_prevPoints, m_currPoints, m_swapPoints );
 	m_needInit = 0;


	if(m_validCount <= m_foundCount * m_resetRatio)
	{
		//qDebug() << "Need to init!";
		if(!m_cleanOutput)
			qDebug() << "Valid points:"<<m_validCount<<", but it's less than "<<(m_foundCount * m_resetRatio)<<", resetting.";
		m_needInit = 1;
	}
	else
	{
		//qDebug() << "Count:"<<m_validCount<<", reset point:"<<(m_foundCount * m_resetRatio);
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
	m_needInit = 1;
	m_cvFlags = 0;
	m_foundCount = 0;
	
	m_timeInit = false;
	m_timeFrameCounter = 0;
	
	m_userTuningMinMove   = TRACKING_DEFAULT_MIN_MOVE; // default 0.075
	m_userTuningMinChange = .1; //TRACKING_DEFAULT_MIN_CHANGE; // default 0.1
	m_userTuningUseChangeValues = true;
	
	for(int i=0; i<HISTORY_MAX_WINDOW_SIZE; i++)
		m_historyWindow[i] = 0;
	
	m_historyWindowTotal = 0;
	m_historyWindowSize = HISTORY_MAX_WINDOW_SIZE / 10;
	m_historyWindowAverage = 0;
	m_historyWindowIndex = 0;
	
	m_resetRatio = RESET_RATIO;
	
	m_autoTuneToFps = true;
	m_lastAutoTuneFps = 0;
	
// 	connect(&m_fpsTimer, SIGNAL(timeout()), this, SLOT(reallyProcessFrame()));
// 	m_fpsTimer.setInterval(1000/10);
// 	m_fpsTimer.start();
	
	m_drawUnused = false;
}

TrackingFilter::~TrackingFilter()
{
}

void TrackingFilter::processFrame()
{
	m_frameImage = frameImage();
	if(!m_fpsTimer.isActive())
		reallyProcessFrame();
}

void TrackingFilter::reallyProcessFrame()
{
// 	QImage image = frameImage();	
// 	QImage histo = trackPoints(image);
	QImage image = m_frameImage;	
	QImage histo = trackPoints(image);
	
	enqueue(new VideoFrame(histo,m_frame->holdTime()));
}

void TrackingFilter::autoTuneToFps(bool flag)
{
	if(m_autoTuneToFps && !flag)
		tune(); // reset to defaults
	
	m_autoTuneToFps = flag;
}

double TrackingFilter_mapValueToSlope(double value1, double fps1, double value2, double fps2, double curFps)
{
	// Simple line formulas - find line slope, find y intercept, calculate new X value from "y" (curFps)
	// slope: (y2-y1)/(x2-x1)
	double slope = 
		((((double)value2) - ((double)value1)) /
		 (((double)fps2)   - ((double)fps1))   );
	// intercept: y1-slope*x1
	double intercept = value1 - slope * fps1;
	// new value: slope * x3 + intercept
	double newValue = slope * ((double)curFps) + intercept;
	return newValue;
}
	
void TrackingFilter::tuneToFps(int fps)
{
	// Values to tune based on fps:
	float minMove = -1; // use default
	bool useChange = true; // default
	
	float minDistChange = (float)TrackingFilter_mapValueToSlope(
		0.100, 10.f,
		0.001, 30.f,
		(double)fps);
	//TRACKING_DEFAULT_MIN_DIST_CHANGE 0.001 -> 0.1
	
	float minChange = (float)TrackingFilter_mapValueToSlope(
		0.100, 10.f,
		0.001, 30.f,
		(double)fps);
	
	// just match fps
	int numValues = fps;
	
	int numChangeValues = (int)TrackingFilter_mapValueToSlope(
		3.f, 10.f, // 3 at 10 fps
		5.f, 30.f, // 5 at 30 fps
		(double)fps); 
	//POINT_INFO_DEFAULT_NUM_CHANGE_VALUES 5 -> 3
	
	// just match fps
	int historyWindowSize = fps;
	
	float resetRatio = (float)TrackingFilter_mapValueToSlope(
		0.25, 10.f,  // 0.25 at 10fps
		0.75, 30.f,  // 0.75 at 30fps
		(double)fps);
	//resetRatio 0.25 - 0.75
	
	qDebug() << "tuneToFps("<<fps<<"): minDistChange:"<<minDistChange<<", minChange:"<<minChange<<", numValues:"<<numValues<<", numChangeValues:"<<numChangeValues<<", resetRatio:"<<resetRatio<<", historyWindowSize:"<<historyWindowSize;
	// apply new parameters
	tune(minMove, useChange, minDistChange, minChange, numValues, numChangeValues, historyWindowSize, resetRatio);
	
}

void TrackingFilter::tune(float minMove, bool useChange, float minDistChange, float minChange, int numValues, int numChangeValues, int historyWindowSize, float resetRatio)
{
	for(int i=0; i<MAX_COUNT; i++)
	{
		if(numValues > 0)
			m_pointInfo[i].userTuningNumValues = numValues;
		if(numChangeValues > 0)
			m_pointInfo[i].userTuningNumChangeValues = numChangeValues;
	}
	
	m_userTuningMinMove         = minMove       >= 0 ? minMove       : TRACKING_DEFAULT_MIN_MOVE;
	m_userTuningMinDistChange   = minDistChange >= 0 ? minDistChange : TRACKING_DEFAULT_MIN_DIST_CHANGE;
	m_userTuningMinChange       = minChange     >= 0 ? minChange     : TRACKING_DEFAULT_MIN_CHANGE;
	m_userTuningUseChangeValues = useChange;
	
	m_historyWindowSize = historyWindowSize > 0 ? 
		(historyWindowSize >= HISTORY_MAX_WINDOW_SIZE ? HISTORY_MAX_WINDOW_SIZE : historyWindowSize)
		:
		HISTORY_MAX_WINDOW_SIZE / 10;
		
	m_resetRatio = resetRatio > 0.f && resetRatio < 1.f ? resetRatio : RESET_RATIO;
}

void TrackingFilter::setTrackingRect(QRect rect)
{
	m_origTrackingRect = rect;
	m_trackingRect = rect;
}
