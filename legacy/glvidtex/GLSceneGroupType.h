#ifndef GLSceneGroupType_H
#define GLSceneGroupType_H

#include <QtGui>

class GLDrawable;
class GLScene;
class GLSceneGroup;
class DirectorWindow;

#include "GLSceneGroup.h"

#include "../livemix/EditorUtilityWidgets.h"

/** \class GLSceneType 
	A generic base class for scene 'types'.
	
	A scene 'type' (scenetype) is is a method for applying unique programatic functionality to an otherwise generic and/or static
	scene. For example, a GLSceneType could implement a 'current weather conditions' scene which looks up the current weather 
	at a zip code and then updates the fields in the scene to reflect the returned values.
	
	Another way to think of a GLSceneType is the 'controller' in an MVC paradigm, which internally supplies its own 'model', and 
	uses a GLScene as a 'view' (or template.) Think of a HTML 'template' typically used in dynamic websites - the controller
	(script, CGI, whatever) does the processing necessary to fill in the fields in the HTML template, then outputs the 
	merger of the data + template as HTML to the browser. 
	 
	GLSceneType instances to basically the same thing:
	 
		-# A user employs a UI (web UI or Director) to look at a list of registered GLSceneTypes (such as the weather example above)
		-# The user then chooses from a list of templates for the chosen scene type (loaded from a directory specific to that scene type)
		-# Once the user chooses the scene type (step 1) and the template (step 2), the UI will use GLSceneType::attachToScene() to 
			attach the GLSceneType to the chosen GLScene, and the scene will be added to the current GLSceneGroup.
			
	At runtime, when the GLScene is about to be loaded into the player, the GLSceneType::setLiveStatus(bool) method is called. This allows
	the scenetype to delay any processing necessary till just before go-live - after all, no need to be constantly updating the scene if 
	not on screen.
	 
	Each GLSceneType exposes a list of type-specific parameters (see GLSceneType::ParameterInfo and GLSceneType::parameters()) which describe
	the various options the user can configure on this template. For a weather scenetype (for example), the GLSceneType may expose a 
	'zipcode' parameter. The list of parameters for a specific scenetype can be queried thru GLSceneType::parameters().
	
	Each GLSceneType also gives a list of 'fields' that it expects to be present in the template the scenetype to be useable. The list of 
	expected fields is available thru GLSceneType::fields() which returns a QList of GLSceneType::FieldInfo instances. UIs which 
	use GLSceneTypes \em should use GLSceneType::auditTemplate() to ensure the GLScene provided has all the required fields.
	
	See GLSceneTypes.h for a list of defined scene types in the library.
*/	
class GLSceneType : public QObject
{
	Q_OBJECT
	
	/** The ID of this GLSceneType. Note this ID should not change between instances - it should uniquely identify a GLSceneType subclass, not a unique instance.
		GLScene will store and use this ID to request an instance of this GLSceneType from GLSceneType factory. You could generate this
		ID for your subclass using a command like \em uuidgen on linux, for example. */
	Q_PROPERTY(QString id READ id);
	
	/** The scene type title. For example, "Current Weather Conditions" */ 
	Q_PROPERTY(QString title READ title);
	
	/** The description of this scene type. For example: "Dynamicaly loads weather information from Google based on ZIP code." */
	Q_PROPERTY(QString description READ description);
	
	/** Used by PlayerWindow to notify this instance that it is about to go live or is no longer live. 
		Will ALWAYS be called BEFORE going live and AFTER going 'not live'. */
	Q_PROPERTY(bool liveStatus READ liveStatus WRITE setLiveStatus);
	
public:
	GLSceneType(QObject *parent=0);
	
	
	/** \class ParameterInfo
		Describes a parameter that can be set on an instance of this GLSceneType. See GLSceneType and GLSceneType::parameters().
	*/ 
	class ParameterInfo
	{
	public:
		/** Setup the fields in this ParameterInfo object with the convenience of a single line of code. */
		ParameterInfo(QString _name="", QString _title="", QString _description="", QVariant::Type _type=QVariant::Invalid, bool _required=true, const char *_slot=0) 
		{ 
			name		= _name;
			title		= _title;
			description	= _description;
			type		= _type;
			required	= _required;
			slot		= _slot;
		} 
		
		/** The internal name of the parameter. For example, 'zipcode' */
		QString name;
		/** The title of the parameter, suitable for displaying as a prompt. For example, 'Zip Code'. If blank, the title
			is guessed from the ParameterInfo::name using PropertyEditorFactory::guessTitle() 
			 
			Example: 'Zip Code'
		*/
		QString title;
		
		/** A description of the parameter, suitable for display as a tooltip or a 'hint' text below the field in a UI.
		 
			Example: 'The 5-digit US Postal ZIP code of the area for which to retrieve the weather.'
		*/ 
		QString description;
		
		/** The type of value expected to be provided. */
		QVariant::Type type;
		
		/** Extra hints describing how this field should be displayed to the user for editing, suitable for passing to PropertyEditorFactory::generatePropertyEditor() */
		PropertyEditorFactory::PropertyEditorOptions hints;
		
		/** The Qt slot on the scene which can be used to set this parameter */
		const char *slot;
		
		/** If true, this parameter must be provided. Primarily for use in a UI - not currently enforced internally. */
		bool required;
	};
	
	/** \class FieldInfo
		Describes a field that may/must be present in a scene template 
		inorder for this scene type's logic to work properly. See GLSceneType and GLSceneType::fields().
	*/ 
	class FieldInfo
	{
	public:
		/** Setup the fields in this FieldInfo object with the convenience of a single line of code. */
		FieldInfo(QString _name="", QString _title="", QString _description="", QString _type="", bool _required=true)
		{
			name		= _name;
			title		= _title;
			description	= _description;
			expectedType	= _type;
			required	= _required;
		};
		
		/** The name of this field as expected to be specified in the 'Item Name' parameter on a drawable.
		
			For exmaple, a GLSceneType may specify a field with name 'WeatherIcon' - which means
			there should be an image drawable in the template named 'WeatherIcon' (exactly that name)
			inorder for this scene type to function properly.
			
			Note that the GLSceneType::auditTemplate() will use the this \em name to check 
			and audit the template.
		*/  
		QString name;
		
		/** A textual title for this field. Example: Weather Icon */ 
		QString title;
		
		/** A textual description of this field - what it does, why its used. 
			Example: Displays an appropriate icon for the current weather conditions at the location given. */
		QString description;
		
		/** The expected class type. (Optionally without the 'GL' and 'Drawable' prefix/suffix) Also used in GLSceneType::auditTemplate().
			Example: \em Image or \em GLImageDrawable or \em ImageDrawable*/ 
		QString expectedType;
		
		/** The default value the scenetype will assume if the user does not provide anything. */
		QVariant defaultValue;
		
		/** If true, this field must be present in the template for auditTemplate() to pass, assuming FieldInfo::expectedType is correct as well.
			If false, then GLSceneType::auditTemplate() will still return an AuditError for this field if its not present in the template, but
			AuditError::isWarning will be \em true
		*/ 
		bool required;
	};
	
	/** \class AuditError
		Describes an error encountered by GLSceneType::auditTemplate()
	*/
	class AuditError
	{
	public:
		/** The constructor initalizes AuditError::item to NULL and AuditError::isWarning to false */ 
		AuditError(QString _error="", GLDrawable *_item = 0, GLSceneType::FieldInfo _info = GLSceneType::FieldInfo(), bool _warning=false)
		{
			item		= _item;
			fieldInfo 	= _info;
			error		= _error;
			isWarning	= _warning;
		};
		
		/** The GLDrawable to which this error refers. This will be set if
			the FieldInfo matches this drawable, but the FieldInfo::expectedType does not 
			match the GLDrawable's class type (See QMetaObject::className()).
		*/
		GLDrawable *item;
		
		/** The GLSceneType::FieldInfo used to audit the GLDrwable AuditError::item referenced in this error. */
		GLSceneType::FieldInfo fieldInfo;
		
		/** A textual description of the error. For example, "Incorrect drawable type for field X - found Y expected Z" */
		QString error;
		
		/** If true, this error is not a 'show stopper' and the template can still be used without correcting the error.
			Warnings are issued, for example, when a field is specified in the FieldInfo list and not found
			in the template, but the FieldInfo::required field is false. */
 		bool isWarning;
 		
 		/** Convenience method to quickly stringify the relevant information from this error for output to console or other output. */
 		QString toString()
 		{
 			return QString("[%1] %2 %3").arg(isWarning ? "WARN" : "ERROR").arg(error).arg(!fieldInfo.name.isEmpty() ? QString("(Field '%1')").arg(fieldInfo.name) : ""); 
 		}
 		
 		/** A static utility function that can be used to easily check the output of GLSceneType::auditTemplate() to see if 
 			the list of errors returned has any erorrs or is just warnings.
 			
 			Even suitable for checking an empty list - will return false (no errors) if the list is empty or if its just all warnings. 
 		*/ 
 		static bool hasErrors(QList<AuditError> list)
 		{
 			foreach(AuditError error, list)
 				if(!error.isWarning)
 					return true;
 			return false;
 		}
	};
	
	/** A convenience typedef for a list of ParameterInfo objects */
	typedef QList<GLSceneType::ParameterInfo> ParamInfoList;
	
	/** A convenience typedef for a list of FieldInfo objects */
	typedef QList<GLSceneType::FieldInfo> FieldInfoList;
	
	/** A convenience typedef for a list of AuditError objects */
	typedef QList<GLSceneType::AuditError> AuditErrorList;
	
	/** The ID of this GLSceneType. Note this ID should not change between instances - it should uniquely identify a GLSceneType subclass, not a unique instance.
		GLScene will store and use this ID to request an instance of this GLSceneType from GLSceneType factory. You could generate this
		ID for your subclass using a command like \em uuidgen on linux, for example. */
	virtual QString id();
	
	/** The scene type title. For example, "Current Weather Conditions" */ 
	virtual QString title();
	
	/** The description of this scene type. For example: "Dynamicaly loads weather information from Google based on ZIP code." */
	virtual QString description();
	
	/** Subclasses should override and return true of the subclass automatically updates the scene duration based on some logic. For example, the GLSceneTypeRandomVideo class 
	    automatically updates the scene duration to match that of the currently playing video. Therefore, GLSceneTypeRandomVideo::doesAutomateSceneDuration() returns true. */
	virtual bool doesAutomateSceneDuration() { return false; }
	
	/** Audit the \a sceneTemplate by checking the list of items in this scene against list of fields (See FieldInfo)
		expected by this GLSceneType.
		 
		Returns a list of AuditError items describing any problems found in this template.
		
		If the returned list is empty (QList::isEmpty()), then there were no errors encountered.
		
		If the returned list is not empty, check AuditError::isWarning to see if any of the errors are 'show stoppers.' 
		See utility method AuditError::hasErrors().
		
		\sa AuditError
		\sa AuditError::hasErrors()
	*/
	virtual QList<AuditError> auditTemplate(GLScene* sceneTemplate);
	
	/** Returns a list of parameters that a user may configure. */
	virtual QList<ParameterInfo> parameters() { return m_paramInfoList; }
	
	/** Returns a list of fields that may/must be present in the template for auditTemplate() to pass the template. */
	virtual QList<FieldInfo> fields() { return m_fieldInfoList; }
	
	/** A GLSceneType instance is 'valid' only if its been applied/attached to a scene.
		
		Before it's attached to a scene, all it provides is title/description and a
		list of parameters and expected fields. After being attached to the scene,
		the scene type instance can be configured (set the parameters) and
		can update the attached scene at will (say, from a slot connected to a QTimer.)
		
		To attach to a scene, use attachToScene() or generateScene().
		
		The correct GLSceneType (along with parameter values, etc) is automatically 
		attached to the scene when the scene is loaded if a scene type was attached 
		before saving to disk.
		
		\retval true if this instance is attached to scene
		\retval false if this instance is not attached to a scene.
		
		\sa attachToScene()
		\sa generateScene()
	*/ 
	virtual bool isValidInstance() { return m_scene.isNull(); }

	/** Duplicates the \a sceneTemplate and generates a new GLScene instance 
		with new ID numbers for each of the drawables. Creates a new
		instance of this scene type and calls GLScene::setSceneType() on the
		new scene instance, and sets scene() to the new scene pointer.
		
		@note The GLScene will take ownership of the new GLSceneType instance and the 
		scenetype instance will be deleted when the scene is deleted.
		
		@note The caller will assume ownership of the returned GLScene pointer and
		must be responsible for deleting it when no longer needed. The caller
		may simply add the GLScene to a GLSceneGroup, which will take care
		of ownership of the GLScene internally.
		
		Calls attachToScene() internally with the new GLScene instance to setup the scene and scene type.
		
		@param scene The template to use for making the new scene
		@return The new GLScene created from the templates (assuming attachToScene() likes the new template)
		@retval NULL if attachToScene() returns false.
	*/	 
	virtual GLScene *generateScene(GLScene *sceneTemplate);
	
	/** Attach this scene type to a scene.
		
		@note If \em this instance of GLSceneType that you call this method on is from the GLSceneTypeFactory::lookup() routine or the GLSceneTypeFactory::list(),
			then attachToScene() will create a new instance of this GLSceneType using GLSceneTypeFactory::newInstance() before attaching to the scene. The GLScene 
			will take ownership of the new instance and delete it when the GLScene is deleted or a new scene type is set.
			 
			
		That means that GLScene::sceneType() will return a different GLSceneType pointer than what you used to call attachToScene() on if the GLSceneType pointer is 
		from the GLSceneTypeFactory class. The following example will cause a new instance of the scene type to be created:
		 
		 \code
			// Assuming GLSceneTypeCurrentWeather.h defines GLSceneTypeCurrentWeather_ID as the ID of the class..
			GLScene *scene = ...;
			GLSceneTypeCurrentWeather *weather = GLSceneTypeFactory::lookup(GLSceneTypeCurrentWeather_ID);
			weather->attachToScene(scene);
			// Now, scene->sceneType() != weather since attachToScene() created a new instance internally
		\endcode
		
		However, if you created GLSceneType directly as in the example below, no new instance is created:
		
		\code
			GLScene *scene = ...;
			GLSceneTypeCurrentWeather *weather = new GLSceneTypeCurrentWeather();
			weather->attachToScene(scene);
			// Now, scene->sceneType() = weather
		\endcode
		
		Calls applyFieldData() internally to apply the current data to the fields in the template.
		
		@retval true if applyFieldData() found all the required fields
		@retval false if applyFieldData() did not find required fields (scene is still attached though)
	*/
	virtual bool attachToScene(GLScene *scene);
	
	/** Called by DirectorWindow to create widgets to be used to control the scene.
		For example, for a sports broadcast scene, you might return a widget
		to update the onscreen score. 
	*/
	virtual QList<QWidget*> createEditorWidgets(GLScene *scene, DirectorWindow *director);

	/** Condenses this scene type's parameters and field data into a byte array.
		 
		Note that this should be called like:
		\code
		GLScene *scene = ...;
		QByteArray data = scene->sceneType()->toByteArray();
		\endcode
		Of course, make sure sceneType() is valid or you'll segfault.
		
		To restore, use and re-attach to a scene, use GLSceneTypeFactory::fromByteArray():
		
		\code
		GLScene *storedScene = ...;
		QByteArray storedData = ...;
		
		GLSceneTypeFactory::fromByteArray(storedData, storedScene);
		\endcode	
		
		GLSceneTypeFactory::fromByteArray() will automatically create a new instance of the
		proper subclass of GLSceneType and attach the new instance to the \em storedScene 
		pointer given. It does return the new GLSceneType* instance, but you dont need to
		keep it if you're just restoring from disk - GLScene will delete the instance
		when itself is deleted.
	*/
	virtual QByteArray toByteArray();
	
	/** Returns the GLScene to which this instance is bound. 
		
		You will never receive a GLSceneType from the GLSceneTypeFactory with the scene() set. This will
		only return a scene after using attachToScene() which returns a new GLSceneType instance that 
		has the scene() set - the original instance of GLSceneType is unchanged. 
		
		You can also use generateScene() to duplicate a scene and a pointer to the new scene. The new scene will have a
		new instance of GLSceneType on it with this method returning the same GLScene pointer - the original
		GLSceneType instance will not be changed.
	*/	
	virtual GLScene *scene() { return (GLScene*)m_scene; }
	
	/** If true, then this scenetype (or rather, the scene ()) is currently live in a player. */ 
	bool liveStatus() { return m_liveStatus; }
	
public slots:
	/** Sets the given \a param to \a value.
		\todo Update PropertyEditorFactory::generatePropertyEditor() to be able to call a slot with the name of the property as the first argument for easy use with these generat slots.
		Each of the setParam() variations below call this setParam(QString,QVariant) internally - therefore, you can safely override this setParam() in your subclass to intercept
		all setParam() calls.
	 */
	virtual void setParam(QString param, QVariant value);
	/** Overloaded for convenience. Sets \a param to \a value. */
	virtual void setParam(QString param, QString value);
	/** Overloaded for convenience. Sets \a param to \a value. */
	virtual void setParam(QString param, QVariantList value);
	/** Overloaded for convenience. Iterates over the list of keys from the \a map and sets each parameter to the value in the map for that key. */ 
	virtual void setParams(QVariantMap map);
	
	/** Used by PlayerWindow to notify this instance that it is about to go live or is no longer live. 
		Will ALWAYS be called BEFORE going live and AFTER going 'not live'.
	*/
	virtual void setLiveStatus(bool flag=true);
	
protected:
	friend class GLSceneTypeFactory;
	
	/** This method is available for subclasses to hook into when a scene is actually attached. The default implementation does nothing. */
	virtual void sceneAttached(GLScene *);
	
	/** Apply the field data to the scene. 
		If \a fieldName is given, only that field is applied, otherwise all fields are applied. */
	bool applyFieldData(QString fieldName = "");
	
	/** Find the drawable for the given field name. Returns NULL if not found. */
	GLDrawable *lookupField(QString fieldName);
	
	/** Updates the internal value of \a field to \a value. 
		If a scene is bound, it will update the field in the scene as well. */ 
	virtual void setField(QString field, QVariant value);
	
	/** The ID of the subclass. This is an empty string in the GLSceneType base class. When subclassing,
		you should set this to a unique ID for your subclass - consider generating a uuid with a tool
		such as \em uuidgen to ensure your ID is unique. */
	QString m_id;
	
	/** A list of parameters. This list should be populated by subclasses to describe what parameters they expect. */
	ParamInfoList m_paramInfoList;
	
	/** A list of fields. This list should be populated by subclasses to describe what fields they require/expect. */
	FieldInfoList m_fieldInfoList;
	
	/** A list of parameter values as set by setParam() */
	QVariantMap m_params;
	
	/** The current value of fields, indexed by FieldInfo::name */
	QVariantMap m_fields;
	
	/** The scene to which this instance is bound */
	QPointer<GLScene> m_scene;
	
	/** Stores the live status of this scene type. See liveStatus() and setLiveStatus() for more information. */
	bool m_liveStatus;
	
	/** GLSceneTypeFactory sets this to \em true when it returns it via GLSceneTypeFactory::lookup() so that attachToScene() knows to call GLSceneTypeFactory::newInstance() internally
		before attaching to the scene. If this is \em false, then attachToScene() won't duplicate the instance prior to attaching */
	bool m_factoryFresh;
};

/** Convenience typedef since the Q_FOREACH macro doesn't like multiple brackets ('>') inside the typde of the first argument. */ 
typedef QList<GLSceneType *> GLSceneTypeList;
	
/** \class GLSceneGroupType 
	A generic base class for group 'types'.
	
	GLSceneGroupType is very closely related to the GLSceneType class - in fact, much of the documentation and API is almost word-for-word the same (almost.) 
	The distinct difference is that GLSceneGroupType is designed to handle a larger scope than GLSceneType. Whereas GLSceneType is specific to managing 
	a single scene and the fields within, GLSceneGroupType is designed to manage a set of GLSceneType instances and a set of GLScenes. 
	
	For example, you might have a GLSceneGroupType which implements a slideshow of pictures based on a directory of images. The GLSceneGroupType manages 
	the loading of the list of files from disk and generating the resulting list of scenes. This fictious slideshow GLSceneGroupType example would implement its 
	own custom GLSceneType which would manage the actual GLScene items and values - the custom scene type would be given the image file as a parameter, and it
	could possibly load the EXIF data to add a comment to the scene along with the image itself.
	
	Example usage scenario for GLSceneGroupType would be as follows:
	 
		-# A user employs a UI (web UI or Director) to look at a list of registered GLSceneGroupType (such as the slideshow example, above.)
		-# The user then chooses from a list of templates for the chosen scene type (loaded from a directory specific to that scene type - such as a 'fancy' slideshow, a 'simple' slideshow, etc.)
		-# Once the user chooses the scene type (step 1) and the template (step 2), the UI will use GLSceneGroupType::attachToGroup() to 
			attach the GLSceneGroupType to the chosen GLSceneGroup, and the scene will be added to the current GLSceneGroupCollection.
			
	At runtime, when the GLSceneGroupType is about to be loaded into the player, the GLSceneGroupType::setLiveStatus(bool) method is called. This allows
	the grouptype to delay any processing necessary till just before go-live - after all, no need to be constantly updating the group if not on screen.
	 
	Each GLSceneGroupType exposes a list of type-specific parameters (see GLSceneType::ParameterInfo and GLSceneType::parameters()) which describe
	the various options the user can configure on this template. For a slideshow scenetype (for example), the GLSceneGroupType may expose a 
	'directory' parameter from which to load a list of images. The list of parameters for a specific scenetype can be queried thru GLSceneGroupType::parameters().
	
	Each GLSceneGroupType also gives a list of one or more GLSceneType instances that it will use to create the GLScenes in sceneTypes(). The method and 
	order it uses for applying GLSceneTypes to the list of GLScenes present in the GLSceneGroup provided as a template is an imeplemtnation decision made by a subclass. 
	
	For example, a slideshow GLSceneGroupType could implement two GLSceneType instances - a 'title' scene and a 'image' scene. Internally, it could choose to 
	use the 'title' scene for the first scene in the GLSceneGroup template - or the last. The GLSceneGroupType should document the manner in which it applies 
	its GLSceneTypes in its description() field which can be shown to the user to aid in designing templates for use by the GLSceneGroupType subclass.
	
	See GLSceneGroupTypes.h for a list of defined group types in the library.
*/
class GLSceneGroupType : public QObject
{
	Q_OBJECT
	
	/** The ID of this GLSceneGroupType. Note this ID should not change between instances - it should uniquely identify a GLSceneGroupType subclass, not a unique instance.
		GLSceneGroup will store and use this ID to request an instance of this GLSceneGroupType from GLSceneGroupTypeFactory. You could generate this
		ID for your subclass using a command like \em uuidgen on linux, for example. */
	Q_PROPERTY(QString id READ id);
	
	/** The group type title. For example, "Current Weather Conditions" */ 
	Q_PROPERTY(QString title READ title);
	
	/** The description of this group type. For example: "Dynamicaly loads weather information from Google based on ZIP code." */
	Q_PROPERTY(QString description READ description);
	
	/** Used by PlayerWindow to notify this instance that it is about to go live or is no longer live. 
		Will ALWAYS be called BEFORE going live and AFTER going 'not live'. */
	Q_PROPERTY(bool liveStatus READ liveStatus WRITE setLiveStatus);
	
public:
	GLSceneGroupType(QObject *parent=0);
	
	/** The ID of this GLSceneGroupType. Note this ID should not change between instances - it should uniquely identify a GLSceneGroupType subclass, not a unique instance.
		GLSceneGroup will store and use this ID to request an instance of this GLSceneGroupType from GLSceneGroupTypeFactory. You could generate this
		ID for your subclass using a command like \em uuidgen on linux, for example. */
	virtual QString id();
	
	/** The group type title. For example, "Current Weather Conditions" */ 
	virtual QString title();
	
	/** The description of this group type. For example: "Dynamicaly loads weather information from Google based on ZIP code." */
	virtual QString description();
	
	/** Returns a list of parameters that a user may configure. */
	virtual QList<GLSceneType::ParameterInfo> parameters() { return m_paramInfoList; }
	
	/** Returns a list of GLSceneType instances that will be used to configure/create the GLScene instances to be added in the final group
		returned from generateGroup() or attachToGroup() */ 
	virtual GLSceneTypeList sceneTypes() { return m_sceneTypeList; }
	
	/** A GLSceneGroupType instance is 'valid' only if its been applied/attached to a group.
		
		Before it's attached to a group, all it provides is title/description and a
		list of parameters. After being attached to the group,
		the group type instance can be configured (set the parameters) and
		can update the attached group at will (say, from a slot connected to a QTimer.)
		
		To attach to a group, use attachToGroup() or generateGroup().
		
		The correct GLSceneGroupType (along with parameter values, etc) is automatically 
		attached to the group when the group is loaded if a group type was attached 
		before saving to disk.
		
		\retval true if this instance is attached to group
		\retval false if this instance is not attached to a group.
		
		\sa attachToGroup()
		\sa generateGroup()
	*/ 
	virtual bool isValidInstance() { return m_group != NULL; }

	/** Duplicates the \a groupTemplate and generates a new GLSceneGroup instance 
		with new ID numbers for each of the drawables. Creates a new
		instance of this group type and calls GLSceneGroup::setSceneType() on the
		new group instance, and sets group() to the new group pointer.
	*/	 
	virtual GLSceneGroup *generateGroup(GLSceneGroup *groupTemplate);
	
	/** Creates a new instance of this group type, sets group() to the \group
		and calls \a group->setSceneType() with the new instance's pointer.
		Also sets up any fields to their initial values.
		@param group The group to which to attach.
		@return The new instance of this group type
	*/
	virtual GLSceneGroupType *attachToGroup(GLSceneGroup *group, QByteArray ba = QByteArray());
	
	/** Called by DirectorWindow to create widgets to be used to control the group.
		For example, for a sports broadcast group, you might return a widget
		to update the onscreen score.
		 
		\todo Will this conflict with the group type method of same name in normal usage?
	*/
	virtual QList<QWidget*> createEditorWidgets(GLSceneGroup *, GLScene*, DirectorWindow *director);

	
	/** Condenses this group type's settings
		and the field->drawable ID pairings for the group into a byte array.
		Settings would be things such as, for a weather group, possibly the zip code or other unique paramters to the type's instance.
		 
		Note that this should be called like:
		\code
		GLSceneGroup *group = ...;
		QByteArray data = group->groupType()->toByteArray();
		\endcode
		Of course, make sure groupType() is valid or you'll segfault.
	*/
	virtual QByteArray toByteArray();
	
	/** Returns the GLSceneGroup to which this instance is bound. 
		
		You will never receive a GLSceneGroupType from the GLSceneGroupTypeFactory with the group() set. This will
		only return a group after using attachToGroup() which returns a new GLSceneGroupType instance that 
		has the group() set - the original instance of GLSceneGroupType is unchanged. 
		
		You can also use generateGroup() to duplicate a group and a pointer to the new group. The new group will have a
		new instance of GLSceneGroupType on it with this method returning the same GLSceneGroup pointer - the original
		GLSceneGroupType instance will not be changed.
		 
		This is also set by using fromByteArray() to create a new instance of a type from a stored QByteArray.
	*/	
	virtual GLSceneGroup *group() { return m_group; }
	
	/** If true, then this grouptype (or rather, the group ()) is currently live in a player. */ 
	bool liveStatus() { return m_liveStatus; }
	
public slots:
	/** Sets the given \a param to \a value.
		\todo Update PropertyEditorFactory::generatePropertyEditor() to be able to call a slot with the name of the property as the first argument for easy use with these generat slots.
	 */
	virtual void setParam(QString param, QVariant value);
	/** Overloaded for convenience. Sets \a param to \a value. */
	virtual void setParam(QString param, QString value);
	/** Overloaded for convenience. Sets \a param to \a value. */
	virtual void setParam(QString param, QVariantList value);
	/** Overloaded for convenience. Iterates over the list of keys from the \a map and sets each parameter to the value in the map for that key. */ 
	virtual void setParams(QVariantMap map);
	
	/** Used by PlayerWindow to notify this instance that it is about to go live or is no longer live. 
		Will ALWAYS be called BEFORE going live and AFTER going 'not live'.
	*/
	virtual void setLiveStatus(bool flag=true);
	
protected:
	/** Used in the loading of a GLSceneGroup from disk to re-attach a GLSceneGroupType to a GLSceneGroup.
		
		Returns a new GLSceneGroupType instance unique to the data given.
		
		@param ba The byte array containing the data used to reconstitute the group type instance
		@return The new instance of this group type which has been setup for the group.
	*/
	//virtual GLSceneGroupType *fromByteArray(QByteArray ba);
		
	/** The ID of the subclass. This is an empty string in the GLSceneType base class. When subclassing,
		you should set this to a unique ID for your subclass - consider generating a uuid with a tool
		such as \em uuidgen to ensure your ID is unique. */
	QString m_id;
	
	
	/** A list of parameters. This list should be populated by subclasses to describe what parameters they expect. */
	GLSceneType::ParamInfoList m_paramInfoList;
	
	/** A list of scene types. This list should be populated by subclasses to describe what scene types they implement/use. */
	GLSceneTypeList m_sceneTypeList;
	
	/** A list of parameter values as set by setParam() */
	QVariantMap m_params;
	
	/** The group to which this instance is bound */
	GLSceneGroup *m_group;
	
	/** Stores the live status of this scene type. See liveStatus() and setLiveStatus() for more information. */
	bool m_liveStatus;	
	
};

/** \class GLSceneTypeFactory
	A simple factory for enumerating available GLSceneType subclasses and looking up subclasses by their ID.
*/
class GLSceneTypeFactory
{
	GLSceneTypeFactory();
public:
	/** Lookup a GLSceneType type given the ID.
		@retval NULL if the ID is invalid or not a known scene type
		@retval GLSceneType* if the ID is valid
	*/
	static GLSceneType *lookup(QString id);
	
	/** Create a new instance of the GLSceneType given the ID. */
	static GLSceneType *newInstance(QString id);
	
	/** Restore a stored GLSceneType from a QByteArray produced by GLSceneType::toByteArray(). */
	static GLSceneType *fromByteArray(QByteArray array, GLScene *sceneToAttach);
	
	/** List all known GLSceneType subcalsses.
		All GLSceneGroupType pointers returned in this list are not bound to any scene - they are just to be used for informational
		purposes. You may call the attachToScene() or generateScene() methods, both of which will duplicate the GLSceneType internally,
		not modifying the original GLSceneType pointer.  */
	static QList<GLSceneType*> list();

private:
// 	static void addType(GLSceneType*);

	QList<GLSceneType*> m_list;
	QHash<QString,GLSceneType*> m_lookup;

	static GLSceneTypeFactory *d();
	static GLSceneTypeFactory *m_inst;
};

/** \class GLSceneGroupTypeFactory
	A simple factory for enumerating available GLSceneGroupType subclasses and looking up subclasses by their ID.
*/
class GLSceneGroupTypeFactory
{
	GLSceneGroupTypeFactory();
public:
	/** Create a new instance of the scene type given by ID.
		@retval NULL if the ID is invalid or not a known scene type
		@retval GLSceneType*-subclass if the ID is valid
	*/
	static GLSceneGroupType *newInstance(QString id);
	/** List all known GLSceneGroupType subcalsses.
		All GLSceneGroupType pointers returned in this list are not bound to any group - they are just to be used for informational
		purposes. You may call the attachToGroup() or generateGroup() methods, both of which will duplicate the GLSceneGroupType internally,
		not modifying the original GLSceneGroupType pointer.  */
	static QList<GLSceneGroupType*> list();

private:
	void addType(GLSceneGroupType*);
	
	QList<GLSceneGroupType*> m_list;
	QHash<QString,GLSceneGroupType*> m_lookup;
	
	static GLSceneGroupTypeFactory *d();
	static GLSceneGroupTypeFactory *m_inst;

};

#endif
