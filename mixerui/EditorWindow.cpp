#include "EditorWindow.h"

#include "GLSceneGroup.h"
#include "GLEditorGraphicsScene.h"
#include "GLDrawables.h"

#include "../livemix/ExpandableWidget.h"
#include "../livemix/EditorUtilityWidgets.h"

#include "../3rdparty/richtextedit/richtexteditor_p.h"
#include "../qtcolorpicker/qtcolorpicker.h"

#include "../ImageFilters.h"

#include "RtfEditorWindow.h"
#include "EditorGraphicsView.h"

#include "ScenePropertiesDialog.h"

#include <QApplication>

#define TOOLBAR_TEXT_SIZE_INC 4

EditorWindow::EditorWindow(QWidget *parent)
	: QMainWindow(parent)
	, m_canvasSize(1000.,750.)
	, m_graphicsScene(0)
	, m_group(0)
	, m_scene(0)
	, m_currentLayerPropsEditor(0)
	, m_rtfEditor(0)
	, m_currentDrawable(0)
	, m_isStandalone(false)
{
	createUI();
	setCanvasSize(m_canvasSize);
	
	setWindowTitle(tr("GLEditor"));
	setUnifiedTitleAndToolBarOnMac(true);
	
	readSettings();
}

void EditorWindow::setIsStandalone(bool flag)
{
	m_isStandalone = flag;
	
	m_collection = new GLSceneGroupCollection();
			
	if(flag && m_fileName.isEmpty())
	{
		QStringList argList = qApp->arguments();
		if(argList.size() > 1)
		{
			QString fileArg = argList.at(1);
			if(!fileArg.isEmpty())
			{
				m_fileName = fileArg;
				
				if(!m_collection->readFile(fileArg))
					qDebug() << "EditorWindow: [DEBUG]: Unable to load"<<fileArg<<", created new scene";
				else
					qDebug() << "EditorWindow: [DEBUG]: Loaded File: "<<fileArg;
			}
		}
	}
	
	if(m_collection->size() <= 0)
	{
		GLSceneGroup *group = new GLSceneGroup();
		group->addScene(new GLScene());
		m_collection->addGroup(group);
	}
	
	setGroup(m_collection->at(0));
}

EditorWindow::~EditorWindow()
{
}


void EditorWindow::closeEvent(QCloseEvent *event)
{
	if(m_graphicsScene)
		m_graphicsScene->setEditingMode(false);
		
	writeSettings();
	event->accept();
	
	if(m_isStandalone && 
	   m_collection)
	{
		if(m_fileName.isEmpty())
			m_fileName = QFileDialog::getSaveFileName(this, tr("Choose a Filename"), m_fileName, tr("GLDirector File (*.gld);;Any File (*.*)"));
			
		if(!m_fileName.isEmpty())
		{
			m_collection->writeFile(m_fileName);
			qDebug() << "EditorWindow: Debug: Saved CollectionID: "<< m_collection->collectionId();
		}
	}
}


void EditorWindow::readSettings()
{
	QSettings settings;
	QPoint pos = settings.value("EditorWindow/pos", QPoint(10, 10)).toPoint();
	QSize size = settings.value("EditorWindow/size", QSize(640,480)).toSize();
	move(pos);
	resize(size);

	m_mainSplitter->restoreState(settings.value("EditorWindow/main_splitter").toByteArray());
	m_centerSplitter->restoreState(settings.value("EditorWindow/center_splitter").toByteArray());
	m_sideSplitter->restoreState(settings.value("EditorWindow/side_splitter").toByteArray());
	
	qreal scaleFactor = (qreal)settings.value("EditorWindow/scaleFactor", 1.).toDouble();
	m_graphicsView->setScaleFactor(scaleFactor);

}

void EditorWindow::writeSettings()
{
	QSettings settings;
	settings.setValue("EditorWindow/pos", pos());
	settings.setValue("EditorWindow/size", size());

	settings.setValue("EditorWindow/main_splitter",m_mainSplitter->saveState());
	settings.setValue("EditorWindow/center_splitter",m_centerSplitter->saveState());
	settings.setValue("EditorWindow/side_splitter",m_sideSplitter->saveState());
	
	settings.setValue("EditorWindow/scaleFactor",m_graphicsView->scaleFactor());
}


void EditorWindow::createUI()
{
	// Create splitters
	m_mainSplitter = new QSplitter();
	m_centerSplitter = new QSplitter();
	m_sideSplitter = new QSplitter();
	
	// Setup splitter orientations
	m_mainSplitter->setOrientation(Qt::Horizontal);
	m_centerSplitter->setOrientation(Qt::Vertical);
	m_sideSplitter->setOrientation(Qt::Vertical);
	
	// Create two main lsit views
	m_slideList  = new QTableView();
	m_layoutList = new QListView();
	
	//m_slideList->setViewMode(QListView::ListMode);
	
	//m_layerListView->setViewMode(QListView::IconMode);
//	m_slideList->setMovement(QListView::Free);
//	m_slideList->setWordWrap(true);
//	m_slideList->setSelectionMode(QAbstractItemView::ExtendedSelection);
// 	m_slideList->setDragEnabled(true);
// 	m_slideList->setAcceptDrops(true);
// 	m_slideList->setDropIndicatorShown(true);

	connect(m_slideList, SIGNAL(activated(const QModelIndex &)), this, SLOT(slideSelected(const QModelIndex &)));
	connect(m_slideList, SIGNAL(clicked(const QModelIndex &)),   this, SLOT(slideSelected(const QModelIndex &)));

	// First, create left panel - the slide list, then everything on the right
	m_toolList = new QScrollArea(m_sideSplitter);
	m_toolList->setWidgetResizable(true);
	m_toolbar = new QToolBar("Items", m_toolList);
	m_toolbar->setOrientation(Qt::Vertical);
	m_toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	m_toolList->setWidget(m_toolbar);
	
	m_sideSplitter->addWidget(m_toolList);
	
	// Create the toolbar for the new slide/delete slide actions
	QWidget *slideListBase = new QWidget(m_sideSplitter);
	QVBoxLayout *slideListBaseLayout = new QVBoxLayout(slideListBase);
	slideListBaseLayout->setContentsMargins(0,0,0,0);
	
	QToolBar *sceneActionToolbar = new QToolBar("Scene Actions", slideListBase); 
	slideListBaseLayout->addWidget(sceneActionToolbar);
	slideListBaseLayout->addWidget(m_slideList);
	
	// Add the two toolbars and side splitter to the main splitter
	m_sideSplitter->addWidget(slideListBase);
	
	m_mainSplitter->addWidget(m_sideSplitter);
	
	// Now create the center panel - on top will be a list view, below it the graphics view
	m_graphicsView = new EditorGraphicsView();
	m_graphicsView->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
	
// 	m_graphicsScene = new GLEditorGraphicsScene();
// 	m_graphicsView->setScene(m_graphicsScene);
// 	m_graphicsScene->setSceneRect(QRectF(0,0,1000.,750.));
// 	connect(m_graphicsScene, SIGNAL(drawableSelected(GLDrawable*)), this, SLOT(drawableSelected(GLDrawable*)));
	
	// Add the layout list (top), and GV (bottom), then add the middle splitter to the right side splitter
	//m_centerSplitter->addWidget(m_layoutList);
	m_centerSplitter->addWidget(m_graphicsView);
	m_mainSplitter->addWidget(m_centerSplitter);
	
	// Now create the control base for the other half of the right side splitter
	QWidget *controlLayoutBase = new QWidget(m_mainSplitter);
	QVBoxLayout *controlLayout = new QVBoxLayout(controlLayoutBase);
	controlLayout->setContentsMargins(0,0,0,0);
	
	QToolBar *itemActionToolbar = new QToolBar("Item Actions", slideListBase); 
	controlLayout->addWidget(itemActionToolbar);
	
	m_controlArea = new QScrollArea(m_mainSplitter);
	m_controlArea->setWidgetResizable(true);
	controlLayout->addWidget(m_controlArea);
	
	m_controlBase = new QWidget(m_controlArea);
	
	QVBoxLayout *layout = new QVBoxLayout(m_controlBase);
	layout->setContentsMargins(0,0,0,0);
	
	m_controlArea->setWidget(m_controlBase);
	
	//m_mainSplitter->addWidget(m_controlArea);
	m_mainSplitter->addWidget(controlLayoutBase);
	
	setCentralWidget(m_mainSplitter);
	
	// Now create the toolbar that users can use to add items to the current slide
	QToolBar * toolbar = m_toolbar; //addToolBar(tr("Editor Actions"));
	
	
	QAction *act;
	
	act = new QAction(QIcon("../data/stock-panel-screenshot.png"), tr("Add Video Input"), this);
	connect(act, SIGNAL(triggered()), this, SLOT(addVideoInput()));
	toolbar->addAction(act);
	
	act = new QAction(QIcon("../data/stock-panel-multimedia.png"), tr("New Video Layer"), this);
	connect(act, SIGNAL(triggered()), this, SLOT(addVideoFile()));
	toolbar->addAction(act);
	
	act = new QAction(QIcon("../data/stock-insert-object.png"), tr("Add Video Loop"), this);
	connect(act, SIGNAL(triggered()), this, SLOT(addVideoLoop()));
	toolbar->addAction(act);
		
	act = new QAction(QIcon("../data/stock-font.png"), tr("New Text Layer"), this);
	connect(act, SIGNAL(triggered()), this, SLOT(addText()));
	toolbar->addAction(act);
	
	act = new QAction(QIcon("../data/stock-insert-image.png"), tr("New Image Layer"), this);
	connect(act, SIGNAL(triggered()), this, SLOT(addImage()));
	toolbar->addAction(act);
	
	act = new QAction(QIcon("../data/stock-jump-to.png"), tr("New SVG Layer"), this);
	connect(act, SIGNAL(triggered()), this, SLOT(addSvg()));
	toolbar->addAction(act);
	
	act = toolbar->addAction(QIcon(":/data/insert-rect-24.png"), tr("New Rectangle"));
	act->setShortcut(QString(tr("CTRL+SHIFT+R")));
	connect(act, SIGNAL(triggered()), this, SLOT(addRect()));
	
	act = toolbar->addAction(QIcon(":/data/stock-connect.png"), tr("Add MJPEG Feed"));
	act->setShortcut(QString(tr("CTRL+SHIFT+M")));
	connect(act, SIGNAL(triggered()), this, SLOT(addMjpeg()));
	
	act = toolbar->addAction(QIcon(":/data/stock-refresh.png"), tr("Add Spinner"));
	act->setShortcut(QString(tr("CTRL+SHIFT+S")));
	connect(act, SIGNAL(triggered()), this, SLOT(addSpinner()));
	
	act = toolbar->addAction(QIcon(":/data/stock-insert-image.png"), tr("Add HTTP Image"));
	act->setShortcut(QString(tr("CTRL+SHIFT+H")));
	connect(act, SIGNAL(triggered()), this, SLOT(addHttpImage()));
	
	//toolbar->addSeparator();
	toolbar = sceneActionToolbar;
	
	act = new QAction(QIcon("../data/action-add.png"), tr("Add New Slide"), this);
	connect(act, SIGNAL(triggered()), this, SLOT(addScene()));
	toolbar->addAction(act);
	
	act = new QAction(QIcon("../data/action-delete.png"), tr("Delete Current Slide"), this);
	connect(act, SIGNAL(triggered()), this, SLOT(delScene()));
	toolbar->addAction(act);
	
	act = new QAction(QIcon("../data/stock-convert.png"), tr("Duplicate Current Slide"), this);
	connect(act, SIGNAL(triggered()), this, SLOT(dupScene()));
	toolbar->addAction(act);
	
	toolbar->addSeparator();
	
	QAction  *slideProp = toolbar->addAction(QIcon(":/data/stock-properties.png"), tr("Scene Properties"));
	slideProp->setShortcut(QString(tr("SHIFT+F2")));
	connect(slideProp, SIGNAL(triggered()), this, SLOT(slideProperties()));

	//toolbar->addSeparator();
	toolbar = itemActionToolbar;
	//addToolBar(tr("Editor Actions"));
	
	QAction  *centerHor = toolbar->addAction(QIcon("../data/obj-center-hor.png"), tr("Center Items Horizontally"));
	centerHor->setShortcut(QString(tr("CTRL+SHIFT+H")));
	connect(centerHor, SIGNAL(triggered()), this, SLOT(centerSelectionHorizontally()));

	QAction  *centerVer = toolbar->addAction(QIcon("../data/obj-center-ver.png"), tr("Center Items Vertically"));
	centerVer->setShortcut(QString(tr("CTRL+SHIFT+V")));
	connect(centerVer, SIGNAL(triggered()), this, SLOT(centerSelectionVertically()));
	
	act = toolbar->addAction(QIcon("../data/stock-fit-in.png"), tr("Fit Items to Natural Size"));
	act ->setShortcut(QString(tr("CTRL+SHIFT+F")));
	connect(act , SIGNAL(triggered()), this, SLOT(naturalItemFit()));
	
	
	
}


void EditorWindow::setGroup(GLSceneGroup *group,  GLScene *currentScene)
{
	if(!group || group == m_group)
		return;
		
	m_group = group;
	
	//qDebug() << "EditorWindow::setGroup: group:"<<group<<", currentScene:"<<currentScene;
	
	// deleting old selection model per http://doc.trolltech.com/4.5/qabstractitemview.html#setModel
	QItemSelectionModel *m = m_slideList->selectionModel();

	m_slideList->setModel(group);
// 	connect(m_sceneModel, SIGNAL(repaintList()), this, SLOT(repaintLayerList()));
// 	connect(m_sceneModel, SIGNAL(layersDropped(QList<LiveLayer*>)), this, SLOT(layersDropped(QList<LiveLayer*>)));

	if(m)
	{
		disconnect(m, 0, this, 0);
		delete m;
		m=0;
	}

	QItemSelectionModel *currentSelectionModel = m_slideList->selectionModel();
	connect(currentSelectionModel, SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(currentSlideChanged(const QModelIndex &, const QModelIndex &)));
	
	if(!currentScene)
		currentScene = group->at(0);
		
	setCurrentScene(currentScene);
}

void EditorWindow::slideSelected(const QModelIndex &idx)
{
	if(idx.isValid())
		setCurrentScene(m_group->at(idx.row()));
}


void EditorWindow::currentSlideChanged(const QModelIndex &idx,const QModelIndex &)
{
	if(idx.isValid())
		slideSelected(idx);
}


void EditorWindow::setCurrentScene(GLScene *scene)
{
	//qDebug() << "EditorWindow::setCurrentScene: [a] "<<scene;
	if(!scene || scene == m_scene)
		return;
	
	if(m_scene)
	{
		disconnect(m_scene, 0, this, 0);
		m_scene = 0;
	}
	if(m_graphicsScene)
	{
		disconnect(m_graphicsScene, 0, this, 0);
		m_graphicsScene->setEditingMode(false);
		m_graphicsScene = 0;
	}
	
	//qDebug() << "EditorWindow::setCurrentScene: "<<scene;
	m_scene = scene;
	
	if(m_scene)
	{
		if(!dynamic_cast<GLEditorGraphicsScene*>(m_scene->graphicsScene()))
			m_scene->setGraphicsScene(new GLEditorGraphicsScene);
		m_graphicsScene = dynamic_cast<GLEditorGraphicsScene*>(m_scene->graphicsScene());
		m_graphicsScene->setEditingMode(true);
		m_graphicsView->setScene(m_graphicsScene);
		m_graphicsScene->setSceneRect(QRectF(0,0,1000.,750.));
		connect(m_graphicsScene, SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));
		connect(m_graphicsScene, SIGNAL(changed ( const QList<QRectF> & )), this, SLOT(graphicsSceneChanged ( const QList<QRectF> & )));
		
  		GLDrawableList list = m_scene->drawableList();
// 		
// 		m_graphicsScene->clear();
// 		
// 		foreach(GLDrawable *drawable, list)
// 			m_graphicsScene->addItem(drawable);
		
// 		connect(m_scene, SIGNAL(drawableAdded(GLDrawable*)), this, SLOT(drawableAdded(GLDrawable*)));
// 		connect(m_scene, SIGNAL(drawableRemoved(GLDrawable*)), this, SLOT(drawableRemoved(GLDrawable*)));
		
		if(!list.isEmpty())
		{
			m_graphicsScene->clearSelection();
			list.first()->setSelected(true);
		}
	}
}

void EditorWindow::graphicsSceneChanged ( const QList<QRectF> & )
{
	if(!m_graphicsScene)
		return;
	if(!m_scene)
		return;
	QPixmap pixmap;
	
	QSize iconSize = m_graphicsScene->sceneRect().size().toSize();
	iconSize.scale(32,32, Qt::KeepAspectRatio);
	
	int icon_w = iconSize.width();
	int icon_h = iconSize.height();
		
	QPixmap icon(icon_w,icon_h);
	QPainter painter(&icon);
	painter.fillRect(0,0,icon_w,icon_h,Qt::white);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setRenderHint(QPainter::TextAntialiasing, true);
	
	m_graphicsScene->render(&painter,QRectF(0,0,icon_w,icon_h),m_graphicsScene->sceneRect());
	painter.setPen(Qt::black);
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(0,0,icon_w-1,icon_h-1);
	
	m_scene->setPixmap(pixmap);
}

void EditorWindow::drawableAdded(GLDrawable */*d*/)
{
	//qDebug() << "EditorWindow::drawableAdded: "<<(QObject*)d;
	//m_graphicsScene->addItem(d);
	graphicsSceneChanged(QList<QRectF>());
}

void EditorWindow::drawableRemoved(GLDrawable */*d*/)
{
	//m_graphicsScene->removeItem(d);
	graphicsSceneChanged(QList<QRectF>());
}

GLSceneGroup * EditorWindow::group()
{
	if(!m_group)
		setGroup(new GLSceneGroup());
	
	return m_group;
}

GLScene *EditorWindow::scene()
{
	if(!m_scene)
	{
		GLScene *scene = group()->at(0);
		if(scene)
			setCurrentScene(scene);
	}
		
	if(!m_scene)
	{
		GLScene *scene = new GLScene();
		m_group->addScene(scene);
		setCurrentScene(scene);
	}
	
	return m_scene;
}	
	
void EditorWindow::addDrawable(GLDrawable *drawable)
{
	drawable->addShowAnimation(GLDrawable::AnimFade);
	//double idx = scene()->size();
	GLDrawableList list = scene()->drawableList();
	double max = 0;
	foreach(GLDrawable *gld, list)
		if(gld->zIndex() > max)
			max = gld->zIndex();
	//qDebug() << "EditorWindow::addDrawable(): drawable:"<<(QObject*)drawable<<", idx:"<<idx;
	drawable->setZIndex(max+1.);
	scene()->addDrawable(drawable);
	
	if(m_graphicsScene)
	{
		drawable->setRect(m_graphicsScene->sceneRect());
		m_graphicsScene->setEditingMode(true);
		m_graphicsScene->clearSelection();
	}
	
	drawable->setSelected(true);
	drawable->show();
}

void EditorWindow::addVideoInput()
{
	GLVideoInputDrawable *drawable = new GLVideoInputDrawable("/dev/video0");
	drawable->setCardInput("S-Video");
	addDrawable(drawable);
}

void EditorWindow::addVideoLoop()
{
	addDrawable(new GLVideoLoopDrawable("../data/Seasons_Loop_3_SD.mpg"));
}

void EditorWindow::addVideoFile()
{
	addDrawable(new GLVideoFileDrawable("../data/Seasons_Loop_3_SD.mpg"));
	//addDrawable(new GLVideoLoopDrawable("../data/Seasons_Loop_3_SD.mpg"));
}

void EditorWindow::addImage()
{
	addDrawable(new GLImageDrawable("Pm5544.jpg"));
}

void EditorWindow::addSvg()
{
	addDrawable(new GLSvgDrawable("animated-clock.svg"));
}

void EditorWindow::addMjpeg()
{
	addDrawable(new GLVideoMjpegDrawable());
}

void EditorWindow::addSpinner()
{
	GLSpinnerDrawable *rect = new GLSpinnerDrawable();
	addDrawable(rect);
	
	if(m_graphicsScene)
	{
		QRectF r = m_graphicsScene->sceneRect();
		// Make square
		r = QRectF(r.width() * .25, r.height() * .25, r.width() * .5, r.width() * .5);
		rect->setRect(r);
	}
}

void EditorWindow::addHttpImage()
{
	GLImageHttpDrawable *rect = new GLImageHttpDrawable();
	addDrawable(rect);
	
	if(m_graphicsScene)
	{
		QRectF r = m_graphicsScene->sceneRect();
		r = QRectF(r.width() * .25, r.height() * .25, r.width() * .5, r.height() * .5);
		rect->setRect(r);
	}
}

void EditorWindow::addRect()
{
	GLRectDrawable *rect = new GLRectDrawable();
	addDrawable(rect);
	
	if(m_graphicsScene)
	{
		QRectF r = m_graphicsScene->sceneRect();
		r = QRectF(r.width() * .25, r.height() * .25, r.width() * .5, r.height() * .5);
		rect->setRect(r);
	}
}

void EditorWindow::addText(const QString& tmp)
{
	QString str = tmp;
	if(str.isEmpty())
		//str = "<span style='font-size:68px;color:white'><b>Lorem Ipsum</b></span>";
		str = 
			"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN" "http://www.w3.org/TR/REC-html40/strict.dtd\">"
			"<html><head><meta name=\"qrichtext\" content=\"1\"/>"
			"<style type=\"text/css\">p, li { white-space: pre-wrap; }</style>"
			"</head>"
			"<body style=\"font-family:'Sans Serif'; font-size:68pt; font-weight:600; font-style:normal;\">"
			"<table style=\"-qt-table-type: root; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px;\">"
			"<tr><td style=\"border: none;\">"
			"<p style=\"margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">"
			"<span style=\"font-size:68pt; font-weight:600; color:#ffffff;\">"
			"Lorem Ipsum"
			"</span></p></td></tr></table></body></html>";

	GLTextDrawable *text = new GLTextDrawable(str);
	addDrawable(text);
	textFitNaturally();
	centerSelectionHorizontally();
	centerSelectionVertically();
}

void EditorWindow::textFitNaturally()
{
	if(!m_currentDrawable)
		return;
		
	if(GLTextDrawable *text = dynamic_cast<GLTextDrawable*>(m_currentDrawable))
	{
		QSizeF size = text->findNaturalSize((int)m_graphicsScene->sceneRect().width());
		
		if(text->isShadowEnabled())
		{
			QSizeF blurSize = ImageFilters::blurredSizeFor(size, (int)text->shadowBlurRadius());
			blurSize.rwidth()  += fabs(text->shadowOffset().x());
			blurSize.rheight() += fabs(text->shadowOffset().y());
			size = blurSize;
		}
		
		text->setRect(QRectF(text->rect().topLeft(),size));
	}
}

void EditorWindow::textPlus()
{
	if(!m_textSizeBox)
		return;
	
	double value = m_textSizeBox->value();
	value += TOOLBAR_TEXT_SIZE_INC;
	m_textSizeBox->setValue(value);
	textSizeBoxChanged();
}

void EditorWindow::textMinus()
{
	if(!m_textSizeBox)
		return;

	double value = m_textSizeBox->value();
	value -= TOOLBAR_TEXT_SIZE_INC;
	m_textSizeBox->setValue(value);
	textSizeBoxChanged();

}
void EditorWindow::textSizeBoxChanged()
{
	if(!m_textSizeBox)
		return;
	
	double pt = m_textSizeBox->value();
	
	if(!m_currentDrawable)
		return;
		
	if(GLTextDrawable *text = dynamic_cast<GLTextDrawable*>(m_currentDrawable))
	{
		text->changeFontSize(pt);
		textFitNaturally();
	}
}

void EditorWindow::selectionChanged()
{
	if(!m_graphicsScene)
		return;
	QList<GLDrawable*> list = m_graphicsScene->selectedDrawables();
	if(list.isEmpty())
		return;
		
	GLDrawable *d = list.first();
	
	if(m_currentLayerPropsEditor)
	{
		m_controlBase->layout()->removeWidget(m_currentLayerPropsEditor);
		m_currentLayerPropsEditor->deleteLater();
		m_currentLayerPropsEditor = 0;
	}

	//m_currentLayer->lockLayerPropertyUpdates(true);
	QWidget *props = createPropertyEditors(d);
	//m_currentLayer->lockLayerPropertyUpdates(false);

	/// TODO What am I doing here?? This should have already been deleted above.
	QVBoxLayout *layout = dynamic_cast<QVBoxLayout*>(m_controlBase->layout());
	while(layout->count() > 0)
	{
		QLayoutItem * item = layout->itemAt(layout->count() - 1);
		layout->removeItem(item);
		delete item;
	}

	layout->addWidget(props);
	layout->addStretch(1);

	m_currentLayerPropsEditor = props;
}

void EditorWindow::addImageBorderShadowEditors(QFormLayout *lay, GLImageDrawable *item)
{
	QtColorPicker * borderColor = new QtColorPicker;
	borderColor->setStandardColors();
	borderColor->setCurrentColor(item->borderColor());
	connect(borderColor, SIGNAL(colorChanged(const QColor &)), item, SLOT(setBorderColor(QColor)));
	
	QHBoxLayout *hbox = new QHBoxLayout();
	QDoubleSpinBox *box = new QDoubleSpinBox();
	box->setSuffix(tr("px"));
	box->setMinimum(0);
	box->setDecimals(2);
	box->setMaximum(50);
	box->setValue(item->borderWidth());
	//connect(m_textSizeBox, SIGNAL(returnPressed()), this, SLOT(textSizeChanged(double)));
	connect(box, SIGNAL(valueChanged(double)), item, SLOT(setBorderWidth(double)));
	hbox->addWidget(box);
	hbox->addWidget(borderColor);
	
	lay->addRow(tr("Border:"), hbox);
	
	QCheckBox *checkbox = new QCheckBox("Enable Dropshadow");
	checkbox->setChecked(item->isShadowEnabled());
	connect(checkbox, SIGNAL(toggled(bool)), item, SLOT(setShadowEnabled(bool)));
	lay->addRow(checkbox);


	QtColorPicker * shadowColor = new QtColorPicker;
	shadowColor->setStandardColors();
	shadowColor->setCurrentColor(item->shadowColor());
	connect(shadowColor, SIGNAL(colorChanged(const QColor &)), item, SLOT(setShadowColor(QColor)));
	
	connect(checkbox, SIGNAL(toggled(bool)), shadowColor, SLOT(setEnabled(bool)));
	shadowColor->setEnabled(item->isShadowEnabled());
	
	lay->addRow(tr("Color:"), shadowColor);
	
	
	PropertyEditorFactory::PropertyEditorOptions opts;
	QWidget *widget = 0;
	
	
// 	QDoubleSpinBox *box2 = new QDoubleSpinBox();
// 	box2->setSuffix(tr("px"));
// 	box2->setMinimum(1);
// 	box2->setDecimals(2);
// 	box2->setMaximum(16);
// 	box2->setValue(item->shadowBlurRadius());
// 	//connect(m_textSizeBox, SIGNAL(returnPressed()), this, SLOT(textSizeChanged(double)));
// 	connect(box2, SIGNAL(valueChanged(double)), item, SLOT(setShadowBlurRadius(double)));
// 	connect(checkbox, SIGNAL(toggled(bool)), box2, SLOT(setEnabled(bool)));
// 	box2->setEnabled(item->isShadowEnabled());


	opts.reset();
	opts.type = QVariant::Int;
	opts.min = 1;
	opts.max = 30;
	opts.step = 2;
	opts.suffix = " px";
	widget = PropertyEditorFactory::generatePropertyEditor(item, "shadowBlurRadius", SLOT(setShadowBlurRadius(int)), opts);
	connect(checkbox, SIGNAL(toggled(bool)), widget, SLOT(setEnabled(bool)));
	widget->setEnabled(item->isShadowEnabled());
	
	lay->addRow(tr("Blur:"), widget);
	
	opts.reset();
	opts.type = QVariant::Int;
	opts.min = 1;
	opts.max = 400;
	opts.step = 5;
	opts.suffix = "%";
	opts.doubleIsPercentage = true;
	widget = PropertyEditorFactory::generatePropertyEditor(item, "shadowOpacity", SLOT(setShadowOpacity(int)), opts);
	connect(checkbox, SIGNAL(toggled(bool)), widget, SLOT(setEnabled(bool)));
	widget->setEnabled(item->isShadowEnabled());
	
	lay->addRow(tr("Opacity:"), widget);
	
	opts.reset();
	widget = PropertyEditorFactory::generatePropertyEditor(item, "shadowOffset", SLOT(setShadowOffset(QPointF)), opts);
	connect(checkbox, SIGNAL(toggled(bool)), widget, SLOT(setEnabled(bool)));
	widget->setEnabled(item->isShadowEnabled());
	
	lay->addRow(tr("Position:"), widget);
}

QWidget *EditorWindow::createPropertyEditors(GLDrawable *gld)
{
	m_currentDrawable = gld;
	
	QWidget * base = new QWidget();
	QVBoxLayout *blay = new QVBoxLayout(base);
	blay->setContentsMargins(0,0,0,0);


	PropertyEditorFactory::PropertyEditorOptions opts;

	opts.reset();
	
	{
		ExpandableWidget *group = new ExpandableWidget("Basic Settings",base);
		blay->addWidget(group);
	
		QWidget *cont = new QWidget;
		QFormLayout *lay = new QFormLayout(cont);
		lay->setContentsMargins(3,3,3,3);
		
		group->setWidget(cont);
		
		lay->addRow(tr("&Item Name:"), PropertyEditorFactory::generatePropertyEditor(gld, "itemName", SLOT(setItemName(const QString&)), opts));
		lay->addRow(PropertyEditorFactory::generatePropertyEditor(gld, "userControllable", SLOT(setUserControllable(bool)), opts));
		
		opts.reset();
		opts.suffix = "%";
		opts.min = 0;
		opts.max = 100;
		opts.defaultValue = 100;
		opts.type = QVariant::Int;
		opts.doubleIsPercentage = true;
		lay->addRow(tr("&Opacity:"), PropertyEditorFactory::generatePropertyEditor(gld, "opacity", SLOT(setOpacity(int)), opts));
		
		opts.reset();
		opts.noSlider = true;
		opts.type = QVariant::Int;
		opts.defaultValue = 0;
		lay->addRow(tr("&Z Value:"), PropertyEditorFactory::generatePropertyEditor(gld, "zIndex", SLOT(setZIndex(int)), opts));
		
	}
	
	{
		ExpandableWidget *group = new ExpandableWidget("Size and Position",base);
		blay->addWidget(group);
	
		QWidget *cont = new QWidget;
		QFormLayout *lay = new QFormLayout(cont);
		lay->setContentsMargins(3,3,3,3);
		
		group->setWidget(cont);
		
		opts.reset();
		opts.value = gld->rect().topLeft();
		
		lay->addRow(tr("&Position:"), PropertyEditorFactory::generatePropertyEditor(gld, "position", SLOT(setPosition(const QPointF&)), opts, SIGNAL(positionChanged(const QPointF&))));
		
		opts.reset();
		opts.value = gld->rect().size();
		
		lay->addRow(tr("&Size:"), PropertyEditorFactory::generatePropertyEditor(gld, "size", SLOT(setSize(const QSizeF&)), opts, SIGNAL(sizeChanged(const QSizeF&))));
		
// 		opts.reset();
// 		opts.suffix = "%";
// 		opts.min = 0;
// 		opts.max = 100;
// 		opts.defaultValue = 100;
// 		opts.type = QVariant::Int;
// 		opts.doubleIsPercentage = true;
// 		lay->addRow(tr("&Opacity:"), PropertyEditorFactory::generatePropertyEditor(gld, "opacity", SLOT(setOpacity(int)), opts));
// 		
// 		opts.reset();
// 		opts.noSlider = true;
// 		opts.type = QVariant::Int;
// 		opts.defaultValue = 0;
// 		lay->addRow(tr("&Z Value:"), PropertyEditorFactory::generatePropertyEditor(gld, "zIndex", SLOT(setZIndex(int)), opts));
		
	}
	
	
	{
		ExpandableWidget *group = new ExpandableWidget("Item Setup",base);
		blay->addWidget(group);
	
		QWidget *cont = new QWidget;
		QFormLayout *lay = new QFormLayout(cont);
		lay->setContentsMargins(3,3,3,3);
		
		group->setWidget(cont);
		
		
		if(GLVideoInputDrawable *item = dynamic_cast<GLVideoInputDrawable*>(gld))
		{
			// show device selection box
			
			opts.type = QVariant::Bool;
			opts.text = "Deinterlace video";
			lay->addRow(PropertyEditorFactory::generatePropertyEditor(item, "deinterlace", SLOT(setDeinterlace(bool)), opts));
			
			opts.text = "Flip horizontal";
			lay->addRow(PropertyEditorFactory::generatePropertyEditor(item, "flipHorizontal", SLOT(setFlipHorizontal(bool)), opts));
			
			opts.text = "Flip vertical";
			lay->addRow(PropertyEditorFactory::generatePropertyEditor(item, "flipVertical", SLOT(setFlipVertical(bool)), opts));
			
			opts.text = "Ignore video aspect ratio";
			lay->addRow(PropertyEditorFactory::generatePropertyEditor(item, "ignoreAspectRatio", SLOT(setIgnoreAspectRatio(bool)), opts));
			
		}
		else
		if(GLVideoLoopDrawable *item = dynamic_cast<GLVideoLoopDrawable*>(gld))
		{
		
			PropertyEditorFactory::PropertyEditorOptions opts;
			opts.stringIsFile = true;
			opts.fileTypeFilter = tr("Video Files (*.wmv *.mpeg *.mpg *.avi *.wmv *.flv *.mov *.mp4 *.m4a *.3gp *.3g2 *.mj2 *.mjpeg *.ipod *.m4v *.gsm *.swf *.dv *.dvd *.asf *.mtv *.roq *.aac *.ac3 *.aiff *.alaw *.iif);;Any File (*.*)");
			
			lay->addRow(tr("&File:"), PropertyEditorFactory::generatePropertyEditor(item, "videoFile", SLOT(setVideoFile(const QString&)), opts, SIGNAL(videoFileChanged(const QString&))));
			
			opts.text = "Ignore video aspect ratio";
			lay->addRow(PropertyEditorFactory::generatePropertyEditor(item, "ignoreAspectRatio", SLOT(setIgnoreAspectRatio(bool)), opts));
		}
		else
		if(GLVideoFileDrawable *item = dynamic_cast<GLVideoFileDrawable*>(gld))
		{
			PropertyEditorFactory::PropertyEditorOptions opts;
			opts.stringIsFile = true;
			opts.fileTypeFilter = tr("Video Files (*.wmv *.mpeg *.mpg *.avi *.wmv *.flv *.mov *.mp4 *.m4a *.3gp *.3g2 *.mj2 *.mjpeg *.ipod *.m4v *.gsm *.swf *.dv *.dvd *.asf *.mtv *.roq *.aac *.ac3 *.aiff *.alaw *.iif);;Any File (*.*)");
			
			lay->addRow(tr("&File:"), PropertyEditorFactory::generatePropertyEditor(item, "videoFile", SLOT(setVideoFile(const QString&)), opts, SIGNAL(videoFileChanged(const QString&))));
			
			opts.text = "Ignore video aspect ratio";
			lay->addRow(PropertyEditorFactory::generatePropertyEditor(item, "ignoreAspectRatio", SLOT(setIgnoreAspectRatio(bool)), opts));
		}
		else
		if(GLTextDrawable *item = dynamic_cast<GLTextDrawable*>(gld))
		{
			opts.reset();
			
// 			QHBoxLayout *countdown = new QHBoxLayout();
// 			QWidget *boolEdit = PropertyEditorFactory::generatePropertyEditor(gld, "isCountdown", SLOT(setIsCountdown(bool)), opts);
// 			QWidget *dateEdit  = PropertyEditorFactory::generatePropertyEditor(gld, "targetDateTime", SLOT(setTargetDateTime(QDateTime)), opts);
// 			QCheckBox *box = dynamic_cast<QCheckBox*>(boolEdit);
// 			if(box)
// 			{
// 				connect(boolEdit, SIGNAL(toggled(bool)), dateEdit, SLOT(setEnabled(bool)));
// 				dateEdit->setEnabled(box->isChecked());
// 			}
// 			countdown->addWidget(boolEdit);
// 			countdown->addWidget(dateEdit);
// 			countdown->addStretch(1);
// 			lay->addRow(countdown);
// 			
// 			m_rtfEditor = new RichTextEditorDialog(this);
// 			m_rtfEditor->setText(item->text());
// 			//m_editor->initFontSize(m_model->findFontSize());
// 			
// 			lay->addRow(m_rtfEditor);
// 			m_rtfEditor->adjustSize();
// 			m_rtfEditor->focusEditor();
// 			
// 			QPushButton *btn = new QPushButton("&Save Text");
// 			connect(btn, SIGNAL(clicked()), this, SLOT(rtfEditorSave()));
// 			
// 			QHBoxLayout *hbox = new QHBoxLayout();
// 			hbox->addStretch(1);
// 			hbox->addWidget(btn);
// 			lay->addRow(hbox); 
			
			QWidget *buttons = new QWidget();
			QHBoxLayout *buttonLayout = new QHBoxLayout(buttons);
			
			QPushButton *btn;
			btn = new QPushButton(QIcon("../data/stock-fit-in.png"),"");
			btn->setToolTip(tr("Fit Box to Text Naturally"));
			connect(btn, SIGNAL(clicked()), this, SLOT(textFitNaturally()));
			buttonLayout->addWidget(btn);
			
			btn = new QPushButton(QIcon(":/data/stock-sort-descending.png"), "");
			btn->setToolTip(tr("Increase Font Size"));
			//btn->setShortcut(QString(tr("CTRL+SHFIT++")));
			connect(btn, SIGNAL(clicked()), this, SLOT(textPlus()));
			buttonLayout->addWidget(btn);
			
			btn = new QPushButton(QIcon("../data/stock-sort-ascending.png"), "");
			btn->setToolTip(tr("Decrease Font Size"));
			//btn->setShortcut(QString(tr("CTRL+SHFIT+-")));
			connect(btn, SIGNAL(clicked()), this, SLOT(textMinus()));
			buttonLayout->addWidget(btn);
			
			m_textSizeBox = new QDoubleSpinBox(buttons);
			m_textSizeBox->setSuffix(tr("pt"));
			m_textSizeBox->setMinimum(1);
			m_textSizeBox->setValue(38);
			m_textSizeBox->setDecimals(1);
			m_textSizeBox->setMaximum(5000);
			m_textSizeBox->setValue(item->findFontSize());
			//connect(m_textSizeBox, SIGNAL(returnPressed()), this, SLOT(textSizeChanged(double)));
			connect(m_textSizeBox, SIGNAL(editingFinished()), this, SLOT(textSizeBoxChanged()));
			buttonLayout->addWidget(m_textSizeBox);
			
			buttonLayout->addStretch(1);
			lay->addRow(buttons);
			
			opts.reset();
		
			QWidget *edit = PropertyEditorFactory::generatePropertyEditor(gld, "plainText", SLOT(setPlainText(const QString&)), opts, SIGNAL(plainTextChanged(const QString&)));
			
// 			QLineEdit *line = dynamic_cast<QLineEdit*>(edit);
// 			if(line)
// 				connect(gld, SIGNAL(plainTextChanged(const QString&)), line, SLOT(setText(const QString&)));
			
			QWidget *base = new QWidget();
			
			RtfEditorWindow *dlg = new RtfEditorWindow(item, base);
			btn = new QPushButton("&Advanced...");
			connect(btn, SIGNAL(clicked()), dlg, SLOT(show()));
			
			QHBoxLayout *hbox = new QHBoxLayout(base);
			hbox->setContentsMargins(0,0,0,0);
			hbox->addWidget(new QLabel("Text:"));
			hbox->addWidget(edit);
			hbox->addWidget(btn);
			
			lay->addRow(base);
			 
			{
				ExpandableWidget *groupAnim = new ExpandableWidget("Border and Shadow",base);
				blay->addWidget(groupAnim);
			
				QWidget *groupAnimContainer = new QWidget;
				QFormLayout *lay = new QFormLayout(groupAnimContainer);
				lay->setContentsMargins(3,3,3,3);
			
				groupAnim->setWidget(groupAnimContainer);
			
				addImageBorderShadowEditors(lay,item);
				
				groupAnim->setExpandedIfNoDefault(true);
			}
		}
		else
		if(GLSvgDrawable *item = dynamic_cast<GLSvgDrawable*>(gld))
		{
			PropertyEditorFactory::PropertyEditorOptions opts;
			opts.stringIsFile = true;
			opts.fileTypeFilter = tr("SVG Files (*.svg);;Any File (*.*)");
			lay->addRow(tr("&SVG File:"), PropertyEditorFactory::generatePropertyEditor(item, "imageFile", SLOT(setImageFile(const QString&)), opts, SIGNAL(imageFileChanged(const QString&))));
			
			opts.text = "Ignore aspect ratio";
			lay->addRow(PropertyEditorFactory::generatePropertyEditor(item, "ignoreAspectRatio", SLOT(setIgnoreAspectRatio(bool)), opts));
			
			{
				ExpandableWidget *groupAnim = new ExpandableWidget("Border and Shadow",base);
				blay->addWidget(groupAnim);
			
				QWidget *groupAnimContainer = new QWidget;
				QFormLayout *lay = new QFormLayout(groupAnimContainer);
				lay->setContentsMargins(3,3,3,3);
			
				groupAnim->setWidget(groupAnimContainer);
			
				addImageBorderShadowEditors(lay,item);
				
				groupAnim->setExpandedIfNoDefault(true);
			}
		}
		else
		if(GLImageHttpDrawable *item = dynamic_cast<GLImageHttpDrawable*>(gld))
		{
			PropertyEditorFactory::PropertyEditorOptions opts;
			//opts.stringIsFile = true;
			//opts.fileTypeFilter = tr("SVG Files (*.svg);;Any File (*.*)");
			lay->addRow(tr("&Image URL:"), PropertyEditorFactory::generatePropertyEditor(item, "url", SLOT(setUrl(const QString&)), opts));//, SIGNAL(imageFileChanged(const QString&))));
			
			opts.text = "Is a DViz Server";
			lay->addRow(PropertyEditorFactory::generatePropertyEditor(item, "pollDviz", SLOT(setPollDviz(bool)), opts));
			
			{
				ExpandableWidget *groupAnim = new ExpandableWidget("Border and Shadow",base);
				blay->addWidget(groupAnim);
			
				QWidget *groupAnimContainer = new QWidget;
				QFormLayout *lay = new QFormLayout(groupAnimContainer);
				lay->setContentsMargins(3,3,3,3);
			
				groupAnim->setWidget(groupAnimContainer);
			
				addImageBorderShadowEditors(lay,item);
				
				groupAnim->setExpandedIfNoDefault(true);
			}
		}
		else
		if(GLRectDrawable *item = dynamic_cast<GLRectDrawable*>(gld))
		{
			QtColorPicker * fillColor = new QtColorPicker;
			fillColor->setStandardColors();
			fillColor->setCurrentColor(item->fillColor());
			connect(fillColor, SIGNAL(colorChanged(const QColor &)), item, SLOT(setFillColor(QColor)));
			
			QtColorPicker * borderColor = new QtColorPicker;
			borderColor->setStandardColors();
			borderColor->setCurrentColor(item->borderColor());
			connect(borderColor, SIGNAL(colorChanged(const QColor &)), item, SLOT(setBorderColor(QColor)));
			
			lay->addRow(tr("Fill:"), fillColor);
			
			QHBoxLayout *hbox = new QHBoxLayout();
			QDoubleSpinBox *box = new QDoubleSpinBox();
			box->setSuffix(tr("px"));
			box->setMinimum(0);
			box->setDecimals(2);
			box->setMaximum(50);
			box->setValue(item->borderWidth());
			//connect(m_textSizeBox, SIGNAL(returnPressed()), this, SLOT(textSizeChanged(double)));
			connect(box, SIGNAL(valueChanged(double)), item, SLOT(setBorderWidth(double)));
			hbox->addWidget(box);
			hbox->addWidget(borderColor);
			
			lay->addRow(tr("Border:"), hbox);

		}
		else
		if(GLSpinnerDrawable *item = dynamic_cast<GLSpinnerDrawable*>(gld))
		{
			QtColorPicker * fillColor = new QtColorPicker;
			fillColor->setStandardColors();
			fillColor->setCurrentColor(item->fillColor());
			connect(fillColor, SIGNAL(colorChanged(const QColor &)), item, SLOT(setFillColor(QColor)));
			
			QtColorPicker * borderColor = new QtColorPicker;
			borderColor->setStandardColors();
			borderColor->setCurrentColor(item->borderColor());
			connect(borderColor, SIGNAL(colorChanged(const QColor &)), item, SLOT(setBorderColor(QColor)));
			
			lay->addRow(tr("Fill:"), fillColor);
			
			QHBoxLayout *hbox = new QHBoxLayout();
			QDoubleSpinBox *box = new QDoubleSpinBox();
			box->setSuffix(tr("px"));
			box->setMinimum(0);
			box->setDecimals(2);
			box->setMaximum(50);
			box->setValue(item->borderWidth());
			//connect(m_textSizeBox, SIGNAL(returnPressed()), this, SLOT(textSizeChanged(double)));
			connect(box, SIGNAL(valueChanged(double)), item, SLOT(setBorderWidth(double)));
			hbox->addWidget(box);
			hbox->addWidget(borderColor);
			
			lay->addRow(tr("Border:"), hbox);
			
			opts.reset();
			opts.min = 1;
			opts.max = 500;
			lay->addRow(tr("Duration:"), PropertyEditorFactory::generatePropertyEditor(item, "cycleDuration", SLOT(setCycleDuration(double)), opts));
			
			opts.reset();
			opts.text = "Loop at End";
			opts.defaultValue = true;
			lay->addRow("", PropertyEditorFactory::generatePropertyEditor(item, "loopAtEnd", SLOT(setLoopAtEnd(bool)), opts));
		}
		else
		if(GLImageDrawable *item = dynamic_cast<GLImageDrawable*>(gld))
		{
			opts.reset();
			opts.stringIsFile = true;
			opts.fileTypeFilter = tr("Image Files (*.png *.jpg *.bmp *.svg *.xpm *.gif);;Any File (*.*)");
			lay->addRow(tr("&Image:"), PropertyEditorFactory::generatePropertyEditor(item, "imageFile", SLOT(setImageFile(const QString&)), opts, SIGNAL(imageFileChanged(const QString&))));
			
			opts.text = "Ignore image aspect ratio";
			lay->addRow(PropertyEditorFactory::generatePropertyEditor(item, "ignoreAspectRatio", SLOT(setIgnoreAspectRatio(bool)), opts));
			
			{
				ExpandableWidget *groupAnim = new ExpandableWidget("Border and Shadow",base);
				blay->addWidget(groupAnim);
			
				QWidget *groupAnimContainer = new QWidget;
				QFormLayout *lay = new QFormLayout(groupAnimContainer);
				lay->setContentsMargins(3,3,3,3);
			
				groupAnim->setWidget(groupAnimContainer);
			
				addImageBorderShadowEditors(lay,item);
				
				groupAnim->setExpandedIfNoDefault(true);
			}
		}
		else
		if(GLVideoMjpegDrawable *item = dynamic_cast<GLVideoMjpegDrawable*>(gld))
		{
			opts.reset();
			//opts.fileTypeFilter = tr("Image Files (*.png *.jpg *.bmp *.svg *.xpm *.gif);;Any File (*.*)");
			lay->addRow(tr("&MJPEG URL:"), PropertyEditorFactory::generatePropertyEditor(item, "url", SLOT(setUrl(const QString&)), opts));
			
			//opts.text = "Ignore image aspect ratio";
			//lay->addRow(PropertyEditorFactory::generatePropertyEditor(item, "ignoreAspectRatio", SLOT(setIgnoreAspectRatio(bool)), opts));
		}
	}
	
// 	{
// 		
// 		ExpandableWidget *groupAnim = new ExpandableWidget("Show/Hide Effects",base);
// 		blay->addWidget(groupAnim);
// 	
// 		QWidget *groupAnimContainer = new QWidget;
// 		QGridLayout *animLayout = new QGridLayout(groupAnimContainer);
// 		animLayout->setContentsMargins(3,3,3,3);
// 	
// 		groupAnim->setWidget(groupAnimContainer);
// 	
// 		opts.reset();
// 		opts.suffix = " ms";
// 		opts.min = 10;
// 		opts.max = 8000;
// 		opts.defaultValue = 300;
// 	
// 		int row = 0;
// 		animLayout->addWidget(PropertyEditorFactory::generatePropertyEditor(gld, "fadeIn", SLOT(setFadeIn(bool)), opts), row, 0);
// 		animLayout->addWidget(PropertyEditorFactory::generatePropertyEditor(gld, "fadeInLength", SLOT(setFadeInLength(int)), opts), row, 1);
// 	
// 		row++;
// 		animLayout->addWidget(PropertyEditorFactory::generatePropertyEditor(gld, "fadeOut", SLOT(setFadeOut(bool)), opts), row, 0);
// 		animLayout->addWidget(PropertyEditorFactory::generatePropertyEditor(gld, "fadeOutLength", SLOT(setFadeOutLength(int)), opts), row, 1);
// 	
// 		opts.reset();
// 	
// 		groupAnim->setExpandedIfNoDefault(false);
// 	}
	
	return base;
	
}

void EditorWindow::naturalItemFit()
{
	if(!m_currentDrawable)
		return;
		
	if(GLTextDrawable *text = dynamic_cast<GLTextDrawable*>(m_currentDrawable))
	{
		Q_UNUSED(text);
		textFitNaturally();
		
// 		QSizeF size = text->findNaturalSize(m_graphicsScene->sceneRect().width());
// 		text->setRect(QRectF(text->rect().topLeft(),size));
	}
	else
	if(GLVideoDrawable *item = dynamic_cast<GLVideoDrawable*>(m_currentDrawable))
	{
		QRectF nat(item->rect().topLeft(), item->sourceRect().size());
		qDebug() << "EditorWindow::naturalItemFit(): item:"<<(QObject*)m_currentDrawable<<", rect:"<<nat;
		item->setRect(nat);
	}
}


void EditorWindow::rtfEditorSave()
{
	GLTextDrawable *text = dynamic_cast<GLTextDrawable*>(m_currentDrawable);
	if(!text)
		return;
	text->setText(m_rtfEditor->text(Qt::RichText));
}

void EditorWindow::setCanvasSize(const QSizeF& size)
{
	m_canvasSize = size;
	if(m_graphicsScene)
		m_graphicsScene->setSceneRect(QRectF(0,0,size.width(),size.height()));
}

void EditorWindow::addScene()
{
	if(!m_group)
		qDebug() << "EditorWindow::addScene(): No group, planning on crashing now...";
		
	GLScene *scene = new GLScene();
	m_group->addScene(scene);
	setCurrentScene(scene);
}

void EditorWindow::delScene()
{
	if(!m_scene)
		return;
	group()->removeScene(m_scene);
	delete m_scene;
	m_scene = 0;
	
	setCurrentScene(scene());
}

void EditorWindow::dupScene()
{
	if(!m_scene)
		return;
	
	QByteArray ba = m_scene->toByteArray();
	GLScene *scene = new GLScene(ba, group());
	
	m_group->addScene(scene);
	setCurrentScene(scene);
}

void EditorWindow::slideProperties()
{
	if(!m_scene)
		return;
	ScenePropertiesDialog d(m_scene,this);
	d.exec();
}

void EditorWindow::centerSelectionHorizontally()
{
	QList<GLDrawable *> selection = m_graphicsScene->selectedDrawables();
	QRectF scene = m_graphicsScene->sceneRect();

	qreal halfX = scene.width()/2;
// 	qreal halfY = scene.height()/2;
	foreach(GLDrawable *item, selection)
	{
		QRectF r = item->rect();
		item->setRect( QRectF( 
			halfX - r.width()/2, // - r.left() + scene.left(), 
			r.top(), r.width(), r.height() 
		) );
	}
}


void EditorWindow::centerSelectionVertically()
{
	QList<GLDrawable *> selection = m_graphicsScene->selectedDrawables();
	QRectF scene = m_graphicsScene->sceneRect();

// 	qreal halfX = scene.width()/2;
	qreal halfY = scene.height()/2;
	foreach(GLDrawable *item, selection)
	{
		QRectF r = item->rect();
		item->setRect( QRectF( 
			r.left(), halfY - r.height()/2,// - r.top() + scene.top(),
			r.width(), r.height() 
		));
	}
}

