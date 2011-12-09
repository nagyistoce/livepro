#ifndef LiveVideoLayer_H
#define LiveVideoLayer_H

#include "LiveLayer.h"

class GLVideoDrawable ;
class CameraThread;
class GLDrawable;
class GLWidget;
class VideoSource;

class LiveVideoLayer : public LiveLayer
{
	Q_OBJECT
	
	Q_PROPERTY(int brightness READ brightness WRITE setBrightness);
	Q_PROPERTY(int contrast READ contrast WRITE setContrast);
	Q_PROPERTY(int hue READ hue WRITE setHue);
	Q_PROPERTY(int saturation READ saturation WRITE setSaturation);
	
	Q_PROPERTY(bool flipHorizontal READ flipHorizontal WRITE setFlipHorizontal);
	Q_PROPERTY(bool flipVertical READ flipVertical WRITE setFlipVertical);
	
	Q_PROPERTY(QPointF cropTopLeft READ cropTopLeft WRITE setCropTopLeft);
	Q_PROPERTY(QPointF cropBottomRight READ cropBottomRight WRITE setCropBottomRight);
	
	Q_PROPERTY(QPointF textureOffset READ textureOffset WRITE setTextureOffset);
	
	Q_PROPERTY(QString alphaMaskFile READ alphaMaskFile WRITE setAlphaMaskFile);
	Q_PROPERTY(QString overlayMaskFile READ overlayMaskFile WRITE setOverlayMaskFile);
	
	Q_PROPERTY(Qt::AspectRatioMode aspectRatioMode READ aspectRatioMode WRITE setAspectRatioMode);
	
public:
	LiveVideoLayer(QObject *parent=0);
	~LiveVideoLayer();
	
	virtual QString typeName() { return "Video"; }
	
	bool deinterlace()	{ return layerProperty("deinterlace").toBool(); }
	
	int brightness()	{ return layerProperty("brightness").toInt(); }
	int contrast()		{ return layerProperty("contrast").toInt(); }
	int hue()		{ return layerProperty("hue").toInt(); }
	int saturation()	{ return layerProperty("saturation").toInt(); }
	
	bool flipHorizontal()	{ return layerProperty("flipHorizontal").toBool(); }
	bool flipVertical()	{ return layerProperty("flipVertical").toBool(); }
	
	QPointF cropTopLeft()	{ return layerProperty("cropTopLeft").toPointF(); }
	QPointF cropBottomRight() { return layerProperty("cropBottomRight").toPointF(); }
	
	QPointF textureOffset()	{ return layerProperty("textureOffset").toPointF(); }
	
	QString alphaMaskFile()	{ return layerProperty("alphaMaskFile").toString(); }
	QString overlayMaskFile() { return layerProperty("overlayMaskFile").toString(); }
	
	Qt::AspectRatioMode aspectRatioMode() { return (Qt::AspectRatioMode)layerProperty("aspectRatioMode").toInt(); }


public slots:
	void setBrightness(int value) 	{ setLayerProperty("brightness", value); }
	void setContrast(int value) 	{ setLayerProperty("contrast", value); }
	void setHue(int value) 		{ setLayerProperty("hue", value); }
	void setSaturation(int value) 	{ setLayerProperty("saturation", value); }
	
	void setFlipHorizontal(bool value) 	{ setLayerProperty("flipHorizontal", value); }
	void setFlipVertical(bool value) 	{ setLayerProperty("flipVertical", value); }
	
	void setCropTopLeft(const QPointF &value)	{ setLayerProperty("cropTopLeft", value); }
	void setCropBottomRight(const QPointF &value)	{ setLayerProperty("cropBottomRight", value); }
	
	void setTextureOffset(const QPointF &value)	{ setLayerProperty("textureOffset", value); }
	
	void setAlphaMaskFile(const QString &value)	{ setLayerProperty("alphaMaskFile", value); }
	void setOverlayMaskFile(const QString &value)	{ setLayerProperty("overlayMaskFile", value); }
	
	void setAspectRatioMode(Qt::AspectRatioMode value)	{ setLayerProperty("aspectRatioMode", (int)value); }
	void setAspectRatioMode(int value)			{ setLayerProperty("aspectRatioMode", value); }
	
	virtual void setLayerProperty(const QString& propertyId, const QVariant& value);

signals:
	void videoSourceChanged(VideoSource*);
	
protected:
	virtual GLDrawable *createDrawable(GLWidget *widget, bool isSecondary=false);
	// If its the first drawable, setup with defaults
	// Otherwise, copy from 'copyFrom'
	virtual void initDrawable(GLDrawable *drawable, bool isFirstDrawable = false);
	virtual QWidget *createLayerPropertyEditors();
	
	virtual void setVideoSource(VideoSource*);
	VideoSource *videoSource() { return m_source; }
	
	
private:
	VideoSource *m_source;
};

#endif
