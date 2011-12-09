#ifndef LiveVideoInputLayer_H
#define LiveVideoInputLayer_H

#include "LiveVideoLayer.h"

class GLVideoDrawable ;
class CameraThread;
class GLDrawable;
class GLWidget;
class VideoSource;

class LiveVideoInputLayer : public LiveVideoLayer
{
	Q_OBJECT
	
	Q_PROPERTY(QString inputDevice READ inputDevice WRITE setInputDevice USER true);
	Q_PROPERTY(bool deinterlace READ deinterlace WRITE setDeinterlace);
	
public:
	Q_INVOKABLE LiveVideoInputLayer(QObject *parent=0);
	~LiveVideoInputLayer();
	
	virtual QString typeName() { return "Video Input"; }
	
	bool deinterlace() { return layerProperty("deinterlace").toBool(); }
	
	QString inputDevice() { return m_cameraDev; }
	void setInputDevice(const QString& dev);

public slots:
	// Set a property (emits instancePropertyChanged)
	virtual void setLayerProperty(const QString& propertyId, const QVariant& value);

	void setDeinterlace(bool);
	
protected:
	virtual GLDrawable *createDrawable(GLWidget *widget, bool isSecondary=false);
	// If its the first drawable, setup with defaults
	// Otherwise, copy from 'copyFrom'
	virtual void initDrawable(GLDrawable *drawable, bool isFirstDrawable = false);
	virtual QWidget *createLayerPropertyEditors();
	
	void setCamera(CameraThread*);
	CameraThread *camera() { return m_camera; }
	
private slots:
	void selectCameraIdx(int);
	
private:
	CameraThread *m_camera;
	QString m_cameraDev;
};

#endif
