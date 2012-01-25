#ifndef TrackingFilter_H
#define TrackingFilter_H

#include <QtGui>
#include "VideoFilter.h"


#define MAX_COUNT 600

/*
#define PointType_Static 0
#define PointType_Live   1
#define PointType_Wierd  2*/

// 4 seconds of data at 30 frames a sec
#define POINT_INFO_MAX_VALUES 120
// Default to just a few frames over 30fps
#define POINT_INFO_DEFAULT_NUM_VALUES 32
#define POINT_INFO_DEFAULT_NUM_CHANGE_VALUES 5

#define TRACKING_DEFAULT_MIN_MOVE 0.075
#define TRACKING_DEFAULT_MIN_DIST_CHANGE 0.001
#define TRACKING_DEFAULT_MIN_CHANGE 0.1

#define HISTORY_MAX_WINDOW_SIZE 300

#include <opencv/cv.h>
#include <opencv/highgui.h>

class TrackingPointInfo 
{
public:
	typedef enum 
	{
		InvalidPoint = 0,
		NonObjectPoint,
		ObjectPoint,
		FrackPoint,
	
	} PointType;
	
	TrackingPointInfo() 
	{
		userTuningNumValues = 0;
		userTuningNumChangeValues = 0;
		reset(); 
	}
	
	void setType(PointType t, float conf=1.0)
	{
		type = t;
		typeConfidence = conf;
	}
	
	void reset() 
	{
		type = NonObjectPoint,
		//qDebug() << "[RESET] setType(), setting NON OBJECT";
		typeConfidence = 1.0;
		
		deltaCounter = 0;
		valid = true;
		
		for(int i=0; i<POINT_INFO_MAX_VALUES; i++)
		{
			values[i] = 0;
			changeValues[i] = 0;
		}
		
		numValuesUsed       = userTuningNumValues > 0       ? userTuningNumValues       : POINT_INFO_DEFAULT_NUM_VALUES;
		numChangeValuesUsed = userTuningNumChangeValues > 0 ? userTuningNumChangeValues : POINT_INFO_DEFAULT_NUM_CHANGE_VALUES;
		
		valuesIdx   = 0;
		valuesTotal = 0;
		valuesAvg   = 0;
		lastValuesAvg = 0;
		
		distanceMovedChangeCount = 0;
		
		changeValuesTotal = 0;
		changeValuesAvg   = 0;
		changeValuesIdx   = 0;
	};
	
	inline float storeValue(float value)
	{
		// Store last average for other uses
		lastValuesAvg = valuesAvg;
		
		valuesTotal -= values[valuesIdx];
		valuesTotal += value;
		values[valuesIdx] = value;
		
		valuesIdx ++;
		int max = numValuesUsed < POINT_INFO_MAX_VALUES ? numValuesUsed : POINT_INFO_MAX_VALUES;
		if(valuesIdx >= max)
			valuesIdx = 0;
		
		valuesAvg = valuesTotal / (float)max;
		return valuesAvg;
	}
	
	inline float storeChangeValue(float value)
	{
		changeValuesTotal -= changeValues[changeValuesIdx];
		changeValuesTotal += value;
		changeValues[changeValuesIdx] = value;
		
		changeValuesIdx ++;
		
		int max = numChangeValuesUsed < POINT_INFO_MAX_VALUES ? numChangeValuesUsed : POINT_INFO_MAX_VALUES;
		if(changeValuesIdx >= max)
			changeValuesIdx = 0;
		
		changeValuesAvg = changeValuesTotal / (float)max;
		return changeValuesAvg;
	}
	
	inline float storeDistAccum(float value)
	{
		distAccumTotal -= distAccum[distAccumIdx];
		distAccumTotal += value;
		distAccum[distAccumIdx] = value;
		
		distAccumIdx ++;
		
		numDistAccumValuesUsed = 10;
		int max = numDistAccumValuesUsed < POINT_INFO_MAX_VALUES ? numDistAccumValuesUsed : POINT_INFO_MAX_VALUES;
		if(distAccumIdx >= max)
			distAccumIdx = 0;
		
		distAccumAvg = distAccumTotal / (float)max;
		return distAccumAvg;
	}
	
	PointType type;
	float typeConfidence;
	
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
	int numChangeValuesUsed;
	
	float distAccum[POINT_INFO_MAX_VALUES];
	float distAccumTotal;
	float distAccumAvg;
	int distAccumIdx; 
	int numDistAccumValuesUsed;
	
	int userTuningNumValues;
	int userTuningNumChangeValues;
	int userTuningNumDistAccumValues;
};



class TrackingFilter : public VideoFilter
{
	Q_OBJECT
public:
	TrackingFilter(QObject *parent=0);
	~TrackingFilter();
	
	void autoTuneToFps(bool flag=true); 
	void tuneToFps(int fps);
	void tune(float minMove=-1, bool useChange=true, float minDistChange=-1, float minChange=-1, int numValues=-1, int numChangeValues=-1, int historyWindowSize=-1, float resetRatio=-1);
	
	void setCleanOutput(bool flag){  m_cleanOutput = flag; }
	
signals:
	void historyAvgZero();
	void historyAvg(int);
	void averageMovement(int);
	
	void trackingRectChanged(QRect);
	
public slots:
	void setTrackingRect(QRect);
	
protected slots:
	void reallyProcessFrame();

protected:
	virtual void processFrame();
	
	QImage trackPoints(QImage);
	
private:
	IplImage *m_iplImage,
		*m_greyImg,
		*m_prevGreyImg,
		*m_pyramid,
		*m_prevPyramid,
		*m_swapTemp;
	
	CvPoint2D32f *m_currPoints, *m_prevPoints, *m_swapPoints;
	char* m_cvStatus;
	int m_validCount;
	int m_needInit;
	int m_cvFlags;
	
	int m_foundCount;
	
	QTime m_timeValue;
	bool m_timeInit;
	int m_timeFrameCounter;
	
	float m_userTuningMinMove; // default 0.075
	float m_userTuningMinDistChange; // default 0.001
	float m_userTuningMinChange; // default 0.1;
	bool m_userTuningUseChangeValues;
	
	TrackingPointInfo m_pointInfo[MAX_COUNT];
	
	QList<int> m_history;
	
	int m_historyWindow[HISTORY_MAX_WINDOW_SIZE];
	int m_historyWindowTotal;
	int m_historyWindowSize;
	int m_historyWindowAverage;
	int m_historyWindowIndex;
	
	float m_resetRatio;
	
	bool m_cleanOutput;

	QTimer m_fpsTimer;
	QImage m_frameImage;
	
	bool m_autoTuneToFps;
	int m_lastAutoTuneFps;
	int m_zeroMoveFrameCounter;
	
	bool m_drawUnused;
	
	QRect m_origTrackingRect;
	QRect m_trackingRect;
};

#endif
