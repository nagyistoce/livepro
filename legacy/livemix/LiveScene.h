#ifndef LiveScene_H
#define LiveScene_H

#include <QObject>
#include <QList>

#include "LiveLayer.h"
class LayerControlWidget;
class GLWidget;

#define LiveScene_Register(a) LiveScene::registerClass(a::staticMetaObject.className(), &a::staticMetaObject);

#define GLW_PROP_NUM_SCENES "LiveScene/NumAttachedScenes"


class LiveScene : public QObject
{
	Q_OBJECT
	
	// Coordinates go through several stages:
	// The physical window on the screen is often a fixed size that can't be configured or changed - a GLWidget fit to the screen or a window.
	// The GLWidget than computes a transform to fit the viewport (or the entire canvas if the viewport is invalid) into an aspect-ratio correct
	// rectangle. If the viewport doesnt match the canvas, it will set up a transform to first translate the viewport into the canvas, then 
	// apply the scaling and translation for the window.
	
	// This is the usable area for arranging layers.
	Q_PROPERTY(QSizeF canvasSize READ canvasSize WRITE setCanvasSize);
	
	// Viewport is an "instructive" property - can be used by GLWidgets to look at a subset of the canvas.
	// By default, the viewport is an invalid rectangle, which will instruct the GLWidget to look at the entire canvas.
	Q_PROPERTY(QRectF viewport READ viewport WRITE setViewport);

private:
	static QHash<QString, const QMetaObject*> s_metaObjects;	

public:
	static const QMetaObject* metaObjectForClass(const QString& name)       { return s_metaObjects[name]; }
	static void registerClass(const QString& name, const QMetaObject *meta) { s_metaObjects[name] = meta; }
	
public:
	LiveScene(QObject *parent=0);
	~LiveScene();
	
	QSizeF canvasSize() { return m_canvasSize; }
	QRectF viewport() { return m_viewport; }
	
	QList<LiveLayer*> layerList() { return m_layers; }
	LiveLayer *layerFromId(int);
	
	void addLayer(LiveLayer*);
	void removeLayer(LiveLayer*);
	
	// Only the first layer attached to a GLWidget manages the viewport - the rest just are along for the ride.
	// Note that layers in each attached scene still respect the individual scenes canvas for layout
	void attachGLWidget(GLWidget*);
	void detachGLWidget(GLWidget*);
	
	QByteArray toByteArray();
	void fromByteArray(QByteArray&);
	
	class KeyFrame
	{
	public:
		KeyFrame(LiveScene *s, int x=-1)
			: scene(s)
			, id(x)
			, playTime(-1)
		{}
		
		LiveScene *scene;
		
		int id;
		
		// Where to look at on the canvas
		QRectF viewport;
		
		// int is the id of the layer
		QHash<int, QVariantMap> layerProperties;
		LiveLayer::AnimParam animParam;
		
		// convenience accessors
		QList<LiveLayer*> layers();
		QVariantMap propsForLayer(LiveLayer*);
		
		// playTime : Time on the 'play clock' to show this frame, in seconds
		double playTime;
		// clockTime : if set, overrides playTime - wall time to show this frame
		QTime clockTime;
		
		// Name for displaying in a list, for example, drop down list
		QString frameName;
		
		// Graphic of the way the scene looks with these properties, scaled to 128x128, aspect ratio preserved
		QPixmap pixmap;
		
		// For loading/saving frames
		QByteArray toByteArray();
		void fromByteArray(QByteArray&);
	};
	
	KeyFrame createKeyFrame(QList<LiveLayer*> layers = QList<LiveLayer*>(), const QPixmap& icon=QPixmap());
	KeyFrame createAndAddKeyFrame(QList<LiveLayer*> layers = QList<LiveLayer*>(), const QPixmap& icon=QPixmap());
	
	KeyFrame updateKeyFrame(int, const QPixmap& newIcon=QPixmap());
	
	// not const, because it sets the .id of the frame
	void addKeyFrame(KeyFrame&);
	void removeKeyFrame(const KeyFrame&);
	void removeKeyFrame(int);
	
	void applyKeyFrame(const KeyFrame&);
	void applyKeyFrame(int);
	
	void setKeyFrameName(int, const QString&);
	void setKeyFrameStartTime(int, double);
	void setKeyFrameAnimLength(int, int);
	
	QList<KeyFrame> keyFrames() { return m_keyFrames; }
	
	// time it would take to play ever key frame, include all animations, in sequence
	double sceneLength();
	
signals:
	void layerAdded(LiveLayer*);
	void layerRemoved(LiveLayer*);
	
	void keyFrameAdded(const KeyFrame&);
	void keyFrameRemoved(const KeyFrame&);
	
	void canvasSizeChanged(const QSizeF&);
	void viewportChanged(const QRectF&);

public slots:
	void setCanvasSize(const QSizeF&);
	void setViewport(const QRectF&);
	void setViewport(const QPointF& point) { setViewport(QRectF(point,viewport().size())); }
	
private:
	void sortKeyFrames();
	
	QList<GLWidget*>  m_glWidgets;
	QList<LiveLayer*> m_layers;
	QHash<int,LiveLayer*> m_idLookup;
	QList<KeyFrame> m_keyFrames;
	
	QSizeF m_canvasSize;
	QRectF m_viewport;
};

	
bool operator==(const LiveScene::KeyFrame& a, const LiveScene::KeyFrame& b);
	

#endif
