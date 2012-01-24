#ifndef DIRECTORWINDOW_H
#define DIRECTORWINDOW_H

#include <QMainWindow>
#include <QtGui>
#include <QPointer>

namespace Ui {
	class DirectorWindow;
}

class GLSceneGroupCollection;
class GLSceneGroup;
class GLScene;
class GLDrawable;
class GLPlaylistItem;
class PlayerConnection;
class PlayerConnectionList;
class GLEditorGraphicsScene;
class FlowLayout;
class VideoInputSenderManager;
class VideoReceiver;
class VideoWidget;
class GroupPlayerWidget;
class OverlayWidget;
class SwitcherWindow;
class PropertyEditorWindow;
class DirectorSourceWidget;
class DirectorMdiSubwindow;
class HistogramWindow;
class InputBalanceWindow;
class CameraMixerWidget;
class GLVideoInputDrawable;
class VideoPlayerWidget;
class GLVideoFileDrawable;
class GLVideoDrawable;
#include "GLDrawable.h"

class DirectorWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit DirectorWindow(QWidget *parent = 0);
	~DirectorWindow();
	
	PlayerConnectionList *players() { return m_players; }
	
	QList<QMdiSubWindow*> subwindows();
	 
	double fadeSpeedTime();
	
	SwitcherWindow *switcher() { return m_switcherWin; }
	
	bool isBlack() { return m_isBlack; }

signals:
	void closed();
	
	void subwindowAdded(QMdiSubWindow*);
	void subwindowActivated(QMdiSubWindow*);
	
public slots:
	
	void showPlayerSetupDialog();
	void showMidiSetupDialog();
	
	
	void fadeBlack(bool toBlack=true);
	void setFadeSpeedPercent(int);
	void setFadeSpeedTime(double);
	
	void showPlayerLiveMonitor();
	
	/// DirectorSourceWidget subclasses can use this to request the property editor be displayed for their instance.
	void showPropertyEditor(DirectorSourceWidget*);

private slots:
	void playerAdded(PlayerConnection *);
	void playerRemoved(PlayerConnection *);
	
	void chooseOutput();
	
	void updateWindowMenu();
	void updateMenus();
	void setActiveSubWindow(QWidget *window);
	
	void videoInputListReceived(const QStringList&);
	
	GroupPlayerWidget * addGroupPlayer();
	OverlayWidget * addOverlay();
	CameraMixerWidget * addCameraMixer();
	VideoPlayerWidget * addVideoPlayer();
	
	void showAllSubwindows();
	void createUserSubwindows();
	void applyTiling();
	
	void showPreviewWin();
	void showPropEditor();
	void showSwitcher();
	
	void showHistoWin();
	void showCamColorWin();
	
	DirectorMdiSubwindow *addSubwindow(QWidget*);
	
protected:
	void closeEvent(QCloseEvent *event);
	void showEvent(QShowEvent *event);
	
	void setupUI();
	void readSettings();
	void writeSettings();
	
	
	void showPlayerLiveMonitor(PlayerConnection *con);
	
	friend class DirectorMdiSubwindow;
	static QMap<QString,QRect> s_storedSystemWindowsGeometry;
	
	QMdiSubWindow *windowForWidget(QWidget*);
	
private:
	void initConnection(PlayerConnection*);
	
	Ui::DirectorWindow *ui;
	
	PlayerConnectionList *m_players;
	
	GLEditorGraphicsScene *m_graphicsScene;
	
	VideoInputSenderManager *m_vidSendMgr;
	
	QList<QPointer<VideoReceiver> > m_receivers;
	
	bool m_connected;
	
	QSignalMapper *m_windowMapper;
	QAction *m_closeAct;
	QAction *m_closeAllAct;
	QAction *m_tileAct;
	QAction *m_cascadeAct;
	QAction *m_nextAct;
	QAction *m_previousAct;
	QAction *m_separatorAct;
	
	bool m_hasVideoInputsList;	
	GLSceneGroup *m_camSceneGroup;
	
	QVariantList m_storedWindowOptions;
	
	QPointer<PropertyEditorWindow> m_propWin;
	QPointer<SwitcherWindow> m_switcherWin;
	QPointer<HistogramWindow> m_histoWin;
	QPointer<InputBalanceWindow> m_inputBalanceWin;
	
	bool m_isBlack;
	
};

class DirectorSourceWidget : public QWidget
{
public:
	DirectorSourceWidget(DirectorWindow *dir)
		: QWidget(dir)
		, m_dir(dir)
		, m_switcher(0)
	{}
	
	virtual ~DirectorSourceWidget() {}
	
	/// Returns the SwitcherWindow currently in use
	virtual SwitcherWindow *switcher() { return m_switcher; }
	
	/// Subclasses are expected to call SwitcherWindow::notifyIsLive(DirectorSourceWidget*) to notify the switcher when they are spontaneously switched live.
	virtual void setSwitcher(SwitcherWindow* switcher) { m_switcher = switcher; }
	
	/// The SwitcherWindow will call this method to request subclasses to go live.
	/// Subclasses may return false if there is some reason they could not go live.
	virtual bool switchTo() { return false; };
	
	/// If, for some reason, a subclass is not able to be switched to (for example, OverlayWidget), then overload this to return false so the SwitcherWindow can ignore this subclass.
	virtual bool canSwitchTo() { return true; }
	
	/// Subclasses are to use these two methods (saveToMap() and loadFromMap()) to load/save properties between sessions
	virtual QVariantMap saveToMap() { return QVariantMap(); }
	virtual void loadFromMap(const QVariantMap&) {}
	
	/// Subclasses are expected to return the currently "active" scene displayed in the window
	virtual GLScene *scene() { return 0; }

protected:
	DirectorWindow *m_dir;
	SwitcherWindow *m_switcher;
};

class GroupPlayerWidget : public DirectorSourceWidget
{
	Q_OBJECT
public:
	GroupPlayerWidget(DirectorWindow*);
	~GroupPlayerWidget();
	
	QString file();// { return m_collection->fileName(); }
	int currentIndex() { return m_combo->currentIndex(); }
	
	virtual QVariantMap saveToMap();
	virtual void loadFromMap(const QVariantMap&);
	virtual GLScene *scene() { return m_scene; }

public slots:
	// DirectorSourceWidget::	
	virtual bool switchTo();
	
	void setCurrentIndex(int x) { m_combo->setCurrentIndex(x); }
 	bool loadFile(QString);
 	
	void saveFile();
	void openEditor();
	void browse();
	void newFile();
 
private slots:
	void selectedGroupIndexChanged(int);

private:
	GLWidget *m_glw;
	GLSceneGroup *m_setGroup;
	GLScene *m_scene;
	GLSceneGroupCollection *m_collection;
	DirectorWindow *m_director;
	QComboBox *m_combo;
};

class CameraWidget : public DirectorSourceWidget
{
	Q_OBJECT
public:
	CameraWidget(DirectorWindow*, VideoReceiver*, QString con, GLSceneGroup *camSceneGroup, int index);
	~CameraWidget();
	
	virtual QVariantMap saveToMap();
	virtual void loadFromMap(const QVariantMap&);
	
	VideoReceiver *receiver() { return m_rx; }
	QString con() { return m_con; }
	
	GLVideoDrawable *drawable() { return m_drawable; }
	GLWidget *glWidget() { return m_glWidget; }
	
public slots:
	// DirectorSourceWidget::	
	virtual bool switchTo();
	void setDeinterlace(bool);
	
	void showPropertyEditor();
	
protected:
	void contextMenuEvent(QContextMenuEvent * event);

private:
	GLWidget *m_glWidget;
	GLVideoDrawable *m_drawable;
	VideoReceiver *m_rx;
	QMenu *m_configMenu;
	QString m_con;
	GLSceneGroup *m_camSceneGroup;
	bool m_deinterlace;
};

class OverlayWidget : public DirectorSourceWidget
{
	Q_OBJECT
public:
	OverlayWidget(DirectorWindow*);
	~OverlayWidget();
	
	QString file();// { return m_collection->fileName(); }
	int currentIndex() { return m_combo->currentIndex(); }
	
	// DirectorSourceWidget::	
	virtual bool canSwitchTo() { return false; } // overlays are added, not switched to
	virtual QVariantMap saveToMap();
	virtual void loadFromMap(const QVariantMap&);
	virtual GLScene *scene() { return m_scene; }
	
public slots:
	void setCurrentIndex(int x) { m_combo->setCurrentIndex(x); }
	bool loadFile(QString);
	
	void showOverlay();
	void hideOverlay();
	
	void saveFile();
	void openEditor();
	void browse();
	void newFile();
	
private slots:
	void clicked();
 	void selectedGroupIndexChanged(int);

private:
	GLWidget *m_glw;
	GLSceneGroup *m_setGroup;
	GLScene *m_scene;
	GLSceneGroupCollection *m_collection;
	DirectorWindow *m_director;
	QComboBox *m_combo;
};

class SwitcherWindow : public QWidget
{
	Q_OBJECT
public:
	SwitcherWindow(DirectorWindow *);
	
public slots:
	void notifyIsLive(DirectorSourceWidget*);
	void activateSource(int);
// 	void activateSource1() { activateSource(0); }
// 	void activateSource2() { activateSource(1); }
// 	void activateSource3() { activateSource(2); }
// 	void activateSource4() { activateSource(3); }
// 	void activateSource5() { activateSource(4); }
// 	void activateSource6() { activateSource(5); }
// 	void activateSource7() { activateSource(6); }
// 	void activateSource8() { activateSource(7); }
// 	void activateSource9() { activateSource(8); }
// 	void activateSource10() { activateSource(9); }

protected:
	bool eventFilter(QObject *, QEvent *);
	void showEvent(QShowEvent*);
	void keyPressEvent(QKeyEvent *event);
	
private slots:
	void buttonClicked();
	void subwindowAdded(QMdiSubWindow*);
	void windowClosed();
	
private:
	void setupButtons();
	
	DirectorWindow *m_dir;
	QHBoxLayout *m_layout;
	QMap<DirectorSourceWidget*,QPushButton*> m_buttons;
	QMap<QPushButton*,DirectorSourceWidget*> m_sources;
	QPushButton *m_lastBtn;
	QList<DirectorSourceWidget*> m_sourceList;
	
};


class PropertyEditorWindow : public QWidget
{
	Q_OBJECT
public:
	PropertyEditorWindow(DirectorWindow *);
	
	void setSourceWidget(DirectorSourceWidget*);
	
private slots:
	void subwindowActivated(QMdiSubWindow*);
	void sourceDestroyed();
	
	void sendVidOpts();
	
	void setAdvancedFilter(QString);
	
	void loadVidOpts(bool setSource=true);
	void saveVidOpts();
	void deleteVidOpt();
	
	void setStraightValue(double);
	
private:
	int indexOfFilter(int);
	
	QVBoxLayout *m_layout;
	QPointer<DirectorSourceWidget> m_source;
	QPointer<QWidget> m_sharpAmountWidget;
	DirectorWindow *m_dir;
	GLVideoDrawable *m_vid;
	
	QPointer<QComboBox> m_settingsCombo;
	QList<QVariantMap>  m_settingsList;
	
};

class HistogramFilter;
class VideoSource;
class HistogramWindow : public QWidget
{
	Q_OBJECT
public:
	HistogramWindow(DirectorWindow *);
	bool setInput(QMdiSubWindow*);
	
private slots:
	void inputIdxChanged(int);
	
	void subwindowAdded(QMdiSubWindow*);
	void windowClosed();
	void buildCombo();
	
private:
// 	class Source
// 	{
// 	public:
// 		QString title;
// 		VideoSource *source;
// 	};
	
	QComboBox *m_combo;
	
	
	DirectorWindow *m_dir;
	HistogramFilter *m_filter;
	QList<VideoSource*> m_sources;
};

class CameraMixerWidget : public DirectorSourceWidget
{
	Q_OBJECT
public:
	CameraMixerWidget(DirectorWindow *);
	
// 	virtual QVariantMap saveToMap();
// 	virtual void loadFromMap(const QVariantMap&);
	virtual GLScene *scene() { return m_scene; }
	
public slots:
	virtual bool switchTo();
	
private slots:
	void input1Changed(int);
	void input2Changed(int);
	void layoutChanged(int);
	
	void subwindowAdded(QMdiSubWindow*);
	void windowClosed();
	void buildCombos();
	
private:
	QComboBox *m_combo1;
	QComboBox *m_combo2;
	QComboBox *m_layoutCombo;
	
	QStringList m_conList;
	
	GLWidget *m_glw;
	GLSceneGroup *m_setGroup;
	GLScene *m_scene;
	GLVideoInputDrawable *m_cam1;
	GLVideoInputDrawable *m_cam2;
	GLSceneGroupCollection *m_collection;
};

class VideoInputColorBalancer;
class InputBalanceWindow : public QWidget
{
	Q_OBJECT
public:
	InputBalanceWindow(DirectorWindow*);
	
private slots:
	void masterChanged(int);
	void slaveChanged(int);
	
	void subwindowAdded(QMdiSubWindow*);
	void windowClosed();
	void buildCombos();
	
private:
// 	class Source
// 	{
// 	public:
// 		QString title;
// 		VideoSource *source;
// 	};
// 	
	VideoInputColorBalancer *m_balancer;
	QList<VideoReceiver*> m_sources;
	QComboBox *m_combo1;
	QComboBox *m_combo2;
	DirectorWindow *m_director;

};


class VideoPlayerWidget : public DirectorSourceWidget
{
	Q_OBJECT
public:
	VideoPlayerWidget(DirectorWindow*);
	~VideoPlayerWidget();
	
	GLVideoDrawable *drawable() { return (GLVideoDrawable*)m_video; }
	
	QString file() { return m_filename; }
// 	double inPoint() { return m_in; }
// 	double outPoint() { return m_out; }
	double position() { return m_pos; }
	bool isMuted() { return m_muted; }
	int volume() { return m_volume; }
	
	virtual GLScene *scene() { return m_scene; }
	virtual QVariantMap saveToMap();
	virtual void loadFromMap(const QVariantMap&);
	
	GLWidget *glWidget() { return m_glw; }
	
public slots:
	virtual bool switchTo();
// 	void setInPoint(double in);
// 	void setOutPoint(double out);
	void setMuted(bool muted);
	void setVolume(int volume);
	void setPosition(double);
	void setPosition(int value) { setPosition((double)value); }
	void play();
	void pause();
	void togglePlay();
	
 	bool loadFile(QString);
 	
	void browse();
	void showPropertyEditor();
 
private slots:
	//void receivedPosition(int);
	
	void positionChanged(qint64 position);
	void durationChanged(double duration); // in seconds
	void statusChanged(int);
	
	void sliderPressed();
	void sliderReleased();

private:
	void syncProperty(QString prop, QVariant value);
	void updatePositionUI(int);
	
	GLWidget *m_glw;
	GLSceneGroup *m_group;
	GLScene *m_scene;
	GLSceneGroupCollection *m_collection;
	DirectorWindow *m_director;
	GLVideoFileDrawable *m_video;
	
	QPushButton *m_playPauseBtn;
	QSlider *m_seekSlider;
	QLabel *m_timeLabel;
	QSlider *m_volumeSlider;
	QPushButton *m_muteButton;
	
	bool m_lockSetPosition;
	
	bool m_isLocalPlayer;
	
	QString m_filename;
// 	double m_in;
// 	double m_out;
	double m_pos;
	bool m_muted;
	int m_volume;
	double m_dur;
	
	bool m_isSliderDown;
	
};


class DirectorMdiSubwindow : public QMdiSubWindow
{
	Q_OBJECT
public:
	DirectorMdiSubwindow(QWidget *child=0);
	
	void setWidget(QWidget *);
	
protected slots:
	void applyGeom();

protected:
	void showEvent(QShowEvent *event);
	void moveEvent(QMoveEvent * moveEvent);
	
	QRect m_geom;
	 
};

#endif // DIRECTORWINDOW_H
