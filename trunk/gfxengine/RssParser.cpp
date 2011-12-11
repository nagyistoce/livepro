#include "RssParser.h"

RssParser::RssParser(QString url, QObject *parent)
	: QObject(parent)
{
	if(!url.isEmpty())
		setUrl(url);

	connect(&m_reloadTimer, SIGNAL(timeout()), this, SLOT(reloadData()));
	setUpdateTime(60);
}

void RssParser::setUpdateTime(int min)
{
	m_reloadTimer.setInterval(min * 60 * 1000);
}

void RssParser::reloadData()
{
	loadUrl(m_url);
}

void RssParser::setUrl(const QString& url)
{
	m_url = url;
	reloadData();
}


void RssParser::loadUrl(const QString &location) 
{
	QUrl url(location);
// 	url.addEncodedQueryItem("hl", "en");
// 	url.addEncodedQueryItem("weather", QUrl::toPercentEncoding(location));
	
	qDebug() << "RssParser::loadUrl(): url:"<<url;

	QNetworkAccessManager *manager = new QNetworkAccessManager(this);
	connect(manager, SIGNAL(finished(QNetworkReply*)),
		this, SLOT(handleNetworkData(QNetworkReply*)));
	manager->get(QNetworkRequest(url));
}

void RssParser::handleNetworkData(QNetworkReply *networkReply) 
{
	QUrl url = networkReply->url();
	if (!networkReply->error())
		parseData(QString::fromUtf8(networkReply->readAll()));
	networkReply->deleteLater();
	networkReply->manager()->deleteLater();
}
/*
#define GET_DATA_ATTR xml.attributes().value("data").toString() \
	.replace("&amp;#39;","'") \
	.replace("&amp;quot;","\"") \
	.replace("&amp;amp;","&")
	*/

void RssParser::parseData(const QString &data) 
{
	qDebug() << "RssParser::parseData()";//: "<<data;
	m_items.clear();
	
	QXmlStreamReader xml(data);
	while (!xml.atEnd()) 
	{
		xml.readNext();
		if (xml.tokenType() == QXmlStreamReader::StartElement) 
		{
// 			qDebug() << "RssParser::parseData(): StartElement/xml.name():"<<xml.name();
			// Parse and collect the news items
			if (xml.name() == "item") 
			{
				QString tag;
				RssItem item;
				while (!xml.atEnd()) 
				{
					xml.readNext();
					if (xml.name() == "item") 
					{
						if (!item.title.isEmpty()) 
						{
							item.date = item.date.replace("T"," ");
							item.date = item.date.replace(QRegExp("[+-]\\d{2}:\\d{2}"),"");
						
							m_items << item;
							qDebug() << "RssParser::parseData(): Added item: title:"<<item.title
								<<", "<<item.url
								<<", "<<item.date
								<<", "<<item.source; 
							
							item = RssItem();
						} 
						break;
					}
					
					if (xml.tokenType() == QXmlStreamReader::StartElement) 
					{
						tag = xml.name().toString();
// 						qDebug() << "RssParser::parseData(): 	StartElement/xml.name():"<<xml.name();
					}
					else 
					if (xml.isCharacters() && !xml.isWhitespace()) 
					{
// 						if(tag != "description")
// 							qDebug() << "RssParser::parseData(): 		isCharacters/tag:"<<tag<<", text:"<<xml.text().toString();
						if (tag == "title")
							item.title 	+= xml.text().toString();
						if (tag == "link") 
							item.url 	+= xml.text().toString();
						if (tag == "creator") 
							item.source 	+= xml.text().toString();
						if (tag == "date")
							item.date 	+= xml.text().toString();
						if (tag == "description")
							item.text 	+= xml.text().toString();
					}
				}
			}
		}
	}
	
	emit itemsAvailable(m_items);
}
