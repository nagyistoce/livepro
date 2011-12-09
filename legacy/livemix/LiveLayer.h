#ifndef LiveLayer_H
#define LiveLayer_H

#include <QtGui>

#include "../glvidtex/GLDrawable.h"
class GLWidget;

class PositionWidget : public QWidget
{
	Q_OBJECT
public:
	PositionWidget(class LiveLayer *layer);

signals:
// 	void 

private slots:
	void setLayerY(double);
	void setLayerX(double);
	
	void setLayerWidth(double);
	void setLayerHeight(double);
	
	void setARLeft(int);
	void setARRight(int);
	
	void layerPropertyChanged(const QString& propertyId, const QVariant& value, const QVariant& oldValue);
	
	void setAspectRatioIdx(int);
	
private:
	double heightFromWidth(double);
	double widthFromHeight(double);
	
	LiveLayer *m_layer;
	bool m_lockAspectRatio;
	double m_lockToAR;
	
	QSpinBox *m_arLeft;
	QSpinBox *m_arRight;
	QDoubleSpinBox *m_posY;
	QDoubleSpinBox *m_posX;
	QDoubleSpinBox *m_sizeWidth;
	QDoubleSpinBox *m_sizeHeight;
	
	QList<QPoint> m_arList;
	
	bool m_lockValueUpdates;
};


class LiveLayer : public QObject
{
	Q_OBJECT

	Q_PROPERTY(int id READ id);
	
	Q_PROPERTY(bool userControllable READ isUserControllable WRITE setUserControllable);
	Q_PROPERTY(QString layerName READ layerName WRITE setLayerName);
	
	Q_PROPERTY(bool isVisible READ isVisible WRITE setVisible);
	Q_PROPERTY(QRectF rect READ rect WRITE setRect);
	Q_PROPERTY(double zIndex READ zIndex WRITE setZIndex);
	Q_PROPERTY(double opacity READ opacity WRITE setOpacity);
	
// 	Q_PROPERTY(double topPercent READ topPercent WRITE setTopPercent);
// 	Q_PROPERTY(double leftPercent READ leftPercent WRITE setLeftPercent);
// 	Q_PROPERTY(double bottomPercent READ bottomPercent WRITE setBottomPercent);
// 	Q_PROPERTY(double rightPercent READ rightPercent WRITE setRightPercent);

	Q_PROPERTY(bool fadeIn READ fadeIn WRITE setFadeIn);
	Q_PROPERTY(int fadeInLength READ fadeInLength WRITE setFadeInLength);

	Q_PROPERTY(bool fadeOut READ fadeOut WRITE setFadeOut);
	Q_PROPERTY(int  fadeOutLength READ fadeOutLength WRITE setFadeOutLength);

	Q_PROPERTY(int showAnimationType READ showAnimationType WRITE setShowAnimationType);
	Q_PROPERTY(int showAnimationLength READ showAnimationLength WRITE setShowAnimationLength);
	Q_PROPERTY(QEasingCurve::Type showAnimationCurve READ showAnimationCurve WRITE setShowAnimationCurve);

	Q_PROPERTY(int hideAnimationType READ hideAnimationType WRITE setHideAnimationType);
	Q_PROPERTY(int hideAnimationLength READ hideAnimationLength WRITE setHideAnimationLength);
	Q_PROPERTY(QEasingCurve::Type hideAnimationCurve READ hideAnimationCurve WRITE setHideAnimationCurve);

// 	Q_PROPERTY(bool showFullScreen READ showFullScreen WRITE setShowFullScreen);
	Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment);
	Q_PROPERTY(QPointF insetTopLeft READ insetTopLeft WRITE setInsetTopLeft);
	Q_PROPERTY(QPointF insetBottomRight READ insetBottomRight WRITE setInsetBottomRight);

	Q_PROPERTY(double alignedSizeScale READ alignedSizeScale WRITE setAlignedSizeScale);
	
	Q_PROPERTY(QVector3D rotation READ rotation WRITE setRotation);
	
	Q_PROPERTY(int hideOnShowLayerId READ hideOnShowLayerId WRITE setHideOnShowLayerId);
	Q_PROPERTY(int showOnShowLayerId READ showOnShowLayerId WRITE setShowOnShowLayerId);
	
	Q_PROPERTY(QPointF desiredAspectRatio READ desiredAspectRatio WRITE setDesiredAspectRatio);

public:
	LiveLayer(QObject *parent=0);
	virtual ~LiveLayer();
	
	int id();
	
	virtual void attachGLWidget(GLWidget*);
	virtual void detachGLWidget(GLWidget*);
	
	// Override and return a descritive static type name for your layer
	virtual QString typeName() { return "Generic Layer"; }
	// Use "setInstanceName to change the name here, which emits instanceNameChanged()
	virtual QString instanceName() { return m_instanceName; }
	
	// Returns the user-assigned layer name, if any
	virtual QString layerName() { return m_layerName; }
	
	virtual bool isUserControllable() { return m_isUserControllable; }

	// Query for layer properties
	virtual QVariantMap layerProperties() { return m_props; }
	virtual QVariant layerProperty(const QString& id) { return m_props[id]; }
	
	// Default impl iterates thru m_props and sets up appropriate editors
	// Caller takes ownership of widget and deletes when done
	virtual QWidget *createLayerPropertyEditors();
	
	virtual void fromByteArray(QByteArray&);
	virtual QByteArray toByteArray();
	
	virtual void loadPropsFromMap(const QVariantMap&, bool onlyApplyIfChanged = false);
	virtual QVariantMap propsToMap();
	
	virtual bool canAnimateProperty(const QString&);
	
	class AnimParam
	{
	public:
		AnimParam()
			: length(300)
			, curve(QEasingCurve::Linear)
			{}
		
		int length;
		QEasingCurve curve;
	};
	
	// Set/get the currently effective animation parameters used
	// for transitioning from one property to another
	void setAnimParam(const AnimParam&);
	const AnimParam & animParam() { return m_animParam; }
	
	// Enable/disable animations. By default, animations are enabled.
	// Disable for setting properties in bulk or loading props
	bool setAnimEnabled(bool);
	bool animEnabled() { return !m_animationsDisabled; }
		

	// Translated from a perl function I wrote to do basically
	// the same thing for an ERP project a few years back.
	static QString guessTitle(QString field);

	bool isVisible() 		{ return m_isVisible; }
	QRectF rect()			{ return layerProperty("rect").toRectF(); }
	double zIndex()			{ return layerProperty("zIndex").toDouble(); }
	double opacity()		{ return layerProperty("opacity").toDouble(); }
	
// 	double topPercent()		{ return layerProperty("topPercent").toDouble(); }
// 	double leftPercent()		{ return layerProperty("leftPercent").toDouble(); }
// 	double bottomPercent()		{ return layerProperty("bottomPercent").toDouble(); }
// 	double rightPercent()		{ return layerProperty("rightPercent").toDouble(); }


	bool fadeIn() 			{ return layerProperty("fadeIn").toBool(); }
	int fadeInLength() 		{ return layerProperty("fadeInLength").toInt(); }

	bool fadeOut() 			{ return layerProperty("fadeOut").toBool(); }
	int fadeOutLength() 		{ return layerProperty("fadeOutLength").toInt(); }

	int showAnimationType() 	{ return layerProperty("showAnimationType").toInt(); }
	int showAnimationLength() 	{ return layerProperty("showAnimationLength").toInt(); }
	QEasingCurve::Type showAnimationCurve() 	{ return (QEasingCurve::Type)layerProperty("showAnimationCurve").toInt(); }

	int hideAnimationType() 	{ return layerProperty("hideAnimationType").toInt(); }
	int hideAnimationLength() 	{ return layerProperty("hideAnimationLength").toInt(); }
	QEasingCurve::Type hideAnimationCurve() 	{ return (QEasingCurve::Type)layerProperty("hideAnimationCurve").toInt(); }

// 	bool showFullScreen()		{ return layerProperty("showFullScreen").toBool(); }
	Qt::Alignment alignment()	{ return (Qt::Alignment)layerProperty("alignment").toInt(); }
	QPointF insetTopLeft()		{ return layerProperty("insetTopLeft").toPointF(); }
	QPointF insetBottomRight()	{ return layerProperty("insetBottomRight").toPointF(); }

	double alignedSizeScale()	{ return layerProperty("alignedSizeScale").toDouble(); }
	
	QVector3D rotation()	{ return layerProperty("rotation").value<QVector3D>(); }
	
	QPointF desiredAspectRatio() { return layerProperty("desiredAspectRatio").toPointF(); }
	
	int showOnShowLayerId()	{ return layerProperty("showOnShowLayerId").toInt(); }
	int hideOnShowLayerId()	{ return layerProperty("hideOnShowLayerId").toInt(); }
	
	LiveLayer * hideOnShow() { return m_hideOnShow; }
	LiveLayer * showOnShow() { return m_showOnShow; }
	
	void setHideOnShow(LiveLayer *layer);
	void setShowOnShow(LiveLayer *layer);
	
	class LiveScene *scene() { return m_scene; }
	void setScene(LiveScene *scene);
	
	bool layerPropertyUpdatesLocked() { return m_lockLayerPropertyUpdates; }
	


signals:
	void isVisible(bool);

	// emitted when a property is changed in this layer
	void layerPropertyChanged(const QString& propertyId, const QVariant& value, const QVariant& oldValue);

	// emitted by setInstanceName()
	void instanceNameChanged(const QString&);
	
	void layerNameChanged(const QString&);
	void userControllableChanged(bool);

public slots:
	void setLayerName(const QString&);
	void setUserControllable(bool);
	
	void setVisible(bool);

	void setRect(const QRectF&);
	void setRect(double x, double y, double w, double h) { setRect(QRectF(x,y,w,h)); }
	void setZIndex(double z);
	void setZIndex(int z); // for sig/slot compat
	void setOpacity(double o);
	void setOpacity(int);
	void setTransparency(int);

	void setX(int);
	void setY(int);
	void setX2(int);
	void setY2(int);
	

	void setWidth(int);
	void setHeight(int);

	void setPos(const QPointF& point)	{ setRect(QRectF(point, rect().size())); }
	void setSize(const QSizeF& size)	{ setRect(QRectF(rect().topLeft(),size)); }
	
// 	void setTopPercent(double value)	{ setLayerProperty("topPercent", value); }
// 	void setLeftPercent(double value)	{ setLayerProperty("leftPercent", value); }
// 	void setBottomPercent(double value)	{ setLayerProperty("bottomPercent", value); }
// 	void setRightPercent(double value)	{ setLayerProperty("rightPercent", value); }
// 
// 	void setTopPercent(int value)		{ setTopPercent(((double)value) / 100.0); }
// 	void setLeftPercent(int value)		{ setLeftPercent(((double)value) / 100.0); }
// 	void setBottomPercent(int value)	{ setBottomPercent(((double)value) / 100.0); }
// 	void setRightPercent(int value)		{ setRightPercent(((double)value) / 100.0); }

	void setFadeIn(bool value) 		{ setLayerProperty("fadeIn", value); }
	void setFadeInLength(int value) 	{ setLayerProperty("fadeInLength", value); }

	void setFadeOut(bool value) 		{ setLayerProperty("fadeOut", value); }
	void setFadeOutLength(int value) 	{ setLayerProperty("fadeOutLength", value); }

	void setShowAnimationType(int value) 	{ setLayerProperty("showAnimationType",   value); }
	void setShowAnimationLength(int value) 	{ setLayerProperty("showAnimationLength", value); }
	void setShowAnimationCurve(QEasingCurve::Type value) 		{ setLayerProperty("showAnimationCurve",  value); }

	void setHideAnimationType(int value) 	{ setLayerProperty("hideAnimationType",   value); }
	void setHideAnimationLength(int value) 	{ setLayerProperty("hideAnimationLength", value); }
	void setHideAnimationCurve(QEasingCurve::Type value) 		{ setLayerProperty("hideAnimationCurve",  value); }

// 	void setShowFullScreen(bool value);//	{ setLayerProperty("showFullScreen", value); }
	void setAlignment(Qt::Alignment value)	{ setLayerProperty("alignment", (int)value); }
	void setInsetTopLeft(QPointF value)	{ setLayerProperty("insetTopLeft", value); }
	void setInsetBottomRight(QPointF value)	{ setLayerProperty("insetBottomRight", value); }

	void setTopInset(int value)	{ setInsetTopLeft(QPointF(insetTopLeft().x(), value)); }
	void setLeftInset(int value)	{ setInsetTopLeft(QPointF(value, insetTopLeft().y())); }
	void setBottomInset(int value)	{ setInsetBottomRight(QPointF(insetBottomRight().x(), value)); }
	void setRightInset(int value)	{ setInsetBottomRight(QPointF(value, insetBottomRight().y())); }

	void setAlignedSizeScale(double value) { setLayerProperty("alignedSizeScale", value); }
	void setAlignedSizeScale(int value) { setAlignedSizeScale((double)value / 100.0); }
	
	
	void setRotation(QVector3D value)	{ setLayerProperty("rotation", value); }
	void setXRotation(double value)		{ QVector3D r = rotation(); r.setX(value); setLayerProperty("rotation", r); }
	void setYRotation(double value)		{ QVector3D r = rotation(); r.setY(value); setLayerProperty("rotation", r); }
	void setZRotation(double value)		{ QVector3D r = rotation(); r.setZ(value); setLayerProperty("rotation", r); }
	
	void setXRotation(int value)		{ setXRotation((double)value); }
	void setYRotation(int value)		{ setYRotation((double)value); }
	void setZRotation(int value)		{ setZRotation((double)value); }
	
	void setDesiredAspectRatio(QPointF value) { setLayerProperty("desiredAspectRatio",value); }
	
	void setShowOnShowLayerId(int);
	void setHideOnShowLayerId(int);
	
	void lockLayerPropertyUpdates(bool);
	
	void setBounceWhenShown(bool);


	// Internally, tries to set the named property on all the drawables if it has such a property
	// If no prop exists on the drawable, then tries to set the prop on the layer object
	// Either way, sets the prop in m_props and emits layerPropertyChanged()
	// This can be overridden to catch property changes internally, since the property editors
	// in MainWindow use this slot to update the props. Most of the time, this will Do The Right Thing
	// for basic properties, but for some custom ones you might have to intercept it.
	virtual void setLayerProperty(const QString& propertyId, const QVariant& value);

protected slots:
	// needed to offset the small list of options into the proper enum value
	void setShowAnim(int);
	void setHideAnim(int);

	// Translate the 'show as' combo box into a set of alignment flags
	void setShowAsType(const QString&);
	
	void setShowOnShow(int);
	void setHideOnShow(int);

protected:
	// Returns a GLDrawable for the specified GLWidget. If none exists,
	// then calls createDrawable() internally, followed by initDrawable()
	virtual GLDrawable * drawable(GLWidget *widget, bool secondary=false);

	
	class PropertyEditorOptions
	{
	public:
		PropertyEditorOptions()
		{
			reset();
		};

		void reset()
		{
			text = "";
			suffix = "";
			noSlider = false;
			min = -9999;
			max =  9999;
			type = QVariant::Invalid;
			value = QVariant();
			doubleIsPercentage = false;
			defaultValue = QVariant();
			stringIsFile = false;
			fileTypeFilter = "";
			step = 1;
		}

		QString text;
		QString suffix;
		bool noSlider;
		double min;
		double max;
		int step;
		QVariant::Type type;
		QVariant value;
		bool doubleIsPercentage;
		QVariant defaultValue;
		bool stringIsFile;
		QString fileTypeFilter;
	};

	QWidget * generatePropertyEditor(QObject *object, const char *property, const char *slot, PropertyEditorOptions opts = PropertyEditorOptions(), const char *changeSignal=0);

	void setInstanceName(const QString&);

	// just emits layerPropertyChanged
	void layerPropertyWasChanged(const QString& propertyId, const QVariant& value, const QVariant& oldValue);

	virtual bool requiresSecondaryDrawable() { return false; }
	
	// The core of the layer - create a new drawable instance for the specified context.
	// drawable() will call initDrawable() on it to set it up as needed
	virtual GLDrawable *createDrawable(GLWidget *widget, bool isSecondary=false);

	// If its the first drawable, setup with defaults and load m_props[] with appros values
	// If not first drawable, load drawable with values from m_props[]
	virtual void initDrawable(GLDrawable *drawable, bool isFirstDrawable = false);
	
	// Helper function - given a list of property names, load the props using QObject::property() from the drawable into m_props
	void loadLayerPropertiesFromObject(const QObject *, const QStringList& list);

	// Helper function - attempts to apply the list of props in 'list' (or all props in m_props if 'list' is empty) to drawable
	// 'Attempts' means that if drawable->metaObject()->indexOfProperty() returns <0, then setProperty is not called
	void applyLayerPropertiesToObject(QObject *object, QStringList list = QStringList());

	// Apply the animation properties to the drawable
	void applyAnimationProperties(GLDrawable *);

	// Helper routine to loop thru all drawables and set 'prop' to 'value'
	void applyDrawableProperty(const QString& prop, const QVariant& value);
	
	// Utility function to load the map of animatiable properties
	void setAnimatePropFlags(const QStringList&, bool flag=false);
	
	// Default impl of canAnimateProperty() will return true UNLESS prop is listed here, then it will return the bool attached to the prop ID in this hash
	QHash<QString, bool> m_animateProps;

	// "pretty" name for this instance, like "SeasonsLoop3.mpg" or some other content-identifying string
	QString m_instanceName;

	// Maintained as a member var rather than in m_props due to different method of setting the value
	bool m_isVisible;

	// Cache for the drawables for this layer
	QHash<GLWidget*, GLDrawable*> m_drawables;
	QHash<GLWidget*, GLDrawable*> m_secondaryDrawables;
	
	// used for cross fading inorder to know which drawable to make visible/invisible when setVisible is called
	bool m_secondarySourceActive;

	// The properties for this layer - to properly set in sub-classes, use setProperty
	QVariantMap m_props;
	
	// List of widgets currently attached to
	QList<GLWidget*>  m_glWidgets;

	// Flagged if user disabled animations
	bool m_animationsDisabled;
	
private:
	void setVisibleGeometryFields(QStringList list = QStringList());
	void applyDrawablePropertyInternal(GLDrawable *drawable, const QString& prop, const QVariant& value);
	
	
	QFormLayout * m_geomLayout;
	QHash<QString,QWidget*> m_propWidget;
	QHash<QString,QWidget*> m_labelCache;
	
	QHash<QString,bool> m_propSetLock;
	
	LiveScene * m_scene;
	LiveLayer * m_hideOnShow;
	LiveLayer * m_showOnShow;
	
	QList<LiveLayer*> m_sortedLayerList;
	bool m_lockVisibleSetter;
	
	AnimParam m_animParam;
	
	int m_layerId;
	bool m_layerIdLoaded;
	bool m_lockLayerPropertyUpdates;
	
	QString m_layerName;
	bool m_isUserControllable;
};

#endif
