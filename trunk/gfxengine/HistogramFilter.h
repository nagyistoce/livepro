#ifndef HistogramFilter_H
#define HistogramFilter_H

#include <QtGui>
#include "VideoFilter.h"

#define HSV_STAT_MIN 0
#define HSV_STAT_MAX 1
#define HSV_STAT_AVG 2

#define HSV_CHAN_HUE 0
#define HSV_CHAN_SAT 1
#define HSV_CHAN_VAL 2

class HistogramFilter : public VideoFilter
{
	Q_OBJECT
public:
	HistogramFilter(QObject *parent=0);
	~HistogramFilter();
	
	enum HistoType { Gray, RGB, Red, Green, Blue, HSV, Everything };
	
	HistoType histoType() { return m_histoType; }
	bool includeOriginalImage() { return m_includeOriginalImage; }
	
	bool chartHsv() { return m_chartHsv; }
	bool calcHsvStats() { return m_calcHsvStats; }
	
	bool frameAccumEnabled() { return m_frameAccumEnabled; }
	int frameAccumNum() { return m_frameAccumNum; }
	bool drawBorder() { return m_drawBorder; }
	
	int hsvStatValue(int channel, int stat=HSV_STAT_MIN);

public slots:
	void setHistoType(HistoType type);
	void setIncludeOriginalImage(bool flag);
	
	void setChartHsv(bool flag);
	void setCalcHsvStats(bool flag);
	
	void setFrameAccumEnabled(bool);
	void setFrameAccumNum(int);
	
	void setDrawBorder(bool flag) { m_drawBorder = flag; }
	
	void setFps(int);

signals:
	void hsvStatsUpdated(int hMin, int hMax, int hAvg, 
	                     int sMin, int sMax, int sAvg,
	                     int vMin, int vMax, int vAvg);

private slots:	
	void generateHistogram();
		
protected:
	virtual void processFrame();
	
	QImage makeHistogram(const QImage& image, bool alwaysMakeImage=false);
	
	void drawBarRect(QPainter *p, int min, int max, int avg, int startX, int startY, int w, int h);
	
	HistoType m_histoType;
	bool m_includeOriginalImage;

	bool m_chartHsv;
	bool m_calcHsvStats;
	
	bool m_frameAccumEnabled;
	int m_frameAccumNum;
	
	QQueue<QImage> m_frameAccum;
	
	bool m_drawBorder;
	
// 	int m_histo[7][256];
	int m_hsvMin[3];
	int m_hsvMax[3];
	int m_hsvAvg[3];
	// true if m_hsv* have valid values (e.g. first frame received) 
	bool m_hsvStatsValid;
	
	QTimer m_generateHistoTimer;
	int m_manualFps;
	
};

#endif
