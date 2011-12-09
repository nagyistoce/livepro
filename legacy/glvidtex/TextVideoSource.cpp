#include "TextVideoSource.h"
#include "../ImageFilters.h"

TextVideoSource::TextVideoSource(QObject *parent)
	: VideoSource(parent)
	, m_textWidth(640)
	, m_frameChanged(false)
{
	setImage(QImage());
	setIsBuffered(false);
	m_renderer = new RichTextRenderer(this);
	connect(m_renderer, SIGNAL(textRendered(QImage)), this, SLOT(setImage(const QImage&)));
}

void TextVideoSource::setHtml(const QString& html)
{
	m_renderer->setHtml(html);
}

void TextVideoSource::setImage(const QImage& img)
{
	m_image = img.convertToFormat(QImage::Format_ARGB32);
	m_frame = VideoFrame(m_image,1000/30);
	//qDebug() << "TextVideoSource::setImage: new frame size: "<<m_image.size();
	m_frameChanged = true;
}

bool TextVideoSource::lockUpdates(bool flag)
{
	return m_renderer->lockUpdates(flag);
}

void TextVideoSource::run()
{
	while(!m_killed)
	{
//		qDebug() << "TextVideoSource::run()";
		if(!m_renderer->updatesLocked())
		{
			if(m_frameChanged)
			{
				m_frameChanged = false;
				enqueue(m_frame);
				emit frameReady();
			}
		}
// 		msleep(m_frame.holdTime);
		msleep(50);
	}
}

void TextVideoSource::update()
{
	m_renderer->update();
}

void TextVideoSource::setTextWidth(int w)
{
	m_textWidth = w;
	m_renderer->setTextWidth(w);
}
