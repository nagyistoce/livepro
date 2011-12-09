#include <QtGui>

#include "MainWindow.h"
#include <QCDEStyle>

// #include "qtvariantproperty.h"
// #include "qttreepropertybrowser.h"

#include "VideoSource.h"
#include "../glvidtex/GLWidget.h"
#include "../glvidtex/GLVideoDrawable.h"
#include "../glvidtex/StaticVideoSource.h"
#include "LayerControlWidget.h"
#include "LiveVideoFileLayer.h"
#include "LiveSceneListModel.h"

MainWindow::MainWindow()
	: QMainWindow()
	, m_currentLayerPropsEditor(0)
	, m_currentScene(0)
	, m_currentLayer(0)
	, m_currentKeyFrameRow(-1)
	, m_lockTimelineTableCellEditorSlot(false)
	, m_currentKeyFrame(0)
	, m_playTime(0)
	, m_outputScreenIdx(0)
	, m_mainOutput(0)
{

	m_mainOutput = new GLWidget();
	m_mainOutput->setObjectName("MainOutput");
	m_mainOutput->setWindowTitle(QString("Live Output - LiveMix")/*.arg(m_output->name())*/);
	m_mainOutput->setWindowIcon(QIcon(":/data/icon-d.png"));
	m_mainOutput->resize(1000,750);
	m_mainOutput->hide(); // keep hidden explicitly


	qRegisterMetaType<VideoSource*>("VideoSource*");

	createActions();
	createMenus();
	createToolBars();
	createStatusBar();

	m_mainSplitter = new QSplitter(this);
	//m_splitter->setOrientation(Qt::Vertical);
	setCentralWidget(m_mainSplitter);

	createLeftPanel();
	createCenterPanel();
	createRightPanel();

	//setupSampleScene();
	newFile();

	readSettings();

	setWindowTitle(tr("LiveMix"));
	setUnifiedTitleAndToolBarOnMac(true);
		
	QTimer::singleShot(500, this, SLOT(loadLastFile()));	
}

void MainWindow::loadLastFile()
{
	QString lastFile = QSettings().value("last-lmx-file","vid.lmx").toString();
	loadFile(lastFile);
}

void MainWindow::setupSampleScene()
{
	LiveScene *scene = new LiveScene();

	scene->addLayer(new LiveVideoInputLayer());
	scene->addLayer(new LiveStaticSourceLayer());
	scene->addLayer(new LiveTextLayer());
	scene->addLayer(new LiveVideoFileLayer());

	loadLiveScene(scene);

	//scene->layerList().at(2)->setVisible(true);


	//m_layerViewer->addDrawable(scene->layerList().at(1)->drawable(m_layerViewer));

	/*



// 	GLWidget *glOutputWin = new GLWidget();
// 	{
// 		glOutputWin->setWindowTitle("Live");
// 		glOutputWin->resize(1000,750);
// 		// debugging...
// 		//glOutputWin->setProperty("isEditorWidget",true);
//
// 	}

	GLWidget *glEditorWin = new GLWidget();
	{
		glEditorWin->setWindowTitle("Editor");
		glEditorWin->resize(400,300);
		glEditorWin->setProperty("isEditorWidget",true);
	}

	// Setup Editor Background
	if(true)
	{
		QSize size = glEditorWin->viewport().size().toSize();
		size /= 2.5;
		//qDebug() << "MainWindow::createLeftPanel(): size:"<<size;
		QImage bgImage(size, QImage::Format_ARGB32_Premultiplied);
		QBrush bgTexture(QPixmap("squares2.png"));
		QPainter bgPainter(&bgImage);
		bgPainter.fillRect(bgImage.rect(), bgTexture);
		bgPainter.end();

		StaticVideoSource *source = new StaticVideoSource();
		source->setImage(bgImage);
		//source->setImage(QImage("squares2.png"));
		source->start();

		GLVideoDrawable *drawable = new GLVideoDrawable(glEditorWin);
		drawable->setVideoSource(source);
		drawable->setRect(glEditorWin->viewport());
		drawable->setZIndex(-100);
		drawable->setObjectName("StaticBackground");
		drawable->show();

		glEditorWin->addDrawable(drawable);
	}



// 	// setup scene
// 	LiveScene *scene = new LiveScene();
// 	{
// 		//scene->addLayer(new LiveVideoInputLayer());
// 		scene->addLayer(new LiveStaticSourceLayer());
// 		scene->addLayer(new LiveTextLayer());
// 	}

	// add to live output
	{
		//scene->attachGLWidget(glOutputWin);
		//scene->layerList().at(0)->drawable(glOutputWin)->setObjectName("Output");
		//glOutputWin->addDrawable(scene->layerList().at(0)->drawable(glOutputWin));
		scene->layerList().at(0)->setVisible(true);
		scene->layerList().at(1)->setVisible(true);
	}

	// add to editor
	{
		scene->layerList().at(0)->drawable(glEditorWin)->setObjectName("Editor");
		//glEditorWin->addDrawable(scene->layerList().at(1)->drawable(glEditorWin));
		glEditorWin->addDrawable(scene->layerList().at(1)->drawable(glEditorWin));
		m_layerViewer->addDrawable(scene->layerList().at(1)->drawable(m_layerViewer));

	}

	// show windows
	{
		//glOutputWin->show();
		glEditorWin->show();
	}*/




	//m_layerViewer->addDrawable(scene->layerList().at(2)->drawable(m_layerViewer));
}

void MainWindow::loadLiveScene(LiveScene *scene)
{
	if(m_currentScene)
	{
		removeCurrentScene();
		updateSceneTimeLength();
		delete m_currentScene;
	}

	m_currentScene = scene;

	// attach to main viewer
	scene->attachGLWidget(m_mainViewer);
	if(m_mainOutput)
		scene->attachGLWidget(m_mainOutput);
	
	m_sceneModel->setLiveScene(scene);
	
	if(!scene->layerList().isEmpty())
		setCurrentLayer(m_sceneModel->itemAt(scene->layerList().size()-1));
	
	loadKeyFramesToTable();
	
	updateSceneTimeLength();
	
	// attach to main output
	/// TODO main output
}

void MainWindow::hideAllLayers(bool flag)
{
	if(!m_currentScene || m_currentScene->layerList().isEmpty())
		return;
	foreach(LiveLayer *layer, m_currentScene->layerList())
	{
		if(flag)
		{
			bool isVis = layer->isVisible();
			layer->setProperty("mw-hideall-wasvis",isVis);
			layer->setVisible(false);
		}
		else
		{
			bool wasVis = layer->property("mw-hideall-wasvis").toBool();
			layer->setVisible(wasVis);
		}
	}
}

void MainWindow::updateSceneTimeLength()
{
	double len = m_currentScene ? m_currentScene->sceneLength() : 0;
	m_positionBox->setMaximum(len);
	m_positionSlider->setMaximum((int)len);
}

void MainWindow::removeCurrentScene()
{
	if(!m_currentScene)
		return;

	disconnect(m_currentScene, 0, this, 0);
	m_currentScene->detachGLWidget(m_mainViewer);
	if(m_mainOutput)
		m_currentScene->detachGLWidget(m_mainOutput);

	m_currentScene = 0;

	setCurrentLayer(0);
	
	loadKeyFramesToTable();

}


void MainWindow::deleteCurrentLayer()
{
	LiveLayer *layer = m_currentLayer;
	if(layer)
	{
		setCurrentLayer(0);
		if(m_currentScene)
			m_currentScene->removeLayer(layer);
		delete layer;
		layer = 0;
	}
}

void MainWindow::duplicateLayer()
{
	if(m_currentLayer)
	{
		QObject *instance = m_currentLayer->metaObject()->newInstance(Q_ARG(QObject*,0));
		
		if(!instance)
		{
			qDebug() << "Error: MainWindow::duplicateCurrentLayer(): Creation of "<<m_currentLayer->metaObject()->className()<<" failed.";
			return;
		}
		
		
		LiveLayer *layer = dynamic_cast<LiveLayer*>(instance);
		if(!layer)
		{
			qDebug() << "Error: MainWindow::duplicateCurrentLayer(): Object created for type "<<m_currentLayer->metaObject()->className()<<" does not inherit from LiveLayer.";
			delete layer;
			return;
		}
		
		QByteArray bytes = m_currentLayer->toByteArray();
		layer->fromByteArray(bytes);
		
		setCurrentLayer(layer);
		if(m_currentScene)
			m_currentScene->addLayer(layer);
	}
}


void MainWindow::setCurrentLayer(LiveLayer *layer)
{
	if(m_currentLayer)
	{
//  		qDebug() << "MainWindow::setCurrentLayer(): removing old layer from editor";
		m_currentLayer->detachGLWidget(m_layerViewer);
	}

	m_currentLayer = layer;
	loadLayerProperties(m_currentLayer);
	
	m_deleteLayerAct->setEnabled(layer!=NULL);
	m_duplicateLayerAct->setEnabled(layer!=NULL);
		
	if(m_currentLayer)
	{
		m_currentLayer->attachGLWidget(m_layerViewer);
		
		QModelIndex idx = m_sceneModel->indexForItem(m_currentLayer);
		if(m_layerListView->currentIndex().row() != idx.row())
			m_layerListView->setCurrentIndex(idx);
	}
}

void MainWindow::loadLayerProperties(LiveLayer *layer)
{
	if(m_currentLayerPropsEditor)
	{
		m_controlBase->layout()->removeWidget(m_currentLayerPropsEditor);
		m_currentLayerPropsEditor->deleteLater();
		m_currentLayerPropsEditor = 0;
	}

	if(!m_currentLayer)
		return;
		
	m_currentLayer->lockLayerPropertyUpdates(true);
	QWidget *props = m_currentLayer->createLayerPropertyEditors();
	m_currentLayer->lockLayerPropertyUpdates(false);

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

void MainWindow::createLeftPanel()
{
	m_leftSplitter = new QSplitter(this);
	m_leftSplitter->setOrientation(Qt::Vertical);
	
	m_layerListView = new QListView(m_leftSplitter);
	m_layerListView->setViewMode(QListView::ListMode);
	//m_layerListView->setViewMode(QListView::IconMode);
	m_layerListView->setMovement(QListView::Free);
	m_layerListView->setWordWrap(true);
	m_layerListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_layerListView->setDragEnabled(true);
	m_layerListView->setAcceptDrops(true);
	m_layerListView->setDropIndicatorShown(true);

	connect(m_layerListView, SIGNAL(activated(const QModelIndex &)), this, SLOT(layerSelected(const QModelIndex &)));
	connect(m_layerListView, SIGNAL(clicked(const QModelIndex &)),   this, SLOT(layerSelected(const QModelIndex &)));

	// deleting old selection model per http://doc.trolltech.com/4.5/qabstractitemview.html#setModel
	QItemSelectionModel *m = m_layerListView->selectionModel();

	m_sceneModel = new LiveSceneListModel();
	m_layerListView->setModel(m_sceneModel);
	connect(m_sceneModel, SIGNAL(repaintList()), this, SLOT(repaintLayerList()));
	connect(m_sceneModel, SIGNAL(layersDropped(QList<LiveLayer*>)), this, SLOT(layersDropped(QList<LiveLayer*>)));

	if(m)
	{
		delete m;
		m=0;
	}

	QItemSelectionModel *currentSelectionModel = m_layerListView->selectionModel();
	connect(currentSelectionModel, SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(currentChanged(const QModelIndex &, const QModelIndex &)));
	
	//parentLyout->addWidget(m_layerListView);
	
	
	m_leftSplitter->addWidget(m_layerListView);
	
	QWidget *tableBase = new QWidget(m_leftSplitter);
	
	QVBoxLayout *layout = new QVBoxLayout(tableBase);
	
	QHBoxLayout *btnLayout = new QHBoxLayout();
	m_keyNewBtn = new QPushButton("Create Frame",tableBase);
	m_keyDelBtn = new QPushButton("Delete Frame",tableBase);
	m_keyUpdateBtn = new QPushButton("Update Frame",tableBase);
	
	btnLayout->addWidget(m_keyNewBtn);
	btnLayout->addWidget(m_keyDelBtn);
	btnLayout->addWidget(m_keyUpdateBtn);
	layout->addLayout(btnLayout);
	
	m_keyDelBtn->setDisabled(true); // enabled when a row is selected
	m_keyUpdateBtn->setDisabled(true);
	
	connect(m_keyNewBtn, SIGNAL(clicked()), this, SLOT(createKeyFrame()));
	connect(m_keyDelBtn, SIGNAL(clicked()), this, SLOT(deleteKeyFrame()));
	connect(m_keyUpdateBtn, SIGNAL(clicked()), this, SLOT(updateKeyFrame()));
	
	
	m_timelineTable = new QTableWidget(tableBase);
	m_timelineTable->verticalHeader()->setVisible(false);
	m_timelineTable->horizontalHeader()->setVisible(false);
	m_timelineTable->setColumnCount(5);
// 	QStringList list;
// 	list << "Nbr" << "Description"<<"Show";
// 	m_timelineTable->setHorizontalHeaderLabels(list);
	
	m_timelineTable->setRowCount(0);
	m_timelineTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_timelineTable->resizeColumnsToContents();
	m_timelineTable->resizeRowsToContents();
	connect(m_timelineTable, SIGNAL(cellClicked(int,int)), this, SLOT(slotTimelineTableCellActivated(int,int)));
	connect(m_timelineTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(slotTimelineTableCellEdited(QTableWidgetItem*)));
	
	layout->addWidget(m_timelineTable);
	
	QWidget *playBoxBase = new QWidget(tableBase);
	QHBoxLayout *playlay = new QHBoxLayout(playBoxBase);
	playlay->setContentsMargins(0,0,0,0);
	
	m_playButton = new QPushButton(style()->standardIcon(QStyle::SP_MediaPlay),"");
	connect(m_playButton, SIGNAL(clicked()), this, SLOT(scenePlay()));
	playlay->addWidget(m_playButton);
	
	m_pauseButton = new QPushButton(style()->standardIcon(QStyle::SP_MediaPause),"");
	connect(m_pauseButton, SIGNAL(clicked()), this, SLOT(scenePause()));
	playlay->addWidget(m_pauseButton);
	
	m_stopButton = new QPushButton(style()->standardIcon(QStyle::SP_MediaStop),"");
	connect(m_stopButton, SIGNAL(clicked()), this, SLOT(sceneStop()));
	playlay->addWidget(m_stopButton);
	
	m_rrButton = new QPushButton(style()->standardIcon(QStyle::SP_MediaSkipBackward),"");
	connect(m_rrButton, SIGNAL(clicked()), this, SLOT(sceneRR()));
	playlay->addWidget(m_rrButton);
	
	m_ffButton = new QPushButton(style()->standardIcon(QStyle::SP_MediaSkipForward),"");
	connect(m_ffButton, SIGNAL(clicked()), this, SLOT(sceneFF()));
	playlay->addWidget(m_ffButton);
	
	m_positionSlider = new QSlider();
	m_positionSlider->setOrientation(Qt::Horizontal);
	m_positionSlider->setMinimum(0);
	m_positionSlider->setMaximum(100000);
	connect(m_positionSlider, SIGNAL(valueChanged(int)), this, SLOT(slotSliderBoxChanged(int)));
	playlay->addWidget(m_positionSlider,2);
	
	m_positionBox = new QDoubleSpinBox();
	m_positionBox->setMinimum(0);
	m_positionBox->setMaximum(100000);
	connect(m_positionBox, SIGNAL(valueChanged(double)), this, SLOT(slotPositionBoxChanged(double)));
	playlay->addWidget(m_positionBox);
	
	layout->addWidget(playBoxBase);
	
	connect(&m_scenePlayTimer, SIGNAL(timeout()), this, SLOT(slotSceneTimerTick()));
	m_scenePlayTimer.setInterval(30);
	
	m_leftSplitter->addWidget(tableBase);
	
	m_mainSplitter->addWidget(m_leftSplitter);

}

void MainWindow::showFrameForTime(double time, bool forceApply)
{
	if(!m_currentScene)
		return;
		
// 	qDebug() << "MainWindow::showFrameForTime: time:"<<time;
			
	QList<LiveScene::KeyFrame> list = m_currentScene->keyFrames();
	LiveScene::KeyFrame frame(0), prevFrame(0);
	foreach(LiveScene::KeyFrame testFrame, list)
	{
		if(prevFrame.playTime > -1)
		{
			if(time >= prevFrame.playTime && 
			   time < testFrame.playTime)
			{
				frame = prevFrame;
//  				qDebug() << "MainWindow::showFrameForTime: 1 found frame at play time:"<<frame.playTime;
				break;
			}
			prevFrame = testFrame;
		}
		else
		{
			prevFrame = testFrame;
			if(time <= testFrame.playTime)
			{
				frame = testFrame;
//  				qDebug() << "MainWindow::showFrameForTime: 2 found frame at play time:"<<frame.playTime;
				break;
			}
		}
	}
	
	if(!frame.scene && time >= list.last().playTime)
	{
		frame = list.last();
//  		qDebug() << "MainWindow::showFrameForTime: 3 found frame at play time:"<<frame.playTime;
	}
	
	if(forceApply || !(m_currentKeyFrame == frame))
	{
//  		qDebug() << "MainWindow::showFrameForTime: SHOWING FRAME "<<frame.frameName;
		showFrame(frame);
	}
	else
	{
//  		qDebug() << "MainWindow::showFrameForTime: frames the same, not changing";
	}
	
	
}
void MainWindow::showFrame(const LiveScene::KeyFrame& frame)
{
	if(!m_currentScene)
		return;
		
	QList<LiveScene::KeyFrame> frames = m_currentScene->keyFrames();
	int idx = frames.indexOf(frame);
// 	qDebug() << "MainWindow::showFrame: indx: "<<idx<<", frame id: "<<frame.id;
	
	if(idx<0 || idx>=frames.size())
		return;
		
	m_timelineTable->setCurrentCell(idx,2);
	
	m_currentKeyFrame = frame;
	m_currentScene->applyKeyFrame(frame);
	
	// Update property editors
	setCurrentLayer(m_currentLayer);
}

void MainWindow::slotSceneTimerTick()
{
	if(!m_currentScene)
	{
		m_scenePlayTimer.stop();
		return;
	}
	
	m_playTime += ((double)m_scenePlayTimer.interval())/1000.0;
	showFrameForTime(m_playTime);
	m_positionBox->setValue(m_playTime);
	
	if(m_playTime > m_currentScene->sceneLength())
		scenePause();
}

void MainWindow::slotPositionBoxChanged(double value)
{
	m_playTime = value;
	showFrameForTime(value);
	if(m_positionSlider->value() != (int)value)
		m_positionSlider->setValue((int)value);
}

void MainWindow::slotSliderBoxChanged(int intValue)
{
	double value = (double)intValue;
	m_playTime = value;
	showFrameForTime(value);
	if(m_positionBox->value() != value)
		m_positionBox->setValue(value);
}

void MainWindow::scenePlay()
{
	m_scenePlayTimer.start();
}

void MainWindow::scenePause()
{
	m_scenePlayTimer.stop();
}

void MainWindow::sceneFF()
{
	if(!m_currentScene)
		return;
	QList<LiveScene::KeyFrame> frames = m_currentScene->keyFrames();
	int idx = m_currentKeyFrame.scene ? frames.indexOf(m_currentKeyFrame) : 0;
	if(idx<0 || idx>=frames.size()-1)
		return;
	idx++;
	LiveScene::KeyFrame frame = frames.at(idx);
	showFrame(frame);
	m_playTime = frame.playTime;
	m_positionBox->setValue(m_playTime);
}

void MainWindow::sceneRR()
{
	if(!m_currentScene)
		return;
	QList<LiveScene::KeyFrame> frames = m_currentScene->keyFrames();
	int idx = m_currentKeyFrame.scene ? frames.indexOf(m_currentKeyFrame) : 0;
	if(idx<1 || idx>=frames.size())
		return;
	idx--;
	LiveScene::KeyFrame frame = frames.at(idx);
	showFrame(frame);
	m_playTime = frame.playTime;
	m_positionBox->setValue(m_playTime);
}

void MainWindow::sceneStop()
{
	m_scenePlayTimer.stop();
	m_playTime = 0;
	m_positionBox->setValue(m_playTime);
}

void MainWindow::slotTimelineTableCellActivated(int row,int)
{
	m_keyDelBtn->setEnabled(true);
	m_keyUpdateBtn->setEnabled(true);
	m_currentKeyFrameRow = row;
	
// 	QList<LiveScene::KeyFrame> frames = m_currentScene->keyFrames();
// 	if(row<0 || row>=frames.size())
// 		return;
// 		
// 	m_positionBox->setValue(frames.at(row).playTime);
}

void MainWindow::createKeyFrame()
{
	if(!m_currentScene)
		return;
		
	QPixmap pixmap = QPixmap::fromImage(m_mainViewer->grabFrameBuffer());
	
	QModelIndexList indexList = m_layerListView->selectionModel()->selectedIndexes();
	
	QList<LiveLayer*> layers;
	foreach(QModelIndex index, indexList)
		layers.append(m_sceneModel->itemFromIndex(index));
		
	//qDebug () << "MainWindow::createKeyFrame: Selected layer list size: "<<layers.size();
	
	m_currentScene->createAndAddKeyFrame(layers, pixmap);
	
	loadKeyFramesToTable();
}

void MainWindow::deleteKeyFrame()
{
	if(m_currentKeyFrameRow<0)
		return;
	if(!m_currentScene)
		return;
		
	m_currentScene->removeKeyFrame(m_currentKeyFrameRow);
	m_currentKeyFrameRow = -1;
	m_keyDelBtn->setEnabled(false);
	m_keyUpdateBtn->setEnabled(false);
	
	loadKeyFramesToTable();
}

void MainWindow::updateKeyFrame()
{
	if(m_currentKeyFrameRow<0)
		return;
	if(!m_currentScene)
		return;
	
	QPixmap pixmap = QPixmap::fromImage(m_mainViewer->grabFrameBuffer());
	m_currentScene->updateKeyFrame(m_currentKeyFrameRow, pixmap);
	
	loadKeyFramesToTable();
}

void MainWindow::slotTimelineTableCellEdited(QTableWidgetItem *item)
{
	if(m_lockTimelineTableCellEditorSlot)
		return;
	m_lockTimelineTableCellEditorSlot = true;
	
	if(!item)
		return;
	
	if(!m_currentScene)
		return;
		
	QList<LiveScene::KeyFrame> frames = m_currentScene->keyFrames();
	int row = item->row();
	if(row < 0 || row >frames.size())
		return;
	
	switch(item->column())
	{
		case 1: // name
			m_currentScene->setKeyFrameName(row,item->text());
			break;
		case 2: // start time
			m_currentScene->setKeyFrameStartTime(row,item->text().toDouble());
			loadKeyFramesToTable(); // call to sort by time, it also calls the resize..() methods, below, so just return
			updateSceneTimeLength();
			return;
		case 3: // anim length
			m_currentScene->setKeyFrameAnimLength(row,item->text().toInt());
			break;
	}
	
	m_timelineTable->resizeColumnsToContents();
	m_timelineTable->resizeRowsToContents();
	
	m_lockTimelineTableCellEditorSlot = false;
	
	//qDebug() << "MainWindow::slotTimelineTableCellEdited(): row:"<<row<<", new text:"<<m_currentScene->keyFrames().at(row).frameName<<", text:"<<item->text();
	
}

void MainWindow::loadKeyFramesToTable()
{
	m_timelineTable->clear();
	if(!m_currentScene)
		return;
		
	m_lockTimelineTableCellEditorSlot = true;
		
	QList<LiveScene::KeyFrame> frames = m_currentScene->keyFrames();
	
	m_timelineTable->setRowCount(frames.size());
	
	QTableWidgetItem *prototype = new QTableWidgetItem();
	prototype->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

	int row=0;
	foreach(LiveScene::KeyFrame frame, frames)
	{
		QTableWidgetItem *t = prototype->clone();
		if(!frame.pixmap.isNull())
			t->setIcon(QIcon(frame.pixmap));
		else
			t->setText(QString::number(frame.id));
		m_timelineTable->setItem(row,0,t);
		
		t = prototype->clone();
		t->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsEditable);
		t->setText(frame.frameName);
		m_timelineTable->setItem(row,1,t);
		
		t = prototype->clone();
		t->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsEditable);
		t->setText(QString::number(frame.playTime));
		t->setTextAlignment(Qt::AlignRight);
		m_timelineTable->setItem(row,2,t);
		
		t = prototype->clone();
		t->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsEditable);
		t->setText(QString::number(frame.animParam.length));
		t->setTextAlignment(Qt::AlignRight);
		m_timelineTable->setItem(row,3,t);
		
		QPushButton *btn = new QPushButton("Show");
		btn->setProperty("keyFrameRow", row);
		
		connect(btn, SIGNAL(clicked()), this, SLOT(keyFrameBtnActivated()));
		m_timelineTable->setCellWidget(row,4,btn);
		
		row++;
	}
	
	m_timelineTable->sortByColumn(2,Qt::AscendingOrder);
	m_timelineTable->resizeColumnsToContents();
	m_timelineTable->resizeRowsToContents();
	
	m_lockTimelineTableCellEditorSlot = false;
}

void MainWindow::keyFrameBtnActivated()
{
	QPushButton *btn = dynamic_cast<QPushButton*>(sender());
	if(!btn)
		return;
	QVariant prop = btn->property("keyFrameRow");
	if(!prop.isValid())
		return;

	int row = prop.toInt();
	
	if(!m_currentScene)
		return;
		
	m_currentKeyFrameRow = row;
	
// 	QList<LiveScene::KeyFrame> frames = m_currentScene->keyFrames();
// 	if(row < 0 || row >frames.size())
// 		return;
// 	
// 	LiveScene::keyFrame frame = frames[row];
	
	//qDebug() << "MainWindow::slotTimelineTableCellEdited(): row:"<<item->row()<<", new text:"<<m_currentScene->keyFrames()[item->row()].frameName();
	m_currentScene->applyKeyFrame(row);
	
	// Update property editors
	setCurrentLayer(m_currentLayer);
}

void MainWindow::createCenterPanel()
{
	m_editSplitter = new QSplitter(m_mainSplitter);
	m_editSplitter->setOrientation(Qt::Vertical);

	m_layerViewer = new GLWidget(m_editSplitter);
	m_layerViewer->setObjectName("Editor");
	m_layerViewer->setProperty("isEditorWidget",true);

// 	QSize size = m_layerViewer->viewport().size().toSize();
// 	size /= 2.5;
// 	qDebug() << "MainWindow::createLeftPanel(): size:"<<size;
// 	QImage bgImage(size, QImage::Format_ARGB32_Premultiplied);
// 	QBrush bgTexture(QPixmap("squares2.png"));
// 	QPainter bgPainter(&bgImage);
// 	bgPainter.fillRect(bgImage.rect(), bgTexture);
// 	bgPainter.end();
// 
// 	StaticVideoSource *source = new StaticVideoSource();
// 	source->setImage(bgImage);
// 	//source->setImage(QImage("squares2.png"));
// 	source->start();
// 	connect(this, SIGNAL(destroyed()), source, SLOT(deleteLater()));
// 
// 	GLVideoDrawable *drawable = new GLVideoDrawable(m_layerViewer);
// 	drawable->setVideoSource(source);
// 	drawable->setRect(m_layerViewer->viewport());
// 	drawable->setZIndex(-100);
// 	drawable->setObjectName("StaticBackground");
// 	drawable->setAlignment(Qt::AlignAbsolute);
// // 	drawable->setBottomPercent(1.);
// // 	drawable->setRightPercent(1.);
// 	drawable->show();
//
// 	m_layerViewer->addDrawable(drawable);
	
	
	m_editSplitter->addWidget(m_layerViewer);
	
	m_controlArea = new QScrollArea(m_editSplitter);
	m_controlArea->setWidgetResizable(true);
	m_controlBase = new QWidget(m_controlArea);
	
	QVBoxLayout *layout = new QVBoxLayout(m_controlBase);
	layout->setContentsMargins(0,0,0,0);
	
	m_controlArea->setWidget(m_controlBase);
	m_editSplitter->addWidget(m_controlArea);
	
	m_mainSplitter->addWidget(m_editSplitter);

}

void MainWindow::layerSelected(const QModelIndex &idx)
{
	LiveLayer *layer = m_sceneModel->itemFromIndex(idx);
	//qDebug() << "SlideEditorWindow::slideSelected(): selected slide#:"<<s->slideNumber();
	if(m_currentLayer != layer)
		setCurrentLayer(layer);
}


void MainWindow::currentChanged(const QModelIndex &idx,const QModelIndex &)
{
	if(idx.isValid())
		layerSelected(idx);
}

void MainWindow::repaintLayerList()
{
	m_layerListView->clearFocus();
	m_layerListView->setFocus();
	m_layerListView->update();
}


void MainWindow::layersDropped(QList<LiveLayer*> list)
{
	QModelIndex idx = m_sceneModel->indexForItem(list.first());
	m_layerListView->setCurrentIndex(idx);
}


void MainWindow::createRightPanel()
{
	m_previewSplitter = new QSplitter(m_mainSplitter);
	m_previewSplitter->setOrientation(Qt::Vertical);
	/*
	QWidget *base = new QWidget(m_mainSplitter);
	QVBoxLayout *layout = new QVBoxLayout(base);*/

	m_mainViewer = new GLWidget(m_previewSplitter);
	m_mainViewer->setObjectName("MainPreview");
	m_previewSplitter->addWidget(m_mainViewer);
	//layout->addStretch(1);

	m_mainSplitter->addWidget(m_previewSplitter);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	writeSettings();
	event->accept();
}


void MainWindow::about()
{
	QMessageBox::about(this, tr("About LiveMix"),
		tr("<b>LiveMix</b> is an open-source video mixer for live and recorded video."));
}

void MainWindow::createActions()
{
	m_newAct = new QAction(QIcon(":/images/new.png"), tr("&New"), this);
	m_newAct->setShortcuts(QKeySequence::New);
	m_newAct->setStatusTip(tr("Create a new file"));
	connect(m_newAct, SIGNAL(triggered()), this, SLOT(newFile()));

	m_openAct = new QAction(QIcon(":/images/open.png"), tr("&Open..."), this);
	m_openAct->setShortcuts(QKeySequence::Open);
	m_openAct->setStatusTip(tr("Open an existing file"));
	connect(m_openAct, SIGNAL(triggered()), this, SLOT(open()));

	m_saveAct = new QAction(QIcon(":/images/save.png"), tr("&Save"), this);
	m_saveAct->setShortcuts(QKeySequence::Save);
	m_saveAct->setStatusTip(tr("Save the document to disk"));
	connect(m_saveAct, SIGNAL(triggered()), this, SLOT(save()));

	m_saveAsAct = new QAction(tr("Save &As..."), this);
	m_saveAsAct->setShortcuts(QKeySequence::SaveAs);
	m_saveAsAct->setStatusTip(tr("Save the document under a new name"));
	connect(m_saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

	//! [0]
	m_exitAct = new QAction(tr("E&xit"), this);
	m_exitAct->setShortcuts(QKeySequence::Quit);
	m_exitAct->setStatusTip(tr("Exit the application"));
	connect(m_exitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));
	//! [0]

// 	cutAct = new QAction(QIcon(":/images/cut.png"), tr("Cu&t"), this);
// 	cutAct->setShortcuts(QKeySequence::Cut);
// 	cutAct->setStatusTip(tr("Cut the current selection's contents to the "
// 				"clipboard"));
// 	connect(cutAct, SIGNAL(triggered()), this, SLOT(cut()));


	m_aboutAct = new QAction(tr("&About"), this);
	m_aboutAct->setStatusTip(tr("Show the application's About box"));
	connect(m_aboutAct, SIGNAL(triggered()), this, SLOT(about()));

	m_aboutQtAct = new QAction(tr("About &Qt"), this);
	m_aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
	connect(m_aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

 	
	m_newCameraLayerAct = new QAction(QIcon("../data/stock-panel-screenshot.png"), tr("New Camera Layer"), this);
	connect(m_newCameraLayerAct, SIGNAL(triggered()), this, SLOT(newCameraLayer()));
	
	m_newVideoLayerAct = new QAction(QIcon("../data/stock-panel-multimedia.png"), tr("New Video Layer"), this);
	connect(m_newVideoLayerAct, SIGNAL(triggered()), this, SLOT(newVideoLayer()));
		
	m_newTextLayerAct = new QAction(QIcon("../data/stock-font.png"), tr("New Text Layer"), this);
	connect(m_newTextLayerAct, SIGNAL(triggered()), this, SLOT(newTextLayer()));
	
	m_newImageLayerAct = new QAction(QIcon("../data/stock-insert-image.png"), tr("New Image Layer"), this);
	connect(m_newImageLayerAct, SIGNAL(triggered()), this, SLOT(newImageLayer()));
	
	m_deleteLayerAct = new QAction(QIcon("../data/action-delete.png"), tr("Delete Current Layer"), this);
	connect(m_deleteLayerAct, SIGNAL(triggered()), this, SLOT(deleteCurrentLayer()));
	
	m_duplicateLayerAct = new QAction(QIcon("../data/stock-convert.png"), tr("Duplicate Current Layer"), this);
	connect(m_duplicateLayerAct, SIGNAL(triggered()), this, SLOT(duplicateLayer()));
	
	

}

void MainWindow::createMenus()
{
	m_fileMenu = menuBar()->addMenu(tr("&File"));

	m_fileMenu->addAction(m_newAct);
	m_fileMenu->addAction(m_openAct);
	m_fileMenu->addAction(m_saveAct);
	m_fileMenu->addAction(m_saveAsAct);
 	
 	m_fileMenu->addSeparator();

	m_fileMenu->addAction(m_exitAct);

	menuBar()->addSeparator();

	m_helpMenu = menuBar()->addMenu(tr("&Help"));
	m_helpMenu->addAction(m_aboutAct);
	m_helpMenu->addAction(m_aboutQtAct);
}

void MainWindow::createToolBars()
{
	m_fileToolBar = addToolBar(tr("Layer Toolbar"));
	
	m_fileToolBar->addAction(m_newCameraLayerAct);
	m_fileToolBar->addAction(m_newVideoLayerAct);
	m_fileToolBar->addAction(m_newTextLayerAct);
	m_fileToolBar->addAction(m_newImageLayerAct);
	
	m_fileToolBar->addSeparator();
	
	m_fileToolBar->addAction(m_deleteLayerAct);
	m_fileToolBar->addAction(m_duplicateLayerAct);
	
	m_fileToolBar->addSeparator();
	m_fileToolBar->addWidget(new QLabel("Output:"));
	
	m_outputCombo = new QComboBox();
	
	QDesktopWidget *desktopWidget = QApplication::desktop();
	
	QSettings settings;
	m_outputScreenIdx = settings.value("MainWindow/last-screen-index",0).toInt();
		
	QStringList itemList;
	int numScreens = desktopWidget->numScreens();
	for(int screenNum = 0; screenNum < numScreens; screenNum ++)
	{
		QRect geom = desktopWidget->screenGeometry(screenNum);
		
		QString text = QString("Screen %1 at (%2 x %3)")
			.arg(screenNum + 1)
			.arg(geom.left())
			.arg(geom.top());
			
		itemList << text;
		m_screenList << geom;
	}
	
	m_outputCombo->addItems(itemList);
	m_outputCombo->setCurrentIndex(m_outputScreenIdx);
	connect(m_outputCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(screenIndexChanged(int)));
	
	m_fileToolBar->addWidget(m_outputCombo);
	
	m_liveBtn = new QPushButton("Go Live");
	m_liveBtn->setCheckable(true);
	connect(m_liveBtn, SIGNAL(toggled(bool)), this, SLOT(showLiveOutput(bool)));
	m_fileToolBar->addWidget(m_liveBtn);
	
	m_fileToolBar->addSeparator();
	
	QPushButton *hideAll = new QPushButton("Hide All");
	hideAll->setCheckable(true);
	connect(hideAll, SIGNAL(toggled(bool)), this, SLOT(hideAllLayers(bool)));
	m_fileToolBar->addWidget(hideAll);
}

void MainWindow::screenIndexChanged(int idx)
{
	if(idx < 0 || idx >= m_screenList.size())
		return;
		
	if(!m_mainOutput)
	{
		qDebug() << "MainWindow::screenIndexChanged: Main Output Widget not created, no output available.";
		return;
	}
	
// 	if(!m_mainOutput->isVisible())
// 	{
// 		qDebug() << "MainWindow::screenIndexChanged: Main Output Widget not enabled, not adjusting geometry yet.";
// 		return;
// 	}
		
	QSettings settings;
	settings.setValue("MainWindow/last-screen-index",idx);
		
	QRect geom = m_screenList[idx];
	
	
	//geom = QRect(0,0,320,240);
	//m_mainOutput->applyGeometry(geom);
	//m_mainOutput->setVisible(true);
	
	//qDebug() << "VideoOutputWidget::applyGeometry(): rect: "<<rect;
 	//if(isFullScreen)
 		m_mainOutput->setWindowFlags(Qt::FramelessWindowHint);// | Qt::ToolTip);
//  	else
//  		m_mainOutput->setWindowFlags(Qt::FramelessWindowHint);
				
				
	m_mainOutput->resize(geom.width(),geom.height());
	m_mainOutput->move(geom.left(),geom.top());
		
	//m_mainOutput->show();
	
	
}

void MainWindow::showLiveOutput(bool flag)
{
	if(m_mainOutput)
	{
		
		// Apply geometry

		
		screenIndexChanged(m_outputCombo->currentIndex());
		m_mainOutput->setVisible(flag);
		//m_mainOutput->setWindowState(m_mainOutput->windowState() ^ Qt::WindowFullScreen);
	
		//if(flag)
		//	screenIndexChanged(m_outputCombo->currentIndex());

	}
	
	
}
	
void MainWindow::createStatusBar()
{
	statusBar()->showMessage(tr("Ready"));
}

void MainWindow::readSettings()
{
	QSettings settings;
	QPoint pos = settings.value("mainwindow/pos", QPoint(10, 10)).toPoint();
	QSize size = settings.value("mainwindow/size", QSize(640,480)).toSize();
	move(pos);
	resize(size);

	m_mainSplitter->restoreState(settings.value("mainwindow/main_splitter").toByteArray());
	m_editSplitter->restoreState(settings.value("mainwindow/left_splitter").toByteArray());

}

void MainWindow::writeSettings()
{
	QSettings settings;
	settings.setValue("mainwindow/pos", pos());
	settings.setValue("mainwindow/size", size());

	settings.setValue("mainwindow/main_splitter",m_mainSplitter->saveState());
	settings.setValue("mainwindow/left_splitter",m_editSplitter->saveState());
}

void MainWindow::newFile()
{
	LiveScene *old = m_currentScene;
	
	setCurrentLayer(0);
	
	LiveScene *scene = new LiveScene();
	loadLiveScene(scene);
	
	if(old)
		old->deleteLater();
		
	setWindowTitle("New File - LiveMix");
}

void MainWindow::open()
{
	QString curFile = m_currentFile;
	if(curFile.trimmed().isEmpty())
		curFile = QSettings().value("last-livemix-file","").toString();

	QString fileName = QFileDialog::getOpenFileName(this, tr("Select LiveMix File"), curFile, tr("LiveMix File (*.livemix *.lmx);;Any File (*.*)"));
	if(fileName != "")
	{
		QSettings().setValue("last-livemix-file",fileName);
		if(QFile(fileName).exists())
		{
			loadFile(fileName);
		}
		else
		{
			QMessageBox::critical(this,tr("File Does Not Exist"),tr("Sorry, but the file you chose does not exist. Please try again."));
		}
	}
	
}

void MainWindow::loadFile(const QString& fileName)
{
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly)) 
	{
		QMessageBox::critical(0, tr("Loading error"), tr("Unable to read file %1").arg(fileName));
		return;
	}
	
	QSettings().setValue("last-lmx-file",fileName);

	m_currentFile = fileName;
	
	QByteArray array = file.readAll();
	
	LiveScene *old = m_currentScene;

	LiveScene *scene = new LiveScene();
	scene->fromByteArray(array);
	
	loadLiveScene(scene);
	
	if(old)
		old->deleteLater();
		
	setWindowTitle(QString("%1 - LiveMix").arg(QFileInfo(fileName).fileName()));
}

void MainWindow::save(const QString& filename)
{
	if(!m_currentScene)
	{
		QMessageBox::warning(0, tr("Save Error"), tr("No current scene."));
		return;
	}
		
	QString tmp = filename;
	if(tmp.isEmpty())
		tmp = m_currentFile;
	else
		m_currentFile = tmp;
		
	if(tmp.isEmpty())
	{
		saveAs();
		return;
	}
	
	QFile file(tmp);
	// Open file
	if (!file.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(0, tr("File Error"), tr("Error saving writing file '%1'").arg(tmp));
		//throw 0;
		return;
	}
	
	//QByteArray array;
	//QDataStream stream(&array, QIODevice::WriteOnly);
	//QVariantMap map;
	
	file.write(m_currentScene->toByteArray());
	file.close();
}

void MainWindow::saveAs()
{
	QString curFile = m_currentFile;
	if(curFile.trimmed().isEmpty())
		curFile = QSettings().value("last-livemix-file","").toString();

	QString fileName = QFileDialog::getSaveFileName(this, tr("Choose a Filename"), curFile, tr("LiveMix File (*.livemix *.lmx);;Any File (*.*)"));
	if(fileName != "")
	{
		//QFileInfo info(fileName);
		//if(info.suffix().isEmpty())
			//fileName += ".dviz";
		QSettings().setValue("last-livemix-file",fileName);
		save(fileName);
		//return true;
	}

	//return false;
}

void MainWindow::addLayer(LiveLayer *layer)
{
	if(!m_currentScene)
		m_currentScene = new LiveScene();
	m_currentScene->addLayer(layer);
	layer->setZIndex(-m_currentScene->layerList().size());
	//loadLiveScene(m_currentScene);
}

void MainWindow::newCameraLayer()
{
	addLayer(new LiveVideoInputLayer());
}

void MainWindow::newVideoLayer()
{
	addLayer(new LiveVideoFileLayer());
}

void MainWindow::newTextLayer()
{
	addLayer(new LiveTextLayer());
}

void MainWindow::newImageLayer()
{
	addLayer(new LiveStaticSourceLayer());
}

