#include "AnalysisFilter.h"

#include "VideoReceiver.h"

#define CHANGE_THRESHOLD 75

AnalysisFilter::AnalysisFilter(QObject* parent)
	: VideoFilter(parent)
	, m_frameAccumEnabled(true)
	, m_frameAccumNum(3) // just a guess
	, m_analysisWindow(12,12)
	, m_deltaHistoryMaxLength(-1)
	, m_outCounter(0)
{
	//setIsThreaded(true);
	connect(&m_fpsTimer, SIGNAL(timeout()), this, SLOT(generateOutputFrame()));
}

AnalysisFilter::~AnalysisFilter() 
{
	m_lastImage = QImage();
}

void AnalysisFilter::setAnalysisWindow(QSize size)
{
	m_analysisWindow = size;
	m_deltaHistoryMaxLength = -1; // recalculated later
	m_lastImage = QImage();
	m_lastMotionNumbers.clear();
}

void AnalysisFilter::setFrameAccumEnabled(bool enab)
{
	m_frameAccumEnabled = enab;
}

void AnalysisFilter::setFrameAccumNum(int n)
{
	m_frameAccumNum = n;
}

void AnalysisFilter::processFrame()
{
	//qDebug() << "AnalysisFilter::processFrame(): source:"<<m_source;
	
	// dont call generateHistogram here if manual fps is set because the 
	// m_fpsTimer will call it when it times out
	if(m_manualFps > 0)
		return;
	
	generateOutputFrame();
}

void AnalysisFilter::setFps(int fps)
{
	m_manualFps = fps;
	
	if(m_manualFps > 0)
	{
		qDebug() << "AnalysisFilter::setFps: New m_manualFps:"<<m_manualFps;
		m_fpsTimer.setInterval(1000/m_manualFps);
		
		if(!m_fpsTimer.isActive())
			m_fpsTimer.start();
	}
	else
	{
		if(m_fpsTimer.isActive())
			m_fpsTimer.stop();
	}
}

void AnalysisFilter::generateOutputFrame()
{
	if(!m_frame)
		return;
	
	QImage image = frameImage();
	
	if(m_deltaHistoryMaxLength < 0)
		m_deltaHistoryMaxLength = image.width() / m_analysisWindow.width();
		
	if(!m_maskImage_preScale.isNull() &&
	    m_maskImage.size() != image.size())
	{
	    m_maskImage = m_maskImage_preScale.scaled(image.size());
	    qDebug() << "AnalysisFilter: Loaded mask, size: "<<m_maskImage.size();
	}
	
	QImage intermImage = processImage(image);
	QImage outputImage = generateImage(intermImage);
	
	if(!m_outputImagePrefix.isEmpty())
	{
		m_outCounter ++;
		QString file = tr("%1-%2.png").arg(m_outputImagePrefix).arg(m_outCounter % 2 == 0 ? 0:1);
		qDebug() << "AnalysisFilter: Writing raw image to "<<file;
		image.save(file);
	}
	
	
	int msecLatency = m_frame->captureTime().msecsTo(QTime::currentTime());
	
	QPainter p(&outputImage);
	p.setFont(QFont("",18,QFont::Bold));
	p.setPen(Qt::blue);
	p.drawText(5,outputImage.rect().bottom() - 5, tr("%1 ms").arg(msecLatency));
	p.end();
	
	
	VideoFrame *frame = new VideoFrame(outputImage, m_fpsTimer.isActive() ? m_fpsTimer.interval() : m_frame->holdTime());
	
	//if(m_includeOriginalImage)
		frame->setCaptureTime(m_frame->captureTime());
	
	enqueue(frame);
}
	/*
void AnalysisFilter::drawBarRect(QPainter *p, int min, int max, int avg, int startX, int startY, int w, int h)
{
	int minScaled = (int) ( ( ((double)min) / 255. ) * w );
	int maxScaled = (int) ( ( ((double)max) / 255. ) * w );
	int avgScaled = (int) ( ( ((double)avg) / 255. ) * w );
	
	//qDebug() << "minScaled:"<<minScaled<<", maxScaled:"<<maxScaled<<", avgScaled:"<<avgScaled<<", w:"<<w;
	
	int x1 = minScaled + startX;
	int x2 = maxScaled + startX;
	int rw = x2-x1;
	
	int avgX = startX + avgScaled;
	int avgX1 = avgX - 2;
	int avgX2 = avgX + 2;
	int avgW = avgX2-avgX1;
	
	p->setOpacity(.33);
	p->drawRect(x1,startY,rw,h);
	
	p->setOpacity(.5);
	p->drawRect(avgX1,startY,avgW,h);
}
	*/
	
// Separating the processImage() and generateImage() routines allow us to 
// optionally only generate the output image if their are consumers connected
// to our filter. Not implemented as such yet, but designed this way so it's
// easy to impl later. 
QImage AnalysisFilter::processImage(const QImage& image)
{
	if(image.isNull())
		return image;
		
	QImage imagePreScaled = image;
	if(m_frameAccumEnabled)
	{
		m_frameAccum.enqueue(image);
		
		if(m_frameAccum.size() >= m_frameAccumNum)
			m_frameAccum.dequeue();
		
		QPainter p(&imagePreScaled);
		p.fillRect(imagePreScaled.rect(), Qt::black);
		
		double opac = 1. / (double)m_frameAccum.size();
		//qDebug() << "HistogramFilter::makeHistogram: [FRAME ACCUM] Queue Size: "<<m_frameAccum.size()<<", opac:"<<opac; 
		
		int counter = 0;
		foreach(QImage img, m_frameAccum)
		{
			p.setOpacity(counter == 0 ? 1 : opac);
			p.drawImage(0,0,img);
			counter ++;
		}
	}
	
	if(!m_maskImage.isNull())
	{
		QPainter p(&imagePreScaled);
		p.drawImage(0,0,m_maskImage);
	}
	
	
	QImage origScaled = imagePreScaled.scaled(m_analysisWindow.width(),m_analysisWindow.height(),Qt::IgnoreAspectRatio);
	if(origScaled.format() != QImage::Format_RGB32)
		origScaled = origScaled.convertToFormat(QImage::Format_RGB32);
	
	bool lastImageInvalid = false;
	if(m_lastImage.isNull())
	{
		m_lastImage = origScaled;
		m_lastMotionNumbers.clear();
		
		//while(m_lastMotionNumbers.size() < m_analysisWindow.height() * m_analysisWindow.width())
		//	m_lastMotionNumbers.append(0);
			
		lastImageInvalid = true;
	}
	
	m_motionNumbers.clear();
	
	QList<int> deltaList;
	int deltaSum = 0;
	bool deltasChangedFlag = false;
	
	QColor hsvConverter;
	int h,s,v;
	
	bool useHsvMode = true;
	
	int bytesPerLine = origScaled.bytesPerLine();
	for(int y=0; y<m_analysisWindow.height(); y++)
	{
		const uchar *line = (const uchar*)origScaled.scanLine(y);
		for(int x=0; x<bytesPerLine; x+=4)
		{
			const uchar b = line[x];
			const uchar g = x+1 >= bytesPerLine ? 0 : line[x+1];
			const uchar r = x+2 >= bytesPerLine ? 0 : line[x+2];
// 			const uchar a = x+3 >= bytesPerLine ? 0 : line[x+3];

			//
			int gray = -1;
			if(r || g || b)
			{
				if(useHsvMode)
				{
					hsvConverter.setRgb(r,g,b);
					hsvConverter.getHsv(&h,&s,&v);
					
					//// HACK scale Hue to be between 0-255 instead of 0-359
					h = (int) ( (((double)h)/359.)*255 );
					
					// Changes in hue are more important, followed by changes in value are more important, then saturation
					gray = (int)( h * .30 +
					              s * .11 +
					              v * .59 );
				}
				else
				{
					// These grayscale conversion values are just rough estimates based on my google research - adjust to suit your tastes
					gray = (int)( r * .30 + g * .59 + b * .11 );
				}
			}
			
			m_motionNumbers << gray;
				
			if(lastImageInvalid)
			{
				deltaList << 0;
			}
			else
			{
				int pointLocation = (y * m_analysisWindow.height()) + (x/4);
				int lastGray = m_lastMotionNumbers[pointLocation];
				int delta = abs(gray - lastGray);
 				//delta /= 128;
 				double partial = (double)(delta) / 100.0;
 				//delta *= 255;
 				delta = (int)(partial * 255.0);
 				
 				if(delta < CHANGE_THRESHOLD)
 					delta = 0;
				
				deltaList << delta;
				deltaSum += delta;
				
				int currentDelta = deltaList[pointLocation] - m_deltaNumbers[pointLocation];
				if(currentDelta > CHANGE_THRESHOLD) // magic threshold
				{
					deltasChangedFlag = true;
				}
			}
			
		}
	}
	
	int deltaAvg = deltaSum / deltaList.size();
	m_deltaNumbers = deltaList;
	m_lastMotionNumbers = m_motionNumbers;	
	m_lastImage = origScaled;
	
	m_deltaHistory.append(m_deltaNumbers);
	if(m_deltaHistory.size() > m_deltaHistoryMaxLength)
		m_deltaHistory.takeFirst();
	
	//qDebug() << "AnalysisFilter::processImage: deltaAvg:"<<deltaAvg;
	
	if(deltasChangedFlag)
	{
		emit deltasChanged(m_deltaNumbers);
		emit motionRatingChanged(deltaAvg);
		
		VideoReceiver *rx = dynamic_cast<VideoReceiver*>(videoSource());
		qDebug() << "AnalysisFilter::processImage: rx:"<<(rx ? tr("%1:%2").arg(rx->host()).arg(rx->port()) : "(null)")<<", deltaAvg:"<<deltaAvg; 
	}
	
	return imagePreScaled;
}

QImage AnalysisFilter::generateImage(const QImage& image)
{
	QImage outputImage = image.copy(); //(image.size(), QImage::Format_ARGB32);
	QPainter p(&outputImage);
	
	double scaleX = (double)image.size().width() / (double)m_analysisWindow.width();
	double scaleY = (double)image.size().height() / (double)m_analysisWindow.height();
	
	for(int wx=0; wx<m_analysisWindow.width(); wx++)
	{
		for(int wy=0; wy<m_analysisWindow.height(); wy++)
		{
			QRect rect((int)(wx * scaleX), (int)(wy * scaleY), 
			           (int)(scaleX),      (int)(scaleY));
			
			int pointLocation = (wy * m_analysisWindow.height()) + wx;
			// since the deltas are really deltas in grayscale values, which are 0-255, the deltas are 0-255 ...
			// ... which is exactly what the alpha channel expects in QColor
			//QColor alphaRed(255,0,0,255 - m_deltaNumbers[pointLocation]);
			int delta = m_deltaNumbers[pointLocation];
			if(delta <0)
				delta = 0;
			if(delta > 255)
				delta = 255; 
			QColor alphaRed(255,0,0,delta);
			
			p.fillRect(rect, alphaRed);
			
			int margin = 2;
			int lineZeroY = rect.bottom(); // small margin from bottom of rect
			int lineX = rect.left();
			int lineLastY = lineZeroY;
			
			p.setPen(Qt::red);
			
			foreach(QIntList list, m_deltaHistory)
			{
				int delta = list[pointLocation];
				if(delta <0)
					delta = 0;
				if(delta > 255)
					delta = 255;
					
				delta = (int)(
						((double)delta/255.0) * 
						(rect.height() - (margin*2))
					);
				
				int lineTop = lineZeroY - delta;
				
				p.drawLine(lineX,lineLastY,lineX+1,lineTop);
				
				lineLastY = lineTop;
				lineX ++;
			}
			
			p.setFont(QFont("",8));
			p.setPen(Qt::green);
			p.drawText( rect.left() + margin, rect.bottom() - margin, tr("%1").arg(delta));
		}
	}
	
	p.setFont(QFont("",12,QFont::Bold));
	if(!m_debugText.isEmpty())
	{
		p.setPen(Qt::black);
		p.drawText(6, 15, m_debugText);
		p.setPen(Qt::white);
		p.drawText(5, 14, m_debugText);
	}
	
	p.drawImage(outputImage.rect().bottomRight() - QPoint(64,64), m_lastImage.scaled(64,64,Qt::KeepAspectRatio));
	
	p.end();
	
	return outputImage;
}

