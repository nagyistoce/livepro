#ifndef VideoDifferenceFilter_H
#define VideoDifferenceFilter_H

#include <QtGui>
#include "VideoFilter.h"

class VideoDifferenceFilter : public VideoFilter
{
	Q_OBJECT
public:
	VideoDifferenceFilter(QObject *parent=0);
	~VideoDifferenceFilter();
	
	bool frameAccumEnabled() { return m_frameAccumEnabled; }
	int frameAccumNum() { return m_frameAccumNum; }
	
	int threshold() { return m_threshold; }
	int minTreshold() { return m_minThreshold; }

	enum FilterType { FrameToFrame, FrameToFirstFrame, FrameToRefImage };
	FilterType filterType() { return m_filterType; }
	
	QImage referenceImage() { return m_refImage; }
	
	enum OutputType { BinarySmoothed, Binary, JustNewPixels, HighlightNewPixels, BackgroundReplace };
	OutputType outputType() { return m_outputType; } 
	
public slots:
	void setFrameAccumEnabled(bool);
	void setFrameAccumNum(int);
	
	void setThreshold(int);
	void setMinThreshold(int);

	void setFilterType(FilterType);
	void setReferenceImage(QImage);
	
	void setOutputType(OutputType);
	
signals:
	void deltaAverages(int h, int s, int v, int metric);
	
protected:
	virtual void processFrame();
	
	QImage createDifferenceImage(QImage);
	
	QImage m_firstImage;
	QImage m_lastImage;
	bool m_includeOriginalImage;

	bool m_frameAccumEnabled;
	int m_frameAccumNum;
	
	int m_threshold;
	int m_minThreshold;
	
	FilterType m_filterType;
	QImage m_refImage; 
	QImage m_newBackground;
	
	OutputType m_outputType;
	
	QQueue<QImage> m_frameAccum;
	
	QQueue<QImage> m_firstFrameAccum;
	
	int m_frameCount;
};

#endif
