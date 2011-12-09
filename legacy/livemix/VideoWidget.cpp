#include <QtGui/QApplication>
#include <QtOpenGL/QGLWidget>
#include <QDebug>

#include "VideoWidget.h"
#include "CameraThread.h"
#include "CameraServer.h"
#include <QPainter>
#include <QApplication>
#include <QMessageBox>

VideoWidget::VideoWidget(QWidget *parent)
	: QGLWidget(parent)
	, m_thread(0)
	, m_frameCount(0)
	, m_opacity(1)
	, m_frame(0)
	, m_oldFrame(0)
	, m_aspectRatioMode(Qt::KeepAspectRatio)
	, m_adjustDx1(0)
	, m_adjustDy1(0)
	, m_adjustDx2(0)
	, m_adjustDy2(0)
	, m_showOverlayText(false)
	, m_overlayText("")
	, m_forceFps(-1)
	, m_renderFps(false)
	, m_fadeLength(0)
	, m_fadeToBlack(false)
	, m_latencyAccum(0)
	, m_queuedSource(0)
	, m_oldThread(0)
	, m_overlayFrame(0)
	, m_overlaySource(0)
{
	//setAttribute(Qt::WA_PaintOnScreen, true);
	//setAttribute(Qt::WA_OpaquePaintEvent, true);
	//setAttribute(Qt::WA_PaintOutsidePaintEvent);
	srand ( time(NULL) );
	connect(&m_paintTimer, SIGNAL(timeout()), this, SLOT(callUpdate()));
	m_paintTimer.setInterval(1000/30);
	
	connect(&m_fadeTimer, SIGNAL(timeout()), this, SLOT(fadeAdvance()));
	
	setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
}

QSize VideoWidget::sizeHint () const { return QSize(160,120); }

void VideoWidget::fadeToBlack(bool flag)
{
	m_fadeToBlack = flag;
	fadeStart(false); // dont move current thread to oldThread
}

void VideoWidget::setFadeLength(int ms)
{
	m_fadeLength = ms;
	
	qreal fps = m_forceFps > 0.0 ? m_forceFps : 30.0;
	qreal sec = (m_fadeLength > 0 ? m_fadeLength : 1000.0) / 1000.0;
	m_opacityInc = 1.0 / (sec * fps);
	
	//qDebug() << "VideoWidget::setFadeLength(): m_fadeLength:"<<m_fadeLength<<", m_opacityInc:"<<m_opacityInc<<", sec:"<<sec<<", fps:"<<fps;
	
	m_fadeTimer.setInterval(sec / fps * 1000.0);
}

void VideoWidget::mouseReleaseEvent(QMouseEvent*)
{
 	emit clicked();
}

void VideoWidget::closeEvent(QCloseEvent*)
{
 	disconnectVideoSource();
	m_paintTimer.stop();
}

void VideoWidget::disconnectVideoSource()
{
	m_paintTimer.stop();
	if(m_thread)
	{
		m_thread->release(this);
		disconnect(m_thread,0,this,0);
		emit sourceDiscarded(m_thread);
	
		m_thread = 0;
	}
}

void VideoWidget::sourceDestroyed()
{
	if(m_thread)
	{
		//qDebug() << "VideoWidget::sourceDestroyed() - destroyed source before disconnecting.";
		m_paintTimer.stop();
		m_thread = 0;
		update();
	}
}

void VideoWidget::showEvent(QShowEvent*)
{
	m_paintTimer.start();
	if(!m_paintTimer.isActive())
	{
// 		setVideoSource(m_thread);

// 		m_thread = CameraThread::threadForCamera(m_camera);
// 		connect(m_thread, SIGNAL(frameReady()), this, SLOT(frameReady()), Qt::QueuedConnection);
// 		m_thread->registerConsumer(this);
// 		m_paintTimer.start();
//
// 		m_elapsedTime.start();
	}
}

void VideoWidget::setVideoSource(VideoSource *source)
{
	if(source == m_thread)
		return;
		
	if(m_fadeToBlack)
	{
		disconnectVideoSource();
		connectVideoSource(source);
	}
	
	if(m_fadeTimer.isActive())
	{
		m_queuedSource = source;
		return;
	}
		
	if(m_fadeLength > 33)
		fadeStart();
		
	if(m_thread && m_fadeLength < 33)
		disconnectVideoSource();

	//qDebug() << "VideoWidget::setVideoSource: In Thread ID "<<QThread::currentThreadId();
	//qDebug() << "VideoWidget::setVideoSource: source: "<<source;
	
	connectVideoSource(source);
}

void VideoWidget::connectVideoSource(VideoSource *source)
{
	m_thread = source;
	if(!m_thread)
	{
		repaint();
		return;
	}

	connect(m_thread, SIGNAL(frameReady()), this, SLOT(frameReady()), Qt::QueuedConnection);
	connect(m_thread, SIGNAL(destroyed()), this, SLOT(sourceDestroyed()));
	m_thread->registerConsumer(this);

	m_elapsedTime.start();
	m_paintTimer.start();
	
	frameReady(); // prime the pump
}


void VideoWidget::fadeStart(bool switchThreads)
{
	// If we're fading in with no thread to start with,
	// then we can't very well use the m_oldThread ptr now can we? :-)
	if(m_thread && switchThreads)
	{
		disconnect(m_thread, 0, this, 0);
		
		m_oldThread = m_thread;
		m_oldSourceRect = m_sourceRect;
		m_oldTargetRect = m_targetRect;
		
		m_thread = 0;
		
		connect(m_oldThread, SIGNAL(frameReady()), this, SLOT(oldFrameReady()));
	}
	
	if(!m_fadeToBlack)
		m_opacity = 0.0;
	else
		m_opacity = 1.0;

		
	// draw the old thread in the current threads place - see drawing code
	
// 	qDebug() << "VideoWidget::fadeStart("<<switchThreads<<"): m_opacity:"<<m_opacity;
	m_fadeTimer.start();
	
	m_fadeElapsed.start();
	m_predictedFadeClock = 0;
	
	double fps = m_forceFps > 0.0 ? m_forceFps : 30.0;
	//double sec = (m_fadeLength > 0 ? m_fadeLength : 1000.0) / 1000.0;
	m_predictedClockInc = 1000.0 / fps;
	
	
	if(m_oldThread)
		oldFrameReady();
}

void VideoWidget::oldFrameReady()
{
	if(!m_oldThread)
		return;
		
	VideoFramePtr f = m_oldThread->frame();
	if(!f)
		return;
		
	if(f->isValid())
	{
// 		if(m_oldFrame && 
// 			m_oldFrame->release())
// 			delete m_oldFrame;

		m_oldFrame = f;
	}
// 	else
// 	{
// 		if(f->release())
// 			delete f;
// 	}

// 	if(m_frame->image().size() != m_origSourceRect.size())
// 		updateRects();
}

void VideoWidget::fadeAdvance()
{
	if(m_fadeToBlack)
	{
		m_opacity -= m_opacityInc;
		if(m_opacity <= 0.0)
			fadeStop();
	}
	else
	{
		m_opacity += m_opacityInc;
		if(m_opacity >= 1.0)
			fadeStop();
	}	
		
	m_predictedFadeClock += m_predictedClockInc;
	
	int fakeMs = (int)m_predictedFadeClock;
	int realMs = m_fadeElapsed.elapsed();
	int diff = realMs - fakeMs;
	bool behind = diff > 10;
	
	//qDebug() << "VideoWidget::fadeAdvanced(): m_opacity:"<<m_opacity<<", fakeMs: "<<fakeMs<<", realMs: "<<realMs<<", diff:"<<diff<<", behind?"<<behind;
	update();
}

void VideoWidget::fadeStop()
{
	//qDebug() << "VideoWidget::fadeStop(): m_oldThread:"<<m_oldThread;
	if(m_fadeToBlack)
		m_opacity = 0.0;
	else
		m_opacity = 1.0;
	
// 	qDebug() << "VideoWidget::fadeStop(): m_opacity:"<<m_opacity;
		
	m_fadeTimer.stop();
	
	discardOldThread();
	
	if(m_queuedSource)
	{
		// this will start a cross fade to this source now
		setVideoSource(m_queuedSource);
		m_queuedSource = 0;
	}
}

void VideoWidget::discardOldThread()
{	
	if(!m_oldThread)
		return;
	
	m_oldThread->release(this);
	disconnect(m_oldThread,0,this,0);
	m_oldThread = 0;
	
	emit sourceDiscarded(m_oldThread);
}


void VideoWidget::callUpdate()
{
	update();
	//repaint();
	//qDebug() << "VideoWidget::callUpdate()";
}


void VideoWidget::setOverlayText(const QString& text)
{
	m_overlayText = text;
	m_showOverlayText = !text.isEmpty();
	updateOverlay();
}

void VideoWidget::showOverlayText(bool flag)
{
	m_showOverlayText = flag;
	updateOverlay();
}

void VideoWidget::updateOverlay()
{
	if(m_sourceRect.isEmpty())
		return;

	if(m_overlay.size() != m_targetRect.size())
	    m_overlay = QPixmap(m_targetRect.width(), m_targetRect.height());

	m_overlay.fill(QColor(0,0,0,0));

	if(!m_showOverlayText)
		return;

	QPainter painter(&m_overlay);

	QRect window = QRect(0,0,m_sourceRect.size().width(),m_sourceRect.size().height());
	painter.setWindow(window);

	painter.setRenderHint(QPainter::Antialiasing, true);
// 	painter.setPen(QPen(Qt::black, 12, Qt::DashDotLine, Qt::RoundCap));
// 	painter.setBrush(QBrush(Qt::green, Qt::SolidPattern));
// 	painter.drawEllipse(80, 80, 400, 240);

	if(!m_overlayText.isEmpty() &&
	    m_showOverlayText)
	{
		int rectHeight = window.height() / 4;
		QRect target(0,rectHeight * 3, window.width(), rectHeight);
		int size = 12;
		QSize textSize(0,0);
		QFont font;
		while(  textSize.width()  < target.width() * .9 &&
			textSize.height() < target.height() * .9)
		{
			size += 4;
			font = QFont("Sans-Serif",size,QFont::Bold);
			textSize = QFontMetrics(font).size(Qt::TextSingleLine, m_overlayText);
		}

		QRect textRect(target.x() + 10, // + target.width() / 2  - textSize.width() / 2,
			       target.y() + target.height() / 2 - textSize.height () / 2 + size,
			       textSize.width(), textSize.height());

		QPainterPath path;
		path.addText(textRect.topLeft(),font,m_overlayText);

		painter.setPen(QPen(Qt::black,1.5));
		painter.setBrush(Qt::white);
		painter.drawPath(path);

		/*painter.setFont(font);

		painter.setPen(Qt::black);
		painter.drawText( target.x() + target.width() / 2  - textSize.width() / 2 - 2,
				  target.y() + target.height() / 2 - textSize.height () / 2 + size - 2,
				  m_overlayText);

		painter.setPen(Qt::black);
		painter.drawText( target.x() + target.width() / 2  - textSize.width() / 2 + 2,
				  target.y() + target.height() / 2 - textSize.height () / 2 + size + 2,
				  m_overlayText);

		painter.setPen(Qt::white);
		painter.drawText( target.x() + target.width() / 2  - textSize.width() / 2,
				  target.y() + target.height() / 2 - textSize.height () / 2 + size,
				  m_overlayText);*/

	}

}


VideoWidget::~VideoWidget()
{
	if(m_thread)
		m_thread->release(this);
}

void VideoWidget::setOpacity(qreal opac)
{
	m_opacity = opac;
	update();
}

void VideoWidget::setRenderFps(bool flag)
{
	m_renderFps = flag;
	update();
}

void VideoWidget::resizeEvent(QResizeEvent*)
{
	updateRects();
}

void VideoWidget::setSourceRectAdjust( int dx1, int dy1, int dx2, int dy2 )
{
	m_adjustDx1 = dx1;
	m_adjustDy1 = dy1;
	m_adjustDx2 = dx2;
	m_adjustDy2 = dy2;
}




void VideoWidget::updateRects()
{
	if(!m_frame)
		return;
		
	m_sourceRect = m_frame->rect(); //image.rect();
	m_origSourceRect = m_sourceRect;

	m_sourceRect.adjust(m_adjustDx1,m_adjustDy1,m_adjustDx2,m_adjustDy2);

	QSize nativeSize = m_frame->size(); //.image.size();

	if (nativeSize.isEmpty())
	{
		m_targetRect = QRect();
	}
	else
	if (m_aspectRatioMode == Qt::IgnoreAspectRatio)
	{
		m_targetRect = rect();
	}
	else
	if (m_aspectRatioMode == Qt::KeepAspectRatio)
	{
		QSize size = nativeSize;
		size.scale(rect().size(), Qt::KeepAspectRatio);

		m_targetRect = QRect(0, 0, size.width(), size.height());
		m_targetRect.moveCenter(rect().center());
	}
	else
	if (m_aspectRatioMode == Qt::KeepAspectRatioByExpanding)
	{
		m_targetRect = rect();

		QSize size = rect().size();
		size.scale(nativeSize, Qt::KeepAspectRatio);

		m_sourceRect = QRect(QPoint(0,0),size);
		m_sourceRect.moveCenter(QPoint(size.width() / 2, size.height() / 2));
	}

	//qDebug() << "updateRects(): source: "<<m_sourceRect<<", target:" <<m_targetRect;
	updateOverlay();


}


void VideoWidget::frameReady()
{
	if(!m_thread)
		return;

// 	if(frame.isEmpty())
// 		qDebug() << "VideoWidget::frameReady(): isEmpty: "<<frame.isEmpty();

	VideoFramePtr f = m_thread->frame();
	if(!f)
		return;
	#ifdef DEBUG_VIDEOFRAME_POINTERS
	qDebug() << "VideoWidget::frameReady(): Received frame ptr:"<<f;
	#endif
		
	if(f->isValid())
	{
// 		if(m_frame && 
// 		   m_frame->release())
// 		{
// 			#ifdef DEBUG_VIDEOFRAME_POINTERS
// 			qDebug() << "VideoWidget::frameReady(): Deleting old m_frame:"<<m_frame;
// 			#endif
// 			
// 			delete m_frame;
// 		}

		m_frame = f;
		
		#ifdef DEBUG_VIDEOFRAME_POINTERS
		qDebug() << "VideoWidget::frameReady(): Received new m_frame:"<<m_frame;
		#endif
	}
// 	else
// 	{
// 		if(f->release())
// 		{
// 			#ifdef DEBUG_VIDEOFRAME_POINTERS
// 			qDebug() << "VideoWidget::frameReady(): Deleting invalid f ptr:"<<f;
// 			#endif
// 			
// 			delete f;
// 		}
// 	}

	if(m_frame && (m_frame->size() != m_origSourceRect.size() || m_targetRect.isEmpty() || m_sourceRect.isEmpty()))
		updateRects();

	//qDebug() << "VideoWidget::frameReady: frame size:"<<m_frame->size()<<", orig source rect size:" <<m_origSourceRect.size(); 
	//QTimer::singleShot(0, this, SLOT(updateTimer()));
	
	repaint();
}

void VideoWidget::updateTimer()
{
	// user has forced a different fps than the video stream has requested
	// typically this is to increase performance of another video stream
	if(m_forceFps > 0 || !m_frame)
		return;

	int fps = qMax((!m_frame->holdTime() ? 33 : m_frame->holdTime()) * .75,5.0);

	if(m_paintTimer.interval() != fps)
	{
		qDebug() << "VideoWidget::updateTimer: new hold time: "<<fps;

		m_paintTimer.setInterval(fps);
	}
}

void VideoWidget::setFps(int fps)
{
	m_forceFps = fps;
	if(m_forceFps > 0)
	{
		qDebug() << "VideoWidget::setFps: New m_forceFps:"<<m_forceFps;
		m_paintTimer.setInterval(1000/m_forceFps);

	}

	// If we use a different fps, dont use buffered frames, just take the last frame as it comes
	if(m_thread)
		m_thread->setIsBuffered(m_forceFps < 0);
}

void VideoWidget::paintEvent(QPaintEvent*)
{
	QPainter p(this);
	//p.setRenderHint(QPainter::SmoothPixmapTransform);
	//p.setRenderHint(QPainter::Antialiasing);

	p.fillRect(rect(),Qt::black);

	if(!m_thread)
	{
		p.fillRect(rect(),Qt::black);
		p.setPen(Qt::white);
		p.drawText(5,15,QString("No Video Input"));
	}
	else
	{
		if(m_oldThread && m_oldFrame)
		{
			//p.setOpacity(1.0-m_opacity);
			p.setOpacity(1.0);
			p.drawImage(m_oldTargetRect,m_oldFrame->image(),m_oldSourceRect);
		}
		
		if(m_opacity > 0.0 && m_frame)
		{ 
			p.setOpacity(m_opacity);
				
			if(m_frame->isRaw())
			{
				if(m_frame->bufferType() == VideoFrame::BUFFER_POINTER) // assume from SimpleV4L2 in ARGB32 format
				{
					//QImage image((const uchar*)m_frame->byteArray().constData(),m_frame->size().width(),m_frame->size().height(),QImage::Format_RGB32);
					const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(m_frame->pixelFormat());
					if(imageFormat != QImage::Format_Invalid)
					{
						QImage image(m_frame->pointer(),
							m_frame->size().width(),
							m_frame->size().height(),
							m_frame->size().width() *
								(imageFormat == QImage::Format_RGB16  ||
								 imageFormat == QImage::Format_RGB555 ||
								 imageFormat == QImage::Format_RGB444 ||
								 imageFormat == QImage::Format_ARGB4444_Premultiplied ? 2 :
								 imageFormat == QImage::Format_RGB888 ||
								 imageFormat == QImage::Format_RGB666 ||
								 imageFormat == QImage::Format_ARGB6666_Premultiplied ? 3 :
								 4),
							imageFormat);
							
						//if(m_displayOpts.flipHorizontal || m_displayOpts.flipVertical)
						//	image = image.mirrored(m_displayOpts.flipHorizontal, m_displayOpts.flipVertical);
			
						//painter->drawImage(target,image,source);
						//updateRects();
						p.drawImage(m_targetRect,image,m_sourceRect);
						//qDebug() << "VideoWidget::paintEvent: Painted RAW frame, size:" << image.size()<<", source:"<<m_sourceRect<<", target:"<<m_targetRect<<", format:"<<image.format();
					}
					else
					{
						qDebug() << "VideoWidget::paintEvent: Invalid pixel format for byte array buffer";
					}
					
					
					
					//qDebug() << "Simple bytearray format: isNull: "<<image.isNull();
				}
				else
				{
					qDebug() << "VideoWidget::paintEvent: Unknown buffer type: "<<m_frame->bufferType();
				}
				// else cannot use frame
			}
			else	
			{
// 				if(!m_frame->image().isNull())
// 				{
// 					QImage img = m_frame->image().copy();
// 					img = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
					p.drawImage(m_targetRect,m_frame->image(),m_sourceRect);
// 				}
			}
	
			// If fading in from black (e.g. no old thread)
			// then fade in the overlay with the frame, otherwise during
			// crossfades, dont fade the overlay
			if(m_oldThread)
				p.setOpacity(1.0);
			
			if(m_overlayFrame && m_overlaySource && !m_overlayFrame->image().isNull())
				p.drawImage(m_overlayTargetRect,m_overlayFrame->image(),m_overlaySourceRect);
			
			if(m_showOverlayText)
				p.drawPixmap(m_targetRect.topLeft(),m_overlay);
		
			m_frameCount ++;
			if(m_renderFps)
			{
				QString framesPerSecond;
				framesPerSecond.setNum(m_frameCount /(m_elapsedTime.elapsed() / 1000.0), 'f', 2);
				
				
				QString latencyString;
				if(!m_frame->captureTime().isNull())
				{
					int msecLatency = m_frame->captureTime().msecsTo(QTime::currentTime());
					
					m_latencyAccum += msecLatency;
					
					QString latencyPerFrame;
					latencyPerFrame.setNum((((double)m_latencyAccum) / ((double)m_frameCount)), 'f', 3);
					
					latencyString = QString("| %1 ms latency").arg(latencyPerFrame);
				}
					
					
	
				p.setPen(Qt::black);
				p.drawText(m_targetRect.x() + 6,m_targetRect.y() + 16,QString("%1 fps %2").arg(framesPerSecond).arg(latencyString));
				p.setPen(Qt::white);
				p.drawText(m_targetRect.x() + 5,m_targetRect.y() + 15,QString("%1 fps %2").arg(framesPerSecond).arg(latencyString));
				
				if(!(m_frameCount % 30))
				{
					m_elapsedTime.start();
					m_frameCount = 0;
					m_latencyAccum = 0;
					qDebug() << "FPS: "<<QString("%1 fps %2").arg(framesPerSecond).arg(latencyString);
				}
			}
		}
	}

	updateTimer();
}

void VideoWidget::setOverlaySource(VideoSource *source)
{
	if(m_overlaySource)
		disconnectOverlaySource();
		
	m_overlaySource = source;
	
	if(source)
	{
		connect(m_overlaySource, SIGNAL(frameReady()), this, SLOT(overlayFrameReady()), Qt::QueuedConnection);
		connect(m_overlaySource, SIGNAL(destroyed()), this, SLOT(sourceDestroyed()));
		m_overlaySource->registerConsumer(this);
		
		overlayFrameReady();
	}
}

void VideoWidget::disconnectOverlaySource()
{
	if(!m_overlaySource)
		return;
	m_overlaySource->release(this);
	disconnect(m_overlaySource,0,this,0);
	emit sourceDiscarded(m_overlaySource);
	m_overlaySource = 0;
}

void VideoWidget::overlayFrameReady()
{
	if(!m_overlaySource)
		return;

	
// 	if(!frame.isEmpty())
// 		m_overlayFrame = frame;
	VideoFramePtr f = m_overlaySource->frame();
	if(!f)
		return;
		
	if(f->isValid())
	{
// 		if(m_overlayFrame && 
// 			m_overlayFrame->release())
// 			delete m_overlayFrame;

		m_overlayFrame = f;
	}
// 	else
// 	{
// 		if(f->release())
// 			delete f;
// 	}
		
	if(m_overlayFrame->image().size() != m_overlaySourceRect.size())
		updateOverlaySourceRects();
}

void VideoWidget::updateOverlaySourceRects()
{
	m_overlaySourceRect = m_overlayFrame->image().rect();
	
	QSize nativeSize = m_overlayFrame->image().size();

	if (nativeSize.isEmpty())
	{
		m_overlayTargetRect = QRect();
	}
	else
	if (m_aspectRatioMode == Qt::IgnoreAspectRatio)
	{
		m_overlayTargetRect = rect();
	}
	else
	if (m_aspectRatioMode == Qt::KeepAspectRatio)
	{
		QSize size = nativeSize;
		size.scale(rect().size(), Qt::KeepAspectRatio);

		m_overlayTargetRect = QRect(0, 0, size.width(), size.height());
		m_overlayTargetRect.moveCenter(rect().center());
	}
	else
	if (m_aspectRatioMode == Qt::KeepAspectRatioByExpanding)
	{
		m_overlayTargetRect = rect();

		QSize size = rect().size();
		size.scale(nativeSize, Qt::KeepAspectRatio);

		m_overlaySourceRect = QRect(QPoint(0,0),size);
		m_overlaySourceRect.moveCenter(QPoint(size.width() / 2, size.height() / 2));
	}

	//qDebug() << "updateRects(): source: "<<m_sourceRect<<", target:" <<m_targetRect;
	//updateOverlay();


}
