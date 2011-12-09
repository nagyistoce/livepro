#ifndef LiveStaticSourceLayer_H
#define LiveStaticSourceLayer_H

#include "LiveVideoLayer.h"

class GLVideoDrawable ;
class GLDrawable;
class GLWidget;
class StaticVideoSource;


class LiveStaticSourceLayer : public LiveVideoLayer
{
	Q_OBJECT
	
	Q_PROPERTY(QString file READ file WRITE setFile USER true)
	
public:
	Q_INVOKABLE LiveStaticSourceLayer(QObject *parent=0);
	~LiveStaticSourceLayer();
	
	virtual QString typeName() { return "Static Source"; }

	QString file() { return layerProperty("file").toString(); }

public slots:
 	// Set a property (emits instancePropertyChanged)
 	virtual void setLayerProperty(const QString& propertyId, const QVariant& value);

	void setFile(const QString&);

protected:
	virtual GLDrawable *createDrawable(GLWidget *widget, bool isSecondary=false);
	// If its the first drawable, setup with defaults
	// Otherwise, copy from 'copyFrom'
	virtual void initDrawable(GLDrawable *drawable, bool isFirstDrawable = false);
	virtual QWidget *createLayerPropertyEditors();
	
private:
	StaticVideoSource *m_staticSource;
};

#endif
