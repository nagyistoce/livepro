#ifndef GLSceneGroup_H
#define GLSceneGroup_H

#include <QtGui>

class GLScene;
class GLWidget;
class GLDrawable;

typedef QList<GLDrawable*> GLDrawableList;

class GLSceneLayoutItem : public QObject
{
	Q_OBJECT
public:
	GLSceneLayoutItem(int id=0, const QVariantMap& props = QVariantMap());
	GLSceneLayoutItem(GLDrawable *drawable=0, const QVariantMap& props = QVariantMap());
	GLSceneLayoutItem(QByteArray&);
	virtual ~GLSceneLayoutItem();
	
	int drawableId() { return m_drawableId; }
	QVariantMap propertyHash() { return m_props; }
	
	QByteArray toByteArray();
	void fromByteArray(QByteArray&);
	
public slots:
	void setDrawable(GLDrawable*);
	void setDrawableId(int);
	void setPropertyHash(const QVariantMap&); 
	
private:
	int m_drawableId;
	QVariantMap m_props;
};

class GLSceneLayout : public QObject
{
	Q_OBJECT
	Q_PROPERTY(int layoutId READ layoutId);
	Q_PROPERTY(QString layoutName READ layoutName WRITE setLayoutName);
	
public:
	GLSceneLayout(GLScene *);
	GLSceneLayout(QByteArray&, GLScene*);
	virtual ~GLSceneLayout();
	
	int layoutId() { return m_layoutId; }
	QString layoutName() { return m_layoutName; }
	QPixmap pixmap() { return m_pixmap; }
	
	
	QList<GLSceneLayoutItem*> layoutItems() { return m_items; }
	void addLayoutItem(GLSceneLayoutItem* x) { m_items << x; }
	void removeLayoutItem(GLSceneLayoutItem* x) { m_items.removeAll(x); }
	
	QByteArray toByteArray();
	void fromByteArray(QByteArray&);
	
public slots:
	void setLayoutName(const QString& name);
	void setPixmap(const QPixmap& pix);

signals:
	void layoutNameChanged(const QString& name);
	void pixmapChanged(const QPixmap& pixmap);

private:
	GLScene *m_scene;
	int m_layoutId;
	QString m_layoutName;
	QPixmap m_pixmap;
	QList<GLSceneLayoutItem*> m_items;
};

class GLScene;
class GLSceneLayoutListModel : public QAbstractListModel
{
	Q_OBJECT
public:
	GLSceneLayoutListModel(GLScene*);
	~GLSceneLayoutListModel();
	
	int rowCount(const QModelIndex &/*parent*/) const;
	QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
	
private slots:
	void layoutAdded(GLSceneLayout*);
	void layoutRemoved(GLSceneLayout*);
	
private:
	GLScene *m_scene;

};

class GLSceneType;
class GLSceneGroup;
class GLScene : public QAbstractListModel
{
	Q_OBJECT
	Q_PROPERTY(int sceneId READ sceneId);
	Q_PROPERTY(QString sceneName READ sceneName WRITE setSceneName);
	
	Q_PROPERTY(double opacity READ opacity WRITE setOpacity);
	
	Q_PROPERTY(double duration READ duration WRITE setDuration);
	Q_PROPERTY(bool autoDuration READ autoDuration WRITE setAutoDuration);
	
	Q_PROPERTY(QDateTime scheduledTime READ scheduledTime WRITE setScheduledTime);
	Q_PROPERTY(bool autoSchedule READ autoSchedule WRITE setAutoSchedule);

	
	Q_PROPERTY(GLSceneType* sceneType READ sceneType WRITE setSceneType);
	
public:
	GLScene(QObject *parent=0);
	GLScene(QByteArray&, QObject *parent=0);
	~GLScene();
	
	int sceneId();
	QString sceneName() { return m_sceneName; }
	QPixmap pixmap() { return m_pixmap; }
	
	QByteArray toByteArray();
	void fromByteArray(QByteArray&);
	GLScene *clone(); 
	
	GLDrawableList drawableList() { return m_itemList; }
	void addDrawable(GLDrawable*);
	void removeDrawable(GLDrawable*);
	
	GLDrawable * lookupDrawable(int id);
	GLDrawable * lookupDrawable(const QString& name);
	
	int size() const;
	GLDrawable * at(int idx);
	
	// QAbstractListModel
	virtual int rowCount(const QModelIndex &/*parent*/) const;
	virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
	
	/// This is the 'crossover' method which
	// connects the drawables in this scene to an actual display widget.
	// Only one widget can be set at a time - if the widget is changed,
	// the drawables are removed from the old and added to the new
	void setGLWidget(GLWidget*, int zIndexOffset=0);
	/// Detech from the current glWidget(), if any.
	void detachGLWidget();
	/// Returns the currently attached GLWidget.
	GLWidget *glWidget() { return m_glWidget; }
	
	void setSceneGroup(GLSceneGroup *group);
	GLSceneGroup *sceneGroup() { return m_group; }
	
	// Layouts specify properties of the drawbles in the scene
	QList<GLSceneLayout*> layouts() { return m_layouts; }
	void addLayout(GLSceneLayout*);
	void removeLayout(GLSceneLayout*);
	
	GLSceneLayoutListModel *layoutListModel();
	
	GLSceneLayout * lookupLayout(int id);
	
	bool listOnlyUserItems() { return m_listOnlyUserItems; }
	
	void setGraphicsScene(QGraphicsScene *);
	QGraphicsScene *graphicsScene();
	
	double opacity() { return m_opacity; }
	double zIndex() { return m_zIndex; }
	
	double duration() { return m_duration; }
	bool autoDuration() { return m_autoDuration; }
	
	QDateTime scheduledTime() { return m_scheduledTime; }
	bool autoSchedule() { return m_autoSchedule; }
	
	GLSceneType *sceneType() { return m_sceneType; }

	bool fadeActive() { return m_fadeActive; }
	
	GLDrawable *rootObj() { return m_rootObj; }

public slots:
	void setSceneName(const QString& name);
	void setListOnlyUserItems(bool);
	void setPixmap(const QPixmap&);
	
	void setZIndex(double zIndex);
	void setOpacity(double opacity, bool animate=false, double animDuration=750);
	
	void setDuration(double duration);
	void setAutoDuration(bool flag);
	
	void setScheduledTime(const QDateTime&);
	void setAutoSchedule(bool flag);
	
	void setSceneType(GLSceneType *);
	
signals:
	void drawableAdded(GLDrawable*);
	void drawableRemoved(GLDrawable*);
	void layoutAdded(GLSceneLayout*);
	void layoutRemoved(GLSceneLayout*);
	
	void sceneNameChanged(const QString&);
	void pixmapChanged(const QPixmap&);
	
	void opacityChanged(double d);
	void zIndexChanged(double d);
	void opacityAnimationFinished();
	
	void durationChanged(double);
	void autoDurationChanged(bool);
	
	void scheduledTimeChanged(QDateTime);
	void autoScheduleChanged(bool);
	
private slots:
	void drawableDestroyed();
	void drawableNameChanging(QString);
	void drawablePlaylistItemChanged();
	void fadeTick();
	
protected:
	friend class GLVideoDrawable;
	void recalcFadeOpacity(bool setOpac=true);

	double calcDuration();
	
	friend class GLSceneLayoutListModel;
	
	int m_sceneId;
	QString m_sceneName;
	QPixmap m_pixmap;
	
	double m_opacity;
	double m_zIndex;
	
	GLDrawableList m_itemList;
	GLDrawableList m_userItemList;
	bool m_listOnlyUserItems;
	QHash<int,GLDrawable*> m_drawableIdLookup;
	QHash<QString,GLDrawable*> m_drawableNameLookup;
	
	QList<GLSceneLayout*> m_layouts;
	QHash<int,GLSceneLayout*> m_layoutIdLookup;
		
	GLWidget *m_glWidget;
	GLSceneLayoutListModel *m_layoutListModel;
	
	QGraphicsScene *m_graphicsScene;
	
	QTimer m_fadeTimer;
	QTime m_fadeClock;
	bool m_fadeClockActive;
	int m_fadeDirection;
	double m_endOpacity;
	double m_startOpacity;
	bool m_fadeActive;
	
	int m_crossfadeSpeed;
	
	double m_duration;
	bool m_autoDuration;
	
	QDateTime m_scheduledTime;
	bool m_autoSchedule;
	
	GLSceneType *m_sceneType;
	GLSceneGroup *m_group;
	
	GLDrawable *m_rootObj;
};

class GLSceneGroupPlaylist;
class GLSceneGroupType;
class GLSceneGroup : public QAbstractItemModel
{
	Q_OBJECT
	
	Q_PROPERTY(int groupId READ groupId);
	Q_PROPERTY(QString groupName READ groupName WRITE setGroupName);
	
	Q_PROPERTY(GLSceneGroupType* groupType READ groupType WRITE setGroupType);
	
public:
	GLSceneGroup(QObject *parent=0);
	GLSceneGroup(QByteArray&, QObject *parent=0);
	~GLSceneGroup();
	
	int groupId();
	QString groupName() { return m_groupName; }
	QPixmap pixmap() { return m_pixmap; }
	
	QByteArray toByteArray();
	void fromByteArray(QByteArray&);
	
	// The core of the scene group is a list of scenes.
	// The order is explicit through their index in the QList, though not relevant
	// as the order they are played in is specified by the GLSchedule and GLScheduleItems.
	// Although the scenes are displayed in order in the 'Director' program
	QList<GLScene*> sceneList() { return m_scenes; }
	void addScene(GLScene*);
	void removeScene(GLScene*);
	
	GLScene * lookupScene(int id);
	
	bool isEmpty() const { return size() <= 0; }
	int size() const { return m_scenes.size(); }
	GLScene * at(int idx) { return idx>-1 && idx<size() ? m_scenes[idx] : 0; }
	
	// QAbstractListModel
	virtual int rowCount(const QModelIndex &/*parent*/) const;
	virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	virtual bool setData(const QModelIndex &index, const QVariant & value, int role) ;
	
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex&) const;
	int columnCount(const QModelIndex&) const;

	// Overlay scene, by definition, is a general scene that is to be overlayed on the content of 
	// the other scenes in the list. 
	GLScene * overlayScene() { return m_overlayScene; }
	void setOverlayScene(GLScene*);
	
	double duration() { return m_duration; }
	bool autoDuration() { return m_autoDuration; }
	
	QDateTime scheduledTime() { return m_scheduledTime; }
	bool autoSchedule() { return m_autoSchedule; }
	
	GLSceneGroupType *groupType() { return m_groupType; }	
	
	GLSceneGroupPlaylist *playlist() { return m_playlist; }

public slots:
	void setGroupName(const QString& name);
	void setPixmap(const QPixmap&);
	
	void setGroupType(GLSceneGroupType*);
	
	void setDuration(double duration);
	void setAutoDuration(bool flag);
	
	void setScheduledTime(const QDateTime&);
	void setAutoSchedule(bool flag);
	
private slots:
	void sceneChanged();
	
signals:
	void sceneAdded(GLScene*);
	void sceneRemoved(GLScene*);
	
	void groupNameChanged(const QString&);
	void overlaySceneChanged(GLScene*);
	void pixmapChanged(const QPixmap&);
	
	void durationChanged(double);
	void autoDurationChanged(bool);
	
	void scheduledTimeChanged(QDateTime);
	void autoScheduleChanged(bool);
	
	void sceneDataChanged();

protected:
	int m_groupId;
	QString m_groupName;
	QPixmap m_pixmap;
	
	QList<GLScene*> m_scenes;
	QHash<int,GLScene*> m_sceneIdLookup;

	GLScene *m_overlayScene;
	
	double m_duration;
	bool m_autoDuration;
	
	QDateTime m_scheduledTime;
	bool m_autoSchedule;
	
	GLSceneGroupType *m_groupType;
	GLSceneGroupPlaylist *m_playlist;
};

class GLSceneGroupPlaylist : public QObject
{
	Q_OBJECT
public:
	GLSceneGroupPlaylist(GLSceneGroup *group);
	~GLSceneGroupPlaylist();
	
	bool isPlaying() { return m_isPlaying; }
	double playTime() { return m_playTime; }
	
	GLSceneGroup *group() { return m_group; } 
	GLScene *currentItem();
	
	double duration();
	
	double timeFor(GLScene*);
	
	bool isRandom() { return m_isRandom; }
	
signals:
	void currentItemChanged(GLScene*);
	void timeChanged(double);
	
	void playlistItemChanged();
	
public slots:
	void setIsPlaying(bool flag);
	void play(bool restart=false);
	void stop();
	
	void playItem(GLScene*);
	
	/** Notifies the playlist that the current item has been changed, likely by the user. The playlist will start
	    a new timer for this item and stop any current timers. 
	    Neither the currentItemChanged() signal nor the timeChanged() signal are emitted by this method. */
	void setCurrentItem(GLScene*);
	bool setPlayTime(double time);
	
	void nextItem();
	void prevItem();
	
	void setIsRandom(bool random=true);
	
	void sceneDurationChanged(double);

private slots:
// 	void sceneAdded(GLScene*);
// 	void sceneRemoved(GLScene*);
	
// 	void playlistItemChangedSlot();
// 	void timerTick();

private:
	GLSceneGroup *m_group;
	
	bool m_isPlaying;
	QTimer m_currentItemTimer;
	double m_playTime;
	int m_currentItemIndex;
	bool m_isRandom;
	
	QTime m_currentElapsedTime;
	GLScene *m_currentItem;
};

class GLSceneGroupCollection : public QAbstractListModel
{
	Q_OBJECT
	
	Q_PROPERTY(int collectionId READ collectionId);
	Q_PROPERTY(QString collectionName READ collectionName WRITE setCollectionName);
	Q_PROPERTY(QSizeF canvasSize READ canvasSize WRITE setCanvasSize);
	
public:
	GLSceneGroupCollection(QObject *parent=0);
	GLSceneGroupCollection(QByteArray&, QObject *parent=0);
	GLSceneGroupCollection(const QString& file, QObject *parent=0);
	~GLSceneGroupCollection();
	
	int collectionId();
	QString collectionName() { return m_collectionName; }
	
	QSizeF canvasSize() { return m_canvasSize; }
	
	QByteArray toByteArray();
	void fromByteArray(QByteArray&);
	
	bool writeFile(const QString& name="");
	bool readFile(const QString& name);
	
	QString fileName() { return m_fileName; }
	
	// The core of the scene group is a list of scenes.
	// The order is explicit through their index in the QList, though not relevant
	// as the order they are played in is specified by the GLSchedule and GLScheduleItems.
	// Although the scenes are displayed in order in the 'Director' program
	QList<GLSceneGroup*> groupList() { return m_groups; }
	void addGroup(GLSceneGroup*);
	void removeGroup(GLSceneGroup*);
	
	GLSceneGroup * lookupGroup(int id);
	
	int size() const { return m_groups.size(); }
	GLSceneGroup * at(int idx) { return idx>-1 && idx<size() ? m_groups[idx] : 0; }
	
	// QAbstractListModel
	virtual int rowCount(const QModelIndex &/*parent*/) const { return size(); }
	virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	virtual bool setData(const QModelIndex &index, const QVariant & value, int role) ;
		
public slots:
	void setCollectionName(const QString& name);
	void setFileName(const QString& name) { m_fileName = name; }
	void setCanvasSize(const QSizeF&);

private slots:
	void groupChanged();

signals:
	void groupAdded(GLSceneGroup*);
	void groupRemoved(GLSceneGroup*);
	
	void collectionNameChanged(const QString&);
	void canvasSizeChanged(const QSizeF&);

protected:
	int m_collectionId;
	QString m_collectionName;
	QString m_fileName;
	
	QSizeF m_canvasSize;
	
	QList<GLSceneGroup*> m_groups;
	QHash<int,GLSceneGroup*> m_groupIdLookup;
};


#endif
