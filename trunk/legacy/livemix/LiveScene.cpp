
#include "LiveScene.h"
#include "LiveLayer.h"
#include "../glvidtex/GLDrawable.h"
#include "../glvidtex/GLWidget.h"
	
QHash<QString, const QMetaObject*> LiveScene::s_metaObjects;

///////////////////////
bool operator==(const LiveScene::KeyFrame& a, const LiveScene::KeyFrame& b) { return a.id == b.id; }

QList<LiveLayer*> LiveScene::KeyFrame::layers()
{
	QList<LiveLayer*> layers;
	foreach(int id, layerProperties.keys())
	{
		layers.append(scene->layerFromId(id));
	}
	
	return layers;
}

QVariantMap LiveScene::KeyFrame::propsForLayer(LiveLayer* layer)
{
	QVariantMap map;
	
	if(layer)
		map = layerProperties[layer->id()];	
	
	return map;
}

QByteArray LiveScene::KeyFrame::toByteArray()
{
	QByteArray array;
	QDataStream stream(&array, QIODevice::WriteOnly);
	QVariantMap map;
	
	if(layerProperties.isEmpty())
		return array;
		
	map["frameName"] = frameName;
	map["clockTime"] = clockTime;
	map["playTime"] = playTime;
	map["viewport"] = viewport;
	map["animParam.length"] = animParam.length;
	map["animParam.type"] = (int)animParam.curve.type();
		
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	pixmap.save(&buffer, "PNG"); // writes pixmap into bytes in PNG format
	buffer.close();
	map["pixmap"] = bytes;
	//qDebug() << "LiveScene::KeyFrame::toByteArray(): pixmap buffer byte count:"<<bytes.size()<<", pixmap size:"<<pixmap.size()<<", isnull:"<<pixmap.isNull();
	
	QVariantList list;
	foreach(int id, layerProperties.keys())
	{
		QVariantMap meta;
		meta["id"] = id;
		meta["map"] = layerProperties[id];
		
		list.append(meta);
	}
	
	map["props"] = list;
	
	stream << map;
	
	return array;
}

void LiveScene::KeyFrame::fromByteArray(QByteArray& array)
{
	QDataStream stream(&array, QIODevice::ReadOnly);
	QVariantMap map;
	stream >> map;
	
	//qDebug() << "LiveScene::fromByteArray(): "<<map;
	if(map.isEmpty())
	{
		qDebug() << "Error: LiveScene::KeyFrame::fromByteArray(): Map is empty, unable to load frame.";
		return;
	}
	
	layerProperties.clear();
	
	frameName = map["frameName"].toString();
	clockTime = map["clockTime"].toTime();
	playTime = map["playTime"].toDouble();
	viewport = map["viewport"].toRectF();
	animParam.length = map["animParam.length"].toInt();
	animParam.curve.setType((QEasingCurve::Type)map["animParam.curveType"].toInt());
	
	QByteArray bytes = map["pixmap"].toByteArray();
	QImage image;
	image.loadFromData(bytes);
	//qDebug() << "LiveScene::KeyFrame::fromByteArray(): image size:"<<image.size()<<", isnull:"<<image.isNull();
	
	pixmap = QPixmap::fromImage(image);
	
	
	QVariantList list = map["props"].toList();
	foreach(QVariant var, list)
	{
		QVariantMap meta = var.toMap();
		layerProperties[meta["id"].toInt()] = meta["map"].toMap();
	}
}

///////////////////////
LiveScene::LiveScene(QObject *parent)
	: QObject(parent)
	, m_canvasSize(1000.,750.)
	, m_viewport(QRectF()) // explicitly set a NULL rect so as to remind us how it starts out
{}

LiveScene::~LiveScene()
{
	//qDebug() << "LiveScene::~LiveScene() - start";
	while(!m_glWidgets.isEmpty())
	{
		// Yes, I know detachGLWidget() removes the widget from the list -
		// But, on the *rare* chance that it doesnt, I don't want this
		// list to run forever - so the takeFirst() call guarantees that
		// eventually the list will be empty, terminating the loop./
		detachGLWidget(m_glWidgets.takeFirst());
	}
	
	//qDebug() << "LiveScene::~LiveScene() - end";
	
	qDeleteAll(m_layers);
	m_layers.clear();
}

LiveLayer * LiveScene::layerFromId(int id)
{
	return m_idLookup[id];
}

void LiveScene::addLayer(LiveLayer *layer)
{
	if(!layer)
		return;
	if(!m_layers.contains(layer))
	{
		m_layers.append(layer);
		m_idLookup[layer->id()] = layer;
		layer->setScene(this);
		
		foreach(GLWidget *glw, m_glWidgets)
			layer->attachGLWidget(glw);

		emit layerAdded(layer);
	}
}

void LiveScene::removeLayer(LiveLayer *layer)
{
	if(!layer)
		return;
	if(m_layers.contains(layer))
	{
		m_layers.removeAll(layer);
		m_idLookup.remove(layer->id());
		foreach(GLWidget *glw, m_glWidgets)
			layer->detachGLWidget(glw);

		layer->setScene(0);
		emit layerRemoved(layer);
	}
}

void LiveScene::attachGLWidget(GLWidget *glw)
{
	if(!glw)
		return;
		
	m_glWidgets.append(glw);
	
	int numLayers = glw->property(GLW_PROP_NUM_SCENES).toInt();
	numLayers ++;
	glw->setProperty(GLW_PROP_NUM_SCENES,numLayers);
	
	if(numLayers == 1)
	{
		connect(this, SIGNAL(canvasSizeChanged(const QSizeF&)), glw, SLOT(setCanvasSize(const QSizeF&)));
		connect(this, SIGNAL(viewportChanged(const QRectF&)),   glw, SLOT(setViewport(const QRectF&)));
		
		glw->setCanvasSize(canvasSize());
		glw->setViewport(viewport());
		
		//qDebug() << "LiveScene::attachGLWidget: canvasSize:"<<canvasSize()<<", viewport:"<<viewport();
	}
	

	foreach(LiveLayer *layer, m_layers)
		layer->attachGLWidget(glw);
}

void LiveScene::detachGLWidget(GLWidget *glw)
{
	if(!glw)
		return;

	//qDebug() << "LiveScene::detachGLWidget(): glw: "<<glw;
	foreach(LiveLayer *layer, m_layers)
		layer->detachGLWidget(glw);
		
	disconnect(this, 0, glw, 0);
	
	int numLayers = glw->property(GLW_PROP_NUM_SCENES).toInt();
	numLayers --;
	glw->setProperty(GLW_PROP_NUM_SCENES,numLayers);

	m_glWidgets.removeAll(glw);
}

void LiveScene::fromByteArray(QByteArray& array)
{
	QDataStream stream(&array, QIODevice::ReadOnly);
	QVariantMap map;
	stream >> map;
	
	//qDebug() << "LiveScene::fromByteArray(): "<<map;
	if(map.isEmpty())
	{
		qDebug() << "Error: LiveScene::fromByteArray(): Map is empty, unable to load scene.";
		return;
	}
	
	qDeleteAll(m_layers);
	m_layers.clear();
	
	m_canvasSize = map["canvasSize"].toSizeF();
	m_viewport   = map["viewport"].toRectF();
	
	if(!m_canvasSize.isValid() || m_canvasSize.isNull()) // probably an old file with no size set
		m_canvasSize = QSizeF(1000.,750.);
	
	QVariantList layers = map["layers"].toList();
	foreach(QVariant layer, layers)
	{
		QVariantMap meta = layer.toMap();
		QString className = meta["class"].toString();
		const QMetaObject *metaObject = LiveScene::metaObjectForClass(className);
		if(!metaObject)
		{
			qDebug() << "Error: LiveScene::fromByteArray(): Unable to create layer of type:"<<className;
			continue;
		}
		
		//qDebug() << "LiveScene::fromByteArray(): Debug: metaObject classname: "<<metaObject->className()<<", asked for:"<<className;
		
		QObject *instance = metaObject->newInstance(Q_ARG(QObject*,0));
		if(!instance)
		{
			qDebug() << "Error: LiveScene::fromByteArray(): Creation of "<<className<<" failed.";
			continue;
		}
		
		
		LiveLayer *layer = dynamic_cast<LiveLayer*>(instance);
		if(!layer)
		{
			qDebug() << "Error: LiveScene::fromByteArray(): Object created for type "<<className<<" does not inherit from LiveLayer.";
			continue;
		}
		
		QByteArray bytes = meta["data"].toByteArray();
		layer->fromByteArray(bytes);
		
		addLayer(layer);
		//qDebug() << "LiveScene::fromByteArray(): Debug: metaObject classname: "<<metaObject->className()<<", layer ID:" <<layer->id();
	}
	
	// Basically, let the layers know that loading is complete
	foreach(LiveLayer *layer, m_layers)
		layer->setScene(this);
	
	m_keyFrames.clear();
	QVariantList keys = map["keys"].toList();
	foreach(QVariant key, keys)
	{
		LiveScene::KeyFrame frame(this);
		QByteArray ba = key.toByteArray();
		frame.fromByteArray(ba);
		frame.id = m_keyFrames.size()+1;
		m_keyFrames.append(frame);
	}
	
	sortKeyFrames();
}

double LiveScene::sceneLength()
{
	if(m_keyFrames.isEmpty())
		return 0;
		
	KeyFrame last = m_keyFrames.last();
	// assumes frames are sorted already by time
	double len = last.playTime + ((double)last.animParam.length)/1000.0;
	//qDebug() << "LiveScene::sceneLength(): len:"<<len;
	return len;
}

QByteArray LiveScene::toByteArray()
{
	QByteArray array;
	QDataStream stream(&array, QIODevice::WriteOnly);
	QVariantMap map;
	
	QVariantList list;
	if(m_layers.isEmpty())
		return array;
		
	map["canvasSize"] = m_canvasSize;
	map["viewport"]   = m_viewport;
		
	foreach(LiveLayer *layer, m_layers)
	{
		QVariantMap meta;
		meta["class"] = layer->metaObject()->className();
		meta["data"]  = layer->toByteArray();
		
		list.append(meta);
	}
	
	map["layers"] = list;
	
	QVariantList keys;
	foreach(LiveScene::KeyFrame frame, m_keyFrames)
	{
		keys.append(frame.toByteArray());
	}
	map["keys"] = keys;
	
	stream << map;
	
	return array;
}

LiveScene::KeyFrame LiveScene::createKeyFrame(QList<LiveLayer*> layers, const QPixmap& icon)
{
	LiveScene::KeyFrame frame(this);
	if(!icon.isNull())
		frame.pixmap = icon.scaled(QSize(128,128),Qt::KeepAspectRatio);
	
	if(layers.isEmpty())
		layers = m_layers;
	
	foreach(LiveLayer *layer, m_layers)
	{
		QVariantMap map = layer->propsToMap();
		// remove props not needed to keyframe
		foreach(QString key, map.keys())
			if(!layer->canAnimateProperty(key))
				map.remove(key);
// 		map.remove("showOnShowLayerId");
// 		map.remove("hideOnShowLayerId");
// 		map.remove("aspectRatioMode");
		frame.layerProperties[layer->id()] = map;
	}
	
	frame.playTime = m_keyFrames.isEmpty() ? 0 : ((int)m_keyFrames.last().playTime) + 1.0;
	
	return frame;
}


LiveScene::KeyFrame LiveScene::updateKeyFrame(int idx, const QPixmap& icon)
{
	if(idx < 0 || idx >= m_keyFrames.size())
		return LiveScene::KeyFrame(0);
	
	LiveScene::KeyFrame frame = m_keyFrames.takeAt(idx);
	if(!icon.isNull())
		frame.pixmap = icon.scaled(QSize(128,128),Qt::KeepAspectRatio);
	
	foreach(LiveLayer *layer, m_layers)
	{
		QVariantMap map = layer->propsToMap();
		// remove props not needed to keyframe
		foreach(QString key, map.keys())
			if(!layer->canAnimateProperty(key))
				map.remove(key);
		frame.layerProperties[layer->id()] = map;
	}
	
	m_keyFrames.insert(idx,frame);
	// play time doesnt change, so we dont need to re-sort
	
	return frame;
}

LiveScene::KeyFrame LiveScene::createAndAddKeyFrame(QList<LiveLayer*> layers, const QPixmap& icon)
{
	LiveScene::KeyFrame frame = createKeyFrame(layers, icon);
	addKeyFrame(frame);
	return frame;
}
	
void LiveScene::addKeyFrame(LiveScene::KeyFrame& frame)
{
	frame.id = m_keyFrames.size()+1;
	m_keyFrames.append(frame);
	sortKeyFrames();
	emit keyFrameAdded(frame);
}

void LiveScene::removeKeyFrame(const LiveScene::KeyFrame& frame)
{
	m_keyFrames.removeAll(frame);
	emit keyFrameRemoved(frame);
}

void LiveScene::removeKeyFrame(int idx)
{
	if(idx < 0 || idx >= m_keyFrames.size())
		return;
		
	removeKeyFrame(m_keyFrames[idx]);
}

void LiveScene::applyKeyFrame(const LiveScene::KeyFrame& frame)
{
	//QHash<LiveLayer*,bool> flags;
	foreach(int id, frame.layerProperties.keys())
	{
		LiveLayer * layer = m_idLookup[id];
		if(layer)
		{
// 			flags[layer] = true;
			layer->setAnimParam(frame.animParam);
			// true = apply only if changed
			layer->loadPropsFromMap(frame.layerProperties[id],true);
			// reset for future value changes
			layer->setAnimParam(LiveLayer::AnimParam());
		}
	}
	
// 	foreach(LiveLayer *layer, m_layers)
// 	{
// 		if(!flags[layer])
// 			layer->setVisible(false);
// 	}
}

void LiveScene::setKeyFrameName(int idx, const QString& name)
{
	if(idx < 0 || idx >= m_keyFrames.size())
		return;
	
	LiveScene::KeyFrame frame = m_keyFrames.takeAt(idx);
	frame.frameName = name;
	m_keyFrames.insert(idx,frame);
}

void LiveScene::setKeyFrameStartTime(int idx, double value)
{
	if(idx < 0 || idx >= m_keyFrames.size())
		return;
	
	LiveScene::KeyFrame frame = m_keyFrames.takeAt(idx);
	frame.playTime = value;
	m_keyFrames.insert(idx,frame);
	
	sortKeyFrames();
}

void LiveScene::setKeyFrameAnimLength(int idx, int value)
{
	if(idx < 0 || idx >= m_keyFrames.size())
		return;
	
	LiveScene::KeyFrame frame = m_keyFrames.takeAt(idx);
	frame.animParam.length = value;
	m_keyFrames.insert(idx,frame);
}


void LiveScene::applyKeyFrame(int idx)
{
	if(idx < 0 || idx >= m_keyFrames.size())
		return;
		
	applyKeyFrame(m_keyFrames[idx]);
}
	
bool LiveScene_keyFrame_compare(LiveScene::KeyFrame a, LiveScene::KeyFrame b)
{
	return a.playTime < b.playTime;
}


void LiveScene::sortKeyFrames()
{
	qSort(m_keyFrames.begin(), m_keyFrames.end(), LiveScene_keyFrame_compare);
}

void LiveScene::setCanvasSize(const QSizeF& size)
{
	m_canvasSize = size;
	emit canvasSizeChanged(size);
}

void LiveScene::setViewport(const QRectF& rect)
{
	m_viewport = rect;
	emit viewportChanged(rect);
}
