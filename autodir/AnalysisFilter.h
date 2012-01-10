#ifndef AnalysisFilter_H
#define AnalysisFilter_H

#include <QtGui>
#include "VideoFilter.h"

class AnalysisFilter : public VideoFilter
{
	Q_OBJECT
public:
	AnalysisFilter(QObject *parent=0);
	~AnalysisFilter();
	
	QSize analysisWindow() { return m_analysisWindow; }

	bool frameAccumEnabled() { return m_frameAccumEnabled; }
	int frameAccumNum() { return m_frameAccumNum; }
	
public slots:
	void setAnalysisWindow(QSize);
	
	void setFrameAccumEnabled(bool);
	void setFrameAccumNum(int);
	
	void setFps(int);
	
	void setDebugText(const QString& txt) { m_debugText = txt; }

signals:
// 	void hsvStatsUpdated(int hMin, int hMax, int hAvg, 
// 	                     int sMin, int sMax, int sAvg,
// 	                     int vMin, int vMax, int vAvg);

	void deltasChanged(QList<int>);
	void motionRatingChanged(int);

private slots:	
	void generateOutputFrame();
		
protected:
	virtual void processFrame();
	
	QImage processImage(const QImage&);
	QImage generateImage(const QImage& image);
	
	QTimer m_fpsTimer;
	int m_manualFps;
	
	bool m_frameAccumEnabled;
	int m_frameAccumNum;
	
	QQueue<QImage> m_frameAccum;
	
	QImage m_lastImage;
	QSize m_analysisWindow;
	QList<int> m_motionNumbers;
	QList<int> m_lastMotionNumbers;
	QList<int> m_deltaNumbers;
	
	QString m_debugText;
	
};

#endif
