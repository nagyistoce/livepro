#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <QMainWindow>

#include "LiveScene.h"
#include "LiveTextLayer.h"
#include "LiveVideoInputLayer.h"
#include "LiveStaticSourceLayer.h"

class QAction;
class QMenu;

class QtVariantProperty;
class QtProperty;

class QtBrowserIndex;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow();


protected:
	void closeEvent(QCloseEvent *event);

private slots:
	void about();
	
	void setCurrentLayer(LiveLayer *layer);
	
	void newFile();
	void open();
	void loadFile(const QString&);
	void save(const QString& file="");
	void saveAs();
	
	void newCameraLayer();
	void newVideoLayer();
	void newTextLayer();
	void newImageLayer();
	
	void addLayer(LiveLayer *);
	
	void layerSelected(const QModelIndex &);
	void currentChanged(const QModelIndex &idx,const QModelIndex &);
	void repaintLayerList();
	void layersDropped(QList<LiveLayer*>);
	
	void slotTimelineTableCellActivated(int,int);
	void slotTimelineTableCellEdited(QTableWidgetItem*);
	void keyFrameBtnActivated();
	
	void createKeyFrame();
	void deleteKeyFrame();
	void updateKeyFrame();
	
	void deleteCurrentLayer();
	
	void slotSceneTimerTick();
	void slotPositionBoxChanged(double);
	void slotSliderBoxChanged(int);
	
	void scenePlay();
	void scenePause();
	void sceneFF();
	void sceneRR();
	void sceneStop();
	
	void loadLastFile();
	
	void duplicateLayer();
	
	void screenIndexChanged(int);
	void showLiveOutput(bool flag=true);
	
	void hideAllLayers(bool flag=true);
	
private:
	void createActions();
	void createMenus();
	void createToolBars();
	void createStatusBar();
	void readSettings();
	void writeSettings();
	
	void createLeftPanel();
	void createCenterPanel();
	void createRightPanel();
	
	void setupSampleScene();
	
	void loadLiveScene(LiveScene*);
	void loadLayerProperties(LiveLayer*);
	void removeCurrentScene();
	
	void loadKeyFramesToTable();
	
	
	QMenu *m_fileMenu;
	QMenu *m_editMenu;
	QMenu *m_helpMenu;
	QToolBar *m_fileToolBar;
	QToolBar *m_editToolBar;
	QAction *m_exitAct;
	QAction *m_aboutAct;
	QAction *m_aboutQtAct;
	
	QAction *m_openAct;
	QAction *m_newAct;
	QAction *m_saveAct;
	QAction *m_saveAsAct;
	
	QAction *m_newCameraLayerAct;
	QAction *m_newVideoLayerAct;
	QAction *m_newTextLayerAct;
	QAction *m_newImageLayerAct;
	
	QAction *m_deleteLayerAct;
	QAction *m_duplicateLayerAct;
	
	
	
	QSplitter *m_mainSplitter;
	QSplitter *m_editSplitter;
	QSplitter *m_previewSplitter;
	QWidget *m_leftPanel;
	QWidget *m_centerPanel;
	QWidget *m_rightPanel;
	
	
	QList<LiveScene*> m_scenes;
	GLWidget *m_mainViewer;
	GLWidget *m_layerViewer;
	GLWidget *m_mainOutput;
	
	QScrollArea *m_controlArea;	
	QWidget *m_controlBase;
	QWidget *m_currentLayerPropsEditor;
	
	QScrollArea *m_layerArea;
	QWidget *m_layerBase; /// TODO make this a drag widget like fridgemagnets example
	
	LiveScene *m_currentScene;
	
	QHash<LiveLayer*, LayerControlWidget*> m_controlWidgetMap;
	LiveLayer *m_currentLayer;
	
	QString m_currentFile;
	
	QListView * m_layerListView;
	class LiveSceneListModel *m_sceneModel;
	
	QSplitter *m_leftSplitter;
	QTableWidget *m_timelineTable;
	
	QPushButton *m_keyDelBtn;
	QPushButton *m_keyNewBtn;
	QPushButton *m_keyUpdateBtn;
	int m_currentKeyFrameRow;
	
	bool m_lockTimelineTableCellEditorSlot;
	
	QTimer m_scenePlayTimer;
	QPushButton *m_playButton;
	QPushButton *m_pauseButton;
	QPushButton *m_ffButton;
	QPushButton *m_rrButton;
	QPushButton *m_stopButton;
	QSlider *m_positionSlider;
	QDoubleSpinBox *m_positionBox;
	
	LiveScene::KeyFrame m_currentKeyFrame;
	double m_playTime;
	
	void showFrameForTime(double, bool forceApply=false);
	void showFrame(const LiveScene::KeyFrame &frame);
	void updateSceneTimeLength();
	
	QComboBox *m_outputCombo;
	int m_outputScreenIdx;
	QList<QRect> m_screenList;
	QPushButton *m_liveBtn;
};

#endif
