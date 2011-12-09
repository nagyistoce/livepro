#ifndef LiveVideoLoopLayer_H
#define LiveVideoLoopLayer_H

#include "LiveVideoLayer.h"

class GLVideoDrawable ;
class CameraThread;
class GLDrawable;
class GLWidget;
class VideoThread;

class LiveVideoLoopLayer : public LiveVideoLayer
{
	Q_OBJECT
	
	Q_PROPERTY(QString file READ file WRITE setFile);
	
public:
	Q_INVOKABLE LiveVideoLoopLayer(QObject *parent=0);
	~LiveVideoLoopLayer();
	
	virtual QString typeName() { return "Video Loop"; }
	
	QString file() { return layerProperty("file").toString(); }

public slots:
	// Set a property (emits instancePropertyChanged)
	void setFile(const QString&);

	virtual void setLayerProperty(const QString& propertyId, const QVariant& value);

protected:
	virtual GLDrawable *createDrawable(GLWidget *widget);
	// If its the first drawable, setup with defaults
	// Otherwise, copy from 'copyFrom'
	virtual void initDrawable(GLDrawable *drawable, bool isFirstDrawable = false);
	virtual QWidget *createLayerPropertyEditors();
	
	void setVideo(VideoThread*);
	VideoThread *video() { return m_video; }
	
private:
	VideoThread *m_video;
};

#endif
