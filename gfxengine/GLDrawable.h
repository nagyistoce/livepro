#ifndef GLDRAWABLE_H
#define GLDRAWABLE_H

#include <QtGui>

//#include "CornerItem.h"
class CornerItem;

class QPropertyAnimation;
class GLWidget;
class GLEditorGraphicsScene;
class GLScene;
class QGLFramebufferObject;

// GLDrawable uses Z_MAX combined with the zIndexModifier to scale the zIndex to where zIndexModifier=<z<zIndexModifier
#define Z_MAX 10000

// This class exists because we want to be able to call start()
// from a slot AND have the animation automatically delete when done.
// However, since we cant pass hardcoded arguments thru a slot, AND
// connecting the animation's finished() signal to deleteLater()
// causes a SEGFLT, then this is the only way I can think of.
class QAutoDelPropertyAnimation : public QPropertyAnimation
{
	Q_OBJECT

public:
	QAutoDelPropertyAnimation(QObject * target, const QByteArray & propertyName, QObject * parent = 0) ;

public slots:
	void resetProperty();

protected:
	QVariant m_originalPropValue;
};

class QAbsoluteTimeAnimation : public QAutoDelPropertyAnimation
{
	Q_OBJECT
public:
	QAbsoluteTimeAnimation(QObject * target, const QByteArray & propertyName, QObject * parent = 0);
	
public slots:
	void updateTime();
	void start(QAbstractAnimation::DeletionPolicy policy = QAbstractAnimation::KeepWhenStopped);
	
protected:
	QTime m_timeElapsed;
};
typedef QPointer<QAbsoluteTimeAnimation> QAbsoluteTimeAnimationPtr;

class GLDrawablePlaylist;
class GLDrawable : public QObject,
		   public QGraphicsItem
{
	Q_OBJECT
	Q_PROPERTY(int id READ id);
	Q_PROPERTY(bool userControllable READ isUserControllable WRITE setUserControllable);
	Q_PROPERTY(QString itemName READ itemName WRITE setItemName);
	
	Q_PROPERTY(QRectF rect READ rect WRITE setRect);
	Q_PROPERTY(double zIndex READ zIndex WRITE setZIndex);
	Q_PROPERTY(double opacity READ opacity WRITE setOpacity);
	Q_PROPERTY(double isVisible READ isVisible WRITE setVisible);
	
	Q_PROPERTY(QSizeF size READ size WRITE setSize);
	Q_PROPERTY(QPointF position READ position WRITE setPosition);

// 	Q_PROPERTY(bool showFullScreen READ showFullScreen WRITE setShowFullScreen);
	Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment);
	Q_PROPERTY(QPointF insetTopLeft READ insetTopLeft WRITE setInsetTopLeft);
	Q_PROPERTY(QPointF insetBottomRight READ insetBottomRight WRITE setInsetBottomRight);
	Q_PROPERTY(double alignedSizeScale READ alignedSizeScale WRITE setAlignedSizeScale);
	
	Q_PROPERTY(QVector3D translation READ translation WRITE setTranslation);
	Q_PROPERTY(QVector3D rotation READ rotation WRITE setRotation);
	// Expressed in a range of 0..1 as a percentage of the relevant size (e.g. (.5,.5,.5) means rotate around center, the default value)
	Q_PROPERTY(QVector3D rotationPoint READ rotationPoint WRITE setRotationPoint);
	
	// This is the usable area for arranging the drawable - if isNull, then it uses the GLWidget's canvas size
	Q_PROPERTY(QSizeF canvasSize READ canvasSize WRITE setCanvasSize);
	
	Q_PROPERTY(bool fadeIn READ fadeIn WRITE setFadeIn);
	Q_PROPERTY(bool fadeOut READ fadeOut WRITE setFadeOut);
	
	Q_PROPERTY(int fadeInLength READ fadeInLength WRITE setFadeInLength);
	Q_PROPERTY(int fadeOutLength READ fadeOutLength WRITE setFadeOutLength);

	Q_ENUMS(AnimationType);

public:
	GLDrawable(QObject *parent=0);
	~GLDrawable();
	
	int id();
	QString itemName() { return m_itemName; }
	bool isUserControllable() { return m_isUserControllable; }

	GLWidget *glWidget() { return m_glw; }

	// Compat with QGraphgicsItem: boundingRect()
	QRectF boundingRect() const;// { return m_rect; }
	
	const QRectF & rect() const { return m_rect; }
	double zIndex();
	
	double zIndexModifier();

	double opacity();

	enum AnimationType
	{
		AnimNone = 0,

		AnimFade,
		AnimZoom,
		AnimSlideTop,
		AnimSlideBottom,
		AnimSlideLeft,
		AnimSlideRight,

		AnimUser = 1000
	};
	
	enum AnimCondition
	{
		OnHide,
		OnShow,
	};

	struct AnimParam
	{
		AnimCondition cond;
		AnimationType type;
		int startDelay;
		int length;
		QEasingCurve curve;

		QByteArray toByteArray();
		void fromByteArray(QByteArray);
	};

	void addAnimation(AnimParam);
	void removeAnimation(AnimParam);

	AnimParam & addShowAnimation(AnimationType type, int length=500);
	AnimParam & addHideAnimation(AnimationType type, int length=500);

	void resetAllAnimations();

	bool isVisible() { return m_isVisible; }

	bool animationsEnabled() { return m_animationsEnabled; }

// 	bool showFullScreen()		{ return m_showFullScreen; }
	Qt::Alignment alignment()	{ return m_alignment; }
	QPointF insetTopLeft()		{ return m_insetTopLeft; }
	QPointF insetBottomRight()	{ return m_insetBottomRight; }

	// This is the normal size of the content in pixels - independent of the specified rect().
	// E.g. if its an image, this is the size of the image, if this is text, then the size
	// of the text unscaled at natural resolution. Used for calculating alignment.
	virtual QSizeF naturalSize() { return QSizeF(0,0); }

	// If the drawable is aligned to a corner or centered, the size isn't specified by the user - 
	// so they can use this property to "scale" the size of the object.
	double alignedSizeScale() { return m_alignedSizeScale; }
	
	// 3d translations that can be applied to the coordinates before redering
	QVector3D translation() { return m_translation; }
	QVector3D rotation() { return m_rotation; }
	QVector3D rotationPoint() { return m_rotationPoint; }
	
	// If m_canvasSize isNull, returns the GLWidget's canvas size
	QSizeF canvasSize();
	
	virtual void fromByteArray(QByteArray&);
	virtual QByteArray toByteArray();
	
	virtual void loadPropsFromMap(const QVariantMap&, bool onlyApplyIfChanged = false);
	virtual QVariantMap propsToMap();
	
	bool isSelected() { return m_selected; }
	
	bool fadeIn() { return m_fadeIn; }
	bool fadeOut() { return m_fadeOut; }
	
	int fadeInLength() { return m_fadeInLength; }
	int fadeOutLength() { return m_fadeOutLength; }
	
	GLDrawablePlaylist *playlist() { return m_playlist; }
	
	GLScene *glScene() { return m_scene; }
	void setGLScene(GLScene *scene);
	  
	QSizeF size() { return rect().size(); }
	QPointF position() { return rect().topLeft(); }
	
	void registerAbsoluteTimeAnimation(QAbsoluteTimeAnimation*);
	//QList<QAbsoluteTimeAnimation*> absoluteTimeAnimations() { return m_absoluteTimeAnimations; }
	
	bool updatesLocked() { return m_updatesLocked; }
	
	QList<GLDrawable*> children() { return m_children; }
	
	QRectF coverageRect() { return m_coverageRect; }
	
	GLDrawable *parent() { return m_parent; }
	
	bool hasFrameBuffer() { return m_frameBuffer!= NULL && m_enableBuffering; }
	
	
public slots:
	bool lockUpdates(bool flag);
	void updateGL(bool now=false);
	void updateAbsoluteTimeAnimations();
	
	void setItemName(const QString&);
	void setUserControllable(bool);

	void setPosition(const QPointF& val) { setRect(QRectF(val, size())); }
	void setSize(const QSizeF& size) { setRect(QRectF(position(), size)); }
	void setRect(const QRectF& rect);
	void moveBy(double x, double y) { setRect(m_rect.translated(x,y)); }
	void moveBy(const QPointF& pnt) { setRect(m_rect.translated(pnt)); }
	void setOpacity(double i);
	void setOpacity(int o) { setOpacity(((double)o)/100.0); }
	void setZIndex(double z);
	void setZIndex(int z) { setZIndex((double)z); } // for sig/slot compat
	void setZIndexModifier(double mod);

	void show();
	void hide();
	void setVisible(bool flag);
	void setHidden(bool flag);

	bool setAnimationsEnabled(bool);

// 	void setShowFullScreen(bool flag, bool animate=false, int animLength=300, QEasingCurve animCurve = QEasingCurve::Linear);
	void setAlignment(Qt::Alignment value, bool animate=false, int animLength=300, QEasingCurve animCurve = QEasingCurve::Linear);
	void setAlignment(int value);
	void setInsetTopLeft(const QPointF& value);
	void setInsetBottomRight(const QPointF& value);

	void setAlignedSizeScale(double);

	void setTranslation(QVector3D value);
	void setRotation(QVector3D value);
	// Expressed in a range of 0..1 as a percentage of the relevant size (e.g. (.5,.5,.5) means rotate around center, the default value)
	void setRotationPoint(QVector3D value);
	
	void setCanvasSize(const QSizeF&);
	
	//void setSelected(bool selected=true);
	
	void setFadeIn(bool);
	void setFadeOut(bool);
	
	void setFadeInLength(int);
	void setFadeOutLength(int);
	
	void addChild(GLDrawable*);
	void removeChild(GLDrawable*);
	
	
signals:
	void sizeChanged(const QSizeF&);
	void positionChanged(const QPointF&);
	void rectChanged(const QRectF&);
	void zIndexChanged(double newZIndex);
	void drawableResized(const QSize& newSize);

	void isVisible(bool);
	
	void propertyChanged(const QString& propName, const QVariant& value);
	
	// emitted right before the name is changed
	void itemNameChanging(const QString& name);
	
	void childAdded(GLDrawable*);
	void chilRemoved(GLDrawable*);

protected slots:
	friend class GLEditorGraphicsScene;
	
	virtual void animationFinished();
	
	virtual void absoluteTimeAnimationFinished();
	
	// remove it from m_propAnims
	void propAnimFinished();
	
	void editingModeChanged(bool flag);
	
	void childZIndexChanged();
	void sortChildren();
	void childRectChanged();
	void calcCoverageRect();

protected:
	void updateAnimValues();
	
	virtual void updateAlignment(bool animateRect=false, int animLength=300, QEasingCurve animCurve = QEasingCurve::Linear);

	friend class GLWidget;
	virtual void setGLWidget(GLWidget*); // when update is called, it calls GLWidget::updateGL()

	virtual void viewportResized(const QSize& newSize);
	virtual void canvasResized(const QSizeF& newSize);
	virtual void drawableResized(const QSizeF& newSize);
	virtual void drawableMoved(const QPointF& newPoint);
	virtual void drawableRectChanged(const QRectF&);

	virtual void paintGL();
	virtual void initGL();
	
	virtual void paintGLChildren(bool under = false);
	virtual void paintChildren(bool under, QPainter *painter, const QStyleOptionGraphicsItem * option, QWidget * widget);
	
	// For compat with QGraphicsItem
	virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget);
	// QGraphicsItem::
// 	virtual void mousePressEvent(QGraphicsSceneMouseEvent * event);
// 	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);
// 	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent * event);
	virtual QVariant itemChange(GraphicsItemChange change, const QVariant & value);
	
	// For corner items
	void createCorner(int corner, bool noRescale);
	void layoutChildren();
	void setControlsVisible(bool visible);
	
	/** If true, scaling (dragging corners in the editor) is not restricted to the current aspect ratio. Default is false - scaling is aspect-ratio limited */
	void setFreeScaling(bool flag);


	void propertyWasChanged(const QString& propName, const QVariant& value);
	
	virtual void startAnimation(const AnimParam & p);
	virtual void startAnimations();
	void forceStopAnimations();
	
	QPropertyAnimation *propAnim(const QString&);
	void registerPropAnim(const QString& prop, QPropertyAnimation *anim);
	
	
	GLWidget 	   *m_glw;
	QList<CornerItem *> m_cornerItems;
	bool                m_controlsVisible;
	
	QList<QAbsoluteTimeAnimationPtr> m_absoluteTimeAnimations;
	
	void setParent(GLDrawable*);

	QGLFramebufferObject *m_frameBuffer;
	bool m_enableBuffering;

private:
	QAbsoluteTimeAnimation* setupRectAnimation(const QRectF& other, bool animateIn);

	QRectF m_rect;
	double m_zIndex;
	double m_zIndexModifier;
	double m_opacity;

	bool m_isVisible;
	bool m_animDirection;
	bool m_animFinished;
	QRectF m_originalRect;
	double m_originalOpacity;

	QList<GLDrawable::AnimParam> m_animations;
	QList<QAbsoluteTimeAnimation*> m_runningAnimations;
	QList<QAbsoluteTimeAnimation*> m_finishedAnimations;

	bool m_animationsEnabled;

// 	bool m_showFullScreen;
	Qt::Alignment m_alignment;
	QPointF m_insetTopLeft;
	QPointF m_insetBottomRight;

	bool m_inAlignment;

	double m_alignedSizeScale;
	
	bool m_animPendingGlWidget;
	bool m_alignmentPending;

	QVector3D m_translation;
	QVector3D m_rotation;
	QVector3D m_rotationPoint;
	
	QSizeF m_canvasSize;
	
	// Used by code to prevent two animations from running on same prop at same time
	QHash<QString,QPropertyAnimation*> m_propAnims;
	
	bool m_lockVisibleSetter;
	
	int m_id;
	bool m_idLoaded;
	//bool m_lockPropertyUpdates;
	
	QString m_itemName;
	bool m_isUserControllable;
	
	bool m_selected;
	
	bool m_fadeIn;
	bool m_fadeOut;
	int m_fadeInLength;
	int m_fadeOutLength;
	
	GLDrawablePlaylist *m_playlist;
	
	GLScene *m_scene;
	
	bool m_updatesLocked;
	
	QList<GLDrawable*> m_children;
	bool m_glInited;
	
	QRectF m_coverageRect;
	bool m_lockCalcCoverage;
	
	GLDrawable *m_parent;
	
	bool m_lockSetRect;
};

bool operator==(const GLDrawable::AnimParam&a, const GLDrawable::AnimParam&b);
Q_DECLARE_METATYPE(GLDrawable::AnimationType);



class GLDrawablePlaylist;
class GLPlaylistItem : public QObject
{
	Q_OBJECT
	
	Q_PROPERTY(QString id READ id);
	Q_PROPERTY(QString title READ title WRITE setTitle);
	Q_PROPERTY(QVariant value READ value WRITE setValue);
	
	Q_PROPERTY(double duration READ duration WRITE setDuration);
	Q_PROPERTY(bool autoDuration READ autoDuration WRITE setAutoDuration);
	
	Q_PROPERTY(QDateTime scheduledTime READ scheduledTime WRITE setScheduledTime);
	Q_PROPERTY(bool autoSchedule READ autoSchedule WRITE setAutoSchedule);
public:
	GLPlaylistItem(GLDrawablePlaylist *list=0);
	GLPlaylistItem(QByteArray& array, GLDrawablePlaylist *list=0);
	
	QByteArray toByteArray();
	void fromByteArray(QByteArray&);
	
	GLDrawablePlaylist *playlist() { return m_playlist; }
	
	int id(); 
	
	QString title() { return m_title; }
	QVariant value() { return m_value; }
	double duration() { return m_duration; }
	bool autoDuration() { return m_autoDuration; }
	
	QDateTime scheduledTime() { return m_scheduledTime; }
	bool autoSchedule() { return m_autoSchedule; }
	
public slots:
	void setTitle(const QString& title);
	void setValue(QVariant value);
	void setDuration(double duration);
	void setAutoDuration(bool flag);
	void setScheduledTime(const QDateTime&);
	void setAutoSchedule(bool flag);
	
	void setPlaylist(GLDrawablePlaylist *playlist);
	
signals:
	void playlistItemChanged();

private:
	GLDrawablePlaylist *m_playlist;
	
	int m_id;
	
	QString m_title;
	QVariant m_value;
	double m_duration;
	bool m_autoDuration;
	QDateTime m_scheduledTime;
	bool m_autoSchedule;
};


class GLDrawablePlaylist : public QAbstractItemModel
{
	Q_OBJECT

protected:
	friend class GLDrawable;
	GLDrawablePlaylist(GLDrawable *drawable);
	
public:
	~GLDrawablePlaylist();
	
	int rowCount(const QModelIndex &/*parent*/) const;
	QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const; 
	Qt::ItemFlags flags(const QModelIndex &index) const;
	bool setData(const QModelIndex &index, const QVariant & value, int role);
	QModelIndex indexOf(GLPlaylistItem*);
	
	QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex&) const;
	int columnCount(const QModelIndex&) const;

	QByteArray toByteArray();
	void fromByteArray(QByteArray&);
	
	QList<GLPlaylistItem*> items() { return m_items; }
	
	int rowOf(GLPlaylistItem *item) { return m_items.indexOf(item); }
	int size() { return m_items.size(); }
	GLPlaylistItem *at(int x) { return x>=0 && x<m_items.size()?m_items.at(x):0; }
	GLPlaylistItem *lookup(int id) { return m_itemLookup[id]; }
	
	GLDrawable *drawable() { return m_drawable; }

	bool isPlaying() { return m_isPlaying; }
	double playTime() { return m_playTime; }
	
	GLPlaylistItem *currentItem();
	
	double duration();
	
	double timeFor(GLPlaylistItem*);
	
	// durationProperty is the prop on the drawable to read inorder to determine the
	// automatic duration of the current item. 
	void setDurationProperty(const QString&);
	QString durationProperty() { return m_durationProperty; }

public slots:
	void addItem(GLPlaylistItem *, GLPlaylistItem *insertAfter=0);
	void removeItem(GLPlaylistItem *);
	
	void setIsPlaying(bool flag);
	void play(bool restart=false);
	void stop();
	
	void playItem(GLPlaylistItem*);
	bool setPlayTime(double time);
	
	void nextItem();
	void prevItem();
		
signals:
	void itemAdded(GLPlaylistItem*);
	void itemRemoved(GLPlaylistItem*);
	
	void currentItemChanged(GLPlaylistItem*);
	void playerTimeChanged(double);
	
	void playlistItemChanged();
	
private slots:
	void playlistItemChangedSlot();
	void timerTick();
	
private:
	QList<GLPlaylistItem *> m_items;
	QHash<int, GLPlaylistItem *> m_itemLookup; 
	GLDrawable *m_drawable;
	
	bool m_isPlaying;
	QTimer m_tickTimer;
	double m_playTime;
	double m_timerTickLength;
	int m_currentItemIndex;
	
	QString m_durationProperty;
	

};


#endif
