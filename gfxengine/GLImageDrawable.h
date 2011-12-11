#ifndef GLImageDrawable_h
#define GLImageDrawable_h

#include "GLVideoDrawable.h"

class GLImageDrawable : public GLVideoDrawable
{
	Q_OBJECT
	
	Q_PROPERTY(QString 	imageFile 	READ imageFile 		WRITE setImageFile USER true);
	
	Q_PROPERTY(QColor	borderColor 	READ borderColor 	WRITE setBorderColor);
	Q_PROPERTY(double	borderWidth 	READ borderWidth 	WRITE setBorderWidth);

	Q_PROPERTY(bool		shadowEnabled	READ isShadowEnabled	WRITE setShadowEnabled);
	Q_PROPERTY(double 	shadowBlurRadius READ shadowBlurRadius	WRITE setShadowBlurRadius);
	Q_PROPERTY(QPointF	shadowOffset 	READ shadowOffset	WRITE setShadowOffset);
	Q_PROPERTY(QColor	shadowColor	READ shadowColor	WRITE setShadowColor);
	Q_PROPERTY(double	shadowOpacity	READ shadowOpacity	WRITE setShadowOpacity);

public:
	GLImageDrawable(QString file="", QObject *parent=0);
	virtual ~GLImageDrawable();
	
	QString imageFile() { return m_imageFile; }
	QImage image() { return m_image; }
	
	bool allowAutoRotate() { return m_allowAutoRotate; }
	
	virtual void loadPropsFromMap(const QVariantMap&, bool onlyApplyIfChanged = false);
	virtual QVariantMap propsToMap();
	
	QColor borderColor() { return m_borderColor; }
	double borderWidth() { return m_borderWidth; }
	
	bool isShadowEnabled() { return m_shadowEnabled; }
	double shadowBlurRadius() { return m_shadowBlurRadius; }
	QPointF shadowOffset() { return m_shadowOffset; }
	QColor shadowColor() { return m_shadowColor; }
	double shadowOpacity() { return m_shadowOpacity; }
		
signals:
	void imageFileChanged(const QString&);

public slots:
	void setImage(const QImage&, bool insidePaint=false);
	virtual bool setImageFile(const QString&);
	
	void setAllowAutoRotate(bool flag);
	
	void setBorderColor(const QColor&);
	void setBorderWidth(double);
	
	void setShadowEnabled(bool);
	void setShadowBlurRadius(double);
	void setShadowBlurRadius(int value) { setShadowBlurRadius((double)value); }
	void setShadowOffset(const QPointF&);
	void setShadowColor(const QColor&);
	void setShadowOpacity(double);
	void setShadowOpacity(int percent) { setShadowOpacity(((double)percent) / 100.); }
	
	
protected:
	void internalSetFilename(QString);
	
	/// Reimplemented from GLVideoDrawable in order to dynamically adjust memory usage
	virtual void setLiveStatus(bool);
	
	virtual void reloadImage();
	virtual void releaseImage();
	virtual bool canReleaseImage();
	
	/// Hook for subclasses that handle their own border rendering such as GLTextDrawable
	virtual void borderSettingsChanged() {}
	/// Hook for subclasses that handle their own border rendering such as GLTextDrawable - return false to disable rendering of the border in GLImageDrawable
	virtual bool renderBorder() { return true; }
	
	QImage m_image;
	QString m_imageFile;
	QString m_fileLastModified;
	
	bool m_releasedImage;
	bool m_allowAutoRotate;
	bool m_needUpdate;
	
	/// Caching for embedding the image data in the stored file
	QString    m_cachedImageFilename;
	QDateTime  m_cachedImageMtime;
	QImage     m_cachedImage;
	QByteArray m_cachedImageBytes;
	
	static int m_allocatedMemory;
	static int m_activeMemory;
	
	/// Border attributes
	QColor	m_borderColor;
	double	m_borderWidth;

	/// Shadow attributes
	bool	m_shadowEnabled;
	double 	m_shadowBlurRadius;
	QPointF	m_shadowOffset;
	QColor	m_shadowColor;
	double	m_shadowOpacity;
	
	/// Starts a timer to call reapplyBorderAndShadow() in 50ms in order to batch updates to the above attributes
	void shadowDirty();
	void borderDirty();
	QTimer m_shadowDirtyBatchTimer;
	QTimer m_borderDirtyBatchTimer;
	
	QImage m_imageWithBorder;
	
	/// Apply the {Border attributes} and {Shadow attributes} to the given image and return the new image, possibly larger than the original depending on settings. 
	QImage applyBorder(const QImage&);
	
	GLImageDrawable *m_shadowDrawable;
	virtual void drawableResized(const QSizeF&);
	
protected slots:
	void reapplyBorder();
	void updateShadow();
	
private:
	void setVideoSource(VideoSource*);
};

#endif
