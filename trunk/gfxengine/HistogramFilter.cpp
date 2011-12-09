#include "HistogramFilter.h"

HistogramFilter::HistogramFilter(QObject* parent)
	: VideoFilter(parent)
	, m_histoType(HSV)
	, m_includeOriginalImage(true)
	, m_chartHsv(true)
	, m_calcHsvStats(true)
	, m_frameAccumEnabled(false)
	, m_frameAccumNum(10) // just a guess
	, m_drawBorder(true)
{
	//setIsThreaded(true);
}

HistogramFilter::~HistogramFilter() {}

void HistogramFilter::setHistoType(HistoType t)
{
	m_histoType = t;
	processFrame();
}

void HistogramFilter::setIncludeOriginalImage(bool flag)
{
	m_includeOriginalImage = flag;
	processFrame();
}

void HistogramFilter::setChartHsv(bool flag)
{
	if(flag && !m_calcHsvStats)
		m_calcHsvStats = true;
	m_chartHsv = flag;
}

void HistogramFilter::setCalcHsvStats(bool flag)
{
	m_calcHsvStats = flag;
}

void HistogramFilter::setFrameAccumEnabled(bool enab)
{
	m_frameAccumEnabled = enab;
}

void HistogramFilter::setFrameAccumNum(int n)
{
	m_frameAccumNum = n;
}

void HistogramFilter::processFrame()
{
	//qDebug() << "HistogramFilter::processFrame(): source:"<<m_source;
	if(!m_frame)
		return;
	
	QImage image = frameImage();
	QImage histo = makeHistogram(image);
	
	VideoFrame *frame = new VideoFrame(histo,m_frame->holdTime());
	
	if(m_includeOriginalImage)
		frame->setCaptureTime(m_frame->captureTime());
	
	enqueue(frame);
}
	
void HistogramFilter::drawBarRect(QPainter *p, int min, int max, int avg, int startX, int startY, int w, int h)
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
	
	
QImage HistogramFilter::makeHistogram(const QImage& image)
{
	if(image.isNull())
		return image;
		
	QSize smallSize = image.size();
	smallSize.scale(200,150,Qt::KeepAspectRatio);
	//smallSize.scale(4,1,Qt::KeepAspectRatio);
	//smallSize.scale(160,120,Qt::KeepAspectRatio);
	
	QImage origScaled = image;//.scaled(640,480).scaled(smallSize);
	if(origScaled.format() != QImage::Format_RGB32)
		origScaled = origScaled.convertToFormat(QImage::Format_RGB32);
		
	if(m_frameAccumEnabled)
	{
		m_frameAccum.enqueue(origScaled);
		
		if(m_frameAccum.size() >= m_frameAccumNum)
			m_frameAccum.dequeue();
		
		QPainter p(&origScaled);
		p.fillRect(origScaled.rect(), Qt::black);
		
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
	

	
	// Setup our list of color values and set to 0
	int histo[7][256];
	for(int c=0;c<7;c++)
		for(int i=0;i<256;i++)
			histo[c][i] = 0;
	
	QColor hsvConverter;
	int h,s,v;
	
	int hsvMin[3] = {255,255,255};
	int hsvMax[3] = {0,0,0};
	int hsvAvg[3] = {0,0,0};
	int avgCounter = 0;
	
	// Convert each pixel to grayscale and count the number of times each
	// grayscale value appears in the image
	int bytesPerLine = origScaled.bytesPerLine();
	for(int y=0; y<smallSize.height(); y++)
	{
		const uchar *line = (const uchar*)origScaled.scanLine(y);
		for(int x=0; x<bytesPerLine; x+=4)
		{
			
// 			const uchar a = line[x];
// 			const uchar r = x+1 >= bytesPerLine ? 0 : line[x+1];
// 			const uchar g = x+2 >= bytesPerLine ? 0 : line[x+2];
// 			const uchar b = x+3 >= bytesPerLine ? 0 : line[x+3];

			// Docs say QImage::Format_RGB32 stores colors as 0xffRRGGBB - 
			// - but when testing with a 4x1 image of (white,red,green,blue),
			//   I find that the colors (on linux 32bit anyway) were stored BGRA...is that only linux or is that all Qt platforms with Format_RGB32?  
			const uchar b = line[x];
			const uchar g = x+1 >= bytesPerLine ? 0 : line[x+1];
			const uchar r = x+2 >= bytesPerLine ? 0 : line[x+2];
// 			const uchar a = x+3 >= bytesPerLine ? 0 : line[x+3];
			if(r || g || b)
			{
				// These grayscale conversion values are just rough estimates based on my google research - adjust to suit your tastes
				int gray = (int)( r * .30 + g * .59 + b * .11 );
				//qDebug() << "val:"<<(int)val<<", r:"<<qRed(pixel)<<", g:"<<qGreen(pixel)<<", b:"<<qBlue(pixel)<<", gray:"<<gray;
				//qDebug() << "r:"<<r<<", g:"<<g<<", b:"<<b<<", gray:"<<gray;
				histo[0][gray] ++;
				histo[1][r]++;
				histo[2][g]++;
				histo[3][b]++;
				
				if(m_calcHsvStats || m_histoType == HSV)
				{
					
					hsvConverter.setRgb(r,g,b);
					hsvConverter.getHsv(&h,&s,&v);
					
					/// HACK scale Hue to be between 0-256 instead of 0-265
					h = (int) ( (((double)h)/359.)*255 );
					
					histo[4][h]++;
					histo[5][s]++;
					histo[6][v]++;
					
					if(h > hsvMax[0])
						hsvMax[0] = h;
					if(s > hsvMax[1])
						hsvMax[1] = s;
					if(v > hsvMax[2])
						hsvMax[2] = v;
					if(h && h < hsvMin[0])
						hsvMin[0] = h;
					if(s && s < hsvMin[1])
						hsvMin[1] = s;
					if(v && v < hsvMin[2])
						hsvMin[2] = v;
					
					hsvAvg[0] += h;
					hsvAvg[1] += s;
					hsvAvg[2] += v;
					
					avgCounter ++;
				}
				
				//qDebug() << "r:"<<r<<" \t g:"<<g<<" \t b:"<<b<<" \t gray:"<<gray<<", h:"<<h<<", s:"<<s<<", v:"<<v<<", x:"<<x<<",y:"<<y; 
			} 
		}
	}
	
	//qDebug() << "avgCounter:"<<avgCounter;
	if(m_calcHsvStats)
	{
		if(avgCounter == 0)
			avgCounter = 1;
		hsvAvg[0] /= avgCounter;
		hsvAvg[1] /= avgCounter;
		hsvAvg[2] /= avgCounter;
		
		emit hsvStatsUpdated(hsvMin[0],hsvMax[0],hsvAvg[0],hsvMin[1],hsvMax[1],hsvAvg[1],hsvMin[2],hsvMax[2],hsvAvg[2]);
		
// 		qDebug() << "HSV min/max/avg: " <<
// 			"\t H: "<<hsvMin[0]<<hsvMax[0]<<hsvAvg[0]<<
// 			"\t S: "<<hsvMin[1]<<hsvMax[1]<<hsvAvg[1]<<
// 			"\t V: "<<hsvMin[2]<<hsvMax[2]<<hsvAvg[2];
	
	}
	
/*	qDebug() << "HSV min/max/avg: " <<
		"\t H: "<<hsvMin[0]<<hsvMax[0]<<hsvAvg[0]<<
		"\t S: "<<hsvMin[1]<<hsvMax[1]<<hsvAvg[1]<<
		"\t V: "<<hsvMin[2]<<hsvMax[2]<<hsvAvg[2];*/
	
	// Calc the max and avg pixel counts
 	int min[7] = {255,255,255,255,255,255,255};
 	int max[7] = {0,0,0,0,0,0,0};
 	int avg[7] = {0,0,0,0,0,0,0};
 	int maxv = 0;
 	
 	for(int c=0;c<7;c++)
 	{
		for(int i=0;i<256;i++)
		{
			int count = histo[c][i];
			if(count > max[c])
				max[c] = count;
			if(count < min[c])
				min[c] = count;
			// maxv is only max of r/g/b
			if(c > 0 && count > maxv)
				maxv = count;
			avg[c] += count;
		}
 		
 		avg[c] /= 255;
 		
//  		if( c == 4 )
//  			qDebug() << "Hue: min:"<<min[c]<<", max:"<<max[c]<<", avg:"<<avg[c];
//  		else
//  		if( c == 5 )
//  			qDebug() << "Sat: min:"<<min[c]<<", max:"<<max[c]<<", avg:"<<avg[c];
//  		else
//  		if( c == 6 )
//  			qDebug() << "Val: min:"<<min[c]<<", max:"<<max[c]<<", avg:"<<avg[c];
 	}
 	//qDebug() << "Avg:"<<avg<<", max:"<<max;

	// Draw a very simple bar graph of the pixel counts
	QImage histogram(QSize(255,128), QImage::Format_RGB32);
	int maxHeight = 128;
	QPainter p(&histogram);
	p.setPen(Qt::blue);
	p.fillRect(histogram.rect(), Qt::gray);
	p.setOpacity(.33);
	
	int chanStart = m_histoType == Gray ? 0 :
			m_histoType == RGB  ? 1 :
			m_histoType == Red  ? 1 :
			m_histoType == Green? 2 :
			m_histoType == Blue ? 3 :
			m_histoType == HSV  ? 4 : 0;
	int chanEnd   = m_histoType == Gray ? 1 :
			m_histoType == RGB  ? 4 :
			m_histoType == Red  ? 2 :
			m_histoType == Green? 3 :
			m_histoType == Blue ? 4 : 
			m_histoType == HSV  ? 7 : 4;
	for(int c=chanStart;c<chanEnd;c++)
	{
		p.setPen(
			c == 1 || c == 4? Qt::red   :
			c == 2 || c == 5? Qt::green :
			c == 3 || c == 6? Qt::blue  :
			        Qt::black
			);
			
		// Decide on an appropriate scaling value to keep the graph somewhat "pretty"
		// The 4 is just a magic number I guessed at...
		//double scaler = (double) ( max[c] );/// (!avg[c]?1:avg[c]) > 4 ? avg[c] * 4 : max[c] );
		
		int av = avg[c] * 3;
		double scaler = (double) ( max[c] > av ? av : max[c] );
		//qDebug() << "scaler:"<<scaler<<" (max[c]:"<<max[c]<<", avg:"<<av<<")";
		
		int last = -1;
		int next = -1;
		if(!scaler)
			scaler = 1; 
		for(int i=0;i<256;i++)
		{
			int count   = histo[c][i];
			int oldCount = count;
			
			next = i<255 ? histo[c][i+1] : -1; 
			
			if(last > -1 && next > -1)
			{
				count = (count+last+next)/3;	
			}
			else
			if(last > -1)
			{
				count = (count+last)/2;
			}
			
			last = oldCount;
			
			double perc = ((double)count)/scaler;
			double val  = perc * maxHeight;
			int scaled  = (int)val;
			
			
			
			//qDebug() << "c:"<<c<<",i:"<<i<<", count:"<<count<<", perc:"<<perc<<", val:"<<val<<", scaled:"<<scaled<<",maxv:"<<maxv<<",max[c]:"<<max[c]<<",avg[c]:"<<avg[c]<<",scaler:"<<scaler;
			
			p.drawLine(i,maxHeight-scaled, i,maxHeight);
			
		}
		
	}
	
		
	if(m_histoType == HSV &&
	   m_chartHsv)
	{ 
		int x1 = 0; 
		//x2 = 177;
		int rh  = 20;
		int y1 = 20;
		int y2 = y1 + rh + 12;
		int y3 = y2 + rh + 12;
		int rw = 254;
			
		//void drawBarRect(QPainter *p, int min, int max, int avg, int startX, int startY, int w, int h)
		p.setPen(QPen());
		p.setBrush(Qt::red);
		drawBarRect(&p, hsvMin[0], hsvMax[0], hsvAvg[0], x1, y1, rw, rh);
		p.setOpacity(1);
		p.drawText(hsvMin[0]+5, y1+15, "Hue");
		p.setBrush(Qt::green);
		drawBarRect(&p, hsvMin[1], hsvMax[1], hsvAvg[1], x1, y2, rw, rh);
		p.setOpacity(1);
		p.drawText(hsvMin[1]+5, y2+15, "Sat");
		p.setBrush(Qt::blue);
		drawBarRect(&p, hsvMin[2], hsvMax[2], hsvAvg[2], x1, y3, rw, rh);
		p.setOpacity(1);
		p.drawText(hsvMin[2]+5, y3+15, "Val");
	}
	
	

	p.end();
	
	if(m_includeOriginalImage)
	{
		smallSize.scale(255,128,Qt::KeepAspectRatio);
		origScaled = origScaled.scaled(smallSize);
		
		// Render side by side with the original image
		QImage histogramOutput(smallSize.width() + 255, smallSize.height(), QImage::Format_RGB32);
		QPainter p2(&histogramOutput);
		p2.fillRect(histogramOutput.rect(),Qt::gray);
		p2.drawImage(QPointF(0,0),origScaled);
		p2.drawImage(QPointF(smallSize.width(),0),histogram);
		if(m_drawBorder)
		{
			p2.setPen(QPen(Qt::black,1.5));
			p2.drawRect(histogramOutput.rect().adjusted(0,0,-1,-1));
		}
		//qDebug() << "Histo size:"<<histogramOutput.size();
	
		return histogramOutput;
	}
	else
	if(m_drawBorder)
	{
		QPainter p2(&histogram);
		p2.setPen(QPen(Qt::black,1.5));
		p2.drawRect(histogram.rect().adjusted(0,0,-1,-1));
		p2.end();
	}
	
	return histogram; 
}
