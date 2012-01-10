#include "GLSceneGroup.h"
#include "GLSceneGroupType.h"
#include "GLDrawable.h"
#include "GLWidget.h"

#include "MetaObjectUtil.h"

#include "GLDrawables.h"

#include "GLEditorGraphicsScene.h"

// A crossfade speed less than this amount will disable crossfading
#define MIN_FADE_DURATION -1

// ****************************
// GLSceneLayoutItem
// ****************************

GLSceneLayoutItem::GLSceneLayoutItem(int id, const QVariantMap& props)
	: m_drawableId(id)
	, m_props(props)
{
}
GLSceneLayoutItem::GLSceneLayoutItem(GLDrawable *drawable, const QVariantMap& props)
	: m_drawableId(drawable ? drawable->id() : -1)
	, m_props(props)
{
}
GLSceneLayoutItem::GLSceneLayoutItem(QByteArray& ba)
	: m_drawableId(-1)
{
	fromByteArray(ba);
}

GLSceneLayoutItem::~GLSceneLayoutItem()
{
}

QByteArray GLSceneLayoutItem::toByteArray()
{
	QByteArray array;
	QDataStream stream(&array, QIODevice::WriteOnly);

	QVariantMap map;
	map["drawableId"]	= m_drawableId;
	map["props"] 		= m_props;

	stream << map;

	return array;
}

void GLSceneLayoutItem::fromByteArray(QByteArray& array)
{
	QDataStream stream(&array, QIODevice::ReadOnly);
	QVariantMap map;
	stream >> map;

	if(map.isEmpty())
		return;

	m_drawableId	= map["drawableId"].toInt();
	m_props		= map["props"].toMap();
}

void GLSceneLayoutItem::setDrawable(GLDrawable* d)
{
	setDrawableId(d ? d->id() : -1);
}

void GLSceneLayoutItem::setDrawableId(int d)
{
	m_drawableId = d;
}

void GLSceneLayoutItem::setPropertyHash(const QVariantMap& props)
{
	m_props = props;
}


// ****************************
// GLSceneLayoutItem
// ****************************

GLSceneLayout::GLSceneLayout(GLScene *s)
	: m_scene(s)
{
}

GLSceneLayout::GLSceneLayout(QByteArray& ba, GLScene* s)
	: m_scene(s)
{
	fromByteArray(ba);
}

GLSceneLayout::~GLSceneLayout()
{
}

QByteArray GLSceneLayout::toByteArray()
{
	QByteArray array;
	QDataStream stream(&array, QIODevice::WriteOnly);

	QVariantMap map;
	map["layoutId"]		= layoutId();
	map["layoutName"] 	= m_layoutName;

	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	m_pixmap.save(&buffer, "PNG"); // writes pixmap into bytes in PNG format
	buffer.close();
	map["pixmap"] = bytes;

	QVariantList items;
	foreach(GLSceneLayoutItem *item, m_items)
		items << item->toByteArray();

	map["items"] = items;

	stream << map;

	return array;
}

void GLSceneLayout::fromByteArray(QByteArray& array)
{
	QDataStream stream(&array, QIODevice::ReadOnly);
	QVariantMap map;
	stream >> map;

	if(map.isEmpty())
		return;

	m_layoutId	= map["layoutId"].toInt();
	m_layoutName	= map["layoutName"].toString();

	QByteArray bytes = map["pixmap"].toByteArray();
	QImage image;
	image.loadFromData(bytes);
	//qDebug() << "GLSceneLayout::fromByteArray(): image size:"<<image.size()<<", isnull:"<<image.isNull();

	m_pixmap = QPixmap::fromImage(image);

	m_items.clear();
	QVariantList items = map["items"].toList();
	foreach(QVariant var, items)
	{
		QByteArray data = var.toByteArray();
		m_items << new GLSceneLayoutItem(data);
	}
}

void GLSceneLayout::setLayoutName(const QString& name)
{
	m_layoutName = name;
	emit layoutNameChanged(name);
}

void GLSceneLayout::setPixmap(const QPixmap& pixmap)
{
	m_pixmap = pixmap;
	emit pixmapChanged(pixmap);
}


/*private:
	GLScene *m_scene;
	int m_layoutId;
	QString m_layoutName;
	QList<GLSceneLayoutItem*> m_items;*/


// ****************************
// GLSceneLayoutListModel
// ****************************
// We make a separate list model for the layouts because the
// GLScene class can only implement a list model for a single datatype - in its case, the main data, GLDrawables

GLSceneLayoutListModel::GLSceneLayoutListModel(GLScene *scene)
	: QAbstractListModel()
	, m_scene(scene)
{
	connect(m_scene, SIGNAL(layoutAdded(GLSceneLayout*)),   this, SLOT(layoutAdded(GLSceneLayout*)));
	connect(m_scene, SIGNAL(layoutRemoved(GLSceneLayout*)), this, SLOT(layoutRemoved(GLSceneLayout*)));
}

GLSceneLayoutListModel::~GLSceneLayoutListModel()
{

}

void GLSceneLayoutListModel::layoutAdded(GLSceneLayout*)
{
	QModelIndex top    = createIndex(m_scene->m_layouts.size()-2, 0),
		    bottom = createIndex(m_scene->m_layouts.size()-1, 0);
	dataChanged(top,bottom);

}

void GLSceneLayoutListModel::layoutRemoved(GLSceneLayout *lay)
{
	int idx = m_scene->m_layouts.indexOf(lay);
	QModelIndex top    = createIndex(idx, 0),
		    bottom = createIndex(m_scene->m_layouts.size(), 0);
	dataChanged(top,bottom);
}

int GLSceneLayoutListModel::rowCount(const QModelIndex &/*parent*/) const
{
	return m_scene->m_layouts.size();
}

QVariant GLSceneLayoutListModel::data( const QModelIndex & index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (index.row() >= rowCount(QModelIndex()))
		return QVariant();

	if (role == Qt::DisplayRole || Qt::EditRole == role)
	{
		GLSceneLayout *d = m_scene->m_layouts.at(index.row());
		QString value = d->layoutName().isEmpty() ? QString("Layout %1").arg(index.row()+1) : d->layoutName();
		return value;
	}
	else if(Qt::DecorationRole == role)
	{
		GLSceneLayout *lay = m_scene->m_layouts.at(index.row());
		return lay->pixmap();
	}
	else
		return QVariant();
}

// ****************************
// GLScene
// ****************************

GLScene::GLScene(QObject *parent)
	: QAbstractListModel(parent)
	, m_sceneId(-1)
	, m_opacity(1)
	, m_zIndex(1)
	, m_listOnlyUserItems(false)
	, m_glWidget(0)
	, m_layoutListModel(0)
	, m_graphicsScene(0)
	, m_fadeClockActive(false)
	, m_crossfadeSpeed(300)
	, m_duration(5.)
	, m_autoDuration(true)
	, m_autoSchedule(true)
	, m_sceneType(0)
	, m_group(0)
	, m_rootObj(0)
{
	connect(&m_fadeTimer, SIGNAL(timeout()), this, SLOT(fadeTick()));
}

GLScene::GLScene(QByteArray& ba, QObject *parent)
	: QAbstractListModel(parent)
	, m_sceneId(-1)
	, m_opacity(1)
	, m_zIndex(1)
	, m_listOnlyUserItems(false)
	, m_glWidget(0)
	, m_layoutListModel(0)
	, m_graphicsScene(0)
	, m_fadeClockActive(false)
	, m_crossfadeSpeed(300)
	, m_duration(5.)
	, m_autoDuration(true)
	, m_autoSchedule(true)
	, m_sceneType(0)
	, m_group(0)
	, m_rootObj(0)
{
	connect(&m_fadeTimer, SIGNAL(timeout()), this, SLOT(fadeTick()));
	fromByteArray(ba);
}

GLScene::~GLScene() 
{
	if(m_sceneType)
		delete m_sceneType;
	if(m_layoutListModel)
		delete m_layoutListModel;
	qDeleteAll(m_itemList);
	m_itemList.clear();
}

GLSceneLayoutListModel *GLScene::layoutListModel()
{
	if(!m_layoutListModel)
		m_layoutListModel = new GLSceneLayoutListModel(this);
	return m_layoutListModel;
}

int GLScene::sceneId()
{
	if(m_sceneId<0)
	{
		QSettings s;
		m_sceneId = s.value("glscene/scene-id-counter",100).toInt() + 1;
		s.setValue("glscene/scene-id-counter", m_sceneId);
	}
	return m_sceneId;
}

QByteArray GLScene::toByteArray()
{
	QByteArray array;
	QDataStream stream(&array, QIODevice::WriteOnly);

	QVariantMap map;
	map["sceneId"]		= sceneId();
	map["sceneName"] 	= m_sceneName;
	map["duration"]		= m_duration;
	map["autoDuration"]	= m_autoDuration;

	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	m_pixmap.save(&buffer, "PNG"); // writes pixmap into bytes in PNG format
	buffer.close();
	map["pixmap"] = bytes;


	QVariantList layouts;
	foreach(GLSceneLayout *layout, m_layouts)
		layouts << layout->toByteArray();

	map["layouts"] = layouts;

	QVariantList drawables;
	foreach(GLDrawable *d, m_itemList)
	{
		QVariantMap drawableData;
		drawableData["class"] = d->metaObject()->className();
		drawableData["bytes"] = d->toByteArray();
		drawables << drawableData;
	}

	map["drawables"] = drawables;
	
	if(sceneType())
		map["typedata"]  = sceneType()->toByteArray();
/*	else
		qDebug() << "GLSceneType::toByteArray: No scene type assigned, not storing byteArray data";*/
	
	stream << map;

	return array;
}

void GLScene::fromByteArray(QByteArray& array)
{
	QDataStream stream(&array, QIODevice::ReadOnly);
	QVariantMap map;
	stream >> map;

	if(map.isEmpty())
		return;

	m_sceneId	= map["sceneId"].toInt();
	m_sceneName	= map["sceneName"].toString();
	m_duration 	= map["duration"].toDouble();
	m_autoDuration	= map["autoDuration"].toBool();

	QByteArray bytes = map["pixmap"].toByteArray();
	QImage image;
	image.loadFromData(bytes);
	m_pixmap = QPixmap::fromImage(image);

	m_layouts.clear();
	QVariantList layouts = map["layouts"].toList();
	foreach(QVariant var, layouts)
	{
		QByteArray data = var.toByteArray();
		addLayout(new GLSceneLayout(data, this));
	}

	m_itemList.clear();
	QVariantList drawables = map["drawables"].toList();
	foreach(QVariant var, drawables)
	{
		QVariantMap meta = var.toMap();
		QString className = meta["class"].toString();
// 		const QMetaObject *metaObject = MetaObjectUtil::metaObjectForClass(className);
// 		if(!metaObject)
// 		{
// 			qDebug() << "Error: GLScene::fromByteArray(): Unable to create drawable of type:"<<className;
// 			continue;
// 		}
//
// 		//qDebug() << "LiveScene::fromByteArray(): Debug: metaObject classname: "<<metaObject->className()<<", asked for:"<<className;
//
// 		QObject *instance = metaObject->newInstance(Q_ARG(QObject*,0));
// 		if(!instance)
// 		{
// 			qDebug() << "Error: GLScene::fromByteArray(): Creation of "<<className<<" failed.";
// 			continue;
// 		}

		GLDrawable *drawable = 0;

		#define IF_CLASS(x) if(className == #x) { drawable = new x(); }

		IF_CLASS(GLImageDrawable);
		IF_CLASS(GLTextDrawable);
		IF_CLASS(GLVideoFileDrawable);
		IF_CLASS(GLVideoInputDrawable);
		IF_CLASS(GLVideoLoopDrawable);
		IF_CLASS(GLVideoReceiverDrawable);
		IF_CLASS(GLVideoMjpegDrawable);
		IF_CLASS(GLSvgDrawable);
		IF_CLASS(GLRectDrawable);
		IF_CLASS(GLSpinnerDrawable);
		IF_CLASS(GLImageHttpDrawable);

		#undef IF_CLASS


		//GLDrawable *drawable = dynamic_cast<GLDrawable*>(instance);
		if(!drawable)
		{
//			qDebug() << "Error: GLScene::fromByteArray(): Object created for type "<<className<<" does not inherit from GLDrawable.";
 			qDebug() << "Error: GLScene::fromByteArray(): Creation of "<<className<<" failed.";
			continue;
		}

		QByteArray bytes = meta["bytes"].toByteArray();
		drawable->fromByteArray(bytes);

		addDrawable(drawable);
	}
	
	if(map["typedata"].isValid())
	{
		QByteArray ba = map["typedata"].toByteArray();
		// fromByteArray() calls our setSceneType function for us
		GLSceneTypeFactory::fromByteArray(ba, this);
	} 
	else
	{
//		qDebug() << "GLScene::fromByteArray: typedata not valid:"<<map["typedata"]; 
	}

}

GLScene *GLScene::clone()
{
	QByteArray ba = toByteArray();
	return new GLScene(ba);
}

void GLScene::setSceneType(GLSceneType *type)
{
	if(m_sceneType)
	{
		delete m_sceneType;
		m_sceneType = 0;
	}
	
	m_sceneType = type;
}

void GLScene::setSceneGroup(GLSceneGroup *g)
{
	m_group = g;
}

void GLScene::setListOnlyUserItems(bool flag)
{
	m_listOnlyUserItems = flag;

	if(flag)
	{
		m_userItemList.clear();
		foreach(GLDrawable *d, m_itemList)
			if(d->isUserControllable())
				m_userItemList.append(d);
	}

	// Notify QListViews of change in data
	QModelIndex top    = createIndex(0, 0),
		    bottom = createIndex(m_itemList.size() > m_userItemList.size() ? m_itemList.size() : m_userItemList.size(), 0);
	dataChanged(top,bottom);

	//qDebug() << "GLScene::setListOnlyUserItems: "<<m_listOnlyUserItems;
}

int GLScene::rowCount(const QModelIndex &/*parent*/) const
{
	return size();
}

int GLScene::size() const
{
	if(m_listOnlyUserItems)
		return m_userItemList.size();
	else
		return m_itemList.size();
}

GLDrawable * GLScene::at(int idx)
{
	if(idx<0 || idx >= size())
		return 0;

	GLDrawable *gld = 0;
	if(m_listOnlyUserItems)
		gld = m_userItemList.at(idx);
	else
		gld = m_itemList.at(idx);

	//qDebug() << "GLScene::at: m_listOnlyUserItems:"<<m_listOnlyUserItems<<", idx:"<<idx<<", gld:"<<(QObject*)gld;

	return gld;
}

QVariant GLScene::data( const QModelIndex & index, int role ) const
{
	if (!index.isValid())
		return QVariant();

	if (index.row() >= rowCount(QModelIndex()))
		return QVariant();

	if (role == Qt::DisplayRole || Qt::EditRole == role)
	{
		GLDrawable *d = m_listOnlyUserItems ? m_userItemList.at(index.row()) : m_itemList.at(index.row());
		QString value = d->itemName().isEmpty() ? QString("Item %1").arg(index.row()+1) : d->itemName();
		//qDebug() << "GLScene::data: row:"<<index.row()<<", value:"<<value;
		return value;
	}
// 	else if(Qt::DecorationRole == role)
// 	{
// 	}
	else
		return QVariant();
}

void GLScene::addDrawable(GLDrawable *d)
{
	if(!d)
		return;
	
	if(m_itemList.contains(d))
		return;
		
	m_itemList << d;
	m_drawableIdLookup[d->id()] = d;
	m_drawableNameLookup[d->itemName()] = d;
	emit drawableAdded(d);
	
	d->setGLScene(this);
	
	connect(d, SIGNAL(destroyed()), this, SLOT(drawableDestroyed()));
	connect(d, SIGNAL(itemNameChanging(QString)), this, SLOT(drawableNameChanging(QString)));
	connect(d->playlist(), SIGNAL(itemAdded(GLPlaylistItem*)), this, SLOT(drawablePlaylistItemChanged()));
	connect(d->playlist(), SIGNAL(itemRemoved(GLPlaylistItem*)), this, SLOT(drawablePlaylistItemChanged()));
	connect(d->playlist(), SIGNAL(playlistItemChanged()), this, SLOT(drawablePlaylistItemChanged()));

	beginInsertRows(QModelIndex(),m_itemList.size()-1,m_itemList.size());

	// Notify QListViews of change in data
	QModelIndex top    = createIndex(m_itemList.size()-2, 0),
		    bottom = createIndex(m_itemList.size()-1, 0);
	dataChanged(top,bottom);

	if(m_glWidget)
		m_glWidget->addDrawable(d);

	if(m_graphicsScene)
		m_graphicsScene->addItem(d);

	endInsertRows();
}

void GLScene::drawableDestroyed()
{
	GLDrawable *d = dynamic_cast<GLDrawable*>(sender());
	//qDebug() << "GLScene::drawableDestroyed(): "<<(QObject*)d;
	if(d)
		removeDrawable(d);
}

void GLScene::removeDrawable(GLDrawable *d)
{
	//qDebug() << "GLScene::removeDrawable(): "<<(QObject*)d;
	if(!d)
		return;

	beginRemoveRows(QModelIndex(),0,m_itemList.size()+1);

	int idx = m_itemList.indexOf(d);

	emit drawableRemoved(d);
	m_itemList.removeAll(d);
	m_userItemList.removeAll(d);
	m_drawableIdLookup.remove(d->id());
	m_drawableNameLookup.remove(d->itemName());
	disconnect(d, 0, this, 0);

	// Notify QListViews of change in data
	QModelIndex top    = createIndex(idx, 0),
		    bottom = createIndex(m_itemList.size(), 0);
	dataChanged(top,bottom);

	if(m_glWidget)
		m_glWidget->removeDrawable(d);

	if(m_graphicsScene)
		m_graphicsScene->removeItem(d);

	endRemoveRows();
}

GLDrawable * GLScene::lookupDrawable(int id)
{
	return m_drawableIdLookup[id];
}

GLDrawable * GLScene::lookupDrawable(const QString &name)
{
	return m_drawableNameLookup[name];
}

void GLScene::drawableNameChanging(QString name)
{
	GLDrawable *gld = dynamic_cast<GLDrawable *>(sender());
	if(!gld)
		return;
		
	m_drawableNameLookup.remove(gld->itemName());
	m_drawableNameLookup[name] = gld;
}

void GLScene::setGraphicsScene(QGraphicsScene *scene)
{
	if(scene)
	{
		foreach(GLDrawable *gld, m_itemList)
			scene->addItem(gld);
	}
	else
	if(m_graphicsScene)
	{
		foreach(GLDrawable *gld, m_itemList)
			m_graphicsScene->removeItem(gld);
	}
	
	m_graphicsScene = scene;
}

QGraphicsScene * GLScene::graphicsScene()
{
	return m_graphicsScene;
}

// This is the 'crossover' method which
// connects the drawables in this scene to an actual display widget.
// Only one widget can be set at a time - if the widget is changed,
// the drawables are removed from the old and added to the new
void GLScene::setGLWidget(GLWidget *glw, int zIndexOffset)
{
	if(m_glWidget)
		detachGLWidget();

	m_glWidget = glw;
	
	//if(!m_rootObj)
	//	m_rootObj = new GLDrawable();
		
	if(m_rootObj)
	{
		foreach(GLDrawable *d, m_itemList)
		{
			//d->setZIndexModifier(zIndexOffset);
			//m_glWidget->addDrawable(d);
			m_rootObj->addChild(d);
		}
		
		m_rootObj->setZIndexModifier(zIndexOffset);
		m_glWidget->addDrawable(m_rootObj);
	}
	else
	{
		foreach(GLDrawable *d, m_itemList)
		{
			d->setZIndexModifier(zIndexOffset);
			m_glWidget->addDrawable(d);
		}
	}
	
	if(sceneType())		
		sceneType()->setLiveStatus(true);
}

void GLScene::detachGLWidget()
{
	if(!m_glWidget)
		return;

	foreach(GLDrawable *d, m_itemList)
		if(m_rootObj)
			m_rootObj->removeChild(d);
		else
			m_glWidget->removeDrawable(d);
	
	if(m_rootObj)
		m_glWidget->removeDrawable(m_rootObj);

	if(sceneType())		
		sceneType()->setLiveStatus(false);
		
	m_glWidget = 0;
}

// Layouts specify properties of the drawbles in the scene
void GLScene::addLayout(GLSceneLayout *lay)
{
	if(!lay)
		return;
	m_layouts << lay;
	m_layoutIdLookup[lay->layoutId()] = lay;
	emit layoutAdded(lay);
}

void GLScene::removeLayout(GLSceneLayout *lay)
{
	if(!lay)
		return;
	emit layoutRemoved(lay);
	m_layouts.removeAll(lay);
	m_layoutIdLookup.remove(lay->layoutId());
}

GLSceneLayout * GLScene::lookupLayout(int id)
{
	return m_layoutIdLookup[id];
}

void GLScene::setSceneName(const QString& name)
{
	m_sceneName = name;
	emit sceneNameChanged(name);
}

void GLScene::setPixmap(const QPixmap& pixmap)
{
	m_pixmap = pixmap;
	emit pixmapChanged(pixmap);
}

void GLScene::setOpacity(double d, bool animate, double animDuration)
{
	if(animate && 
	   animDuration >= MIN_FADE_DURATION)
	{
// 		QPropertyAnimation *anim = new QPropertyAnimation(this, "opacity");
// 		anim->setDuration(animDuration);
// 		anim->setEndValue(d);
// 		connect(anim, SIGNAL(finished()), this, SIGNAL(opacityAnimationFinished()));
// 		anim->start(QAbstractAnimation::DeleteWhenStopped);
// 		
		if(m_fadeTimer.isActive())
			m_fadeTimer.stop();
			
		//qDebug() << "GLScene::setOpacity: "<<this<<" opac:"<<d<<", duration:"<<animDuration<<", current opac:"<<opacity();

		m_fadeTimer.setInterval(1000 / 25); // 25fps fade
		m_crossfadeSpeed = (int)animDuration;
		
		m_fadeDirection = d < opacity() ?  -1 : 1;
		m_endOpacity = d;
		m_startOpacity = opacity();
		
		m_fadeClockActive = false;
		m_fadeTimer.start();
		
		//qDebug() << "GLScene::setOpacity: Timer started";
		
		return;
	}
	else
	{
		//qDebug() << "GLScene::setOpacity: animate flag false";
	}
	
	//qDebug() << "GLScene::setOpacity: "<<d;
	
	m_opacity = d;
	emit opacityChanged(d);
	if(m_rootObj)
	{
		//m_rootObj->updateGL();
		m_rootObj->setOpacity(d);
		//qDebug() << "GLScene::setOpacity: Set root "<<(QObject*)m_rootObj<<" opacity "<<d;
	}
	else
	{
		//qDebug() << "GLScene::setOpacity: Calling updateGL in hopes they honor our opacity "<<d;
		foreach(GLDrawable *d, m_itemList)
			d->updateGL();
	}
	
	if(animate && 
	   animDuration < MIN_FADE_DURATION)
	{
		emit opacityAnimationFinished();
	}
}



void GLScene::fadeTick()
{
	//qDebug() << "GLScene::fadeTick: mark";
	if(!m_fadeClockActive)
	{
		m_fadeClockActive = true;
		m_fadeClock.start();
		m_fadeActive = true;
	}
	if(m_fadeClock.elapsed() >= m_crossfadeSpeed)
	{
		//qDebug() << "GLScene::fadeTick: fade ended ("<<m_fadeClock.elapsed()<<" > "<<m_crossfadeSpeed<<")";
		m_fadeActive = false;
		m_fadeTimer.stop();
		setOpacity(m_endOpacity);
		emit opacityAnimationFinished();
	}
	else
	{
		recalcFadeOpacity();
	}
}

void GLScene::recalcFadeOpacity(bool setOpac)
{
	if(!m_fadeClockActive)
		return;
		
	int time = m_fadeClock.elapsed();

	double progress = ((double)time) / ((double)m_crossfadeSpeed);
        double valueLength = fabs(m_endOpacity - m_startOpacity);
        double fadeVal = valueLength * progress;
        if(m_fadeDirection < 0)
        	fadeVal = m_startOpacity - fadeVal;
	
        if(fadeVal < 0)
        	return;
        if(fadeVal > 1.0)
        	return;

	if(setOpac)
		setOpacity(fadeVal);
	else
	{
		//qDebug() << "GLScene::recalcFadeOpacity (!setOpac): "<<fadeVal;
		m_opacity = fadeVal;
	}
}



void GLScene::setZIndex(double d)
{
	m_zIndex = d;
	emit zIndexChanged(d);
	
	foreach(GLDrawable *gld, m_itemList)
		gld->setZIndexModifier(d);
}

void GLScene::setDuration(double duration)
{
	m_duration = duration;
	//qDebug() << "GLScene::setDuration(): new duration:"<<duration;
	emit durationChanged(duration);
}

void GLScene::setAutoDuration(bool flag)
{
	m_autoDuration = flag;
	emit autoDurationChanged(flag);
	
	if(flag)
		setDuration(calcDuration());
}

double GLScene::calcDuration()
{
	double max = -1;
	foreach(GLDrawable *gld, m_itemList)
	{
		double dur = gld->playlist()->duration();
		
		if(dur > max)
			max = dur;
	}
	return max;
}

void GLScene::drawablePlaylistItemChanged()
{
	//qDebug() << "GLScene::drawablePlaylistItemChanged()";
	if(autoDuration())
		setDuration(calcDuration());
}

void GLScene::setScheduledTime(const QDateTime& date)
{
	m_scheduledTime = date;
	emit scheduledTimeChanged(date);
}

void GLScene::setAutoSchedule(bool flag)
{
	m_autoSchedule = flag;
	emit autoScheduleChanged(flag);
}


// ****************************
// GLSceneGroup
// ****************************

GLSceneGroup::GLSceneGroup(QObject *parent)
	: QAbstractItemModel(parent)
	, m_groupId(-1)
	, m_duration(5.)
	, m_autoDuration(true)
	, m_autoSchedule(true)
	, m_groupType(0)
	, m_playlist(new GLSceneGroupPlaylist(this))
{}

GLSceneGroup::GLSceneGroup(QByteArray& ba, QObject *parent)
	: QAbstractItemModel(parent)
	, m_groupId(-1)
	, m_duration(5.)
	, m_autoDuration(true)
	, m_autoSchedule(true)
	, m_groupType(0)
	, m_playlist(new GLSceneGroupPlaylist(this))
{
	fromByteArray(ba);
}

GLSceneGroup::~GLSceneGroup()
{
	qDeleteAll(m_scenes);
	m_scenes.clear();
}

int GLSceneGroup::groupId()
{
	if(m_groupId<0)
	{
		QSettings s;
		m_groupId = s.value("glscenegroup/group-id-counter",100).toInt() + 1;
		s.setValue("glscenegroup/group-id-counter", m_groupId);
	}
	return m_groupId;
}

QByteArray GLSceneGroup::toByteArray()
{
	QByteArray array;
	QDataStream stream(&array, QIODevice::WriteOnly);

	QVariantMap map;
	map["groupId"]		= groupId();
	map["groupName"] 	= m_groupName;

	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	m_pixmap.save(&buffer, "PNG"); // writes pixmap into bytes in PNG format
	buffer.close();
	map["pixmap"] = bytes;


	QVariantList scenes;
	foreach(GLScene *scene, m_scenes)
		scenes << scene->toByteArray();

	map["scenes"] = scenes;

	stream << map;

	return array;
}

void GLSceneGroup::fromByteArray(QByteArray& array)
{
	QDataStream stream(&array, QIODevice::ReadOnly);
	QVariantMap map;
	stream >> map;

	if(map.isEmpty())
		return;

	m_groupId	= map["groupId"].toInt();
	m_groupName	= map["groupName"].toString();

	QByteArray bytes = map["pixmap"].toByteArray();
	QImage image;
	image.loadFromData(bytes);
	m_pixmap = QPixmap::fromImage(image);

	// TODO...
	//qDeleteAll(m_scenes);
	m_scenes.clear();
	QVariantList scenes = map["scenes"].toList();
	foreach(QVariant var, scenes)
	{
		QByteArray data = var.toByteArray();
		GLScene *scene = new GLScene(data, this);
		//m_scenes << scene;
		//m_sceneIdLookup[scene->sceneId()] = scene;
		addScene(scene);
	}
}

void GLSceneGroup::setGroupType(GLSceneGroupType *type)
{
	m_groupType = type;
}


int GLSceneGroup::rowCount(const QModelIndex &/*parent*/) const
{
	int sz = m_scenes.size();
	//qDebug() << "GLSceneGroup::rowCount: "<<this<<" sz:"<<sz;
	return sz;
}

QVariant GLSceneGroup::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(orientation != Qt::Horizontal ||
		section < 0 || 
		section > 1 ||
		role != Qt::DisplayRole)
		return QVariant();
		
	
	return section == 0 ? "Title" : "Duration";
}

QModelIndex GLSceneGroup::index(int row, int column, const QModelIndex&) const
{
	return createIndex(row,column);
}

QModelIndex GLSceneGroup::parent(const QModelIndex&) const
{
	return QModelIndex();
}

int GLSceneGroup::columnCount(const QModelIndex&) const
{
	return 2;
}


QVariant GLSceneGroup::data( const QModelIndex & index, int role) const
{
	//qDebug() << "GLSceneGroup::data: index:"<<index;
	if (!index.isValid())
		return QVariant();

	if (index.row() >= rowCount(QModelIndex()))
		return QVariant();

	GLScene *d = m_scenes.at(index.row());
			
	if(index.column() == 0)
	{
		if (role == Qt::DisplayRole || Qt::EditRole == role)
		{
			QString value = d->sceneName().isEmpty() ? QString("Scene %1").arg(index.row()+1) : d->sceneName();
			//qDebug() << "GLSceneGroup::data: "<<this<<" row:"<<index.row()<<", value:"<<value;
			return value;
		}
		else if(Qt::DecorationRole == role)
		{
			return d->pixmap();
		}
	}
	else 
	if(index.column() == 1)
	{
		if(role == Qt::DisplayRole || Qt::EditRole == role)
		{
			QString dur = QString().sprintf("%.02f", d->duration());
			return dur;
		}
		else
		if(role == Qt::TextAlignmentRole)
		{
			return Qt::AlignRight;
		}
		else
		if(role == Qt::BackgroundRole)
		{
			if(d->autoDuration())
				return Qt::gray;
			else
				return QVariant();	
		}
	}
	
	return QVariant();
}

Qt::ItemFlags GLSceneGroup::flags(const QModelIndex &index) const
{
	if (index.isValid())
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable; //| Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
		
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;// | Qt::ItemIsDropEnabled ;
}

bool GLSceneGroup::setData(const QModelIndex &index, const QVariant & value, int /*role*/)
{
	if (!index.isValid())
		return false;

	if (index.row() >= rowCount(QModelIndex()))
		return false;

	GLScene *d = m_scenes.at(index.row());
	qDebug() << "GLSceneGroup::setData: "<<this<<" row:"<<index.row()<<", value:"<<value;
	if(value.isValid() && !value.isNull())
	{
		if(index.column() == 0)
		{	
			d->setSceneName(value.toString());
		}
		else
		{
			double val = value.toDouble();
			if(!val)
			{
				d->setAutoDuration(true);
			}
			else
			{
				d->setDuration(val);
				d->setAutoDuration(false);
			}
			//emit itemDurationEdited(d);
		}
		
		dataChanged(index,index);
		return true;
	}

	return false;
}


// The core of the scene group is a list of scenes.
// The order is explicit through their index in the QList, though not relevant
// as the order they are played in is specified by the GLSchedule and GLScheduleItems.
// Although the scenes are displayed in order in the 'Director' program
void GLSceneGroup::addScene(GLScene* s)
{
	if(!s)
		return;

	m_scenes << s;
	m_sceneIdLookup[s->sceneId()] = s;
	connect(s, SIGNAL(sceneNameChanged(const QString&)), 	this, SLOT(sceneChanged()));
	connect(s, SIGNAL(pixmapChanged(const QPixmap&)), 	this, SLOT(sceneChanged()));
	connect(s, SIGNAL(durationChanged(double)), 		this, SLOT(sceneChanged()));
	connect(s, SIGNAL(autoDurationChanged(bool)), 		this, SLOT(sceneChanged()));
	connect(s, SIGNAL(scheduledTimeChanged(QDateTime)), 	this, SLOT(sceneChanged()));
	connect(s, SIGNAL(autoScheduleChanged(bool)), 		this, SLOT(sceneChanged()));

	s->setSceneGroup(this);
	
	emit sceneAdded(s);
	//qDebug() << "GLSceneGroup::addScene: "<<this<<" scene:"<<s<<", m_scenes.size():"<<m_scenes.size()<<", rowCount:"<<rowCount(QModelIndex());

	beginInsertRows(QModelIndex(),m_scenes.size()-1,m_scenes.size());

	// Notify QListViews of change in data
	QModelIndex top    = createIndex(m_scenes.size()-2, 0),
		    bottom = createIndex(m_scenes.size()-1, 0);
	dataChanged(top,bottom);
	endInsertRows();
}

void GLSceneGroup::removeScene(GLScene* s)
{
	if(!s)
		return;

	beginRemoveRows(QModelIndex(),0,m_scenes.size()+1);
	int idx = m_scenes.indexOf(s);

	emit sceneRemoved(s);

	disconnect(s, 0, this, 0);
	m_scenes.removeAll(s);
	m_sceneIdLookup.remove(s->sceneId());
	
	s->setSceneGroup(0);

	// Notify QListViews of change in data
	QModelIndex top    = createIndex(idx, 0),
		    bottom = createIndex(m_scenes.size(), 0);
	dataChanged(top,bottom);
	endRemoveRows();
}

void GLSceneGroup::sceneChanged()
{
	GLScene *scene = dynamic_cast<GLScene*>(sender());
	if(!scene)
		return;
		
	int row = m_scenes.indexOf(scene);
	// Notify QListViews of change in data
	QModelIndex idx1 = createIndex(row, 0);
	QModelIndex idx2 = createIndex(row, 1);
	dataChanged(idx1, idx2);
	
	emit sceneDataChanged();
	
	/// TODO recalc scene datetimes if they are auto scheduled based on cumulative duration 
	/// TODO recalc our duration based on scene durations IF our duration is 'autoDuration'
}

GLScene * GLSceneGroup::lookupScene(int id)
{
	return m_sceneIdLookup[id];
}

// Overlay scene, by definition, is a general scene that is to be overlayed on the content of
// the other scenes in the list.
void GLSceneGroup::setOverlayScene(GLScene* s)
{
	m_overlayScene = s;
	emit overlaySceneChanged(s);
}

void GLSceneGroup::setGroupName(const QString& name)
{
	m_groupName = name;
	emit groupNameChanged(name);
}


void GLSceneGroup::setPixmap(const QPixmap& pixmap)
{
	m_pixmap = pixmap;
	emit pixmapChanged(pixmap);
}

void GLSceneGroup::setDuration(double duration)
{
	m_duration = duration;
	emit durationChanged(duration);
}

void GLSceneGroup::setAutoDuration(bool flag)
{
	m_autoDuration = flag;
	emit autoDurationChanged(flag);
}

void GLSceneGroup::setScheduledTime(const QDateTime& date)
{
	m_scheduledTime = date;
	emit scheduledTimeChanged(date);
}

void GLSceneGroup::setAutoSchedule(bool flag)
{
	m_autoSchedule = flag;
	emit autoScheduleChanged(flag);
}

// ****************************
// GLSceneGroupPlaylist
// ****************************

GLSceneGroupPlaylist::GLSceneGroupPlaylist(GLSceneGroup *group)
	: QObject(group)
	, m_group(group)
	, m_playTime(0)
	, m_currentItemIndex(-1)
	, m_isRandom(false)
	, m_currentItem(0)
{
	connect(&m_currentItemTimer, SIGNAL(timeout()), this, SLOT(nextItem()));
	m_currentItemTimer.setSingleShot(true);
	m_currentItemTimer.setInterval(0);
}

GLSceneGroupPlaylist::~GLSceneGroupPlaylist()
{

}

GLScene *GLSceneGroupPlaylist::currentItem()
{
	if(m_currentItemIndex < 0 || m_currentItemIndex >= m_group->size())
		return 0;
	return m_group->at(m_currentItemIndex);
}

double GLSceneGroupPlaylist::duration()
{
	double dur=0;
	foreach(GLScene *item, m_group->sceneList())
		dur += item->duration();
	return dur;
}

double GLSceneGroupPlaylist::timeFor(GLScene* timeForItem)
{
	double time=0;
	foreach(GLScene *item, m_group->sceneList())
	{
		if(item == timeForItem)
			return time;
		else
			time += item->duration();
	}
	return -1;
}


void GLSceneGroupPlaylist::setIsPlaying(bool flag)
{
	if(flag)
		play();
	else
		stop();
}
void GLSceneGroupPlaylist::play(bool restart)
{
	if(restart)
		m_playTime = 0;
	m_currentItemTimer.start();
}
void GLSceneGroupPlaylist::stop()
{
	m_currentItemTimer.stop();
}

void GLSceneGroupPlaylist::playItem(GLScene *item)
{
	int idx = m_group->sceneList().indexOf(item);
	if(idx < 0)
		return;
		
	if(idx == m_currentItemIndex)
		return;
		
	if(!item)
		return;
	
	m_currentItemIndex = idx;
	
	if(item->duration() <= 0)
	{
		nextItem();
		return;
	}
		
	emit currentItemChanged(item);
	
	m_playTime = timeFor(item);
	emit timeChanged(m_playTime);
	
	setCurrentItem(item);
	//qDebug() << "GLSceneGroupPlaylist::playItem: Showing scene:"<<item<<", num:"<<m_currentItemIndex<<"for"<<item->duration()<<"sec";
	
}

void GLSceneGroupPlaylist::setCurrentItem(GLScene *item)
{
	int idx = m_group->sceneList().indexOf(item);
	if(idx < 0)
		return;
	
	m_currentItemIndex = idx;
	m_playTime = timeFor(item);
	
	m_currentItemTimer.stop();
	
	int dur = (int)(item->duration() * 1000);
		
	if(item->duration() <= 0.01)
		return;
	else
	{
		//qDebug() << "GLSceneGroupPlaylist::setCurrentItem: Starting timer on item:"<<item<<", num:"<<m_currentItemIndex<<"for"<<item->duration()<<"sec";
		m_currentItemTimer.stop();
		m_currentItemTimer.setInterval(dur);
		m_currentItemTimer.start();
		m_currentElapsedTime.restart();
		
		if(m_currentItem)
			disconnect(m_currentItem, 0, this, 0);
			
		m_currentItem = item;
		connect(m_currentItem, SIGNAL(durationChanged(double)), this, SLOT(sceneDurationChanged(double)));
	}
}

void GLSceneGroupPlaylist::sceneDurationChanged(double d)
{
	if(sender() != m_currentItem)
		return;
		
	double e = ((double)m_currentElapsedTime.elapsed()) / 1000.;
	double newDur = d - e;
	if(newDur < 0)
		newDur = d;
	if(newDur > 0)
	{
		qDebug() << "GLSceneGroupPlaylist::sceneDurationChanged: New duration:"<<d<<", already elapsed:"<<e<<", new timer length:"<<newDur;
		m_currentItemTimer.stop();
		m_currentItemTimer.setInterval((int)( /*newDur */ d * 1000. ));
		m_currentItemTimer.start();
		m_currentElapsedTime.restart();
	}
	else
	{
		qDebug() << "GLSceneGroupPlaylist::sceneDurationChanged: Not restarting timer because newDur<0:"<<newDur<<", d:"<<d<<", already elapsed:"<<e;
	}
}

bool GLSceneGroupPlaylist::setPlayTime(double time)
{
	//qDebug() << "GLSceneGroupPlaylist::setPlayTime: "<<time<<"/"<<duration();

	double timeSum = 0;
	GLScene *foundItem =0;
	foreach(GLScene *item, m_group->sceneList())
	{
		if(time >= timeSum && time < (timeSum + item->duration()))
		{
			foundItem = item;
			break;
		}
		timeSum += item->duration();
	}
	if(foundItem)
	{
		playItem(foundItem);
		return true;
	}
	else
	{
		m_playTime = time;
		emit timeChanged(time);

		qDebug() << "GLSceneGroupPlaylist::setPlayTime: "<<time<<": Could not find item at time "<<time;
		return false;
	}

}

void GLSceneGroupPlaylist::nextItem()
{
	if(m_group->isEmpty())
		return;
	
	int next = 0;
	
	if(m_isRandom)
	{
		int high = m_group->size() - 1;
		int low = 0;
		next = qrand() % ((high + 1) - low) + low;
	}
	else
	{
		next = m_currentItemIndex + 1;
	}
	
	if(next >= m_group->size())
		next = 0;
		
	double e = ((double)m_currentElapsedTime.elapsed()) / 1000.;
	qDebug() << "GLSceneGroupPlaylist::nextItem(): Scene#:"<<next<<", elapsed:"<<e;
	playItem(m_group->at(next));
}

void GLSceneGroupPlaylist::prevItem()
{
	if(m_group->isEmpty())
		return;
	
	int next = m_currentItemIndex - 1;
	if(next < 0)
		next = m_group->size() - 1;
		
	playItem(m_group->at(next));
}

void GLSceneGroupPlaylist::setIsRandom(bool flag)
{
	m_isRandom = flag;
}

// void GLSceneGroupPlaylist::timerTick()
// {
// 	if(!setPlayTime(m_playTime + m_timerTickLength))
// 		if(!setPlayTime(0))
// 			stop();
// }



// protected:
// 	int m_groupId;
// 	QString m_groupName;
//
// 	QList<GLScene*> m_scenes;
// 	QHash<int,GLScene*> m_sceneIdLookup;
//
// 	GLScene *m_overlayScene;


// ****************************
// GLSceneGroupCollection
// ****************************

GLSceneGroupCollection::GLSceneGroupCollection(QObject *parent)
	: QAbstractListModel(parent)
	, m_collectionId(-1)
	, m_canvasSize(1000.,750.)
{}

GLSceneGroupCollection::GLSceneGroupCollection(QByteArray& ba, QObject *parent)
	: QAbstractListModel(parent)
	, m_collectionId(-1)
	, m_canvasSize(1000.,750.)
{
	fromByteArray(ba);
}

GLSceneGroupCollection::GLSceneGroupCollection(const QString& file, QObject *parent)
	: QAbstractListModel(parent)
	, m_collectionId(-1)
	, m_canvasSize(1000.,750.)
{
	readFile(file);
}

GLSceneGroupCollection::~GLSceneGroupCollection()
{
	qDeleteAll(m_groups);
	m_groups.clear();
}

int GLSceneGroupCollection::collectionId()
{
	if(m_collectionId<0)
	{
		QSettings s;
		m_collectionId = s.value("GLSceneGroupCollection/collection-id-counter",100).toInt() + 1;
		s.setValue("GLSceneGroupCollection/collection-id-counter", m_collectionId);
	}
	return m_collectionId;
}

QByteArray GLSceneGroupCollection::toByteArray()
{
	QByteArray array;
	QDataStream stream(&array, QIODevice::WriteOnly);

	QVariantMap map;
	map["collectionId"]	= collectionId();
	map["collectionName"] 	= m_collectionName;
	map["canvasSize"] 	= m_canvasSize;

	QVariantList groups;
	foreach(GLSceneGroup *group, m_groups)
		groups << group->toByteArray();

	map["groups"] = groups;

	//qDebug() << "GLSceneGroupCollection::toByteArray(): Saved "<<groups.size()<<" groups";


	stream << map;

	return array;
}

void GLSceneGroupCollection::fromByteArray(QByteArray& array)
{
	QDataStream stream(&array, QIODevice::ReadOnly);
	QVariantMap map;
	stream >> map;

	if(map.isEmpty())
	{
		qDebug() << "GLSceneGroupCollection::fromByteArray(): [WARN] Unable to read collection from byte array because map is empty.";
		return;
	}

	m_collectionId		= map["collectionId"].toInt();
	m_collectionName	= map["collectionName"].toString();
	m_canvasSize 		= map["canvasSize"].toSizeF();

	if(m_canvasSize.isEmpty())
		m_canvasSize = QSizeF(1000.,750.);

	m_groups.clear();
	QVariantList groups = map["groups"].toList();

	//qDebug() << "GLSceneGroupCollection::fromByteArray(): Loaded "<<groups.size()<<" groups";

	foreach(QVariant var, groups)
	{
		QByteArray data = var.toByteArray();
		//m_groups << new GLSceneGroup(data, this);
		GLSceneGroup *group = new GLSceneGroup(data, this);
		addGroup(group);
	}
}

bool GLSceneGroupCollection::writeFile(const QString& name)
{
	if(name.isEmpty() && m_fileName.isEmpty())
		return false;

	QString fileName = name.isEmpty() ? m_fileName : name;
	m_fileName = fileName;

	QFile file(fileName);
	// Open file
	if (!file.open(QIODevice::WriteOnly))
	{
		qDebug() << "GLSceneGroupCollection::writeFile: Unable to open "<<fileName<<" for writing.";
		return false;
	}
	else
	{
		file.write(toByteArray());
		file.close();

		//qDebug() << "Debug: Saved SceneID: "<< scene->sceneId();
	}
	return true;
}

bool GLSceneGroupCollection::readFile(const QString& name)
{
	m_fileName = name;
	QFile file(name);
	if (!file.open(QIODevice::ReadOnly))
	{
		qDebug() << "GLSceneGroupCollection::writeFile: Unable to open "<<name<<" for writing.";
		return false;
	}
	else
	{
		QByteArray array = file.readAll();
		fromByteArray(array);

		//qDebug() << "Debug: Loaded SceneID: "<< scene->sceneId();
	}
	return true;
}

Qt::ItemFlags GLSceneGroupCollection::flags(const QModelIndex &index) const
{
	if (index.isValid())
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable; //| Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled ;
}

bool GLSceneGroupCollection::setData(const QModelIndex &index, const QVariant & value, int /*role*/)
{
	if (!index.isValid())
		return false;

	if (index.row() >= rowCount(QModelIndex()))
		return false;

	GLSceneGroup *d = m_groups.at(index.row());
	qDebug() << "GLSceneGroupCollection::setData: "<<this<<" row:"<<index.row()<<", value:"<<value;
	if(value.isValid() && !value.isNull())
	{
		d->setGroupName(value.toString());
		dataChanged(index,index);
		return true;
	}
	return false;
}


QVariant GLSceneGroupCollection::data( const QModelIndex & index, int role ) const
{
	if (!index.isValid())
		return QVariant();

	if (index.row() >= rowCount(QModelIndex()))
		return QVariant();

	if (role == Qt::DisplayRole || Qt::EditRole == role)
	{
		GLSceneGroup *d = m_groups.at(index.row());
		QString value = d->groupName().isEmpty() ? QString("Group %1").arg(index.row()+1) : d->groupName();
		return value;

	}
	else if(Qt::DecorationRole == role)
	{
		GLSceneGroup *d = m_groups.at(index.row());
		return d->pixmap();
	}
	else
		return QVariant();
}

// The core of the scene collection is a list of scenes.
// The order is explicit through their index in the QList, though not relevant
// as the order they are played in is specified by the GLSchedule and GLScheduleItems.
// Although the scenes are displayed in order in the 'Director' program
void GLSceneGroupCollection::addGroup(GLSceneGroup* s)
{
	if(!s)
		return;

	// Effect appending
	m_groups << s;
	m_groupIdLookup[s->groupId()] = s;

	connect(s, SIGNAL(groupNameChanged(const QString&)), 	this, SLOT(groupChanged()));
	connect(s, SIGNAL(pixmapChanged(const QPixmap&)), 	this, SLOT(groupChanged()));
	connect(s, SIGNAL(durationChanged(double)), 		this, SLOT(groupChanged()));
	connect(s, SIGNAL(autoDurationChanged(bool)), 		this, SLOT(groupChanged()));
	connect(s, SIGNAL(scheduledTimeChanged(QDateTime)), 	this, SLOT(groupChanged()));
	connect(s, SIGNAL(autoScheduleChanged(bool)), 		this, SLOT(groupChanged()));

	emit groupAdded(s);

	// Notify list view of intent to change list sizes
	beginInsertRows(QModelIndex(),m_groups.size()-1,m_groups.size());

	// Notify QListViews of change in data
	QModelIndex top    = createIndex(m_groups.size()-2, 0),
		    bottom = createIndex(m_groups.size()-1, 0);
	dataChanged(top,bottom);

	endInsertRows();
}

void GLSceneGroupCollection::removeGroup(GLSceneGroup* s)
{
	if(!s)
		return;

	// Notify list view of intent to change list size
	beginRemoveRows(QModelIndex(),0,m_groups.size()+1);

	int idx = m_groups.indexOf(s);

	// Effect removal
	emit groupRemoved(s);
	disconnect(s, 0, this, 0);
	m_groups.removeAll(s);
	m_groupIdLookup.remove(s->groupId());

	// Notify QListViews of change in data
	QModelIndex top    = createIndex(idx, 0),
		    bottom = createIndex(m_groups.size(), 0);
	dataChanged(top,bottom);

	endRemoveRows();
}

GLSceneGroup * GLSceneGroupCollection::lookupGroup(int id)
{
	return m_groupIdLookup[id];
}


void GLSceneGroupCollection::setCollectionName(const QString& name)
{
	m_collectionName = name;
	emit collectionNameChanged(name);
}

void GLSceneGroupCollection::setCanvasSize(const QSizeF& size)
{
	m_canvasSize = size;
	emit canvasSizeChanged(size);
}


void GLSceneGroupCollection::groupChanged()
{
	GLSceneGroup *scene = dynamic_cast<GLSceneGroup*>(sender());
	if(!scene)
		return;
	int row = m_groups.indexOf(scene);
	// Notify QListViews of change in data
	QModelIndex idx = createIndex(row, 0);
	dataChanged(idx, idx);
	
	/// TODO recalc group datetimes if they are auto scheduled based on cumulative duration 
	/// TODO recalc our duration based on group durations IF our duration is 'autoDuration'

}
