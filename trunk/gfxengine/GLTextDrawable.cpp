#include "GLTextDrawable.h"
#include "RichTextRenderer.h"
#include "EntityList.h"
#include "GLWidget.h"

#include "GLRectDrawable.h"

#define DEFAULT_CLOCK_FORMAT "yyyy-MM-dd h:mm:ss ap"

GLTextDrawable::GLTextDrawable(QString text, QObject *parent)
	: GLImageDrawable("",parent)
	, m_isClock(false)
	, m_clockFormat(DEFAULT_CLOCK_FORMAT)
	, m_isScroller(false)
	, m_scrollerSpeed(15.)
	, m_iconFile("../data/stock-media-rec.png")
	, m_isRssReader(false)
	, m_rssRefreshTime(1000 * 60 * 30) // 30 min
	, m_dataReceived(false)
	, m_lockScrollerRender(false)
	, m_lockSetPlainText(false)
	, m_lockSetText(false)
{
	m_borderWidth = 2.0;
	
	QDateTime now = QDateTime::currentDateTime();
	m_targetTime = QDateTime(QDate(now.date().year()+1, 12, 25), QTime(0, 0));

	m_isCountdown = false;


	m_renderer = new RichTextRenderer();
	m_renderer->setShadowEnabled(false);
	connect(m_renderer, SIGNAL(textRendered(QImage)), this, SLOT(setImage(const QImage&)));

	m_renderer->setTextWidth(1000); // just a guess
	setImage(QImage("dot.gif"));

	if(!text.isEmpty())
		setText(text);

	//QTimer::singleShot(1500, this, SLOT(testXfade()));

	// dont restrict to aspect ratio
	setFreeScaling(true);
	
	//setAspectRatioMode(Qt::KeepAspectRatioByExpanding);

	// Countdown timer
	connect(&m_countdownTimer, SIGNAL(timeout()), this, SLOT(countdownTick()));
	m_countdownTimer.setInterval(250);

	// Clock timer
	connect(&m_clockTimer, SIGNAL(timeout()), this, SLOT(clockTick()));
	m_clockTimer.setInterval(250);
	
	// Scroller timer
	m_idealScrollFrameLength = 1000 / 30;
	connect(&m_scrollerTimer, SIGNAL(timeout()), this, SLOT(scrollerTick()));
	m_scrollerTimer.setInterval((int)m_idealScrollFrameLength);
	
	connect(&m_rssHttp, SIGNAL(readyRead(const QHttpResponseHeader &)), this, SLOT(rssReadData(const QHttpResponseHeader &)));
	connect(&m_rssRefreshTimer, SIGNAL(timeout()), this, SLOT(reloadRss()));
	
	//setIsScroller(true);
	//setRssUrl(QUrl()); //"http://www.mypleasanthillchurch.org/phc/boards/rss"));
	QTimer::singleShot(100,this,SLOT(testXfade()));
	
// 	GLRectDrawable *child = new GLRectDrawable();
// 	child->setBorderColor(Qt::white);
// 	child->setFillColor(Qt::red);
// 	child->setRect(QRectF(rect().topLeft() - QPointF(-50, 50),QSizeF(400,200)));
// 	child->setZIndex(-1);
// 	addChild(child);

}
GLTextDrawable::~GLTextDrawable()
{
	if(m_renderer)
	{
		delete m_renderer;
		m_renderer = 0;
	}
}

void GLTextDrawable::testXfade()
{
	//qDebug() << "GLTextDrawable::testXfade(): loading text #2";
	//setText("Friday 2010-11-26");
// 	setIconFile("/home/josiah/Downloads/phc-logo-transparent.png");
 	//setIsScroller(false);
// 	setRssUrl(QUrl("http://www.mypleasanthillchurch.org/phc/boards/rss"));
	
	//setRssUrl(QUrl());
}

void GLTextDrawable::setIsCountdown(bool flag)
{
	m_isCountdown = flag;
	if(m_countdownTimer.isActive() && !flag)
		m_countdownTimer.stop();

	if(!m_countdownTimer.isActive() && flag)
		m_countdownTimer.start();

	if(flag)
	{
		setXFadeEnabled(false);
		countdownTick();
	}
	else
		setXFadeEnabled(true);
}

void GLTextDrawable::setTargetDateTime(const QDateTime& date)
{
	m_targetTime = date;
}


QString GLTextDrawable::formatTime(double time)
{
	bool isNeg = time < 0;
	if(isNeg)
		time *= -1;
	double h = time/60/60;
	int hour = (int)(h);
	int min  = (int)(h * 60) % 60;
	int sec  = (int)( time ) % 60;
	return  (isNeg ? "+" : "") +
	                           QString::number(hour) + ":" +
		(min<10? "0":"") + QString::number(min)  + ":" +
		(sec<10? "0":"") + QString::number(sec);// + "." +
		//(ms <10? "0":"") + QString::number((int)ms );
}

void GLTextDrawable::countdownTick()
{
	if(!liveStatus())
		return;
		
	QDateTime now = QDateTime::currentDateTime();

	int secsTo = now.secsTo(m_targetTime);

	QString newText = formatTime(secsTo);
	//qDebug() << "GLTextDrawable::countdownTick: secsTo:"<<secsTo<<", newText:"<<newText;
	setXFadeEnabled(false);
	setPlainText(newText);
}

void GLTextDrawable::setIsClock(bool flag)
{
	m_isClock = flag;
	if(m_clockTimer.isActive() && !flag)
		m_clockTimer.stop();

	if(!m_clockTimer.isActive() && flag)
		m_clockTimer.start();

	if(flag)
	{
		setXFadeEnabled(false);
		clockTick();
	}
	else
		setXFadeEnabled(true);
}

void GLTextDrawable::setClockFormat(const QString& format)
{
	m_clockFormat = format;
	if(m_clockFormat.isEmpty())
		m_clockFormat = DEFAULT_CLOCK_FORMAT;
}

void GLTextDrawable::clockTick()
{
	if(!liveStatus())
		return;
		
	QDateTime now = QDateTime::currentDateTime();
	QString newText = now.toString(clockFormat());
	//qDebug() << "GLTextDrawable::clockTick: now:"<<now<<", format:"<<clockFormat()<<", newText:"<<newText;

	setXFadeEnabled(false);
	setPlainText(newText);
}

void GLTextDrawable::setIsScroller(bool flag)
{
	bool wasScroller = m_isScroller;
	m_isScroller = flag;
	if(m_scrollerTimer.isActive() && !flag)
		m_scrollerTimer.stop();

	if(!m_scrollerTimer.isActive() && flag)
		m_scrollerTimer.start();

	m_scrollPos = (int)(-rect().width());
	//m_lastItem = 0;
	if(flag)
	{
		m_scrollItems.clear();
		m_scrollFrameTime.start();
		m_lastScrollPos = 0;
		scrollerTick();
		setXFadeEnabled(false);
		connect(playlist(), SIGNAL(itemAdded(GLPlaylistItem*)),   this, SLOT(playlistChanged()));
		connect(playlist(), SIGNAL(itemRemoved(GLPlaylistItem*)), this, SLOT(playlistChanged()));
	}
	else
	{
		// rerender text, only if wasScroller tho - this prevents rerendering text
		// received as a cached image via the network if not needed
		if(wasScroller)
			m_renderer->setHtml(m_text);
		setXFadeEnabled(true);
		disconnect(playlist(), SIGNAL(itemAdded(GLPlaylistItem*)),   this, SLOT(playlistChanged()));
		disconnect(playlist(), SIGNAL(itemRemoved(GLPlaylistItem*)), this, SLOT(playlistChanged()));
	}
}

void GLTextDrawable::setIconFile(const QString& file)
{
	m_iconFile = file;
	m_scrollItems.clear();
}

void GLTextDrawable::setIsRssReader(bool flag)
{
	m_isRssReader = flag;
}

void GLTextDrawable::setRssUrl(const QUrl& url)
{
	m_rssUrl = url;
	
	if(!m_rssUrl.isValid())
		return;
	
	m_dataReceived = false;
	m_lockScrollerRender = true;
	
	QList<GLPlaylistItem*> items = playlist()->items();
	if(!items.isEmpty())
		m_rssTextTemplate = items.first()->value().toString();
	else
		m_rssTextTemplate = text();
	
	foreach(GLPlaylistItem *item, items) 
		playlist()->removeItem(item);
		
	//qDebug() << "GLTextDrawable::setRssUrl: RSS URL is:"<<url;
	
	m_lockScrollerRender = false;
	if(isScroller())
	{
		m_scrollItems.clear();
		scrollerTick();
	}
	
	m_rssHttp.setHost(url.host());
        m_rssHttp.get(url.path());
}

void GLTextDrawable::setRssRefreshTime(int x)
{
	if(m_rssRefreshTimer.isActive())
		m_rssRefreshTimer.stop();
		
	m_rssRefreshTimer.setInterval(x);
	
	if(x > 0)
		m_rssRefreshTimer.start();
}

void GLTextDrawable::reloadRss()
{
	setRssUrl(m_rssUrl);
}

void GLTextDrawable::rssReadData(const QHttpResponseHeader &resp)
{
	if(!m_rssUrl.isValid())
		return;
		
	if (resp.statusCode() != 200)
		m_rssHttp.abort();
	else 
	{
		m_rssXml.addData(m_rssHttp.readAll());
		m_lockScrollerRender = true;
		
		parseRssXml();
		
		m_lockScrollerRender = false;
		if(isScroller())
		{
			m_scrollItems.clear();
			scrollerTick();
		}
		
		m_dataReceived = true;
	}

}

void GLTextDrawable::parseRssXml()
{
	QString currentTag;
	QString linkString;
	QString titleString;
	QString dateString;
	QString urlString;
	
	while (!m_rssXml.atEnd()) 
	{
		m_rssXml.readNext();
		if (m_rssXml.isStartElement()) 
		{
			if (m_rssXml.name() == "item")
			{
		
				if (titleString!="")
				{
// 					feed = new QTreeWidgetItem;
// 					feed->setText(0, titleString);
// 					feed->setText(2, linkString);
// 					ui->treeWidget->addTopLevelItem(feed);
				}
		
				linkString.clear();
				titleString.clear();
				dateString.clear();
				urlString.clear();
			}
		
			currentTag = m_rssXml.name().toString();
		} 
		else 
		if (m_rssXml.isEndElement()) 
		{
			if (m_rssXml.name() == "item") 
			{
				GLPlaylistItem *item = new GLPlaylistItem();
				item->setTitle(titleString);
				
				
				QTextDocument doc;
				QString origText = m_rssTextTemplate;
				if (Qt::mightBeRichText(origText))
					doc.setHtml(origText);
				else
					doc.setPlainText(origText);
				
				QTextCursor cursor(&doc);
				cursor.select(QTextCursor::Document);
			
				QTextCharFormat format = cursor.charFormat();
				if(!format.isValid())
					format = cursor.blockCharFormat();
				//qDebug() << "Format at cursor: "<<format.fontPointSize()<<", "<<format.font()<<", "<<format.fontItalic()<<", "<<format.font().rawName();
				QString newText = titleString;
				//if(replaceNewlineSlash)
				//	newText = newText.replace(QRegExp("\\s/\\s")," \n ");
			
				if(format.fontPointSize() > 0)
				{
					cursor.insertText(newText);
			
					// doesnt seem to be needed
					//cursor.mergeCharFormat(format);
			
					item->setValue(doc.toHtml());
				}
				else
				{
					item->setValue(newText);
				}
				
				
				
				if(!titleString.isEmpty())
				{
					//qDebug() << "GLTextDrawable::parseRssXml(): Added item:"<<titleString;//<<", value:"<<item->value().toString();
					
					//if(playlist()->items().size() < 10)
						playlist()->addItem(item);
					//qDebug() << "GLTextDrawable::parseRssXml(): After addition, playlist size:"<<playlist()->size();
				}
				else
				{
					delete item;
				}
				
		
				titleString.clear();
				linkString.clear();
				dateString.clear();
				urlString.clear();
			}
			else
			if (m_rssXml.name() == "image")
			{
				// TODO handle image downloading
			}
	
		} 
		else 
		if (m_rssXml.isCharacters() && !m_rssXml.isWhitespace()) 
		{
			if (currentTag == "title")
				titleString += m_rssXml.text().toString();
			else if (currentTag == "link")
				linkString += m_rssXml.text().toString();
			else if (currentTag == "pubDate")
				dateString += m_rssXml.text().toString();
			else if (currentTag == "url")
				urlString += m_rssXml.text().toString();
		}
	}
	if (m_rssXml.error() && m_rssXml.error() != QXmlStreamReader::PrematureEndOfDocumentError) 
	{
		qWarning() << "XML ERROR:" << m_rssXml.lineNumber() << ": " << m_rssXml.errorString();
		m_rssHttp.abort();
	}
	
	playlist()->play();
}

void GLTextDrawable::updateAnimations(bool insidePaint)
{
	if(m_isScroller)
		scrollerTick(insidePaint);
	GLVideoDrawable::updateAnimations(insidePaint);
}

void GLTextDrawable::scrollerTick(bool insidePaint)
{
	if(!liveStatus())
		return;
		
	// Smooth out the increment by which we scroll by adjusting the increment up or down based on how long 
	// the thread spent processing before calling scrollerTick() compared to the ideal length of time
	int elapsed = m_scrollFrameTime.elapsed();
	double fractionOfFrame = ((double)elapsed) / m_idealScrollFrameLength;
	double realInc = m_scrollerSpeed * fractionOfFrame;
	m_scrollFrameTime.start();
	
	// dont waste time on (relativly) small increments
	if(realInc < 1.0)
		return;
		
	//if(fractionOfFrame < 0.5)
	//	qDebug() <<  "GLTextDrawable::scrollerTick(): m_idealScrollFrameLength:"<<m_idealScrollFrameLength<<", m_scrollerSpeed:"<<m_scrollerSpeed<<", elapsed:"<<elapsed<<", fractionOfFrame:"<<fractionOfFrame<<", realInc:"<<realInc;
	
	m_scrollPos += realInc;
	
	// dont re-process the code below if no visible change (QImage::copy does not work on sub-pixel values)
	if(m_lastScrollPos == (int)m_scrollPos)
	{
		m_lastScrollPos = (int)m_scrollPos; 
		//qDebug() <<  "GLTextDrawable::scrollerTick(): Not enoughs scroll pos change, not re-rendering";
		return; 
	}
	
	m_lastScrollPos = (int)m_scrollPos;
	
	// Render the list of items to be scrolled
	if(m_scrollItems.isEmpty())
	{
		RichTextRenderer r;
		QList<GLPlaylistItem*> items = playlist()->items();
		m_scrollerMaxHeight = 0;
		int widthSum = 0;
		
		// Load the separator icon
		QImage iconImgTmp(m_iconFile);
		if(iconImgTmp.size().height() > 64)
			iconImgTmp = iconImgTmp.scaled(64,64,Qt::KeepAspectRatio);
			
		// Add a 50% horizontal padding around the separator icon
		QImage iconImg(iconImgTmp.width() * 2, iconImgTmp.height(), iconImgTmp.format());
		memset(iconImg.scanLine(0),0,iconImg.byteCount());
		QPainter iconPainter(&iconImg);
		iconPainter.drawImage(iconImgTmp.width()/2,0,iconImgTmp);
		iconPainter.end();
		
		m_scrollItems.clear();
		
		#define _ADD_IMAGE() \
			if(img.height() > m_scrollerMaxHeight) \
				m_scrollerMaxHeight = img.height(); \
			widthSum += img.width(); 
		
//		int minFont = 99999; 
// 		int count = 0;
		foreach(GLPlaylistItem *item, items)
		{
			QString text = item->value().toString();
			
			r.setHtml(text);
			if(!Qt::mightBeRichText(text))
			{
				r.changeFontSize(40);
				r.changeFontColor(Qt::white);
			}
				
			
			int fontSize = (int)r.findFontSize();
				
			if(fontSize == 9)
			{
				r.changeFontSize(40);
				r.changeFontColor(Qt::white);
				fontSize = 40;
			}
			
// 			if(fontSize < minFont)
// 				fontSize = minFont;
			
			r.setTextWidth(r.findNaturalSize().width());
			
			QImage img = r.renderText();
			
			img = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
			
			//qDebug() << "Debug: img.width:"<<img.width();
			if(img.isNull() || img.width()<0)
				continue;
				
			ScrollableItem s1(widthSum, img, item);
			m_scrollItems << s1;
			_ADD_IMAGE();
			qDebug() << "GLTextDrawable::scrollerTick():  x:"<<s1.x<<", x2:"<<s1.x2<<", w:"<<s1.w<<", text:"<<htmlToPlainText(r.html());
			
			if(item != items.last())
			{
				img = iconImg;
				ScrollableItem s2(widthSum, img);
				m_scrollItems << s2;
				_ADD_IMAGE();
				qDebug() << "GLTextDrawable::scrollerTick():  x:"<<s2.x<<", x2:"<<s2.x2<<", w:"<<s2.w<<", [ICON]";
			}
		}
		#undef _ADD_IMAGE
		
		m_scrollerTotalWidth = widthSum;
		
		m_scrollerFirstItem = 0;
		m_scrollerLastItem  = -1;
	}
	
	// Now we have a list of images and their horizontal (X) positions
	// Now we just need to do horizontal tile-based image scrolling
	
	// 1. Calculate which "tiles" the current window (m_scrollPos - m_scrollPos+rect().width) covers
	// 2. If tiles changed, render buffer, store starting window pos of buffer into m_scrollerWindowPos
	// 3. Copy rect starting at (m_scrollPos - m_scrollerWindowPos) to output image for rendering
	
	// Calculate window tile coverage
	int wx = (int)m_scrollPos;
	int wx2 = (int)m_scrollPos + (int)rect().width();
	
	int item1=-1,item2=-1;
	for(int i=m_scrollerFirstItem;i<m_scrollItems.size();i++)
	{
		const ScrollableItem& item = m_scrollItems.at(i);
		if(wx >= item.x && wx <= item.x2)
			item1 = i;
		if(wx2 >= item.x && wx2 <= item.x2)
		{
			item2 = i;
			break;
		}
		
		//qDebug() << "GLTextDrawable::scrollerTick(): find tiles: wx:"<<wx<<", wx2:"<<wx2<<", i:"<<i<<", item.x:"<<item.x<<", item.x2:"<<item.x2<<", item1:"<<item1<<", item2:"<<item2; 
	}
	
	if(item2 < 0)
		item2 = m_scrollItems.size() - 1;
	if(item1 < 0)
		item1 = 0;
	
	//qDebug() << "GLTextDrawable::scrollerTick(): find tiles: wx:"<<wx<<", wx2:"<<wx2<<", item1:"<<item1<<", item2:"<<item2;
	
	// Re-render the scrolling window only if the tile coverage changes
	if(item1 != m_scrollerFirstItem ||
	   item2 != m_scrollerLastItem)
	{
		if(item2 < item1)
			item2 = item1;
		
		//qDebug() << "GLTextDrawable::scrollerTick(): m_scrollPos:"<<m_scrollPos<<"/"<<m_scrollerTotalWidth<<", item1:"<<item1<<", item2:"<<item2<<", first:"<<m_scrollerFirstItem<<", last:"<<m_scrollerLastItem;
	
		int width = 0;
		for(int i=item1; i<=item2; i++)
			width += m_scrollItems[i].w;
		
		m_scrollerWindowPos = m_scrollItems[item1].x;
		m_scrollerImage = QImage(width, m_scrollerMaxHeight, QImage::Format_ARGB32_Premultiplied);
		
		// The memset is necessary for /some reason/ on linux...dunno why - otherwise, corrupted memory is visible
		memset(m_scrollerImage.scanLine(0),0,m_scrollerImage.byteCount());
	
		QPainter p(&m_scrollerImage);
		int pos = 0;
		for(int i=item1; i<=item2; i++)
		{
			QImage img = m_scrollItems[i].img;
			
			int y = (m_scrollerMaxHeight - img.height()) / 2;
			p.drawImage(pos,y,img);
			pos += img.width();
		}
		//qDebug() << "GLTextDrawable::scrollerTick(): Rendered "<<(item2-item1+1)<<" images to final size "<<m_scrollerImage.size()<<", bytes: "<<m_scrollerImage.byteCount() / 1024 / 1024<<" MB";
		
		m_scrollerFirstItem = item1;
		m_scrollerLastItem  = item2;
	}
	
	
	if(!m_scrollerImage.isNull())
	{
		QRect sub((int)m_scrollPos - m_scrollerWindowPos,0,(int)rect().width(),m_scrollerImage.height());
		qDebug() << "GLTextDrawable::scrollerTick(): m_scrollPos:"<<m_scrollPos<<"/"<<m_scrollerTotalWidth<<", sub rect:"<<sub<<", buffer:"<<m_scrollerImage.rect().size();
		QImage copy = m_scrollerImage.copy(sub);
		setImage(copy,insidePaint);
	}
	else
	{
		//qDebug() << "GLTextDrawable::scrollerTick(): m_scrollPos:"<<m_scrollPos<<"/"<<m_scrollerTotalWidth<<" - null image"; 
	}
	
	if(m_scrollPos > m_scrollerTotalWidth)
	{
		m_scrollPos = (int)(-rect().width());
		m_scrollerFirstItem = 0;
		m_scrollerLastItem  = -1;
		//qDebug() << "GLTextDrawable::scrollerTick(): Reset m_scrollPos to 0";
	}
}

void GLTextDrawable::playlistChanged()
{
	if(m_lockScrollerRender)
		return;
		
	m_scrollItems.clear();
	scrollerTick();
}

void GLTextDrawable::setScrollerSpeed(double x)
{
	m_scrollerSpeed = x;
}

void GLTextDrawable::transformChanged()
{
	if(m_renderer && m_glw)
	{
		QTransform tx = m_glw->transform();
		if(tx.m11() > 1.25 || tx.m22() > 1.25)
		{
			m_renderer->setScaling(tx.m11(), tx.m22());
			m_renderer->update();
		}
		else
		{
			//m_renderer->setScaling(1,1);
		}
		//qDebug() << "GLTextDrawable::transformChanged(): New scale:"<<tx.m11()<<"x"<<tx.m22();
		
	}
}

void GLTextDrawable::setText(const QString& text)
{
	
	if(!Qt::mightBeRichText(text))
	{
		if(m_lockSetText)
			return;
		m_lockSetText = true;
	
		setPlainText(text);
		
		m_lockSetText = false;
		return;
	}
	
	if(text == m_text)
		return;

	m_text = text;
	if(m_isScroller)
	{
		//qDebug() << "GLTextDrawable::setText(): Scrolling enabled, not setting text the normal method.";
		return;
	}
	
	//qDebug() << "GLTextDrawable::setText(): "<<(QObject*)this<<" text:"<<htmlToPlainText(text);
	bool lock = false;
	
	QPointF scale = m_glw ? QPointF(m_glw->transform().m11(), m_glw->transform().m22()) : QPointF(1,1);
	if(scale.x() > 1.25 || scale.y() > 1.25)
	{
		m_cachedImageText = "";
		m_cachedImage = QImage();
		
		m_renderer->setScaling(scale);
	}
	else
	{
		m_renderer->setScaling(QPointF(1,1));
	
		if(m_cachedImageText == text &&
		  !m_cachedImage.isNull())
		{
			//qDebug() << "GLTextDrawable::setText: Cached image matches text, not re-rendering.";
			lock = m_renderer->lockUpdates(true);
			setImage(m_cachedImage);
		}
	}

	m_renderer->setHtml(text);
	if(!Qt::mightBeRichText(text))
	{
		changeFontSize(40);
		changeFontColor(Qt::white);
	}

	m_renderer->lockUpdates(lock);

	emit textChanged(text);

	emit plainTextChanged(plainText());
}

QString GLTextDrawable::plainText()
{
	return htmlToPlainText(m_text);
}

/* static */
QString GLTextDrawable::htmlToPlainText(const QString& text)
{
	QString plain = text;
	plain = plain.replace( QRegExp("&amp;"), "&");
	//plain = EntityList::decodeEntities(plain);
	plain = plain.replace( QRegExp("<style[^>]*>.*</style>", Qt::CaseInsensitive), "" );
	plain = plain.replace( QRegExp("<[^>]*>"), "" );
	plain = plain.replace( QRegExp("(^\\s+)"), "" );
	return plain;
}

void GLTextDrawable::setPlainText(const QString& text, bool /*replaceNewlineSlash*/)
{
	//qDebug() << "GLTextDrawable::setPlainText(): text:"<<text;
	if(m_lockSetPlainText)
		return;
	m_lockSetPlainText = true;
	//qDebug() << "GLTextDrawable::setPlainText(): mark1";

	
	QTextDocument doc;
	QString origText = m_text;
	if (Qt::mightBeRichText(origText))
		doc.setHtml(origText);
	else
		doc.setPlainText(origText);

	QTextCursor cursor(&doc);
	cursor.select(QTextCursor::Document);

	QTextCharFormat format = cursor.charFormat();
	if(!format.isValid())
		format = cursor.blockCharFormat();
	//qDebug() << "Format at cursor: "<<format.fontPointSize()<<", "<<format.font()<<", "<<format.fontItalic()<<", "<<format.font().rawName();
	QString newText = text;
	//if(replaceNewlineSlash)
	//	newText = newText.replace(QRegExp("\\s/\\s")," \n ");

	if(format.fontPointSize() > 0)
	{
		cursor.insertText(newText);

		// doesnt seem to be needed
		//cursor.mergeCharFormat(format);

		setText(doc.toHtml());
	}
	else
	{
		setText(newText);
	}

	m_lockSetPlainText = false;
}

void GLTextDrawable::drawableResized(const QSizeF& newSize)
{
	if(m_renderer->textWidth() != (int)newSize.width())
		m_renderer->setTextWidth((int)newSize.width());
	GLVideoDrawable::drawableResized(newSize);
}

QSize GLTextDrawable::findNaturalSize(int atWidth)
{
	return m_renderer->findNaturalSize(atWidth);
}

double GLTextDrawable::findFontSize()
{
	return m_renderer->findFontSize();
}

void GLTextDrawable::changeFontSize(double size)
{
	m_renderer->changeFontSize(size);
	m_text = m_renderer->html();
	emit textChanged(m_text);
}

void GLTextDrawable::changeFontColor(QColor color)
{
	m_renderer->changeFontColor(color);
	m_text = m_renderer->html();
	emit textChanged(m_text);
}

void GLTextDrawable::borderSettingsChanged()
{
	m_renderer->setOutlineEnabled(borderWidth() > 0.001 ? true : false);
	m_renderer->setOutlinePen(QPen(borderColor(),borderWidth()));
	/// TODO does this break the loading of cached text renders over the network??
	m_renderer->setHtml(m_text);
}

void GLTextDrawable::updateRects(bool secondSource)
{
	QRectF sourceRect = m_frame->rect();

	updateTextureOffsets();

	// Honor croping since the rendering will honor croping - but not really needed for text.
	QRectF adjustedSource = sourceRect.adjusted(
		m_displayOpts.cropTopLeft.x(),
		m_displayOpts.cropTopLeft.y(),
		m_displayOpts.cropBottomRight.x(),
		m_displayOpts.cropBottomRight.y());
		
	if(m_renderer && m_glw)
	{
		QPointF scale = m_renderer->scaling();
		if(scale.x() > 1.25 || scale.y() > 1.25)
		{
			adjustedSource = QRectF(
				adjustedSource.topLeft(),
				//m_renderer->unscaledImageSize()
				QSize(
					adjustedSource.width() / scale.x(),
					adjustedSource.height() / scale.y()
				)
			);
		}
		
		//qDebug() << "GLTextDrawable::updateRects: adjustedSource:"<<adjustedSource;
	}

	QRectF targetRect = QRectF(rect().topLeft(),adjustedSource.size());
	
	bool centerVertical = text().indexOf("vertical-align:middle") > 0;
	
	if(centerVertical)
	{
		QRectF origTarget = targetRect;
		targetRect = QRectF(0, 0, adjustedSource.width(), sourceRect.height());
		targetRect.moveCenter(rect().center());
		//qDebug() << "GLTextDrawable::updateRects: "<<(QObject*)this<<" origTarget:"<<origTarget<<", newTarget:"<<targetRect;
	}

	if(!secondSource)
	{
		//qDebug() << "GLTextDrawable::updateRects: source:"<<sourceRect<<", target:"<<targetRect;
		m_sourceRect = sourceRect;
		m_targetRect = targetRect;
	}
	else
	{
		m_sourceRect2 = sourceRect;
		m_targetRect2 = targetRect;
	}
}

void GLTextDrawable::loadPropsFromMap(const QVariantMap& map, bool /*onlyApplyIfChanged*/)
{

	QByteArray bytes = map["text_image"].toByteArray();
	QImage image;
	image.loadFromData(bytes);
	m_cachedImage = image;
	
	if(!image.isNull())
		setImage(image);
	m_cachedImageText = map["text_image_alt"].toString();
	//qDebug() << "GLTextDrawable::loadPropsFromMap(): image size:"<<image.size()<<", isnull:"<<image.isNull()<<", m_cachedImageText:"<<m_cachedImageText;


	GLDrawable::loadPropsFromMap(map);
}

QVariantMap GLTextDrawable::propsToMap()
{
	QVariantMap map = GLDrawable::propsToMap();

	// Save the image to the map for sending a cached render over the network so the player
	// can cheat and use this image that was rendered by the director as opposed to re-rendering
	// the same text
	if(!m_isClock &&
	   !m_isScroller)
	{
		// Only send if NOT a clock/scroller - because clocks/scrollers will update the text image anyway 
		// so its a waste of time to transmit the rendered image
		
		QByteArray bytes;
		QBuffer buffer(&bytes);
		buffer.open(QIODevice::WriteOnly);
		image().save(&buffer, "PNG"); // writes pixmap into bytes in PNG format
		buffer.close();
		map["text_image"] = bytes;
		map["text_image_alt"] = m_text;
	}

	return map;
}

