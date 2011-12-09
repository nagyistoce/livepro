#include "VideoDifferenceFilter.h"

#include <QtGui>
#include <QDebug>

VideoDifferenceFilter::VideoDifferenceFilter(QObject *parent)
	: VideoFilter(parent)
	, m_includeOriginalImage(true)
	, m_frameAccumEnabled(false)
	, m_frameAccumNum(5) // just a guess
	, m_threshold(25)
	, m_minThreshold(19)
	, m_filterType(FrameToFirstFrame)
	//, m_outputType(BinarySmoothed)
	, m_outputType(BackgroundReplace)
	, m_frameCount(0)
{
	m_newBackground = QImage("Pm5544.jpg");
}

VideoDifferenceFilter::~VideoDifferenceFilter()
{
}


void VideoDifferenceFilter::setFrameAccumEnabled(bool enab)
{
	m_frameAccumEnabled = enab;
}

void VideoDifferenceFilter::setFrameAccumNum(int n)
{
	m_frameAccumNum = n;
}

void VideoDifferenceFilter::setThreshold(int x)
{
	m_threshold = x;
}

void VideoDifferenceFilter::setMinThreshold(int x)
{
	m_minThreshold = x;
}

void VideoDifferenceFilter::setFilterType(FilterType type)
{
	m_filterType = type;
}

void VideoDifferenceFilter::setReferenceImage(QImage img)
{
	m_refImage = img;
}

void VideoDifferenceFilter::setOutputType(OutputType type)
{
	m_outputType = type;
}

void VideoDifferenceFilter::processFrame()
{
	QImage image = frameImage();	
	QImage histo = createDifferenceImage(image);
	
	VideoFrame *frame = new VideoFrame(histo,m_frame->holdTime());
	
	if(m_includeOriginalImage)
		frame->setCaptureTime(m_frame->captureTime());
	
	enqueue(frame);
}

// QImage IplImageToQImage(IplImage *iplImg)
// {
// 	int h        = iplImg->height;
// 	int w        = iplImg->width;
// 	int step     = iplImg->widthStep;
// 	char *data   = iplImg->imageData;
// 	int channels = iplImg->nChannels;
// 	
// 	QImage qimg(w, h, QImage::Format_ARGB32);
// 	for (int y = 0; y < h; y++, data += step)
// 	{
// 		for (int x = 0; x < w; x++)
// 		{
// 			char r=0, g=0, b=0, a=0;
// 			if (channels == 1)
// 			{
// 				r = data[x * channels];
// 				g = data[x * channels];
// 				b = data[x * channels];
// 			}
// 			else if (channels == 3 || channels == 4)
// 			{
// 				r = data[x * channels + 2];
// 				g = data[x * channels + 1];
// 				b = data[x * channels];
// 			}
// 			
// 			if (channels == 4)
// 			{
// 				a = data[x * channels + 3];
// 				qimg.setPixel(x, y, qRgba(r, g, b, a));
// 			}
// 			else
// 			{
// 				qimg.setPixel(x, y, qRgb(r, g, b));
// 			}
// 		}
// 	}
// 	return qimg;
// 
// }


 
/* qsort int comparison function */ 
int int_cmp(const void *a, const void *b) 
{ 
    const int *ia = (const int *)a; // casting pointer types 
    const int *ib = (const int *)b;
    return *ia  - *ib; 
	/* integer comparison: returns negative if b > a 
	and positive if a > b */ 
} 


namespace VideoFilter_NS
{
	// NOTE: This code was copied from the iLab saliency project by USC
	// found at http://ilab.usc.edu/source/. The note following is from
	// the USC code base. I've just converted it to use uchar* instead of
	// the iLab (I assume) specific typedef 'byte'. - Josiah 20100730

void bobDeinterlace(const uchar* src, const uchar* const srcend,
		    uchar* dest, uchar* const destend,
		    const int height, const int stride,
		    const bool in_bottom_field)
{

	// NOTE: this deinterlacing code was derived from and/or inspired by
	// code from tvtime (http://tvtime.sourceforge.net/); original
	// copyright notice here:

	/**
	* Copyright (c) 2001, 2002, 2003 Billy Biggs <vektor@dumbterm.net>.
	*
	* This program is free software; you can redistribute it and/or modify
	* it under the terms of the GNU General Public License as published by
	* the Free Software Foundation; either version 2, or (at your option)
	* any later version.
	*
	* This program is distributed in the hope that it will be useful,
	* but WITHOUT ANY WARRANTY; without even the implied warranty of
	* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	* GNU General Public License for more details.
	*
	* You should have received a copy of the GNU General Public License
	* along with this program; if not, write to the Free Software Foundation,
	* Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
	*/

// 	assert(height > 0);
// 	assert(stride > 0);

	// NOTE: on x86 machines with glibc 2.3.6 and g++ 3.4.4, it looks
	// like std::copy is faster than memcpy, so we use that to do the
	// copying here:

#if 1
#  define DOCOPY(dst,src,n) std::copy((src),(src)+(n),(dst))
#else
#  define DOCOPY(dst,src,n) memcpy((dst),(src),(n))
#endif

	if (in_bottom_field)
	{
		src += stride;

		DOCOPY(dest, src, stride);

		dest += stride;
	}

	DOCOPY(dest, src, stride);

	dest += stride;

	const int N = (height / 2) - 1;
	for (int i = 0; i < N; ++i)
	{
		const uchar* const src2 = src + (stride*2);

		for (int k = 0; k < stride; ++k)
			dest[k] = (src[k] + src2[k]) / 2;

		dest += stride;

		DOCOPY(dest, src2, stride);

		src += stride*2;
		dest += stride;
	}

	if (!in_bottom_field)
	{
		DOCOPY(dest, src, stride);

		src += stride*2;
		dest += stride;
	}
	else
		src += stride;

	// consistency check: make sure we've done all our counting right:

	if (src != srcend)
		qFatal("deinterlacing consistency check failed: %d src %p-%p=%d",
			int(in_bottom_field), src, srcend, int(src-srcend));

	if (dest != destend)
		qFatal("deinterlacing consistency check failed: %d dst %p-%p=%d",
			int(in_bottom_field), dest, destend, int(dest-destend));
}

#undef DOCOPY

}


QImage VideoDifferenceFilter::createDifferenceImage(QImage image)
{
	if(image.isNull())
		return image;
		
	QSize smallSize = image.size();
	smallSize.scale(200,150,Qt::KeepAspectRatio);
	//smallSize.scale(4,1,Qt::KeepAspectRatio);
	//smallSize.scale(160,120,Qt::KeepAspectRatio);
	
	QImage origScaled = image.scaled(smallSize);
	if(origScaled.format() != QImage::Format_RGB32)
		origScaled = origScaled.convertToFormat(QImage::Format_RGB32);
		
// 	if(!m_lastImage.isNull())
// 	{
// 		QImage smooth = QImage(smallSize, QImage::Format_RGB32);
// 		{
// 			unsigned int *dest = (unsigned int *)smooth.bits(); // use scanLine() instead of bits() to prevent deep copy
// 			/*const*/ unsigned int *src  = (/*const*/ unsigned int *)origScaled.bits();
// 			/*const*/ unsigned int *last = (/*const*/ unsigned int *)m_lastImage.bits();
// 			const int height = origScaled.height();
// 			const int stride = origScaled.bytesPerLine();
// 			
// 			smooth_native_32bit(last, dest, src, origScaled.byteCount());
// 		}
// 	
// 		origScaled = smooth.copy();
// 		origScaled.bits();
// 	}
	
	
	QImage deint = QImage(smallSize, QImage::Format_RGB32);
	bool bottomFrame = false;//m_frameCount ++ % 2 == 1;

	uchar * dest = deint.scanLine(0); // use scanLine() instead of bits() to prevent deep copy
	uchar * src  = origScaled.scanLine(0);
	const int height =  origScaled.height();
	const int stride = origScaled.bytesPerLine();

	VideoFilter_NS::bobDeinterlace( src,  src +height*stride,
					dest, dest+height*stride,
					height, stride, bottomFrame);
		
	origScaled = deint;
	
	
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
	else
	{
		// do fast 32bit smoothing of images


// 		QSize big = smallSize;
// 		big.scale(image.size() * 1.5,Qt::KeepAspectRatio);
// 		origScaled = origScaled.scaled(big,Qt::KeepAspectRatio,Qt::SmoothTransformation);
// 		origScaled = origScaled.scaled(smallSize,Qt::KeepAspectRatio,Qt::SmoothTransformation);
	}
	
	QImage baseFrame;
	
	if(m_lastImage.isNull())
		m_lastImage = origScaled.copy();

	
	if(m_filterType == FrameToRefImage)
		baseFrame = m_refImage;
	else
	if(m_filterType == FrameToFirstFrame)
	{
		if(m_firstImage.isNull())
			m_firstImage = QImage(origScaled.size(), origScaled.format());
			
		if(m_firstFrameAccum.size() < 3)
		{
			m_firstFrameAccum.enqueue(origScaled);
			
			QPainter p(&m_firstImage);
			p.fillRect(m_lastImage.rect(), Qt::black);
			
			double opac = 1. / (double)m_firstFrameAccum.size();
			//qDebug() << "HistogramFilter::makeHistogram: [FRAME ACCUM] Queue Size: "<<m_firstFrameAccum.size()<<", opac:"<<opac; 
			
			int counter = 0;
			foreach(QImage img, m_firstFrameAccum)
			{
				p.setOpacity(counter == 0 ? 1 : opac);
				p.drawImage(0,0,img);
				counter ++;
			}
		}
		
		baseFrame = m_firstImage;
	}
	else
	{
		baseFrame = m_lastImage;
	}
		
		
	
	QImage outputImage(smallSize, QImage::Format_RGB32);
	memset(outputImage.scanLine(0),0,outputImage.byteCount());
	
	QImage backgroundImage = m_newBackground.scaled(smallSize, Qt::IgnoreAspectRatio);
	
	QColor hsvConverter;
	int h1,s1,v1,
	    h2,s2,v2,
	    h,s,v,gray;
	
	int hsvMin[3] = {255,255,255};
	int hsvMax[3] = {0,0,0};
	int hsvAvg[3] = {0,0,0};
	int avgCounter = 0;
	
	int testAvg = 0;
	int testAvgCounter = 0;
	
// 	#define FILTER_SIZE 3
// 	int hueValues1[FILTER_SIZE];
// 	int hueValues2[FILTER_SIZE];
// 	memset(&hueValues1, 0, FILTER_SIZE * sizeof(int));
// 	memset(&hueValues2, 0, FILTER_SIZE * sizeof(int));
// 	
// 	int huePos1 = 0;
// 	int huePos2 = 0;
	
	// Convert each pixel to grayscale and count the number of times each
	// grayscale value appears in the image
	int bytesPerLine = origScaled.bytesPerLine();
	//qDebug() << "dest bpl:"<<outputImage.bytesPerLine()<<", source bpl:"<<bytesPerLine;
	for(int y=0; y<smallSize.height(); y++)
	{
		const uchar *lastLine = (const uchar*)m_lastImage.scanLine(y);
		const uchar *baseLine = (const uchar*)baseFrame.scanLine(y);
		/*const*/ uchar *thisLine = (/*const*/ uchar*)origScaled.scanLine(y);
		const uchar *backLine = (const uchar*)backgroundImage.scanLine(y);
		uchar *destLine = outputImage.scanLine(y);
// 		huePos1 = 0;
// 		huePos2 = 0;
		for(int x=0; x<bytesPerLine; x+=4)
		{
			
// 			const uchar a = line[x];
// 			const uchar r = x+1 >= bytesPerLine ? 0 : line[x+1];
// 			const uchar g = x+2 >= bytesPerLine ? 0 : line[x+2];
// 			const uchar b = x+3 >= bytesPerLine ? 0 : line[x+3];

			int columnNum = x;

			// Docs say QImage::Format_RGB32 stores colors as 0xffRRGGBB - 
			// - but when testing with a 4x1 image of (white,red,green,blue),
			//   I find that the colors (on linux 32bit anyway) were stored BGRA...is that only linux or is that all Qt platforms with Format_RGB32?  
			const uchar lb = lastLine[x];
			const uchar lg = x+1 >= bytesPerLine ? 0 : lastLine[x+1];
			const uchar lr = x+2 >= bytesPerLine ? 0 : lastLine[x+2];
			
			const uchar xb = baseLine[x];
			const uchar xg = x+1 >= bytesPerLine ? 0 : baseLine[x+1];
			const uchar xr = x+2 >= bytesPerLine ? 0 : baseLine[x+2];
			
			uchar tb = thisLine[x];
			uchar tg = x+1 >= bytesPerLine ? 0 : thisLine[x+1];
			uchar tr = x+2 >= bytesPerLine ? 0 : thisLine[x+2];
			
			tr = (tr+lr)/2;
			tg = (tg+lg)/2;
			tb = (tb+lb)/2;
			
			thisLine[x]   = tb;
			thisLine[x+1] = tg;
			thisLine[x+2] = tr;
			
// 			const uchar a = x+3 >= bytesPerLine ? 0 : line[x+3];
			
			// These grayscale conversion values are just rough estimates based on my google research - adjust to suit your tastes
// 			int thisGray = (int)( tr * .30 + tg * .59 + tb * .11 );
// 			int lastGray = (int)( xr * .30 + xg * .59 + xb * .11 );
			//qDebug() << "val:"<<(int)val<<", r:"<<qRed(pixel)<<", g:"<<qGreen(pixel)<<", b:"<<qBlue(pixel)<<", gray:"<<gray;
			//qDebug() << "r:"<<r<<", g:"<<g<<", b:"<<b<<", gray:"<<gray;
			
			hsvConverter.setRgb(tr,tg,tb);
			hsvConverter.getHsv(&h1,&s1,&v1);
			
			hsvConverter.setRgb(xr,xg,xb);
			hsvConverter.getHsv(&h2,&s2,&v2);
			
// 			// accumulate hue values
// 			hueValues1[huePos1] = h1;
// 			hueValues2[huePos2] = h2;
// 			
// 			// move hue accumulation index
// 			if(huePos1++ > FILTER_SIZE)
// 				huePos1 = FILTER_SIZE;
// 			if(huePos2++ > FILTER_SIZE)
// 				huePos2 = FILTER_SIZE;
// 				
// 			int hueAvg1 = 0;
// 			for(int i=0;i<huePos1;i++)
// 				hueAvg1 += hueValues1[i];
// 			hueAvg1 /= huePos1;
// 			
// 			int hueAvg2 = 0;
// 			for(int i=0;i<huePos2;i++)
// 				hueAvg2 += hueValues2[i];
// 			hueAvg2 /= huePos2;
			
			// sort hue values
// 			qsort(hueValues1, huePos1, sizeof(int), int_cmp);
// 			qsort(hueValues2, huePos2, sizeof(int), int_cmp);
				
			// if this hue is the highest of its neighbors, replace with median
			//if(h1 == hueValues1[huePos1-1])
				//h1 = hueAvg1; //hueValues1[huePos1/2];
				
			//if(h2 == hueValues2[huePos2-1])
				//h2 = hueAvg2; //hueValues2[huePos2/2];
			
// 			int inc = 10;
// 			h1 =  int((double)h1 / (double)inc) * inc;
// 			h2 =  int((double)h2 / (double)inc) * inc;
// 			s1 =  int((double)s1 / (double)inc) * inc;
// 			s2 =  int((double)s2 / (double)inc) * inc;
			
			// Find deltas for hsv and gray
			h = abs(h1-h2);
			s = abs(s1-s2);
			v = abs(v1-v2);
			//gray = thisGray - lastGray;
			
			//h = (int) ( (((double)h)/359.)*255 );
			
			
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
			
			//m_minThreshold = 14;
			//m_threshold = 55;
			
			if(!gray)
				gray =1;
			int testVal = v * .8 + h*.05; //h*.25 + v*.75 + s*.5; 
			//v;// / 255. + gray / 255.; 
			
			//gray * (h>50 ? 1.+(double)h/255. : 1.0);
			// * ((double)v/255.) + h * ((double)s/255.);
			// * .75 + v * .25;
			// (int)( gray * (h > 5 ? 2 : 1) );
			//(((double)h) / 360.));
			//h * .75 + v * .25; //2 + gray * .25/2;
			
			// threshold
			if(m_outputType == Binary)
			{
				//if(gray > m_threshold)
				if(testVal > m_threshold)
				{
					int v = 255;
					destLine[x+0] = v; //b
					destLine[x+1] = v; //g
					destLine[x+2] = v; //r
					destLine[x+3] = 0;   //a
				}
			}
			else
			if(m_outputType == BinarySmoothed)
			{
				if(testVal > m_minThreshold)
				{
					if(testVal > m_threshold)
						testVal = m_threshold;
					double size = (double)(m_threshold - m_minThreshold);
					int v = (int)( ((double)(testVal - m_minThreshold)) / size * 255. );
					destLine[columnNum  ] = v ; //b
					destLine[columnNum+1] = v; //g
					destLine[columnNum+2] = v; //r
					destLine[columnNum+3] = 0;   //a
				}
			}
			else
			if(m_outputType == JustNewPixels)
			{
				if(testVal > m_minThreshold)
				{
					destLine[columnNum  ] = tb ; //b
					destLine[columnNum+1] = tg; //g
					destLine[columnNum+2] = tr; //r
					destLine[columnNum+3] = 0;   //a
				}
			}
			else
			if(m_outputType == HighlightNewPixels)
			{
				int nr = tr,
					ng = tg,
					nb = tb;
				if(testVal > m_minThreshold)
				{
					if(testVal > m_threshold)
						testVal = m_threshold;
					double size = (double)(m_threshold - m_minThreshold);
					double a = ((double)(testVal - m_minThreshold)) / size + .75; 
					
					//double a = .75;
					int r=255, g=0, b=0; 
					nr = tr*a + r*a;
					ng = tg;//*a + g*a;
					nb = tb;//*a + b*a;
					
				}
				
				destLine[columnNum  ] = nb; //b
				destLine[columnNum+1] = ng; //g
				destLine[columnNum+2] = nr; //r
				destLine[columnNum+3] = 0;   //a
			}
			else
			if(m_outputType == BackgroundReplace)
			{
				uchar bb = backLine[x];
				uchar bg = x+1 >= bytesPerLine ? 0 : backLine[x+1];
				uchar br = x+2 >= bytesPerLine ? 0 : backLine[x+2];
				uchar ba = x+3 >= bytesPerLine ? 0 : backLine[x+3];

				//testVal = gray;
				if(testVal > m_minThreshold)
				{
					testAvg += testVal;
					testAvgCounter++;
					
					if(testVal > m_threshold)
						testVal = m_threshold;
					double size = (double)(m_threshold - m_minThreshold);
					double a = ((double)(testVal - m_minThreshold)) / size;// + .15;// + .75;
					
					if(a>1) a=1; 
					
					//ba = a * ba;
					int a1 = 1.0-a;
					
					//double a = .75;
					//int r=255, g=0, b=0; 
					
					br = br*a1 + tr*a;
					bg = bg*a1 + tg*a;
					bb = bb*a1 + tb*a;

// 						br = br + a*(tr-br);
// 						br = bg + a*(tg-bg);
// 						br = bb + a*(tb-bb);
					
				}
				
				destLine[columnNum  ] = bb; //b
				destLine[columnNum+1] = bg; //g
				destLine[columnNum+2] = br; //r
				destLine[columnNum+3] = ba;   //a
			}
			
			
			//qDebug() << "r:"<<r<<" \t g:"<<g<<" \t b:"<<b<<" \t gray:"<<gray<<", h:"<<h<<", s:"<<s<<", v:"<<v<<", x:"<<x<<",y:"<<y; 
		}
	}
	
	//if(m_filterType == FrameToFrame)
	m_lastImage = origScaled.copy();
	
	//qDebug() << "avgCounter:"<<avgCounter;
	//if(m_calcHsvStats)
	{
		if(avgCounter == 0)
			avgCounter = 1;
		hsvAvg[0] /= avgCounter;
		hsvAvg[1] /= avgCounter;
		hsvAvg[2] /= avgCounter;
		testAvg /= testAvgCounter?testAvgCounter:1;
		
		//qDebug() << "Avg: "<<testAvg;
		//emit hsvStatsUpdated(hsvMin[0],hsvMax[0],hsvAvg[0],hsvMin[1],hsvMax[1],hsvAvg[1],hsvMin[2],hsvMax[2],hsvAvg[2]);
		
/*		qDebug() << "HSV Deltas min/max/avg: " <<
			"\t H: "<<hsvMin[0]<<hsvMax[0]<<hsvAvg[0]<<
			"\t S: "<<hsvMin[1]<<hsvMax[1]<<hsvAvg[1]<<
			"\t V: "<<hsvMin[2]<<hsvMax[2]<<hsvAvg[2];*/
	
		emit deltaAverages(hsvAvg[0],hsvAvg[1],hsvAvg[2],testAvg);
		
		//qDebug() << "HSV Delta Averages: " <<hsvAvg[0]<<hsvAvg[1]<<hsvAvg[2]<<testAvg;
	
	}
	
	if(m_includeOriginalImage)
	{
// 		smallSize.scale(255,128,Qt::KeepAspectRatio);
// 		origScaled = origScaled.scaled(smallSize);
		
		// Render side by side with the original image
		QImage histogramOutput(smallSize.width() * 2, smallSize.height(), QImage::Format_RGB32);
		QPainter p2(&histogramOutput);
		p2.fillRect(histogramOutput.rect(),Qt::gray);
		p2.drawImage(QPointF(0,0),origScaled);
		p2.drawImage(QPointF(smallSize.width(),0),outputImage);
	
		return histogramOutput;
	}
	
	return outputImage;
}




