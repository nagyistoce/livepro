#include "QtVideoSource.h"

#include <QDebug>
#include <QtGui>
#include <qabstractvideosurface.h>
#include <qvideosurfaceformat.h>
#include <qmediaservice.h>
#include <qmediaplaylist.h>
#include <qmediaservice.h>
#include <qvideorenderercontrol.h>

VideoFormat QtVideoSource::videoFormat()
{ 
	//qDebug() << "QtVideoSource::videoFormat()";
	
	return VideoFormat(VideoFrame::BUFFER_IMAGE,m_surfaceAdapter ? m_surfaceAdapter->pixelFormat() : QVideoFrame::Format_RGB32); 
}

QtVideoSource::QtVideoSource(QObject *parent)
	: VideoSource(parent)
{
	present(QImage("dot.gif"));
	
	m_player   = new QMediaPlayer(this);
	m_playlist = new QMediaPlaylist(this);
	m_player->setPlaylist(m_playlist);
	
	//m_playlist->setPlaybackMode(QMediaPlaylist::Loop);
	
	m_surfaceAdapter = new VideoSurfaceAdapter(this);
	
	QVideoRendererControl* rendererControl = m_player->service()->requestControl<QVideoRendererControl*>();
	if (rendererControl)
		rendererControl->setSurface(m_surfaceAdapter);
	else
		qDebug() << "QtVideoSource: Unable to get QVideoRenderControl for video integration. No video will be emitted from this video source.";
	

}

QtVideoSource::~QtVideoSource()
{
	m_player->stop();
	//m_player->deleteLater();
	delete m_player;
	m_player = 0;
	
	m_playlist->clear();
	//m_playlist->deleteLater();
	delete m_playlist;
	m_playlist = 0;
	
	m_surfaceAdapter->stop();
	//m_surfaceAdapter->deleteLater();
	delete m_surfaceAdapter;
	m_surfaceAdapter = 0;
}

void QtVideoSource::start(QThread::Priority /*priority*/)
{
	//qDebug() << "QtVideoSource::start: starting...";
	m_player->play();
}
	
void QtVideoSource::setFile(const QString& file)
{
	m_file = file;
	
	m_playlist->clear();

	QFileInfo fileInfo(file);
	if (fileInfo.exists()) 
	{
		QUrl url = QUrl::fromLocalFile(fileInfo.absoluteFilePath());
		if (fileInfo.suffix().toLower() == QLatin1String("m3u"))
			m_playlist->load(url);
		else
			m_playlist->addMedia(url);
			
		//qDebug() << "QtVideoSource::setFile: Added file:"<<url;
	} 
	else 
	{
		QUrl url(file);
		if (url.isValid()) 
		{
			m_playlist->addMedia(url);
			//qDebug() << "QtVideoSource::setFile: Added URL:"<<url;
		}
	}
	
}

void QtVideoSource::run()
{

}

void QtVideoSource::present(QImage image)
{
	//qDebug() << "QtVideoSource::present()";
	// TODO is there some way to get the FPS from the QMediaPlayer or friends?
	//qDebug()<< "QtVideoSource::present: Got image, size:"<<image.size();
	enqueue(new VideoFrame(image,1000/60));
	emit frameReady();
}

/// VideoSurfaceAdapter


VideoSurfaceAdapter::VideoSurfaceAdapter(QtVideoSource *e, QObject *parent)
	: QAbstractVideoSurface(parent)
	, m_hasFirstFrame(false)
	, emitter(e)
	, m_pixelFormat(QVideoFrame::Format_Invalid)
	, imageFormat(QImage::Format_Invalid)
{
	//qDebug() << "VideoSurfaceAdapter::ctor()";
	
}

QList<QVideoFrame::PixelFormat> VideoSurfaceAdapter::supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const
{
	if (handleType == QAbstractVideoBuffer::NoHandle) 
	{
// 		qDebug() << "VideoSurfaceAdapter::supportedPixelFormats() list";
	
		return QList<QVideoFrame::PixelFormat>()
			<< QVideoFrame::Format_RGB32
			<< QVideoFrame::Format_ARGB32
			<< QVideoFrame::Format_ARGB32_Premultiplied
			<< QVideoFrame::Format_RGB565
			<< QVideoFrame::Format_RGB555;
	} 
	else 
	{
// 		qDebug() << "VideoSurfaceAdapter::supportedPixelFormats() empty list";
	
		return QList<QVideoFrame::PixelFormat>();
	}
}

bool VideoSurfaceAdapter::isFormatSupported(
        const QVideoSurfaceFormat &format, QVideoSurfaceFormat *similar) const
{
	Q_UNUSED(similar);
	//qDebug() << "VideoSurfaceAdapter::isFormatSupported()";
	
	const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat());
	const QSize size = format.frameSize();
	
	qDebug()<< "VideoSurfaceAdapter::isFormatSupported: got imageFormat:"<<imageFormat<<" for pixelFormat:"<<format.pixelFormat();
	
	return imageFormat != QImage::Format_Invalid
		&& !size.isEmpty()
		&& format.handleType() == QAbstractVideoBuffer::NoHandle;
}

bool VideoSurfaceAdapter::start(const QVideoSurfaceFormat &format)
{
	const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat());
	const QSize size = format.frameSize();
	
	//qDebug() << "VideoSurfaceAdapter::start()";
	
	if (imageFormat != QImage::Format_Invalid && !size.isEmpty()) 
	{
		this->imageFormat = imageFormat;
		this->m_pixelFormat = format.pixelFormat();
		imageSize = size;
		sourceRect = format.viewport();
	
		QAbstractVideoSurface::start(format);
	
		//widget->updateGeometry();
		updateVideoRect();
		
		//qDebug()<< "VideoSurfaceAdapter::start: Started with imageFormat:"<<imageFormat<<", pixelFormat:"<<format.pixelFormat();
	
		return true;
	} 
	else 
	{
		qDebug()<< "VideoSurfaceAdapter::start: NOT starting, either format invalid or size empty.";
		return false;
	}
}

void VideoSurfaceAdapter::stop()
{
	//qDebug() << "VideoSurfaceAdapter::stop()";
	
	currentFrame = QVideoFrame();
	targetRect = QRect();
	
	QAbstractVideoSurface::stop();
	
	//widget->update();
	emitter->present(QImage());
}

bool VideoSurfaceAdapter::present(const QVideoFrame &frame)
{
	//qDebug() << "VideoSurfaceAdapter::present()";
	
	if (surfaceFormat().pixelFormat() != frame.pixelFormat()
	 || surfaceFormat().frameSize() != frame.size()) 
	{
		QVideoSurfaceFormat format(frame.size(),frame.pixelFormat());
		start(format);
		
		//setError(IncorrectFormatError);
		//stop();
		qDebug()<< "VideoSurfaceAdapter::present: Format/size mismatch..."; //, stopping.";
		qDebug()<< "	surfaceFormat().pixelFormat():"<<surfaceFormat().pixelFormat()<<",	frame.pixelFormat():"<<frame.pixelFormat();
	 	qDebug()<< "	surfaceFormat().frameSize()  :"<<surfaceFormat().frameSize()<<",	frame.size()       :"<<frame.size();
	
		return true; //false;
	} 
	else 
	{
		currentFrame = frame;
	
		//widget->repaint(targetRect);
		if (currentFrame.map(QAbstractVideoBuffer::ReadOnly)) 
		{
			if(!m_hasFirstFrame)
			{
				m_hasFirstFrame = true;
				emit firstFrameReceived();
			}
			

// 			const QTransform oldTransform = painter->transform();
// 		
// 			if (surfaceFormat().scanLineDirection() == QVideoSurfaceFormat::BottomToTop) {
// 			painter->scale(1, -1);
// 			painter->translate(0, -widget->height());
// 			}
			
			QImage image(
				currentFrame.bits(),
				currentFrame.width(),
				currentFrame.height(),
				currentFrame.bytesPerLine(),
				imageFormat);
				
 			//qDebug()<< "VideoSurfaceAdapter::present: Presenting image with "<<image.byteCount()<<" bytes";
				
			emitter->present(image); //.copy());
		
			//qDebug() << "Painting image size: "<<image.size();
		
// 			painter->drawImage(targetRect, image, sourceRect);
// 		
// 			painter->setTransform(oldTransform);
		
			currentFrame.unmap();
		}
	
		return true;
	}
}

void VideoSurfaceAdapter::updateVideoRect()
{
	//qDebug() << "VideoSurfaceAdapter::updateVideoRect()";
	
	QSize size = surfaceFormat().sizeHint();
// 	size.scale(widget->size().boundedTo(size), Qt::KeepAspectRatio);
	
	targetRect = QRect(QPoint(0, 0), size);
// 	targetRect.moveCenter(widget->rect().center());
}

// void VideoSurfaceAdapter::paint(QPainter *painter)
// {
// 	if (currentFrame.map(QAbstractVideoBuffer::ReadOnly)) 
// 	{
// 		const QTransform oldTransform = painter->transform();
// 	
// 		if (surfaceFormat().scanLineDirection() == QVideoSurfaceFormat::BottomToTop) {
// 		painter->scale(1, -1);
// 		painter->translate(0, -widget->height());
// 		}
// 		
// 		QImage image(
// 			currentFrame.bits(),
// 			currentFrame.width(),
// 			currentFrame.height(),
// 			currentFrame.bytesPerLine(),
// 			imageFormat);
// 	
// 		//qDebug() << "Painting image size: "<<image.size();
// 	
// 		painter->drawImage(targetRect, image, sourceRect);
// 	
// 		painter->setTransform(oldTransform);
// 	
// 		currentFrame.unmap();
// 	}
// }


