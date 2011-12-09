#ifndef TextVideoSource_H
#define TextVideoSource_H

#include "../livemix/VideoFrame.h"
#include "../livemix/VideoSource.h"
#include "RichTextRenderer.h"

#include <QtGui>

class RichTextRenderer;

class TextVideoSource : public VideoSource
{
	Q_OBJECT
	
	Q_PROPERTY(QString html READ html WRITE setHtml)
	
	Q_PROPERTY(int textWidth READ textWidth	 WRITE setTextWidth);
	
public:
	TextVideoSource(QObject *parent=0);
	virtual ~TextVideoSource() {}

	VideoFormat videoFormat() { return VideoFormat(VideoFrame::BUFFER_IMAGE,QVideoFrame::Format_ARGB32); }
	
	const QString & html() { return m_renderer->html(); }
	
	RichTextRenderer * renderer() { return m_renderer; }
		
	// not valid until setHtml called
	QSize size() { return m_image.size(); }

	int textWidth() { return m_textWidth; }
	
	bool updatesLocked() { return m_renderer->updatesLocked(); }
	bool lockUpdates(bool); // returns old status
	
public slots:
	void setHtml(const QString&);
	void update();
	void setTextWidth(int);
	
signals:
	void frameReady();

protected:
	void run();
	
private slots:
	void setImage(const QImage& img);

private:
	
	VideoFrame m_frame;
	QImage m_image;
	
	int m_textWidth;
	bool m_frameChanged;
	
	RichTextRenderer *m_renderer;
};

#endif
