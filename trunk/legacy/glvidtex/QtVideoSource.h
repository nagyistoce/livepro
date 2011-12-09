#ifndef QtVideoSource_H
#define QtVideoSource_H

#include <QtCore/QRect>
#include <QtGui/QImage>

#include <qabstractvideosurface.h>
#include <qvideoframe.h>
#include <qmediaplayer.h>
#include <qmediaplaylist.h>
#include <qvideowidget.h>
#include <qaudioendpointselector.h>

#include "../livemix/VideoFrame.h"
#include "../livemix/VideoSource.h"


class QtVideoSource;
class VideoSurfaceAdapter : public QAbstractVideoSurface
{
	Q_OBJECT
public:
	VideoSurfaceAdapter(QtVideoSource*, QObject *parent = 0);
	
	QList<QVideoFrame::PixelFormat> supportedPixelFormats(
		QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle) const;
	bool isFormatSupported(const QVideoSurfaceFormat &format, QVideoSurfaceFormat *similar) const;
	
	bool start(const QVideoSurfaceFormat &format);
	void stop();
	
	bool present(const QVideoFrame &frame);
	
	QRect videoRect() const { return targetRect; }
	void updateVideoRect();
	
	void paint(QPainter *painter);
	
	QVideoFrame::PixelFormat pixelFormat() { return m_pixelFormat; }

signals:
	void firstFrameReceived();

private:
	bool m_hasFirstFrame;
	QtVideoSource *emitter;
	QVideoFrame::PixelFormat m_pixelFormat;
	QImage::Format imageFormat;
	QRect targetRect;
	QSize imageSize;
	QRect sourceRect;
	QVideoFrame currentFrame;
};

class QtVideoSource : public VideoSource
{
	Q_OBJECT
	
	Q_PROPERTY(QString file READ file WRITE setFile);
	
public:
	QtVideoSource(QObject *parent=0);
	virtual ~QtVideoSource();

	VideoFormat videoFormat(); // { return VideoFormat(VideoFrame::BUFFER_BYTEARRAY,QVideoFrame::Format_ARGB32); }
	
	QString file() { return m_file; }
	
	QMediaPlaylist * playlist() { return m_playlist; }
	QMediaPlayer * player() { return m_player; }
	VideoSurfaceAdapter *surfaceAdapter() { return m_surfaceAdapter; }
	
public slots:	
	void start(QThread::Priority priority = QThread::InheritPriority);
	void setFile(const QString&);

signals:
	void frameReady();

protected:
	friend class VideoSurfaceAdapter;
	void run();
	void present(QImage);
	
private:
	VideoSurfaceAdapter *m_surfaceAdapter;
	QMediaPlayer *m_player;
	QMediaPlaylist *m_playlist;
	QString m_file;
};


#endif
