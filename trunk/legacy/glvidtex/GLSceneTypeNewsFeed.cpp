#include "GLSceneTypeNewsFeed.h"
#include "GLImageDrawable.h"
#include "GLTextDrawable.h"
#include "QRCodeQtUtil.h"

GLSceneTypeNewsFeed::GLSceneTypeNewsFeed(QObject *parent)
	: GLSceneType(parent)
	, m_currentIndex(0)
	, m_parser(0)
	, m_requestLocked(false)
{
	m_fieldInfoList 
		<< FieldInfo("title", 
			"News Item Title", 
			"Title of the current news item", 
			"Text", 
			true)
			
		<< FieldInfo("text", 
			"Text snippet of the current news item.", 
			"A short four-six line summary of the current news item.", 
			"Text", 
			true)
			
		<< FieldInfo("source", 
			"Name of the news source", 
			"A short string giving the source of the news item.", 
			"Text", 
			false)
			
		<< FieldInfo("qrcode", 
			"QR Code", 
			"An optional image QR Code with a link to the news item.", 
			"Image", 
			false)
		
		<< FieldInfo("date", 
			"Date", 
			"A string giving the date of the item, such as '13 minutes ago'", 
			"Text", 
			false)
		;
		
	m_paramInfoList
		<< ParameterInfo("newsUrl",
			"News Feed URL",
			"RSS URL to download or type 'google' to use Google News",
			QVariant::String,
			true,
			SLOT(setNewsUrl(QString)))
			
		<< ParameterInfo("updateTime",
			"Update Time",
			"Time in minutes to wait between updates",
			QVariant::Int,
			true,
			SLOT(setUpdateTime(int)));
			
	PropertyEditorFactory::PropertyEditorOptions opts;
	
	opts.reset();
	opts.min = 1;
	opts.max = 30;
	m_paramInfoList[1].hints = opts;

	m_parser = new RssParser("", this);
        connect(m_parser, SIGNAL(itemsAvailable(QList<RssParser::RssItem>)), this, SLOT(itemsAvailable(QList<RssParser::RssItem>)));

	connect(&m_reloadTimer, SIGNAL(timeout()), this, SLOT(reloadData()));
	setParam("updateTime", 15);
	
	setParam("newsUrl", "google");
	
	reloadData();
	
}

void GLSceneTypeNewsFeed::setLiveStatus(bool flag)
{
	GLSceneType::setLiveStatus(flag);
	
	if(flag)
	{
		m_reloadTimer.start();
		applyFieldData();
	}
	else
	{
		//m_reloadTimer.stop();
		QTimer::singleShot( 0, this, SLOT(showNextItem()) );
	}
}

void GLSceneTypeNewsFeed::showNextItem()
{
	if(m_currentIndex < 0 || m_currentIndex >= m_news.size())
		m_currentIndex = 0;
	
	if(m_news.isEmpty())
		return;
		
	RssParser::RssItem item = m_news[m_currentIndex];
	
	item.title = item.title.replace("\n","");

	setField("title", 	item.title);
	setField("text", 	item.text.left(1024)); // only use the first 1k of text
	setField("source",	item.source);
	setField("date",	item.date);

	// Override RSS feeds that include HTML in their output
	GLDrawable *gld = lookupField("text");
	if(GLTextDrawable *text = dynamic_cast<GLTextDrawable*>(gld))
	{
		text->changeFontSize(42);
		text->changeFontColor(Qt::white);
	}
	
	GLDrawable *qrdest = lookupField("qrcode");
	if(qrdest)
	{
		GLImageDrawable *dest = dynamic_cast<GLImageDrawable*>(qrdest);
		if(dest)
		{
			QImage image = QRCodeQtUtil::encode(item.url);
			
			dest->setImageFile("");
			dest->setImage(image);
		}
	}
	
	m_currentIndex++;
}

void GLSceneTypeNewsFeed::setParam(QString param, QVariant value)
{
	GLSceneType::setParam(param, value);
	
	if(param == "newsUrl")
	{
		requestData(value.toString());
	}
	else
	if(param == "updateTime")
	{
// 		if(m_parser)
// 			m_parser->setUpdateTime(value.toInt());
		
		m_reloadTimer.setInterval(value.toInt() * 60 * 1000);
	}
}

void GLSceneTypeNewsFeed::reloadData()
{
	requestData(newsUrl());
}

void GLSceneTypeNewsFeed::requestData(const QString &location) 
{
	if(location.isEmpty())
		return;
		
	if(location == m_lastLocation &&
	   m_requestLocked)
	   return;
	
	m_requestLocked = true;
	m_lastLocation = location;
		
	qDebug() << "GLSceneTypeNewsFeed::requestData("<<location<<")";
	if(location.toLower() == "google")
	{
		QUrl url("http://www.google.com/ig/api?news");
	// 	url.addEncodedQueryItem("hl", "en");
		
		//qDebug() << "GLSceneTypeNewsFeed::requestData(): url:"<<url;
	
		QNetworkAccessManager *manager = new QNetworkAccessManager(this);
		connect(manager, SIGNAL(finished(QNetworkReply*)),
			this, SLOT(handleNetworkData(QNetworkReply*)));
		manager->get(QNetworkRequest(url));
	}
	else
	{
		m_parser->setUrl(location);
	}

}

void GLSceneTypeNewsFeed::handleNetworkData(QNetworkReply *networkReply) 
{
	//qDebug() << "GLSceneTypeNewsFeed::handleNetworkData()";
	QUrl url = networkReply->url();
	if (!networkReply->error())
	{
		//qDebug() << "GLSceneTypeNewsFeed::handleNetworkData(): No error";
		parseData(QString::fromUtf8(networkReply->readAll()));
	}
	else
	{
		qDebug() << "GLSceneTypeNewsFeed::handleNetworkData(): ERROR: "<<networkReply->error()<<" - "<<networkReply->errorString();
		m_requestLocked = false;
	}
	networkReply->deleteLater();
	networkReply->manager()->deleteLater();
}

#define GET_DATA_ATTR xml.attributes().value("data").toString() \
	.replace("&amp;#39;","'") \
	.replace("&amp;quot;","\"") \
	.replace("&amp;amp;","&")

void GLSceneTypeNewsFeed::parseData(const QString &data) 
{
	//qDebug() << "GLSceneTypeNewsFeed::parseData()";
	m_news.clear();
	m_currentIndex = 0;

	QXmlStreamReader xml(data);
	while (!xml.atEnd()) 
	{
		xml.readNext();
		if (xml.tokenType() == QXmlStreamReader::StartElement) 
		{
			// Parse and collect the news items
			if (xml.name() == "news_entry") 
			{
				RssParser::RssItem item;
				while (!xml.atEnd()) 
				{
					xml.readNext();
					if (xml.name() == "news_entry") 
					{
						if (!item.title.isEmpty()) 
						{
							m_news << item;
							//qDebug() << "GLSceneTypeNewsFeed::parseData(): Added item: "<<item.title;
							item = RssParser::RssItem();
						} 
						break;
					}
					
					if (xml.tokenType() == QXmlStreamReader::StartElement) 
					{
						if (xml.name() == "title")
							item.title = GET_DATA_ATTR;
						if (xml.name() == "url") 
							item.url = GET_DATA_ATTR;
						if (xml.name() == "source") 
							item.source = GET_DATA_ATTR;
						if (xml.name() == "date") 
							item.date = GET_DATA_ATTR;
						if (xml.name() == "snippet")
							item.text = GET_DATA_ATTR;
					}
				}
			}
		}
	}
	
	m_requestLocked = false;
	
	if(scene())
		showNextItem();
}

void GLSceneTypeNewsFeed::itemsAvailable(QList<RssParser::RssItem> list)
{
	m_news = list;
	m_currentIndex = 0;
	m_requestLocked = false;
	
	if(scene())
		showNextItem();
}
