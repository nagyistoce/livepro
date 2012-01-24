#ifndef EditorWindow_H
#define EditorWindow_H

#include <QtGui>

class GLEditorGraphicsScene;
class GLSceneGroupCollection;
class GLSceneGroup;
class GLScene;
class GLDrawable;
class RichTextEditorDialog;
class EditorGraphicsView;
class GLImageDrawable;

class EditorWindow : public QMainWindow
{
	Q_OBJECT
public:
	EditorWindow(QWidget *parent=0);
	~EditorWindow();
	
	GLSceneGroup * group();
	GLScene *scene();
	
	const QSizeF& canvasSize() { return m_canvasSize; }
	
	// isStandalone: If true, then the window will attempt to load the first argument
	// 		on the command line as a group file, and save to it when closed.
	bool isStandalone() { return m_isStandalone; }
	
public slots:
	void setGroup(GLSceneGroup*, GLScene *currentScene=0);
	void setCurrentScene(GLScene *);
	
	void addVideoInput();
	void addVideoLoop();
	void addVideoFile();
	void addImage();
	void addSvg();
	void addText(const QString& text="");
	void addRect();
	void addMjpeg();
	void addSpinner();
	void addHttpImage();
	
	void addScene();
	void delScene();
	void dupScene();
	
	void setCanvasSize(const QSizeF&);
	
	void setIsStandalone(bool);
	
	void centerSelectionHorizontally();
	void centerSelectionVertically();
	
	void textFitNaturally();
	void textPlus();
	void textMinus();
	
	void naturalItemFit();
	
protected slots:
	void slideSelected(const QModelIndex &);
	void currentSlideChanged(const QModelIndex &idx,const QModelIndex &);
	
	void selectionChanged();
	
	void drawableAdded(GLDrawable*);
	void drawableRemoved(GLDrawable*);
	
	void rtfEditorSave();
	
	void textSizeBoxChanged();
	
	void graphicsSceneChanged ( const QList<QRectF> & region );
	
	void slideProperties();
	
protected:
	void closeEvent(QCloseEvent *event);
	
	void createUI();
	void readSettings();
	void writeSettings();
	
	void addDrawable(GLDrawable*);
	
	QWidget *createPropertyEditors(GLDrawable*);
	
	void addImageBorderShadowEditors(QFormLayout *lay, GLImageDrawable *item);
	
	QSizeF m_canvasSize;
	GLEditorGraphicsScene *m_graphicsScene;

	GLSceneGroup *m_group;
	GLScene *m_scene;
	
	QTableView *m_slideList;
	QListView  *m_layoutList;
	
	QSplitter *m_mainSplitter;
	QSplitter *m_centerSplitter;
	QSplitter *m_sideSplitter;
	
	QScrollArea *m_toolList;
	QToolBar *m_toolbar;
	
	QScrollArea *m_controlArea;	
	QWidget *m_controlBase;
	QWidget *m_currentLayerPropsEditor;
	
	RichTextEditorDialog *m_rtfEditor;
	GLDrawable *m_currentDrawable;

	EditorGraphicsView *m_graphicsView;
	
	QString m_fileName;
	
	bool m_isStandalone;
	GLSceneGroupCollection *m_collection;
	
	QPointer<QDoubleSpinBox> m_textSizeBox;
};


#endif
