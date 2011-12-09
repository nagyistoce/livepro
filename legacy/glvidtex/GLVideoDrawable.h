#ifndef GLVIDEODRAWABLE_H
#define GLVIDEODRAWABLE_H

#include <QtGui>

#include <QVideoFrame>
#include <QVideoSurfaceFormat>
#include <QGLShaderProgram>
#include "../livemix/VideoFrame.h"
#include "../livemix/VideoSource.h"

#include "GLDrawable.h"

class VideoDisplayOptions
{
public:
	VideoDisplayOptions(
		bool flipX=false, 
		bool flipY=false, 
		QPointF cropTL = QPointF(0,0), 
		QPointF cropBR = QPointF(0,0), 
		int b=0, 
		int c=0, 
		int h=0, 
		int s=0
	)
	:	flipHorizontal(flipX)
	,	flipVertical(flipY)
	,	cropTopLeft(cropTL)
	,	cropBottomRight(cropBR)
	,	brightness(b)
	,	contrast(c)
	,	hue(h)
	,	saturation(s)
	{}
	
	bool flipHorizontal;
	bool flipVertical;
	QPointF cropTopLeft;
	QPointF cropBottomRight;
	int brightness;
	int contrast;
	int hue;
	int saturation;
	
	QByteArray toByteArray();
	void fromByteArray(QByteArray);
};
QDebug operator<<(QDebug dbg, const VideoDisplayOptions &);

class GLVideoDrawable;
class VideoDisplayOptionWidget : public QWidget
{
	Q_OBJECT
public:
	VideoDisplayOptionWidget(GLVideoDrawable *drawable, QWidget *parent=0);
	VideoDisplayOptionWidget(const VideoDisplayOptions&, QWidget *parent=0);
	
signals:
	void displayOptionsChanged(const VideoDisplayOptions&);

public slots:
	void setDisplayOptions(const VideoDisplayOptions&);
	void undoChanges();
	
private slots:
	void flipHChanged(bool);
	void flipVChanged(bool);
	void cropX1Changed(int);
	void cropY1Changed(int);
	void cropX2Changed(int);
	void cropY2Changed(int);
	void bChanged(int);
	void cChanged(int);
	void hChanged(int);
	void sChanged(int);

private:
	void initUI();
	
	VideoDisplayOptions m_optsOriginal;
	VideoDisplayOptions m_opts;
	GLVideoDrawable *m_drawable;
	
	QCheckBox *m_cbFlipH;
	QCheckBox *m_cbFlipV;
	QSpinBox *m_spinCropX1;
	QSpinBox *m_spinCropX2;
	QSpinBox *m_spinCropY1;
	QSpinBox *m_spinCropY2;
	QSpinBox *m_spinB;
	QSpinBox *m_spinC;
	QSpinBox *m_spinH;
	QSpinBox *m_spinS;
	
};

class GLVideoDrawable : public GLDrawable
{
	Q_OBJECT
	
	Q_PROPERTY(int brightness READ brightness WRITE setBrightness);
	Q_PROPERTY(int contrast READ contrast WRITE setContrast);
	Q_PROPERTY(int hue READ hue WRITE setHue);
	Q_PROPERTY(int saturation READ saturation WRITE setSaturation);
	
	Q_PROPERTY(bool flipHorizontal READ flipHorizontal WRITE setFlipHorizontal);
	Q_PROPERTY(bool flipVertical READ flipVertical WRITE setFlipVertical);
	
	//Q_PROPERTY(QPointF cropTopLeft READ cropTopLeft WRITE setCropTopLeft);
	//Q_PROPERTY(QPointF cropBottomRight READ cropBottomRight WRITE setCropBottomRight);
	
	Q_PROPERTY(double cropTop READ cropTop WRITE setCropTop);
	Q_PROPERTY(double cropLeft READ cropLeft WRITE setCropLeft);
	Q_PROPERTY(double cropBottom READ cropBottom WRITE setCropBottom);
	Q_PROPERTY(double cropRight READ cropRight WRITE setCropRight);
	
	Q_PROPERTY(int whiteLevel READ whiteLevel WRITE setWhiteLevel);
	Q_PROPERTY(int blackLevel READ blackLevel WRITE setBlackLevel);
	Q_PROPERTY(int midLevel READ midLevel WRITE setMidLevel);
	Q_PROPERTY(double gamma READ gamma WRITE setGamma);
	
	
	Q_PROPERTY(QPointF textureOffset READ textureOffset WRITE setTextureOffset);
	
	//Q_PROPERTY(VideoSource* videoSource READ videoSource WRITE setVideoSource);
	
	Q_PROPERTY(Qt::AspectRatioMode aspectRatioMode READ aspectRatioMode WRITE setAspectRatioMode);
	Q_PROPERTY(bool ignoreAspectRatio READ ignoreAspectRatio WRITE setIgnoreAspectRatio);
	
	Q_PROPERTY(bool xfadeEnabled READ xfadeEnabled WRITE setXFadeEnabled);
	Q_PROPERTY(int xfadeLength READ xfadeLength WRITE setXFadeLength);
	
	Q_PROPERTY(bool videoSenderEnabled READ videoSenderEnabled WRITE setVideoSenderEnabled);
	Q_PROPERTY(int videoSenderPort READ videoSenderPort WRITE setVideoSenderPort);
	
	Q_PROPERTY(FilterType filterType READ filterType WRITE setFilterType);
	Q_PROPERTY(double sharpAmount READ sharpAmount WRITE setSharpAmount);
	Q_PROPERTY(double blurAmount READ blurAmount WRITE setBlurAmount); 
	Q_PROPERTY(int kernelSize READ kernelSize WRITE setKernelSize);
	Q_PROPERTY(QVariantList customKernel READ customKernel WRITE setCustomKernel);
	
public:
	GLVideoDrawable(QObject *parent=0);
	virtual ~GLVideoDrawable();
	
	const VideoFormat& videoFormat();
	
	int brightness() const;
	int contrast() const;
	int hue() const;
	int saturation() const;
	
	bool flipHorizontal() { return m_displayOpts.flipHorizontal; }
	bool flipVertical() { return m_displayOpts.flipVertical; }
	
	const QPointF & cropTopLeft() { return m_displayOpts.cropTopLeft; }
	const QPointF & cropBottomRight() { return m_displayOpts.cropBottomRight; }
	
	VideoSource *videoSource() { return m_source; }
	
	VideoDisplayOptions displayOptions() { return m_displayOpts; }
	
	QImage alphaMask() { return m_alphaMask; }
	
	QPointF textureOffset() { return m_textureOffset; }
	
	Qt::AspectRatioMode aspectRatioMode() { return m_aspectRatioMode; }
	bool ignoreAspectRatio() { return m_ignoreAspectRatio; }
	
	// This is the normal size of the content in pixels - independent of the specified rect().
	// E.g. if its an image, this is the size of the image, if this is text, then the size
	// of the text unscaled at natural resolution. Used for calculating alignment.
	virtual QSizeF naturalSize() { return m_sourceRect.size(); }
	
	float fpsLimit() { return m_rateLimitFps; }
	
	bool xfadeEnabled() { return m_xfadeEnabled; }
	int xfadeLength() { return m_xfadeLength; }
	
	bool videoSenderEnabled() { return m_videoSenderEnabled; }
	int videoSenderPort() { return m_videoSenderPort; }
	
	QRectF sourceRect() { return m_sourceRect; }
	QSize sourceSize() { return m_sourceRect.size().toSize(); }
	
	enum CrossFadeMode { FrontAndBack, JustFront };
	CrossFadeMode crossFadeMode() { return m_crossFadeMode; }
	
	bool liveStatus() { return m_liveStatus; }
	
	int blackLevel() { return m_blackLevel; }
	int whiteLevel() { return m_whiteLevel; }
	int midLevel() { return m_midLevel; }
	double gamma() { return m_gamma; } // used in levels adjustment
	
	bool isLevelsEnabled() { return m_levelsEnabled; }
	
	double cropTop() { return cropTopLeft().y(); }
	double cropLeft() { return cropTopLeft().x(); }
	double cropRight() { return cropBottomRight().x(); }
	double cropBottom() { return cropBottomRight().y(); }
	
	typedef enum FilterType {
		Filter_None,
		Filter_Blur,
		Filter_Emboss,
		Filter_Bloom,
		Filter_Mean,
		Filter_Sharp,
		Filter_Edge,
		Filter_CustomConvultionKernel
	};
	
	Q_ENUMS(FilterType);
	
	QVariantList customKernel() { return m_customKernel; }
	FilterType filterType() { return m_filterType; }
	double sharpAmount() { return m_sharpAmount; }
	double blurAmount() { return m_blurAmount; } 
	int kernelSize() { return m_kernelSize; }
	
public slots:
	void setFpsLimit(float);
	void setVisible(bool flag, bool waitOnFrameSignal=false);
	
	void setAlphaMask(const QImage&);
	void setTextureOffset(double x, double y);
	void setTextureOffset(const QPointF&);

	void setBrightness(int brightness);
	void setContrast(int contrast);
	void setHue(int hue);
	void setSaturation(int saturation);
	
	void setFlipHorizontal(bool);
	void setFlipVertical(bool);
	void setCropTopLeft(QPointF);
	void setCropBottomRight(QPointF);
	
	void setCropTop(double value) 		{ setCropTopLeft(QPointF(cropTopLeft().x(),value)); }
	void setCropLeft(double value) 		{ setCropTopLeft(QPointF(value,cropTopLeft().y())); }
	void setCropBottom(double value) 	{ setCropBottomRight(QPointF(cropBottomRight().x(),value)); }
	void setCropRight(double value) 	{ setCropBottomRight(QPointF(value,cropBottomRight().y())); }
	
	void setCropTop(int value) 		{ setCropTopLeft(QPointF(cropTopLeft().x(),((double)value)/100.)); }
	void setCropLeft(int value) 		{ setCropTopLeft(QPointF(((double)value)/100.,cropTopLeft().y())); }
	void setCropBottom(int value) 		{ setCropBottomRight(QPointF(cropBottomRight().x(),((double)value)/100.)); }
	void setCropRight(int value) 		{ setCropBottomRight(QPointF(((double)value)/100.,cropBottomRight().y())); }
	
	
	//void setLevels(int black=0, int white=255, int mid=-1, double gamma=1.61);
	void setLevelsEnabled(bool);
	void setBlackLevel(int black=0);
	void setWhiteLevel(int white=255);
	void setMidLevel(int mid=-1);
	void setGamma(double gamma=1.61);
	
	void setAspectRatioMode(Qt::AspectRatioMode mode);
	void setIgnoreAspectRatio(bool flag);
	
	void setXFadeEnabled(bool flag);
	void setXFadeLength(int length);
	
	/// PS
	void setVideoSource(VideoSource*);
	void disconnectVideoSource();
	
	void setDisplayOptions(const VideoDisplayOptions&);
	
	/// PS
	// Returns false if format won't be acceptable
	bool setVideoFormat(const VideoFormat& format, bool secondSource=false);
	
	void setVideoSenderEnabled(bool);
	void setVideoSenderPort(int);
	
	void setCrossFadeMode(CrossFadeMode mode);
	
	void setFilterType(int type) { setFilterType((FilterType)type); }
	void setFilterType(FilterType filterType); 
	void setSharpAmount(double value);
	void setBlurAmount(double value);
	void setKernelSize(int size);
	void setCustomKernel(QVariantList list);
	
signals:
	void displayOptionsChanged(const VideoDisplayOptions&);
	void sourceDiscarded(VideoSource*);

protected:
	// Overridden from GLDrawable
	virtual void setGLWidget(GLWidget* widget);
	
	virtual void aboutToPaint();
	
	/* This WILL be called BEFORE 'going live' and AFTER going 'not live' */
	// Default Impl sets m_liveStatus=live - MUST call this to make sure flag is set if you override in subclass.
	virtual void setLiveStatus(bool live);
	
	
	/// PS
	void paintGL();
	void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget);
	
	virtual void viewportResized(const QSize& newSize);
	virtual void canvasResized(const QSizeF& newSize);
	virtual void drawableRectChanged(const QRectF& newRect);
	
	void initGL();

	/// PS
	void initYv12TextureInfo(const QSize &size, bool secondSource=false);
	void initYuv420PTextureInfo(const QSize &size, bool secondSource=false);
	void initYuv442TextureInfo( const QSize &size, bool secondSource=false );
	void initRgbTextureInfo(GLenum internalFormat, GLuint format, GLenum type, const QSize &size, bool secondSource=false);
	
	void updateColors(int brightness, int contrast, int hue, int saturation);
	
	/// PS
	virtual void updateRects(bool secondSource=false);
	
	virtual void updateAnimations(bool insidePaint = false);
	
	/// PS
	QByteArray resizeTextures(const QSize& size, bool secondSource=false);
	
	void updateTextureOffsets();
	
	/// PS
	VideoFramePtr m_frame;
	VideoFramePtr m_frame2;
	bool m_visiblePendingFrame;
	bool m_tempVisibleValue;
	
	Qt::AspectRatioMode m_aspectRatioMode;
	
	VideoDisplayOptions m_displayOpts;
	
	/// PS
	QRectF m_targetRect;
	QRectF m_targetRect2;
	/// PS
	QRectF m_sourceRect;
	QRectF m_sourceRect2;
	
	
	// QGraphicsItem::
	virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
 
	void xfadeStart(bool invertStart=false);
	void xfadeStop();
	
protected slots:
	/// PS
	void frameReady();
	void frameReady2();
	void updateTexture(bool secondSource=false);
	
	void disconnectVideoSource2();
	void xfadeTick(bool updateGL=true);
	
	virtual void transformChanged();
	
private:
	/// PS
	VideoFormat m_videoFormat;
	VideoFormat m_videoFormat2;
	bool m_glInited;
	
	/// PS
	QGLShaderProgram *m_program;
	QGLShaderProgram *m_program2;
	
	QList<QVideoFrame::PixelFormat> m_imagePixelFormats;
	QList<QVideoFrame::PixelFormat> m_rawPixelFormats;
	
	QMatrix4x4 m_colorMatrix;
	//QGLContext *m_context;
	
	/// PS
	QVideoSurfaceFormat::Direction m_scanLineDirection;
	/// PS
	GLenum m_textureFormat;
	GLenum m_textureFormat2;
	/// PS
	GLuint m_textureInternalFormat;
	GLuint m_textureInternalFormat2;
	/// PS
	GLenum m_textureType;
	GLenum m_textureType2;
	
	/// PS
	int m_textureCount;
	int m_textureCount2;
	
	/// PS
	GLuint m_textureIds[3];
	GLuint m_textureIds2[3];
	/// PS
	int m_textureWidths[6];
	/// PS
	int m_textureHeights[6];
	/// PS
	int m_textureOffsets[6];
	
	/// PS
	// new YUV format support...
	GLuint m_textureFormats[6];
	GLuint m_textureInternalFormats[6];
	
	
	/// PS
	bool m_yuv;
	bool m_yuv2;
	
	bool m_packed_yuv;
	bool m_packed_yuv2;
	
	bool m_colorsDirty;
	
	/// PS
	QSize m_frameSize;
	QSize m_frameSize2;
	
	#ifndef QT_OPENGL_ES
	typedef void (APIENTRY *_glActiveTexture) (GLenum);
	_glActiveTexture glActiveTexture;
	#endif
	
	/// PS
	VideoSource *m_source;
	VideoSource *m_source2;
	
	QTime m_time;
	int m_frameCount;
	int m_latencyAccum;
	bool m_debugFps;
	
	QTimer m_timer;
	
	/// PS
	bool m_validShader;
	bool m_validShader2;
	
	QImage m_alphaMask;
	QImage m_alphaMask_preScaled;
	GLuint m_alphaTextureId;
	qint64 m_uploadedCacheKey;
	
	QPointF m_textureOffset;
	QPointF m_invertedOffset; // calculated as m_textureOffset.x() * 1/textureWidth, etc
	
	/// PS
	bool m_texturesInited;
	bool m_texturesInited2;
	
	QMutex m_frameReadyLock;
	
	bool m_useShaders;
	
	float m_rateLimitFps;
	QTimer m_fpsRateLimiter;
	
	// If m_xfadeEnabled, then when setVideoSource() is called,
	// m_fadeTick will be started at 1000/25 ms interval and
	// m_fadeTime will be started. The xfadeTick() slot will 
	// be called to update m_fadeValue while m_fadeTime.elapsed()
	// is less than m_xfadeLength; While fade is active,
	// m_fadeActive is true, which will tell the paint routines
	// to honor m_fadeValue. When fade is finished, xfadeDone()
	// slot is called and the second source is discarded. 
	bool m_xfadeEnabled;
	int m_xfadeLength; // in ms
	QTimer m_fadeTick;
	QTime m_fadeTime;
	double m_fadeValue;
	bool m_fadeActive;
	bool m_fadeTimeStarted;
	QEasingCurve m_fadeCurve;
	double m_startOpacity;
	
	class VideoSender *m_videoSender;
	bool m_videoSenderEnabled;
	int m_videoSenderPort;
	
	bool m_ignoreAspectRatio;
	
	CrossFadeMode m_crossFadeMode;
	
	// If setVideoSource() receives a CameraThread(), then this is called to hold an election among all the GLVideoDrawables
	// on the GLWidget with a CameraThread source. The video drawable with the highest avg FPS is elected the
	// updateLeader and will be the only drawable with a camerathread that calls updateGL() on frameReady() - all other 
	// drawables with camera threads will NOT call updateGL() in frameReady() unless they are elected updateLeader() later.
	void electUpdateLeader(GLVideoDrawable *ignore=0);
	
	// True if m_source is a CameraThread
	bool m_isCameraThread;
	// If this is a camera thread, elections among all the CameraThread-sourced-GLVideoDrawables in the GLWidget's drawable list
	// are held (in electUpdateLeader()) to find the one with the highest avg FPS. Elections are held by the update leader every
	// 100 frames, or when setVideoSource() receives a CameraThread. 
	GLVideoDrawable *m_updateLeader;
	// true if need election. 
	// Also, calls to setVideoSource() prior to setGLWidget() will set this true - then setGLWidget() will trigger election.
	bool m_electionNeeded;
	// Store avg FPS for use in election.
	double m_avgFps;
	// Count the number of unrendered frames - if this gets too high, then the updateLeader probably was deleted or some other problem - time to trigger a reelection
	int m_unrenderedFrames;
	
	bool m_liveStatus;
	
	bool m_textureUpdateNeeded;
	
	bool m_levelsEnabled;
	int m_blackLevel;
	int m_whiteLevel;
	int m_midLevel;
	double m_gamma;
	
	QTransform m_lastKnownTransform;
	
	QSize m_lastConvOffsetSize;
	QVarLengthArray<GLfloat> m_convOffsets;
	QVarLengthArray<GLfloat> m_convKernel;
	int m_kernelSize;
	
	FilterType m_filterType;
	double m_sharpAmount;
	double m_blurAmount;
	QVariantList m_customKernel;
	
	static int m_videoSenderPortAllocator;
};

#endif 

