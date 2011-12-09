#ifndef EditorUtilityWidgets_H
#define EditorUtilityWidgets_H

#include <QtGui>

/** \class PropertyEditorFactory
	Attempts to automagically generate appropriate widgets for editing Q_PROPERTYs on QObject-based classes.
	
	See generatePropertyEditor().
*/
class PropertyEditorFactory : public QObject
{
	Q_OBJECT
public:
	
	/** \class PropertyEditorOptions
		A set of a variety of parameters for use by generatePropertyEditor().
		Not all values in this class may be used - the fields used depends on the QVariant::Type of value being edited.
	*/ 
	class PropertyEditorOptions
	{
	public:
		PropertyEditorOptions()
		{
			reset();
		};

		/** Reset all values to their default settings. */
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
			stringIsDir = false;
			step = 1;
		}

		/** Used for boolean values as the text for the checkbox */ 
		QString text;
		/** Used as the suffix for a QSpinBox or QDoubleSpinBox for ints/doubles */
		QString suffix;
		
		/** Enable/disable the use of a QSlider for ints */
		bool noSlider;
		
		/** The minimum for a QSPinBox for use with ints/doubles. Casts to an int of the type being edited is QVariant::Int. */
		double min;
		
		/** The maximum for a QSPinBox for use with ints/doubles. Casts to an int of the type being edited is QVariant::Int. */
		double max;
		
		/** The step for a QSPinBox/QSlider for use with ints/doubles. \todo Should this be a double?? */
		int step;
		
		/** If set, this overrides the automatic type detection in generatePropertyEditor() */
		QVariant::Type type;
		
		/** If set, this ovverides the automatic value loading in generatePropertyEditor() */
		QVariant value;
		
		/** If true, generatePropertyEditor() assumes that the double it is editing is in the range of 0-1 and expresses it to the user as 0% - 100% for use in a slider/spinbox. */
		bool doubleIsPercentage;
		
		/** The default value which generatePropertyEditor() will reset back to if the user clicks the 'X' button displayed for ints/doubles. */
		QVariant defaultValue;
		
		/** Sets the maximum length expected for a QString. \todo Implement internally. */
		int maxLength;
		
		/** If true and type is a QVariant::String, generatePropertyEditor() will configure the text field for a directory lookup auto-complete mode and add a 'Browse' button and dialog. */ 
		bool stringIsFile;
		
		/** If stringIsFile is true, you can set the type of files to be loaded. For example:
			\code
				PropertyEditorFactory::PropertyEditorOptions opts;
				opts.stringIsFile = true;
				opts.fileTypeFilter = tr("Image Files (*.png *.jpg *.bmp *.svg *.xpm *.gif);;Any File (*.*)");
			\endcode
		*/
		QString fileTypeFilter;
		
		/** If true and type is a QVariant::String, generatePropertyEditor() will configure the text field for a directory lookup auto-complete mode and add a 'Browse' button and dialog configured to choose a directory. */ 
		bool stringIsDir;
	};
	
	/** Attempts to magically guess the title for a string based on a simple set of string replacements and basic logic.
		For example, given the following inputs, it will return the strings shown below:
			-# \em zipCode = Zip Code
			-# \em image_file = Image File
			-# \em friend1Name = Friend #1 Name
	*/ 
	static QString guessTitle(QString value);

	/** Attempts to automagically generate a suitable editing widget based on the \a property of \a object and connect to the given \a slot to automatically set the value 
		on the \a object. Uses \a opts to control how the returned widget is setup and displayed. 
		
		If \a changeSignal is provided, a listener will be connected to the \a object and listen for the given \a changeSignal and update the editor when the signal is emitted. 
		
		@warning Be careful when using a \a changeSignal as it can create a loop which will frustrate the user (I know, I've frustrated myself with it!) For example, if you have a 
		'text' QString property and the object emits a 'textChanged(QString)' signal - dont use it! 
		
		What will happen is as soon as the user types a letter, the \a slot will be called to set the new text, and then the 'textChanged()' signal is emitted, which will 
		update the editor with the new text - which is fine if all your doing is appending to existing text. But if the user's cursor is in the middle of the text field,
		the line edit will set the new text and jump the cursor to the end of the field - so when the user presses the next key, it will be appended instead of inserted.
		
		\todo I suppose this could be fixed by adding an option to PropertyEditorOptions which changes it from a 'set text as soon as key pressed' to 'set text when enter pressed' mode.
		
		It will return a widget which may contain a layout (multiple widgets, such as input + slider), or it may be just the widget itself - such as a QLineEdit. Either way,
		you can safely just treat the resulting widget as a black box and blindly add it to a layout - nothing needs to be done with the widdget other than show it to the user. 
	*/
	static QWidget * generatePropertyEditor(QObject *object, const char *property, const char *slot, PropertyEditorOptions opts = PropertyEditorOptions(), const char *changeSignal=0);

};

/** \class ObjectValueSetter
	Used by PropertyEditorFactory to emit a signal when a slot is called - for example, when a QPushButton is clicked, emit a signal with a predefined value.
*/ 
class ObjectValueSetter : public QObject
{
	Q_OBJECT
public:
	ObjectValueSetter(QObject *attached, const char *slot, QVariant value);
	
public slots:
	void executeSetValue();
	
signals:
	void setValue(int);
	void setValue(double);
	void setValue(const QString&);

private:
	QVariant m_value;
};


/** \class EditorUpdateAdapter
*/ 
class EditorUpdateAdapter : public QObject
{
	Q_OBJECT
public:
	EditorUpdateAdapter(QWidget *editor);
	
	static EditorUpdateAdapter *getAdapterFromObject(QObject *object, const char *propBaseName);
	 
	QWidget *widget() { return m_editor; }
	
// public slots:
// 	void setValue(QVariant);
	
private:
	QWidget *m_editor;
};

// Q_DECLARE_METATYPE(EditorUpdateAdapter)

/** \class BrowseDialogLauncher
	Launch a QFileDialog in response to a slot with a predefined title and filter and emit a signal when the dialog is closed with the chosen file. 
	Used by PropertyEditorFactory to implement the 'browse' functionality.
*/  
class BrowseDialogLauncher : public QObject
{
	Q_OBJECT
public:
	BrowseDialogLauncher(QObject *attached, const char *slot, QVariant value, bool isFileBrowser=true);
	
public slots:
	void browse();
	
	void setTitle(const QString&);
	void setSettingsKey(const QString&);
	void setFilter(const QString&);
	
signals:
	void setValue(const QString&);

private:
	QObject * m_attached;
	QVariant m_value;
	QString m_settingsKey;
	QString m_title;
	QString m_filter;
	bool m_isFileBrowser;
};


class QSpinBox;
class QDoubleSpinBox;

/** \class PropertyChangeListener
	Listens for a signal and emits a signal with the QVariant properly cast to the proper type. Used internally by PropertyChangeListener when the changeSlot given emits a QVariant and 
	the setter slot given requires a Qt type such as a QSize or QPoint - this class casts the QVariant to the proper type before emitting the signal.
	
	This class takes care of connecting the source and receiver internally - PropertyChangeListener does not handle the connection logic.
*/
class PropertyChangeListener : public QObject
{
	Q_OBJECT
public:
	PropertyChangeListener(QObject *source, const char *changeSignal, QObject *receiver, const char *receiverSlot, QVariant value, QString propertyName = "");
	
signals:
	void value(int);
	void value(bool);
	void value(double);
	void value(const QString&);
	void value(const QSize&);
	void value(const QSizeF&);
	void value(const QPoint&);
	void value(const QPointF&);
	
private slots:
	void receiver(const QString&, const QVariant&);
	void receiver(const QVariant&);
	
private:
	QVariant m_value;
	QString m_property;
};

/** \class DoubleEditorWidget
	Used internally by PropertyChangeListener to handle editing QVariant::Double values.
*/
class DoubleEditorWidget : public QWidget
{
	Q_OBJECT
public:
	DoubleEditorWidget(QWidget *parent=0);

public slots:
	void setValue(double);
	void setMinMax(double,double);
	void setShowSlider(bool);
	void setSuffix(const QString&);

signals:
	void valueChanged(double);

private slots:
	void sliderValueChanged(int);
	void boxValueChanged(double);

private:
	double m_value;
	QSlider *m_slider;
	QDoubleSpinBox *m_box;

};

/** \class PointEditorWidget
	Used internally by PropertyChangeListener to edit QPointF and QPoint values
*/
class PointEditorWidget : public QWidget
{
	Q_OBJECT
public:
	PointEditorWidget(QWidget *parent=0);

public slots:
	void setValue(const QPointF&);
	void setXMinMax(int,int);
	void setYMinMax(int,int);
	void setSufix(const QString&);
	
	void reset();

signals:
	void valueChanged(const QPointF&);

private slots:
	void xValueChanged(int);
	void yValueChanged(int);

private:
	QPointF m_point;
	QSpinBox *x_box;
	QSpinBox *y_box;
	QPointF m_orig;

};

/** \class SizeEditorWidget
	Used internally by PropertyChangeListener to edit QSize and QSizeF values.
*/
class SizeEditorWidget : public QWidget
{
	Q_OBJECT
public:
	SizeEditorWidget(QWidget *parent=0);

public slots:
	void setValue(const QSizeF&);
	void setWMinMax(int,int);
	void setHMinMax(int,int);
	void setSufix(const QString&);
	
	void reset();

signals:
	void valueChanged(const QSizeF&);

private slots:
	void wValueChanged(int);
	void hValueChanged(int);


private:
	QSizeF m_size;
	QSpinBox *w_box;
	QSpinBox *h_box;
	QSizeF m_orig;
	bool m_lockOrig;
	bool m_lockValue;
};

/*class RectEditorWidget : public QWidget
{
	Q_OBJECT
public:
	RectEditorWidget(QWidget *parent=0);

public slots:
	void setValue(const QRectF&);
	void setXMinMax(int,int);
	void setYMinMax(int,int);
	void setWMinMax(int,int);
	void setHMinMax(int,int);
	void setSufix(const QString&);
	
	void reset();

signals:
	void valueChanged(const QRectF&);

private slots:
	void wValueChanged(double);
	void hValueChanged(double);
	void xValueChanged(double);
	void yValueChanged(double);


private:
	QRectF m_rect;
	QSpinBox *w_box;
	QSpinBox *h_box;
	QSpinBox *x_box;
	QSpinBox *y_box;
	QRectF m_orig;
};*/
/*
class ColorEditorWidget : public QWidget
{
	Q_OBJECT
public:
	ColorEditorWidget(QWidget *parent=0);

public slots:
	void setValue(const QColor&);

signals:
	void valueChanged(const QColor&);

private slots:
	void rValueChanged(int);
	void gValueChanged(int);
	void bValueChanged(int);

private:
	QColor m_point;
	QSpinBox *r_box;
	QSpinBox *g_box;
	QSpinBox *b_box;
};*/



#endif

