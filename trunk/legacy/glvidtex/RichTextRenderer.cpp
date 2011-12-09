#include "RichTextRenderer.h"
#include "../ImageFilters.h"

QCache<QString,double> RichTextRenderer::static_autoTextSizeCache;

RichTextRenderer::RichTextRenderer(QObject *parent)
	: QObject(parent)
	, m_textWidth(640)
	, m_outlineEnabled(true)
	, m_outlinePen(Qt::black, 2.0)
	, m_fillEnabled(true)
	, m_fillBrush(Qt::white)
	, m_shadowEnabled(true)
	, m_shadowBlurRadius(16)
	, m_shadowOffsetX(3)
	, m_shadowOffsetY(3)
	, m_updatesLocked(false)
	, m_scaling(1.,1.)
{
// 	qDebug() << "RichTextRenderer::ctor(): \t in thread:"<<QThread::currentThreadId();
	connect(&m_updateTimer, SIGNAL(timeout()), this, SLOT(renderText()));
	m_updateTimer.setInterval(100);
	m_updateTimer.setSingleShot(true);
}

void RichTextRenderer::setHtml(const QString& html)
{
	m_html = html;
	// dont block calling thread by updating - since we are in a thread ourself, 
	// this should put the update into our thread
	//QTimer::singleShot(0,this,SLOT(update()));
	//qDebug() << "RichTextRenderer::setHtml: m_updatesLocked:"<<m_updatesLocked; 
	if(m_updatesLocked)
	{
		if(m_updateTimer.isActive())
 			m_updateTimer.stop();
		return;
	}
		
	update();
}

bool RichTextRenderer::lockUpdates(bool flag)
{
	bool oldValue = m_updatesLocked;
	m_updatesLocked = flag;
	return oldValue;
}

void RichTextRenderer::setScaling(QPointF scale)
{
	m_scaling = scale;
}

void RichTextRenderer::changeFontSize(double size)
{
	QTextDocument doc;
	if (Qt::mightBeRichText(html()))
		doc.setHtml(html());
	else
		doc.setPlainText(html());

	QTextCursor cursor(&doc);
	cursor.select(QTextCursor::Document);

	QTextCharFormat format;
	format.setFontPointSize(size);
	cursor.mergeCharFormat(format);
	cursor.mergeBlockCharFormat(format);

	setHtml(doc.toHtml());
}


void RichTextRenderer::changeFontColor(QColor color)
{
	QTextDocument doc;
	if (Qt::mightBeRichText(html()))
		doc.setHtml(html());
	else
		doc.setPlainText(html());

	QTextCursor cursor(&doc);
	cursor.select(QTextCursor::Document);

	QTextCharFormat format;
	//format.setFontPointSize(size);
	format.setForeground(color);
	cursor.mergeCharFormat(format);
	cursor.mergeBlockCharFormat(format);

	setHtml(doc.toHtml());
}


double RichTextRenderer::findFontSize()
{
	QTextDocument doc;
	if (Qt::mightBeRichText(html()))
		doc.setHtml(html());
	else
		doc.setPlainText(html());

	QTextCursor cursor(&doc);
	cursor.select(QTextCursor::Document);
	QTextCharFormat format = cursor.charFormat();
	return format.fontPointSize();
}

int RichTextRenderer::fitToSize(const QSize& size, int minimumFontSize, int maximumFontSize)
{
	int width = size.width();
	int height = size.height();
	
	const QString sizeKey = QString("%1:%2:%3:%4").arg(html()).arg(width).arg(height).arg(minimumFontSize);
	
	// for centering
	qreal boxHeight = -1;
		
	double ptSize = -1;
	if(static_autoTextSizeCache.contains(sizeKey))
	{
		ptSize = *(static_autoTextSizeCache[sizeKey]);
		
		//qDebug()<<"RichTextRenderer::fitToSize(): size search: CACHE HIT: loaded size:"<<ptSize;
		
		// We go thru the much-more-verbose method of creating
		// the document and setting the html, width, merge cursor,
		// etc, just so we can get the document height after
		// setting the font size inorder to use it to center the textbox.
		// If we didnt nead the height, we could just use autoText->setFontSize()
		
		QTextDocument doc;
		doc.setTextWidth(width);
		if (Qt::mightBeRichText(html()))
			doc.setHtml(html());
		else
			doc.setPlainText(html());

			
		QTextCursor cursor(&doc);
		cursor.select(QTextCursor::Document);
		
		QTextCharFormat format;
		format.setFontPointSize(ptSize);
		cursor.mergeCharFormat(format);
		
		boxHeight = doc.documentLayout()->documentSize().height();
		
		setHtml(doc.toHtml());
	}
	else
	{
		double ptSize = minimumFontSize > 0 ? minimumFontSize : findFontSize();
		double sizeInc = 1;	// how big of a jump to add to the ptSize each iteration
		int count = 0;		// current loop iteration
		int maxCount = 100; 	// max iterations of the search loop
		bool done = false;
		
		double lastGoodSize = ptSize;
		QString lastGoodHtml = html();
		
		QTextDocument doc;
		
		qreal heightTmp;
		
		doc.setTextWidth(width);
		if (Qt::mightBeRichText(html()))
			doc.setHtml(html());
		else
			doc.setPlainText(html());

			
		QTextCursor cursor(&doc);
		cursor.select(QTextCursor::Document);
		
		QTextCharFormat format;
			
		while(!done && count++ < maxCount)
		{
			format.setFontPointSize(ptSize);
			cursor.mergeCharFormat(format);
			
			heightTmp = doc.documentLayout()->documentSize().height();
			
			if(heightTmp < height &&
			      ptSize < maximumFontSize)
			{
				lastGoodSize = ptSize;
				//lastGoodHtml = html();
				boxHeight = heightTmp;

				sizeInc *= 1.1;
// 				qDebug()<<"size search: "<<ptSize<<"pt was good, trying higher, inc:"<<sizeInc<<"pt";
				ptSize += sizeInc;

			}
			else
			{
// 				qDebug()<<"fitToSize: size search: last good ptsize:"<<lastGoodSize<<", stopping search";
				done = true;
			}
		}
		
		if(boxHeight < 0 && minimumFontSize <= 0) // didnt find a size
		{
			ptSize = 100;
			
			count = 0;
			done = false;
			sizeInc = 1;
			
			//qDebug()<<"RichTextRenderer::fitToSize(): size search: going UP failed, now I'll try to go DOWN";
			
			while(!done && count++ < maxCount)
			{
				
				format.setFontPointSize(ptSize);
				cursor.mergeCharFormat(format);
				
				heightTmp = doc.documentLayout()->documentSize().height();
				
				if(heightTmp < height)
				{
					lastGoodSize = ptSize;
					//lastGoodHtml = html();
					boxHeight = heightTmp;
	
					sizeInc *= 1.1;
					//qDebug()<<"size search: "<<ptSize<<"pt was good, trying higher, inc:"<<sizeInc<<"pt";
					ptSize -= sizeInc;
	
				}
				else
				{
					//qDebug()<<"SongSlideGroup::textToSlides(): size search: last good ptsize:"<<lastGoodSize<<", stopping search";
					done = true;
				}
			}
		}

		format.setFontPointSize(lastGoodSize);
		cursor.mergeCharFormat(format);
		
		setHtml(doc.toHtml());
		
		//qDebug()<<"RichTextRenderer::fitToSize(): size search: caching ptsize:"<<lastGoodSize<<", count: "<<count<<"( minimum size was:"<<minimumFontSize<<")";
		boxHeight = heightTmp;
		//static_autoTextSizeCache[sizeKey] = lastGoodSize;
		
		// We are using a QCache instead of a plain QMap, so that requires a pointer value 
		// Using QCache because the key for the cache could potentially become quite large if there are large amounts of HTML
		// and I dont want to just keep accumlating html in the cache infinitely
		static_autoTextSizeCache.insert(sizeKey, new double(lastGoodSize),1);
	}
	
	return (int)boxHeight;
	
}

QSize RichTextRenderer::findNaturalSize(int atWidth)
{
	QTextDocument doc;
	if(atWidth > 0)
		doc.setTextWidth(atWidth);
	if (Qt::mightBeRichText(html()))
		doc.setHtml(html());
	else
		doc.setPlainText(html());
	
	QSize firstSize = doc.documentLayout()->documentSize().toSize();
	QSize checkSize = firstSize;
	
// 	qDebug() << "RichTextRenderer::findNaturalSize: atWidth:"<<atWidth<<", firstSize:"<<firstSize;
	
	#define RUNAWAY_LIMIT 500
	
	int counter = 0;
	int deInc = 10;
	while(checkSize.height() == firstSize.height() &&
	      checkSize.height() > 0 &&
	      counter < RUNAWAY_LIMIT)
	{
		int w = checkSize.width() - deInc;
		doc.setTextWidth(w);
		checkSize = doc.documentLayout()->documentSize().toSize();
		
// 		qDebug() << "RichTextRenderer::findNaturalSize: w:"<<w<<", checkSize:"<<checkSize<<", counter:"<<counter;
		counter ++;
	}
	
	if(checkSize.width() != firstSize.width())
	{
		int w = checkSize.width() + deInc;
		doc.setTextWidth(w);
		checkSize = doc.documentLayout()->documentSize().toSize();
// 		qDebug() << "RichTextRenderer::findNaturalSize: Final Size: w:"<<w<<", checkSize:"<<checkSize;
		return checkSize;
	}
	else
	{
// 		qDebug() << "RichTextRenderer::findNaturalSize: No Change, firstSize:"<<checkSize;
		return firstSize;
	}
}

void RichTextRenderer::update()
{
 	if(m_updateTimer.isActive())
 		m_updateTimer.stop();
 	//qDebug() << "RichTextRenderer::update(): \t in thread:"<<QThread::currentThreadId();
	m_updateTimer.start();
}

QImage RichTextRenderer::renderText()
{
// 	qDebug()<<itemName()<<"TextBoxWarmingThread::run(): htmlCode:"<<htmlCode;
	//qDebug() << "RichTextRenderer::renderText(): HTML:"<<html();
	//qDebug() << "RichTextRenderer::update(): Update Start...";
 	//qDebug() << "RichTextRenderer::renderText(): \t in thread:"<<QThread::currentThreadId();
	if(m_updateTimer.isActive())
		m_updateTimer.stop();
		
	QTime renderTime;
	renderTime.start();
	
	QTextDocument doc;
	QTextDocument shadowDoc;
	
	if (Qt::mightBeRichText(html()))
	{
		doc.setHtml(html());
		shadowDoc.setHtml(html());
	}
	else
	{
		doc.setPlainText(html());
		shadowDoc.setPlainText(html());
	}
	
	int textWidth = m_textWidth;

	doc.setTextWidth(textWidth);
	shadowDoc.setTextWidth(textWidth);
	
	// Apply outline pen to the html
	QTextCursor cursor(&doc);
	cursor.select(QTextCursor::Document);

	QTextCharFormat format;

	QPen p(Qt::NoPen);
	if(outlineEnabled())
	{
		p = outlinePen();
		p.setJoinStyle(Qt::MiterJoin);
	}

	format.setTextOutline(p);
	//format.setForeground(fillEnabled() ? fillBrush() : Qt::NoBrush); //Qt::white);

	cursor.mergeCharFormat(format);
	
	// Setup the shadow text formatting if enabled
	if(shadowEnabled())
	{
		if(shadowBlurRadius() <= 0.05)
		{
			QTextCursor cursor(&shadowDoc);
			cursor.select(QTextCursor::Document);
	
			QTextCharFormat format;
			format.setTextOutline(Qt::NoPen);
			format.setForeground(shadowBrush());
	
			cursor.mergeCharFormat(format);
		}
	}
	
			
	QSizeF shadowSize = shadowEnabled() ? QSizeF(shadowOffsetX(),shadowOffsetY()) : QSizeF(0,0);
	QSizeF docSize = doc.size();
	QSizeF padSize(12.,12.);
	QSizeF sumSize = (docSize + shadowSize + padSize);//.toSize();
	
	QSizeF scaledSize = QSizeF(sumSize.width() * m_scaling.x(), sumSize.height() * m_scaling.y());
	if(m_scaling.x() != 1. || m_scaling.y() != 1.)
	{
		//qDebug() << "RichTextRenderer::renderText(): Orig size:"<<sumSize<<", scaled size:"<<scaledSize<<", scaling:"<<m_scaling;
		m_rawSize = sumSize;
	}
	//qDebug() << "RichTextRenderer::update(): textWidth: "<<textWidth<<", shadowSize:"<<shadowSize<<", docSize:"<<docSize<<", sumSize:"<<sumSize;
	QImage cache(scaledSize.toSize(),QImage::Format_ARGB32); //_Premultiplied);
	memset(cache.scanLine(0),0,cache.byteCount());
	
	double padSizeHalfX = padSize.width() / 2;
	double padSizeHalfY = padSize.height() / 2;
			
	
	QPainter textPainter(&cache);
	textPainter.scale(m_scaling.x(), m_scaling.y());
	//textPainter.fillRect(cache.rect(),Qt::transparent);
	
	QAbstractTextDocumentLayout::PaintContext pCtx;

	//qDebug() << "RichTextRenderer::renderText(): shadowEnabled():"<<shadowEnabled()<<", shadowBlurRadius():"<<shadowBlurRadius(); 
	if(shadowEnabled())
	{
		if(shadowBlurRadius() <= 0.05)
		{
			// render a "cheap" version of the shadow using the shadow text document
			textPainter.save();

			textPainter.translate(shadowOffsetX(),shadowOffsetY());
			shadowDoc.documentLayout()->draw(&textPainter, pCtx);

			textPainter.restore();
		}
		else
		{
			double radius = shadowBlurRadius();
			
			// create temporary pixmap to hold a copy of the text
			QSizeF blurSize = ImageFilters::blurredSizeFor(doc.size(), (int)radius);
			
			QSizeF scaledBlurSize = QSize(blurSize.width() * m_scaling.x(), blurSize.height() * m_scaling.y());
			//QSize docSize = doc.size();
			//qDebug() << "RichTextRenderer::renderText(): [shadow] radius:"<<radius<<" blurSize:"<<blurSize<<", scaling:"<<m_scaling<<", scaledBlurSize:"<<scaledBlurSize;
			
			
			//qDebug() << "Blur size:"<<blurSize<<", doc:"<<doc.size()<<", radius:"<<radius;
			QImage tmpImage(scaledBlurSize.toSize(),QImage::Format_ARGB32_Premultiplied);
			memset(tmpImage.scanLine(0),0,tmpImage.byteCount());
			
			// render the text
			QPainter tmpPainter(&tmpImage);
			tmpPainter.scale(m_scaling.x(), m_scaling.y());
			
			tmpPainter.save();
			tmpPainter.translate(radius + padSizeHalfX, radius + padSizeHalfY);
			doc.documentLayout()->draw(&tmpPainter, pCtx);
			tmpPainter.restore();
			
			// blacken the text by applying a color to the copy using a QPainter::CompositionMode_DestinationIn operation. 
			// This produces a homogeneously-colored pixmap.
			QRect rect = tmpImage.rect();
			tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
			tmpPainter.fillRect(rect, shadowBrush().color());
			tmpPainter.end();

			// blur the colored text
			ImageFilters::blurImage(tmpImage, (int)radius);
			
			// render the blurred text at an offset into the cache
			textPainter.save();
			textPainter.translate(shadowOffsetX() - radius,
					      shadowOffsetY() - radius);
			textPainter.drawImage(0, 0, tmpImage);
			textPainter.restore();
		}
	}
	
	textPainter.translate(padSizeHalfX, padSizeHalfY);
	doc.documentLayout()->draw(&textPainter, pCtx);
	
	textPainter.end();
	
	m_image = cache.convertToFormat(QImage::Format_ARGB32);
	emit textRendered(m_image);
	
	//qDebug() << "RichTextRenderer::renderText(): Render finished, elapsed:"<<renderTime.elapsed()<<"ms";
	//m_image.save("debug-text.png");
	return m_image;
	
}

ITEM_PROPSET(RichTextRenderer, TextWidth,	int,	textWidth);

ITEM_PROPSET(RichTextRenderer, OutlineEnabled,	bool,	outlineEnabled);
ITEM_PROPSET(RichTextRenderer, OutlinePen,	QPen,	outlinePen);

ITEM_PROPSET(RichTextRenderer, FillEnabled,	bool,	fillEnabled);
ITEM_PROPSET(RichTextRenderer, FillBrush,	QBrush,	fillBrush);

ITEM_PROPSET(RichTextRenderer, ShadowEnabled,	bool,	shadowEnabled);
ITEM_PROPSET(RichTextRenderer, ShadowBlurRadius,	double,	shadowBlurRadius);
ITEM_PROPSET(RichTextRenderer, ShadowOffsetX,	double,	shadowOffsetX);
ITEM_PROPSET(RichTextRenderer, ShadowOffsetY,	double,	shadowOffsetY);
ITEM_PROPSET(RichTextRenderer, ShadowBrush,	QBrush,	shadowBrush);
