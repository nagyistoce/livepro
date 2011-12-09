#include "DirectorWindow.h"
#include "ui_DirectorWindow.h"
#include "PlayerSetupDialog.h"
#include "EditorWindow.h"
#include "../livemix/ExpandableWidget.h"
#include "../livemix/EditorUtilityWidgets.h"
#include "RtfEditorWindow.h"
#include "GLEditorGraphicsScene.h"
#include "../qtcolorpicker/qtcolorpicker.h"

#include "VideoInputColorBalancer.h"
#include "HistogramFilter.h"

#include "FlowLayout.h"

#include <QFileInfo>
#include <QFileDialog>
#include <QApplication>
#include <QGLWidget>

#include "GLDrawables.h"
#include "GLSceneGroup.h"
#include "PlayerConnection.h"

#include "../livemix/VideoWidget.h"
#include "VideoInputSenderManager.h"
#include "VideoReceiver.h"

#include "DrawableDirectorWidget.h"

#include "GLWidget.h"

#include "DirectorMidiInputAdapter.h"
#include "MidiInputSettingsDialog.h"

#include <QCDEStyle>
#include <QCleanlooksStyle>

QMap<QString,QRect> DirectorWindow::s_storedSystemWindowsGeometry;

DirectorWindow::DirectorWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::DirectorWindow)
	, m_players(0)
	, m_hasVideoInputsList(false)
	, m_isBlack(0)
{
	ui->setupUi(this);
	
	setWindowTitle("LiveMix Director");
	
	setupUI();
	
	// For some reason, these get unset from NULL between the ctor init above and here, so reset to zero
	m_players = 0;
	
	m_vidSendMgr = 0;
// 	m_vidSendMgr = new VideoInputSenderManager();
// 	m_vidSendMgr->setSendingEnabled(true);
	
	readSettings();
	
	// Connect to any players that have the 'autoconnect' option checked in the PlayerSetupDialog
// 	foreach(PlayerConnection *con, m_players->players())
// 		if(!con->isConnected() && con->autoconnect())
// 			con->connectPlayer();

	m_camSceneGroup = new GLSceneGroup();
	GLScene *scene = new GLScene();
	GLVideoInputDrawable *vid = new GLVideoInputDrawable();
	scene->addDrawable(vid);
	m_camSceneGroup->addScene(scene);
	
	chooseOutput();
	

	setFocusPolicy(Qt::StrongFocus);
}


DirectorWindow::~DirectorWindow()
{
	delete ui;
}

QList<QMdiSubWindow*> DirectorWindow::subwindows()
{
	return ui->mdiArea->subWindowList();
}

void DirectorWindow::chooseOutput()
{
	m_hasVideoInputsList = false;
		
	bool wasOpen = false;
	if(!ui->mdiArea->subWindowList().isEmpty())
	{
		wasOpen = true;
		ui->mdiArea->closeAllSubWindows();
		writeSettings();
	}
	
	// Dont prompt the user if only one player is connected
	QList<PlayerConnection*> playerList = m_players->players();
	if(playerList.size() == 1)
	{
		initConnection(playerList.first());
		return;
	}
	
	QDialog dlg;
	dlg.setWindowTitle("Choose Output");
	
	QVBoxLayout *vbox = new QVBoxLayout(&dlg);
		
	QHBoxLayout *lay1 = new QHBoxLayout();
	
	QComboBox *sourceBox = new QComboBox();
	QStringList itemList;
	
	qSort(playerList.begin(), playerList.end(), PlayerConnection::sortByUseCount);
	foreach(PlayerConnection *con, playerList)
	{
		//itemList << QString("Player: %1").arg(con->name());
		itemList << con->name();
		qDebug() << "DirectorWindow::choseOutput:"<<con->name()<<" useCount:"<<con->useCount();
	}
	
	
	sourceBox->addItems(itemList);
	sourceBox->setCurrentIndex(0);
	
	lay1->addWidget(new QLabel("Output:"));
	lay1->addWidget(sourceBox);
	 
	vbox->addLayout(lay1);
	
	QHBoxLayout *buttons = new QHBoxLayout();
	QPushButton *setup = new QPushButton("Setup");
	buttons->addWidget(setup);
	
	buttons->addStretch(1);
	
	QPushButton *ok = new QPushButton("ok");
	buttons->addWidget(ok);
	ok->setDefault(true);
	
	connect(setup, SIGNAL(clicked()), &dlg, SLOT(reject()));
	connect(ok, SIGNAL(clicked()), &dlg, SLOT(accept()));
	
	m_connected = false;
		
	//vbox->addStretch(1);
	vbox->addLayout(buttons);
	dlg.setLayout(vbox);
	dlg.adjustSize();
	if(dlg.exec())
	{
		int idx = sourceBox->currentIndex();
		PlayerConnection *player = playerList.at(idx);
		
		initConnection(player);
	}
	else
	{
		showPlayerSetupDialog();
	}
	
	if(wasOpen)
	{
		readSettings();
		QTimer::singleShot(5, this, SLOT(showAllSubwindows()));
		QTimer::singleShot(10, this, SLOT(createUserSubwindows()));
	}
}


void DirectorWindow::initConnection(PlayerConnection *player)
{
	connect(player, SIGNAL(videoInputListReceived(const QStringList&)), this, SLOT(videoInputListReceived(const QStringList&)));
	
	foreach(PlayerConnection *con, m_players->players())
	{
		if(con->isConnected() && con != player)
			con->disconnectPlayer();
	}
	
	if(!player->isConnected())
		player->connectPlayer();
		
	if(player->videoIputsReceived())
		videoInputListReceived(player->videoInputs());
	
	m_connected = true;
	
	showPlayerLiveMonitor(player);
}

void DirectorWindow::videoInputListReceived(const QStringList& inputs)
{
	qDebug() << "DirectorWindow::videoInputListReceived: "<<inputs;
	//qDebug() << "DirectorWindow::videoInputListReceived: m_hasVideoInputsList:"<<m_hasVideoInputsList;
	if(m_hasVideoInputsList)
		return;
	if(inputs.isEmpty())
		return;
	m_hasVideoInputsList = true;
 	int index = 0;
 	//qDebug() << "DirectorWindow::videoInputListReceived: Creating windows";
	foreach(QString con, inputs)
	{
		QStringList opts = con.split(",");
		//qDebug() << "DirectorWindow::videoInputListReceived: Con string: "<<con;
		foreach(QString pair, opts)
		{
			QStringList values = pair.split("=");
			QString name = values[0].toLower();
			QString value = values[1];
			
			//qDebug() << "DirectorWindow::videoInputListReceived: Parsed name:"<<name<<", value:"<<value;
			if(name == "net")
			{
				QStringList url = value.split(":");
				QString host = url[0];
				int port = url.size() > 1 ? url[1].toInt() : 7755;
				
				VideoReceiver *rx = VideoReceiver::getReceiver(host,port);
				
				if(!rx)
				{
					qDebug() << "DirectorWindow::videoInputListReceived: Unable to connect to "<<host<<":"<<port;
					QMessageBox::warning(this,"Video Connection",QString("Sorry, but we were unable to connect to the video transmitter at %1:%2 - not sure why.").arg(host).arg(port));
				}
				else
				{
					m_receivers << QPointer<VideoReceiver>(rx);
					
					qDebug() << "DirectorWindow::videoInputListReceived: Connected to "<<host<<":"<<port<<", creating widget...";

					CameraWidget *camera = new CameraWidget(this, rx, con, m_camSceneGroup, ++ index);
					
					addSubwindow(camera);
				}
			}
		}
	}
	
	// DirectorMidiInputAdapter uses the switcher window to switch sources - so add it automatically once cameras are received
	showSwitcher();
	
}

DirectorMdiSubwindow *DirectorWindow::addSubwindow(QWidget *widget)
{
	DirectorMdiSubwindow *win = new DirectorMdiSubwindow(widget);
	ui->mdiArea->addSubWindow(win);
	//qDebug() << "DirectorWindow::addSubwindow: win:"<<win<<", widget:"<<widget;
	emit subwindowAdded(win);
	return win;
}

void DirectorWindow::showPlayerLiveMonitor()
{
	// Compile list of connected player names for the combo box selection
	QComboBox *sourceBox = new QComboBox();
	QStringList itemList;
	QList<PlayerConnection*> players;
	foreach(PlayerConnection *con, m_players->players())
		if(con->isConnected())
		{
			//itemList << QString("Player: %1").arg(con->name());
			itemList << con->name();
			players  << con;
		}
	
	// Dont prompt the user if only one player is connected
	if(players.size() == 1)
	{
		showPlayerLiveMonitor(players.first());
		return;
	}
	
	// Create the dialog box layout
	sourceBox->addItems(itemList);
	QDialog dlg;
	dlg.setWindowTitle("Choose Player");
	
	QVBoxLayout *vbox = new QVBoxLayout(&dlg);
	
	QFormLayout *form = new QFormLayout();
	 
	form->addRow("Player to View:", sourceBox);
	 
	QHBoxLayout *buttons = new QHBoxLayout();
	buttons->addStretch(1);
	QPushButton *cancel = new QPushButton("Cancel");
	buttons->addWidget(cancel);
	
	QPushButton *ok = new QPushButton("ok");
	buttons->addWidget(ok);
	ok->setDefault(true);
	
	connect(cancel, SIGNAL(clicked()), &dlg, SLOT(reject()));
	connect(ok, SIGNAL(clicked()), &dlg, SLOT(accept()));
	
	vbox->addLayout(form);
	vbox->addStretch(1);
	vbox->addLayout(buttons);
	dlg.setLayout(vbox);
	dlg.adjustSize();
	
	if(dlg.exec())
	{
		int idx = sourceBox->currentIndex();
		if(idx < 0 || idx >= players.size())
		{
			qDebug() << "Player selection error";
			return;	
		}
		
		PlayerConnection *con = players[idx];
		
		showPlayerLiveMonitor(con);
		
	}
}

void DirectorWindow::showPlayerLiveMonitor(PlayerConnection *con)
{
	QString host = con->host();
	int port = 9978;
	VideoReceiver *rx = VideoReceiver::getReceiver(host,port);
			
	if(!rx)
	{
		qDebug() << "DirectorWindow::showPlayerLiveMonitor: Unable to connect to "<<host<<":"<<port;
		QMessageBox::warning(this,"Video Connection",QString("Sorry, but we were unable to connect to the video transmitter at %1:%2 - not sure why.").arg(host).arg(port));
	}
	else
	{
		m_receivers << QPointer<VideoReceiver>(rx);
		
		qDebug() << "DirectorWindow::showPlayerLiveMonitor: Connected to "<<host<<":"<<port<<", creating widget...";
		
		//VideoWidget *vid = new VideoWidget();
		//qDebug() << "DirectorWindow::showPlayerLiveMonitor: Created VideoWidget:"<<vid;
		//vid->setVideoSource(rx);
		
		GLWidget *vid = new GLWidget();
		
		GLVideoDrawable *gld = new GLVideoDrawable();
		gld->setVideoSource(rx);
		vid->addDrawable(gld);
		
		vid->setWindowTitle(QString("Live - %1").arg(con->name()));
		gld->setObjectName(qPrintable(vid->windowTitle()));
		vid->resize(320,240);
				
		addSubwindow(vid);
		
		connect(this, SIGNAL(closed()), vid, SLOT(deleteLater()));
	}
}

void DirectorWindow::showMidiSetupDialog()
{
	MidiInputSettingsDialog *d = new MidiInputSettingsDialog(DirectorMidiInputAdapter::instance(this),this);
	d->exec();
}


void DirectorWindow::setupUI()
{
	// Create adapter instance in case not already created
	(void*)DirectorMidiInputAdapter::instance(this);
	
// 	m_graphicsScene = new QGraphicsScene();
// 	ui->graphicsView->setScene(m_graphicsScene);
	//ui->graphicsView->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
// 	ui->graphicsView->setBackgroundBrush(Qt::black);
// 	ui->graphicsView->setAutoResize(true);
	//m_graphicsScene->setSceneRect(QRectF(0,0,1000.,750.));
	
	connect(ui->fadeBlackBtn, SIGNAL(toggled(bool)), this, SLOT(fadeBlack(bool)));
	connect(ui->fadeSpeedSlider, SIGNAL(valueChanged(int)), this, SLOT(setFadeSpeedPercent(int)));
	connect(ui->fadeSpeedBox, SIGNAL(valueChanged(double)), this, SLOT(setFadeSpeedTime(double)));
	ui->fadeSpeedSlider->setValue((int)(300. / 3000. * 100));
	
	connect(ui->actionMonitor_Players_Live, SIGNAL(triggered()), this, SLOT(showPlayerLiveMonitor()));
	connect(ui->actionChoose_Output, SIGNAL(triggered()), this, SLOT(chooseOutput()));
	connect(ui->actionMIDI_Input_Settings, SIGNAL(triggered()), this, SLOT(showMidiSetupDialog()));
	connect(ui->actionExit, SIGNAL(triggered()), qApp, SLOT(quit()));
	connect(ui->actionPlayer_Setup, SIGNAL(triggered()), this, SLOT(showPlayerSetupDialog()));
	
	connect(ui->actionAdd_Video_Player, SIGNAL(triggered()), this, SLOT(addVideoPlayer()));
	connect(ui->actionAdd_Group_Player, SIGNAL(triggered()), this, SLOT(addGroupPlayer()));
	connect(ui->actionAdd_Camera_Mixer, SIGNAL(triggered()), this, SLOT(addCameraMixer()));
	connect(ui->actionAdd_Overlay, SIGNAL(triggered()), this, SLOT(addOverlay()));
	connect(ui->actionShow_Preview, SIGNAL(triggered()), this, SLOT(showPreviewWin()));
	connect(ui->actionShow_Switcher, SIGNAL(triggered()), this, SLOT(showSwitcher()));
	connect(ui->actionShow_Histogram, SIGNAL(triggered()), this, SLOT(showHistoWin()));
	connect(ui->actionShow_InputBalance, SIGNAL(triggered()), this, SLOT(showCamColorWin()));
	
	
	connect(ui->mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(updateMenus()));
	connect(ui->mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SIGNAL(subwindowActivated(QMdiSubWindow*)));
	m_windowMapper = new QSignalMapper(this);
	connect(m_windowMapper, SIGNAL(mapped(QWidget*)), this, SLOT(setActiveSubWindow(QWidget*)));
	
	m_closeAct = new QAction(tr("Cl&ose"), this);
	m_closeAct->setStatusTip(tr("Close the active window"));
	connect(m_closeAct, SIGNAL(triggered()),
		ui->mdiArea, SLOT(closeActiveSubWindow()));
	
	m_closeAllAct = new QAction(tr("Close &All"), this);
	m_closeAllAct->setStatusTip(tr("Close all the windows"));
	connect(m_closeAllAct, SIGNAL(triggered()),
		ui->mdiArea, SLOT(closeAllSubWindows()));
	
	m_tileAct = new QAction(tr("&Tile"), this);
	m_tileAct->setStatusTip(tr("Tile the windows"));
	connect(m_tileAct, SIGNAL(triggered()), ui->mdiArea, SLOT(tileSubWindows()));
	
	m_cascadeAct = new QAction(tr("&Cascade"), this);
	m_cascadeAct->setStatusTip(tr("Cascade the windows"));
	connect(m_cascadeAct, SIGNAL(triggered()), ui->mdiArea, SLOT(cascadeSubWindows()));
	
	m_nextAct = new QAction(tr("Ne&xt"), this);
	m_nextAct->setShortcuts(QKeySequence::NextChild);
	m_nextAct->setStatusTip(tr("Move the focus to the next window"));
	connect(m_nextAct, SIGNAL(triggered()),
		ui->mdiArea, SLOT(activateNextSubWindow()));
	
	m_previousAct = new QAction(tr("Pre&vious"), this);
	m_previousAct->setShortcuts(QKeySequence::PreviousChild);
	m_previousAct->setStatusTip(tr("Move the focus to the previous "
					"window"));
	connect(m_previousAct, SIGNAL(triggered()),
		ui->mdiArea, SLOT(activatePreviousSubWindow()));
	
	m_separatorAct = new QAction(this);
	m_separatorAct->setSeparator(true);
	
	updateWindowMenu();
	connect(ui->menuWindow, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu()));
}

void DirectorWindow::closeEvent(QCloseEvent */*event*/)
{
	writeSettings();
	emit closed();
}

void DirectorWindow::readSettings()
{
	QSettings settings;
	QPoint pos = settings.value("DirectorWindow/pos", QPoint(10, 10)).toPoint();
	QSize size = settings.value("DirectorWindow/size", QSize(900,700)).toSize();
	move(pos);
	resize(size);

	
// 	qreal scaleFactor = (qreal)settings.value("DirectorWindow/scaleFactor", 1.).toDouble();
// 	//qDebug() << "DirectorWindow::readSettings: scaleFactor: "<<scaleFactor;
// 	ui->graphicsView->setScaleFactor(scaleFactor);
	
	m_players = new PlayerConnectionList();
	
	connect(m_players, SIGNAL(playerAdded(PlayerConnection *)), this, SLOT(playerAdded(PlayerConnection *)));
	connect(m_players, SIGNAL(playerRemoved(PlayerConnection *)), this, SLOT(playerRemoved(PlayerConnection *)));
	
	m_players->fromByteArray(settings.value("DirectorWindow/players").toByteArray());
	
	
	QByteArray array = settings.value("DirectorWindow/windows").toByteArray();
	QDataStream stream(&array, QIODevice::ReadOnly);
	QVariantMap map;
	stream >> map;
	
	// s_storedSystemWindowsGeometry contains geometry info for 
	// camera windows and the preview window, indexed by windowtitle()
	s_storedSystemWindowsGeometry.clear();
	QVariantMap sys = map["syswin"].toMap();
	foreach(QString key, sys.keys())
	{
		QRect geom = sys[key].toRect();
		//qDebug() << "DirectorWindow::readSettings(): key:"<<key<<", geom:"<<geom;
		s_storedSystemWindowsGeometry[key] = geom;
	}
	
	// m_storedWindowOptions contains geometry and options for any
	// windows added by the user after program init
	m_storedWindowOptions = map["winopts"].toList();
	
}

void DirectorWindow::createUserSubwindows()
{
	return;
	foreach(QVariant data, m_storedWindowOptions)
	{
		QVariantMap opts = data.toMap();
		
		QString className = opts["_class"].toString();
		//qDebug() << "DirectorWindow::readSettings(): Class:"<<className<<", opts:"<<opts;
		
		QWidget *widgetPtr = 0;
		if(className == "OverlayWidget")
		{
			if(!opts["file"].toString().isEmpty())
			{
				OverlayWidget *widget = addOverlay();
				
				widget->loadFromMap(opts);
				
				qDebug() << "DirectorWindow::createUserSubwindows(): Added overlay widget:"<<widget;
				
				widgetPtr = widget;
			}
		}
		else
// 		if(className == "CameraMixerWidget")
// 		{
// 			CameraMixerWidget *widget = addCameraMixer();
// 			
// 			widget->loadFromMap(opts);
// 			
// 			qDebug() << "DirectorWindow::createUserSubwindows(): Added CameraMixerWidget widget:"<<widget;
// 			
// 			widgetPtr = widget;
// 		}
// 		else
		if(className == "GroupPlayerWidget")
		{
			if(!opts["file"].toString().isEmpty())
			{
				GroupPlayerWidget *widget = addGroupPlayer();
				
				widget->loadFromMap(opts);
				
				qDebug() << "DirectorWindow::createUserSubwindows(): Added group player widget:"<<widget;
				
				widgetPtr = widget;
			}
		}
		else
		if(className == "SwitcherWindow")
		{
			showSwitcher();
		}
		else
		if(className == "HistogramWindow")
		{
			showHistoWin();
		}
		else
		if(className == "InputBalanceWindow")
		{
			showCamColorWin();
		}
		
		if(widgetPtr)
		{
			if(QMdiSubWindow *win = windowForWidget(widgetPtr))
			{
				QRect rect = opts["_geom"].toRect();
				qDebug() << "DirectorWindow::createUserSubwindows(): Win:"<<win<<", Widget Ptr:"<<widgetPtr<<", geom:"<<rect;
				win->setGeometry(rect);
			}
			else
			{
				qDebug() << "DirectorWindow::createUserSubwindows(): Can't find window for widget ptr:"<<widgetPtr;
			}
		}
			
		// TODO add more...
	}
		
	//QTimer::singleShot(1000, this, SLOT(applyTiling()));
}

QMdiSubWindow *DirectorWindow::windowForWidget(QWidget *widget)
{
	QList<QMdiSubWindow *> windows = ui->mdiArea->subWindowList();
	foreach(QMdiSubWindow *win, windows)
		if(win->widget() == widget)
			return win;
	return 0;
}

void DirectorWindow::applyTiling()
{	
	QList<QMdiSubWindow *> windows = ui->mdiArea->subWindowList();
	if(!windows.isEmpty())
	{
		windows.first()->setFocus(Qt::OtherFocusReason);
		ui->mdiArea->tileSubWindows();
	}
}

void DirectorWindow::writeSettings()
{
	QSettings settings;
	settings.setValue("DirectorWindow/pos", pos());
	settings.setValue("DirectorWindow/size", size());

	
	//settings.setValue("DirectorWindow/scaleFactor",ui->graphicsView->scaleFactor());
	//qDebug() << "DirectorWindow::writeSettings: scaleFactor: "<<ui->graphicsView->scaleFactor();
	
	settings.setValue("DirectorWindow/players", m_players->toByteArray());
	
	QByteArray array;
	QDataStream stream(&array, QIODevice::WriteOnly);

	QVariantMap map;
	
	QVariantMap syswins;
	m_storedWindowOptions.clear();
	
	QList<QMdiSubWindow *> windows = ui->mdiArea->subWindowList();
	foreach(QMdiSubWindow *win, windows)
	{
		QWidget *widget = win->widget();
		
// 		if(GLWidget *tmp = dynamic_cast<GLWidget*>(widget))
// 		{
// 			Q_UNUSED(tmp);
// 			// camera window or preview window - just store geometry
			QRect geom = win->geometry();
			qDebug() << "DirectorWindow::writeSettings: System Window: "<<win->windowTitle()<<", Geometry:"<<geom;
			syswins[win->windowTitle()] = geom;
// 		}
// 		else
		if(OverlayWidget *tmp = dynamic_cast<OverlayWidget*>(widget))
		{
			QVariantMap opts = tmp->saveToMap();
			opts["_class"] = "OverlayWidget";
			opts["_geom"]  = win->geometry();
			m_storedWindowOptions << opts;
		}
		else
		if(GroupPlayerWidget *tmp = dynamic_cast<GroupPlayerWidget*>(widget))
		{
			QVariantMap opts = tmp->saveToMap();
			opts["_class"] = "GroupPlayerWidget";
			opts["_geom"]  = win->geometry();
			m_storedWindowOptions << opts;
		}
		else
		if(CameraMixerWidget *tmp = dynamic_cast<CameraMixerWidget*>(widget))
		{
			QVariantMap opts = tmp->saveToMap();
			opts["_class"] = "CameraMixerWidget";
			opts["_geom"]  = win->geometry();
			m_storedWindowOptions << opts;
		}
		else
		{
// 			QVariantMap opts;
// 			opts["_class"] = "OverlayWidget";
// 			opts["_geom"]  = win->geometry();
// 			m_storedWindowOptions << opts;
		}
	}
	
	map["syswin"]  = syswins;
	map["winopts"] = m_storedWindowOptions;
	stream << map;
	 
	settings.setValue("DirectorWindow/windows", array);
}

void DirectorWindow::showEvent(QShowEvent */*event*/)
{
	QTimer::singleShot(5, this, SLOT(showAllSubwindows()));
	QTimer::singleShot(10, this, SLOT(createUserSubwindows()));
}

void DirectorWindow::showAllSubwindows()
{
	QList<QMdiSubWindow *> windows = ui->mdiArea->subWindowList();
	foreach(QMdiSubWindow *win, windows)
		win->show();
}

/*

	Ui::DirectorWindow *ui;
	
	PlayerConnectionList *m_players;
	GLSceneGroupCollection *m_collection;
	
	GLSceneGroup *m_currentGroup;
	GLScene *m_currentScene;
	GLDrawable *m_currentDrawable;
	GLPlaylistItem *m_currentItem;

*/

void DirectorWindow::fadeBlack(bool toBlack)
{
	m_isBlack = toBlack;
	foreach(PlayerConnection *con, m_players->players())
		con->fadeBlack(toBlack);
}

void DirectorWindow::setFadeSpeedPercent(int value)
{
	double perc = ((double)value)/100.;
	int ms = (int)(3000 * perc);
	
	double sec = ((double)ms) / 1000.;
	if(ui->fadeSpeedBox->value() != sec)
	{
		bool block = ui->fadeSpeedBox->blockSignals(true);
		ui->fadeSpeedBox->setValue(sec);
		ui->fadeSpeedBox->blockSignals(block);
	}
	
	qDebug() << "DirectorWindow::setFadeSpeedPercent: value:"<<value<<", ms:"<<ms;
	if(m_players)
		foreach(PlayerConnection *con, m_players->players())
			con->setCrossfadeSpeed(ms);
	
}

void DirectorWindow::setFadeSpeedTime(double sec)
{
	int ms = (int)(sec * 1000.);
	if(ms < 1)
		ms = 1;
	if(ms > 3000)
		ms = 3000;
		
	int perc = (int)((((double)ms) / 3000.) * 100);
	if(ui->fadeSpeedSlider->value() != perc)
	{
		bool block = ui->fadeSpeedSlider->blockSignals(true);
		ui->fadeSpeedSlider->setValue(perc);
		ui->fadeSpeedSlider->blockSignals(block);
	}
	
	if(ui->fadeSpeedBox->value() != sec)
	{
		bool block = ui->fadeSpeedBox->blockSignals(true);
		ui->fadeSpeedBox->setValue(sec);
		ui->fadeSpeedBox->blockSignals(block);
	}
	
	qDebug() << "DirectorWindow::setFadeSpeedTime: sec:"<<sec<<", ms:"<<ms;
	foreach(PlayerConnection *con, m_players->players())
		con->setCrossfadeSpeed(ms);
	
}

double DirectorWindow::fadeSpeedTime()
{
	return ui->fadeSpeedBox->value();
}


void DirectorWindow::showPlayerSetupDialog()
{
	PlayerSetupDialog dlg;
	dlg.setPlayerList(m_players);
	dlg.exec();
	
	if(!m_connected)
		chooseOutput();
}

/*void DirectorWindow::showEditWindow()
{
	if(!m_currentGroup)
		return;
	
// 	EditorWindow *edit = new EditorWindow();
// 	edit->setGroup(m_currentGroup, m_currentScene);
// 	edit->setCanvasSize(m_collection->canvasSize());
// 	edit->show();
}
*/

void DirectorWindow::playerAdded(PlayerConnection * con)
{
	if(!con)
		return;
	
// 	connect(con, SIGNAL(playlistTimeChanged(GLDrawable*, double)), this, SLOT(playlistTimeChanged(GLDrawable*, double)));
// 	connect(con, SIGNAL(currentPlaylistItemChanged(GLDrawable*, GLPlaylistItem*)), this, SLOT(playlistItemChanged(GLDrawable*, GLPlaylistItem *)));
}

void DirectorWindow::playerRemoved(PlayerConnection * con)
{
	if(!con)
		return;
	disconnect(con, 0, this, 0);
}


void DirectorWindow::updateMenus()
{
    bool hasMdiChild = (ui->mdiArea->activeSubWindow() != 0);
    m_closeAct->setEnabled(hasMdiChild);
    m_closeAllAct->setEnabled(hasMdiChild);
    m_tileAct->setEnabled(hasMdiChild);
    m_cascadeAct->setEnabled(hasMdiChild);
    m_nextAct->setEnabled(hasMdiChild);
    m_previousAct->setEnabled(hasMdiChild);
    m_separatorAct->setVisible(hasMdiChild);
}

void DirectorWindow::updateWindowMenu()
{
	ui->menuWindow->clear();
	ui->menuWindow->addAction(m_closeAct);
	ui->menuWindow->addAction(m_closeAllAct);
	ui->menuWindow->addSeparator();
	ui->menuWindow->addAction(m_tileAct);
	ui->menuWindow->addAction(m_cascadeAct);
	ui->menuWindow->addSeparator();
	ui->menuWindow->addAction(m_nextAct);
	ui->menuWindow->addAction(m_previousAct);
	ui->menuWindow->addAction(m_separatorAct);
	
	QList<QMdiSubWindow *> windows = ui->mdiArea->subWindowList();
	m_separatorAct->setVisible(!windows.isEmpty());
	
	for (int i = 0; i < windows.size(); ++i) 
	{
		QWidget *widget = windows.at(i);
	
		QString text = tr("%1%2 %3").arg(i < 9 ? "&" : "")
					    .arg(i + 1)
					    .arg(widget->windowTitle());

		QAction *action = ui->menuWindow->addAction(text);
		action->setCheckable(true);
		action->setChecked(widget == ui->mdiArea->activeSubWindow());
		connect(action, SIGNAL(triggered()), m_windowMapper, SLOT(map()));
		m_windowMapper->setMapping(action, windows.at(i));
	}
}

void DirectorWindow::setActiveSubWindow(QWidget *window)
{
	if (!window)
		return;
	ui->mdiArea->setActiveSubWindow(qobject_cast<QMdiSubWindow *>(window));
}


VideoPlayerWidget *DirectorWindow::addVideoPlayer()
{
	VideoPlayerWidget *vid = new VideoPlayerWidget(this); 

	addSubwindow(vid);
	
	return vid;
}

GroupPlayerWidget *DirectorWindow::addGroupPlayer()
{
	GroupPlayerWidget *vid = new GroupPlayerWidget(this); 

	addSubwindow(vid);
	
	return vid;
}


CameraMixerWidget *DirectorWindow::addCameraMixer()
{
	CameraMixerWidget *vid = new CameraMixerWidget(this); 
	
	addSubwindow(vid);
	
	return vid;
}

OverlayWidget *DirectorWindow::addOverlay()
{
	OverlayWidget *vid = new OverlayWidget(this); 
	
	addSubwindow(vid);
	
	return vid;
}

void DirectorWindow::showPreviewWin()
{
	/// TODO
}

void DirectorWindow::showPropEditor()
{
// 	if(m_propWin)
// 	{
// 		m_propWin->raise();
// 		m_propWin->show();
// 	}
// 	else
// 	{
		m_propWin = new PropertyEditorWindow(this);
		addSubwindow(m_propWin);
//	}
}

void DirectorWindow::showPropertyEditor(DirectorSourceWidget *source)
{
	showPropEditor();
	if(m_propWin)
		m_propWin->setSourceWidget(source);
}

void DirectorWindow::showSwitcher()
{
	if(m_switcherWin)
	{
		m_switcherWin->raise();
		m_switcherWin->show();
	}
	else
	{
		m_switcherWin = new SwitcherWindow(this);
		addSubwindow(m_switcherWin);
	} 
}

void DirectorWindow::showHistoWin()
{
	if(m_histoWin)
	{
		m_histoWin->raise();
		m_histoWin->show();
	}
	else
	{
		m_histoWin = new HistogramWindow(this);
		addSubwindow(m_histoWin);
	} 
}

void DirectorWindow::showCamColorWin()
{
	if(m_inputBalanceWin)
	{
		m_inputBalanceWin->raise();
		m_inputBalanceWin->show();
	}
	else
	{
		m_inputBalanceWin = new InputBalanceWindow(this);
		addSubwindow(m_inputBalanceWin);
	} 
}





//////////////////////////

GroupPlayerWidget::GroupPlayerWidget(DirectorWindow *d)
	: DirectorSourceWidget(d)
	, m_setGroup(0)
	, m_scene(0)
	, m_collection(0)
	, m_director(d)
{
	m_collection = new GLSceneGroupCollection();
	GLSceneGroup *group = new GLSceneGroup();
	GLScene *scene = new GLScene();
	scene->setSceneName("New Scene");
	group->addScene(scene);
	m_collection->addGroup(group);
	
	QVBoxLayout *vbox = new QVBoxLayout(this);
	m_glw = new GLWidget(this);
	m_glw->setWindowTitle("GroupPlayerWidget");
	vbox->addWidget(m_glw);
	
	QHBoxLayout *hbox = new QHBoxLayout();
	
	m_combo = new QComboBox();
	hbox->addWidget(m_combo);
	hbox->addStretch(1);
	
	QPushButton *newf = new QPushButton(QPixmap(":/data/stock-new.png"), "");
	QPushButton *edit = new QPushButton(QPixmap(":/data/stock-edit.png"), "");
	QPushButton *browse = new QPushButton(QPixmap(":/data/stock-open.png"), "");
	hbox->addWidget(newf);
	hbox->addWidget(edit);
	hbox->addWidget(browse);
	
	vbox->addLayout(hbox);
	
	connect(newf, SIGNAL(clicked()), this, SLOT(newFile()));
	connect(edit, SIGNAL(clicked()), this, SLOT(openEditor()));
	connect(browse, SIGNAL(clicked()), this, SLOT(browse()));
	connect(m_glw, SIGNAL(clicked()), this, SLOT(switchTo()));
	connect(m_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(selectedGroupIndexChanged(int)));
	
	newFile();
}

void GroupPlayerWidget::newFile()
{
	m_collection = new GLSceneGroupCollection();
	GLSceneGroup *group = new GLSceneGroup();
	GLScene *scene = new GLScene();
	scene->setSceneName("New Scene");
	group->addScene(scene);
	m_collection->addGroup(group);
	
	m_combo->setModel(m_collection->at(0));
	
	setWindowTitle("Player - New File");
}

QVariantMap GroupPlayerWidget::saveToMap()
{
	QVariantMap map;
	map["file"] = file();
	map["idx"] = currentIndex();
	return map;
}

void GroupPlayerWidget::loadFromMap(const QVariantMap& map)
{
	loadFile(map["file"].toString());
	setCurrentIndex(map["idx"].toInt());
}

void GroupPlayerWidget::openEditor()
{
	int idx = m_combo->currentIndex();
	
	EditorWindow *edit = new EditorWindow();
	edit->setGroup(m_collection->at(0), 
		       m_collection->at(0)->at(idx));
	edit->setCanvasSize(m_collection->canvasSize());
	edit->show();
	edit->setAttribute(Qt::WA_DeleteOnClose, true);
	
	connect(edit, SIGNAL(destroyed()), this, SLOT(saveFile()));
}

void GroupPlayerWidget::browse()
{
	QSettings settings;
	QString curFile = m_collection->fileName();
	if(curFile.trimmed().isEmpty())
		curFile = settings.value("last-collection-file").toString();

	QString fileName = QFileDialog::getOpenFileName(this, tr("Select Group Collection File"), curFile, tr("GLDirector File (*.gld);;Any File (*.*)"));
	if(fileName != "")
	{
		settings.setValue("last-collection-file",fileName);
		if(!loadFile(fileName))
		{
			QMessageBox::critical(this,tr("File Does Not Exist"),tr("Sorry, but the file you chose does not exist. Please try again."));
		}
	}
}

QString GroupPlayerWidget::file() { return m_collection->fileName(); }

bool GroupPlayerWidget::loadFile(QString fileName)
{
	if(m_collection->readFile(fileName))
	{
		setWindowTitle(QString("Player - %1").arg(QFileInfo(fileName).fileName()));
		m_combo->setModel(m_collection->at(0));
		return true;
	}
	return false;
}


void GroupPlayerWidget::saveFile()
{
	QString curFile = m_collection->fileName();
	
	QSettings settings;
	
	if(curFile.trimmed().isEmpty())
	{
		curFile = settings.value("last-collection-file").toString();

		QString fileName = QFileDialog::getSaveFileName(this, tr("Choose a Filename"), curFile, tr("GLDirector File (*.gld);;Any File (*.*)"));
		if(fileName != "")
		{
			QFileInfo info(fileName);
			settings.setValue("last-collection-file",fileName);
			
			curFile = fileName;
		}
	}
	
	if(!curFile.isEmpty())
	{
		m_collection->writeFile(curFile);
	}
	
	int idx = m_combo->currentIndex();
	m_combo->setModel(m_collection->at(0));
	selectedGroupIndexChanged(idx);
}

// DirectorSourceWidget::	
bool GroupPlayerWidget::switchTo()
{
	int idx = m_combo->currentIndex();
	
	GLSceneGroup *group = m_collection->at(0); 
	GLScene *scene = group->at(idx);
	
	foreach(PlayerConnection *player, m_director->players()->players())
 	{
 		if(player->isConnected())
 		{
 			//if(player->lastGroup() != group)
 				player->setGroup(group, scene);
 			m_setGroup = group;
			//player->setUserProperty(gld, con, "videoConnection");
		}
	}
	
	if(m_switcher)
		m_switcher->notifyIsLive(this);
	
	return true;
}


GroupPlayerWidget::~GroupPlayerWidget()
{
	// TODO...?
}


void GroupPlayerWidget::selectedGroupIndexChanged(int idx)
{
	GLSceneGroup *group = m_collection->at(0); 
	GLScene *scene = group->at(idx);
	
	if(m_scene)
		m_scene->detachGLWidget();
		
	if(scene)
	{
		scene->setGLWidget(m_glw);
		m_scene = scene;
	}
	
// 	foreach(PlayerConnection *player, m_director->players()->players())
// 	{
// 		if(player->isConnected())
// 		{
// 			if(player->lastGroup() == m_setGroup)
// 				player->setGroup(m_setGroup, scene);
// 		}
// 	}
}

//////////////////////////

OverlayWidget::OverlayWidget(DirectorWindow *d)
	: DirectorSourceWidget(d)
	, m_setGroup(0)
	, m_scene(0)
	, m_collection(0)
	, m_director(d)
{
	
	QVBoxLayout *vbox = new QVBoxLayout(this);
	m_glw = new GLWidget(this);
	m_glw->setWindowTitle("OverlayWidget");
	vbox->addWidget(m_glw);
	
	// Setup Square Background
	if(true)
	{
		QSize size = QSize(1000,750);//m_glw->viewport().size().toSize();
		size /= 2.5;
		//qDebug() << "MainWindow::createLeftPanel(): size:"<<size;
		QImage bgImage(size, QImage::Format_ARGB32_Premultiplied);
		QBrush bgTexture(QPixmap("../livemix/squares2.png"));
		QPainter bgPainter(&bgImage);
		bgPainter.fillRect(bgImage.rect(), bgTexture);
		bgPainter.end();
		
		//bgImage.save("debug/bgimage.png");

		GLImageDrawable *drawable = new GLImageDrawable();
		drawable->setImage(bgImage);
		drawable->setRect(QRectF(0,0,1000,750));
		drawable->setZIndex(-100);
		drawable->setObjectName("StaticBackground");
		drawable->show();

		m_glw->addDrawable(drawable);
	}
	
	QHBoxLayout *hbox = new QHBoxLayout();
	
	m_combo = new QComboBox();
	hbox->addWidget(m_combo);
	hbox->addStretch(1);
	
	QPushButton *show = new QPushButton(QPixmap(":/data/stock-media-play.png"), "");
	QPushButton *hide = new QPushButton(QPixmap(":/data/stock-media-stop.png"), "");
	
	QPushButton *newf = new QPushButton(QPixmap(":/data/stock-new.png"), "");
	QPushButton *edit = new QPushButton(QPixmap(":/data/stock-edit.png"), "");
	QPushButton *browse = new QPushButton(QPixmap(":/data/stock-open.png"), "");
	
	hbox->addWidget(show);
	hbox->addWidget(hide);
	hbox->addWidget(newf);
	hbox->addWidget(edit);
	hbox->addWidget(browse);
	
	vbox->addLayout(hbox);
	
	connect(m_glw, SIGNAL(clicked()), this, SLOT(clicked()));
	connect(show, SIGNAL(clicked()), this, SLOT(showOverlay()));
	connect(hide, SIGNAL(clicked()), this, SLOT(hideOverlay()));
	connect(newf, SIGNAL(clicked()), this, SLOT(newFile()));
	connect(edit, SIGNAL(clicked()), this, SLOT(openEditor()));
	connect(browse, SIGNAL(clicked()), this, SLOT(browse()));
	connect(m_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(selectedGroupIndexChanged(int)));
	
	newFile();
}

QVariantMap OverlayWidget::saveToMap()
{
	QVariantMap map;
	map["file"] = file();
	map["idx"] = currentIndex();
	return map;
}

void OverlayWidget::loadFromMap(const QVariantMap& map)
{
	loadFile(map["file"].toString());
	setCurrentIndex(map["idx"].toInt());
}

void OverlayWidget::newFile()
{
	m_collection = new GLSceneGroupCollection();
	GLSceneGroup *group = new GLSceneGroup();
	GLScene *scene = new GLScene();
	scene->setSceneName("New Scene");
	group->addScene(scene);
	m_collection->addGroup(group);
	
	m_combo->setModel(m_collection->at(0));
	
	setWindowTitle("Overlay - New File");
}

void OverlayWidget::openEditor()
{
	int idx = m_combo->currentIndex();
	
	EditorWindow *edit = new EditorWindow();
	edit->setGroup(m_collection->at(0), 
		       m_collection->at(0)->at(idx));
	edit->setCanvasSize(m_collection->canvasSize());
	edit->show();
	edit->setAttribute(Qt::WA_DeleteOnClose, true);
	
	connect(edit, SIGNAL(destroyed()), this, SLOT(saveFile()));
}

void OverlayWidget::browse()
{
	QSettings settings;
	QString curFile = m_collection->fileName();
	if(curFile.trimmed().isEmpty())
		curFile = settings.value("last-collection-file").toString();

	QString fileName = QFileDialog::getOpenFileName(this, tr("Select Group Collection File"), curFile, tr("GLDirector File (*.gld);;Any File (*.*)"));
	if(fileName != "")
	{
		settings.setValue("last-collection-file",fileName);
		if(!loadFile(fileName))
		{
			QMessageBox::critical(this,tr("File Does Not Exist"),tr("Sorry, but the file you chose does not exist. Please try again."));
		}
	}
}

QString OverlayWidget::file() { return m_collection->fileName(); }

bool OverlayWidget::loadFile(QString fileName)
{
	if(m_collection->readFile(fileName))
	{
		setWindowTitle(QString("Overlay - %1").arg(QFileInfo(fileName).fileName()));
		m_combo->setModel(m_collection->at(0));
		return true;
	}
	return false;
}

void OverlayWidget::saveFile()
{
	QString curFile = m_collection->fileName();
	
	QSettings settings;
	
	if(curFile.trimmed().isEmpty())
	{
		curFile = settings.value("last-collection-file").toString();

		QString fileName = QFileDialog::getSaveFileName(this, tr("Choose a Filename"), curFile, tr("GLDirector File (*.gld);;Any File (*.*)"));
		if(fileName != "")
		{
			QFileInfo info(fileName);
			settings.setValue("last-collection-file",fileName);
			
			curFile = fileName;
		}
	}
	
	if(!curFile.isEmpty())
	{
		m_collection->writeFile(curFile);
	}
	
	int idx = m_combo->currentIndex();
	m_combo->setModel(m_collection->at(0));
	selectedGroupIndexChanged(idx);
}

void OverlayWidget::showOverlay()
{
	int idx = m_combo->currentIndex();
	
	GLSceneGroup *group = m_collection->at(0); 
	GLScene *scene = group->at(idx);
	
	foreach(PlayerConnection *player, m_director->players()->players())
		if(player->isConnected())
			player->addOverlay(scene);
}

void OverlayWidget::hideOverlay()
{
	int idx = m_combo->currentIndex();
	
	GLSceneGroup *group = m_collection->at(0); 
	GLScene *scene = group->at(idx);
	
	foreach(PlayerConnection *player, m_director->players()->players())
		if(player->isConnected())
			player->removeOverlay(scene);
}

OverlayWidget::~OverlayWidget()
{
	// TODO...?
}


void OverlayWidget::selectedGroupIndexChanged(int idx)
{
	GLSceneGroup *group = m_collection->at(0); 
	GLScene *scene = group->at(idx);
	
	if(m_scene)
		m_scene->detachGLWidget();
		
	if(scene)
	{
		scene->setGLWidget(m_glw);
		m_scene = scene;
	}
	
// 	foreach(PlayerConnection *player, m_director->players()->players())
// 	{
// 		if(player->isConnected())
// 		{
// 			if(player->lastGroup() == m_setGroup)
// 				player->setGroup(m_setGroup, scene);
// 		}
// 	}
}

void OverlayWidget::clicked()
{
	if(m_dir)
		m_dir->showPropertyEditor(this);
}

//////////////////////////////////////////////////////


DirectorMdiSubwindow::DirectorMdiSubwindow(QWidget *child)
	: QMdiSubWindow()
{
	if(child)
		setWidget(child);
}
	
void DirectorMdiSubwindow::setWidget(QWidget *widget)
{
	//setStyle(new QCDEStyle());
	setStyle(new QCleanlooksStyle());
	//subWindow->setWidget(new QPushButton(item->itemName()));
	QMdiSubWindow::setWidget(widget);
	setAttribute(Qt::WA_DeleteOnClose);
	//subWindow->adjustSize();
	//subWindow->setWindowFlags(Qt::FramelessWindowHint);

	QTimer::singleShot(0, this, SLOT(show()));
	resize(320,270);
	
	m_geom = DirectorWindow::s_storedSystemWindowsGeometry[windowTitle()];
	qDebug() << "DirectorMdiSubwindow::setWidget(): windowTitle:"<<windowTitle()<<", m_geom:"<<m_geom;
}

void DirectorMdiSubwindow::showEvent(QShowEvent *)
{
	//QTimer::singleShot(5, this, SLOT(applyGeom()));
	applyGeom();
}

void DirectorMdiSubwindow::applyGeom()
{
	if(m_geom.isValid())
		setGeometry(m_geom);
		
	qDebug() << "DirectorMdiSubwindow::applyGeometry(): windowTitle:"<<windowTitle()<<", m_geom:"<<m_geom<<", confirming geom:"<<geometry();
} 

	
	//connect((QMdiSubWindow*)this, SIGNAL(closed()), widget, SLOT(deleteLater()));
	
void DirectorMdiSubwindow::moveEvent(QMoveEvent * moveEvent)
{
	qDebug() << "DirectorMdiSubwindow::moveEvent: newpos:"<<moveEvent->pos()<<", oldpos:"<<moveEvent->oldPos();
	QMdiSubWindow::moveEvent(moveEvent);
}

///////////////////////////////////////////////////////////



CameraWidget::CameraWidget(DirectorWindow* dir, VideoReceiver *rx, QString con, GLSceneGroup *group, int index)
	: DirectorSourceWidget(dir)
	, m_rx(rx)
	, m_con(con)
	, m_camSceneGroup(group)
{

	m_camSceneGroup = new GLSceneGroup();
	GLScene *scene = new GLScene();
	GLVideoInputDrawable *vidgld = new GLVideoInputDrawable();
	vidgld->setVideoConnection(con);
	scene->addDrawable(vidgld);
	m_camSceneGroup->addScene(scene);
	
	m_drawable = vidgld;
	
	QVBoxLayout *vbox = new QVBoxLayout(this);
	vbox->setContentsMargins(0,0,0,0);
	
	m_glWidget = new GLWidget();
	vbox->addWidget(m_glWidget);

	//GLVideoDrawable *gld = new GLVideoDrawable();
	//gld->setVideoSource(rx);
	//vid->addDrawable(gld);
	vidgld->setVideoConnection(con);
	m_glWidget->addDrawable(vidgld);
	
	setWindowTitle(QString("Camera %1").arg(index));
	//gld->setObjectName(qPrintable(windowTitle()));
	vidgld->setObjectName(qPrintable(windowTitle()));
	resize(320,240);
	//vid->show();
	
	m_configMenu = new QMenu(this);
	QAction * action;
	QSettings settings;
	
	// Add Properties menu item
	action = m_configMenu->addAction("Properties...");
	//action->setCheckable(true);
	//action->setChecked(m_deinterlace);
	connect(action, SIGNAL(triggered()), this, SLOT(showPropertyEditor()));
	
	
	// Add Deinterlace menu item
	m_deinterlace = settings.value(QString("%1/deint").arg(con),false).toBool();
	
	action = m_configMenu->addAction("Deinterlace");
	action->setCheckable(true);
	action->setChecked(m_deinterlace);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(setDeinterlace(bool)));
	
	// Switch to cam on click
	connect(m_glWidget, SIGNAL(clicked()), this, SLOT(switchTo()));
}
	
void CameraWidget::showPropertyEditor()
{	
	if(m_dir)
		m_dir->showPropertyEditor(this);
}	
	
void CameraWidget::contextMenuEvent(QContextMenuEvent * event)
{
	if(m_configMenu)
		m_configMenu->popup(event->globalPos());
}

CameraWidget::~CameraWidget() {}
	
// DirectorSourceWidget::	
bool CameraWidget::switchTo()
{
	qDebug() << "CameraWidget::switchTo: Using con string: "<<m_con;
	
 	GLDrawable *gld = m_camSceneGroup->at(0)->at(0);
 	//gld->setProperty("videoConnection", m_con);
// 	
	if(!m_dir->players())
		return false;
		
	bool cutFlag = (QApplication::keyboardModifiers() & Qt::ShiftModifier);
		
	qDebug() << "CameraWidget::switchTo: cutFlag: "<<cutFlag;
	
	double fadeSpeed = m_dir->fadeSpeedTime();
	if(cutFlag)
	{
		m_dir->setFadeSpeedTime(0);
	}
	 
	foreach(PlayerConnection *player, m_dir->players()->players())
	{
		if(player->isConnected())
		{
			if(player->lastGroup() != m_camSceneGroup)
				player->setGroup(m_camSceneGroup, m_camSceneGroup->at(0));
			player->setUserProperty(gld, m_con, "videoConnection");
		}
	}
	 
	if(cutFlag)
	{
		m_dir->setFadeSpeedTime(fadeSpeed);
	}
	
	setDeinterlace(m_deinterlace);
	
	if(m_switcher)
		m_switcher->notifyIsLive(this);
	
	return true;
}

void CameraWidget::setDeinterlace(bool flag)
{
	if(!m_dir->players())
 		return;
 		
 	m_deinterlace = flag;
 	
 	GLDrawable *gld = m_camSceneGroup->at(0)->at(0);
 	
 	if(gld->property("videoConnection").toString() != m_con)
 	{
 		qDebug() << "CameraWidget::setDeinterlace("<<flag<<"): Not setting, not currently the live camera";
 		return;
 	}
 	
 	foreach(PlayerConnection *player, m_dir->players()->players())
 	{
 		if(player->isConnected())
 		{
 			if(player->lastGroup() == m_camSceneGroup)
 			{
 				//player->setGroup(m_camSceneGroup, m_camSceneGroup->at(0));
	 			player->setUserProperty(gld, flag, "deinterlace");
	 		}
	 		else
	 		{
	 			qDebug() << "CameraWidget::setDeinterlace("<<flag<<"): Not setting, not currently the live group";
	 		}
	 	}
	 }
}

QVariantMap CameraWidget::saveToMap() { return QVariantMap(); }
void CameraWidget::loadFromMap(const QVariantMap&) {}
	

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


SwitcherWindow::SwitcherWindow(DirectorWindow * dir)
	: QWidget(dir)
	, m_dir(dir)
{
	/// TODO
	
	connect(dir, SIGNAL(subwindowAdded(QMdiSubWindow*)), this, SLOT(subwindowAdded(QMdiSubWindow*)));
	
	// Loop tru all open windows
	// Find DirectorSourceWidgets
	// Set switcher pointer on DirectorSourceWidget
	// Create buttons for all widgets (call subwindowAdded() directly)
	// Insert event filter
	// ....
	dir->installEventFilter(this);
	
	m_layout = new QHBoxLayout(this);
	m_lastBtn = 0;
	setupButtons();
	
}
	
void SwitcherWindow::notifyIsLive(DirectorSourceWidget* src)
{
	/// TODO
	// DirectorSourceWidget* will call this to notify of live
	// - find button for this DirectorSourceWidget* pointer and change status (background color...?)
	
	if(!src)
		return;
		
	QPushButton *btn = m_buttons[src];
	if(!btn)
		return;
	
	if(m_lastBtn)
		m_lastBtn->setIcon(QPixmap(":/data/stock-media-play.png"));
	m_lastBtn = btn;
	
	btn->setIcon(QPixmap(":/data/stock-media-rec.png"));
}

bool SwitcherWindow::eventFilter(QObject *obj, QEvent *event)
{
	// Translate keypress into a specific DirectorSourceWidget* and call switchTo() on that window
	if (event->type() == QEvent::KeyPress) 
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		
		int idx = keyEvent->text().toInt();
		if(idx > 0 && idx <= m_sourceList.size())
		{
			qDebug() << "SwitcherWindow::eventFilter(): "<<keyEvent->text()<<", activating input #"<<idx;
			idx --; 
			DirectorSourceWidget *src = m_sourceList.at(idx);
			src->raise();
			src->switchTo();
			notifyIsLive(src);
		}
	}
	return QObject::eventFilter(obj, event);
}
	
void SwitcherWindow::buttonClicked()
{
	/// TODO
	// Figure out which DirectorSourceWidget* goes to this button and call switchTo()
	
	QPushButton *btn = dynamic_cast<QPushButton*>(sender());
	if(!btn)
		return;
	
	DirectorSourceWidget *src = m_sources[btn];
	if(!src)
		return;
		
	if(m_lastBtn)
		m_lastBtn->setIcon(QPixmap(":/data/stock-media-play.png"));
	m_lastBtn = btn;
	
	btn->setIcon(QPixmap(":/data/stock-media-rec.png"));
	
	//src->setFocus(Qt::OtherFocusReason);
	src->raise();
	src->switchTo();
	
}

void SwitcherWindow::subwindowAdded(QMdiSubWindow* win)
{
	/// TODO
	// When window added, add button to layout
	// Connect listern for window destruction to remove button
	//qDebug() << "SwitcherWindow::subwindowAdded: win:"<<win<<", widget:"<<win->widget();
	 
	if(!win)
		return;
		
	if(DirectorSourceWidget *src = dynamic_cast<DirectorSourceWidget*>(win->widget()))
		if(src->canSwitchTo())
			setupButtons();
	
}

void SwitcherWindow::windowClosed()
{
	setupButtons();
}

void SwitcherWindow::setupButtons()
{
	
	while(m_layout->count() > 0)
	{
		QLayoutItem *item = m_layout->itemAt(m_layout->count() - 1);
		m_layout->removeItem(item);
		if(QWidget *widget = item->widget())
		{
			m_layout->removeWidget(widget);
			delete widget;
		}
			
		delete item;
		item = 0;
	}
	
	m_sourceList.clear();
	m_sources.clear();
	m_buttons.clear();
	
	QList<QMdiSubWindow*> windows = m_dir->subwindows();
	int count = 1;
	foreach(QMdiSubWindow *win, windows)
	{
		if(DirectorSourceWidget *src = dynamic_cast<DirectorSourceWidget*>(win->widget()))
		{	
			if(!src->canSwitchTo())
				continue;
				
			connect(src, SIGNAL(destroyed()), this, SLOT(windowClosed()));
			
			QPushButton *btn = new QPushButton(QPixmap(":/data/stock-media-play.png"),tr("%1: %2").arg(count++).arg(src->windowTitle()));
			//btn->setToolTip(src->windowTitle());
			//count++;
			
			connect(btn, SIGNAL(clicked()), this, SLOT(buttonClicked()));
			m_layout->addWidget(btn);
			
			m_sources[btn] = src;
			m_buttons[src] = btn;
			
			m_sourceList << src;
		}
	}
	
	//adjustSize();
	
	setWindowTitle("Switcher");
}

void SwitcherWindow::showEvent(QShowEvent*)
{
	adjustSize();
}

void SwitcherWindow::keyPressEvent(QKeyEvent *event)
{
	int idx = event->text().toInt();
	if(idx > 0 && idx <= m_sourceList.size())
	{
		qDebug() << "SwitcherWindow::keyPressEvent(): "<<event->text()<<", activating input #"<<idx;
		idx --; 
		activateSource(idx);
	}
}

void SwitcherWindow::activateSource(int idx)
{
	if(idx >= 0 && idx < m_sourceList.size())
	{
		DirectorSourceWidget *src = m_sourceList.at(idx);
		src->raise();
		src->switchTo();
		notifyIsLive(src);
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


PropertyEditorWindow::PropertyEditorWindow(DirectorWindow *win)
	: m_dir(win)
{
	// Just create a static UI, possibly with dropdown of DirectorSourceWidget* to select from...?
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0,0,0,0);
	
	QScrollArea *controlArea = new QScrollArea(this);
	controlArea->setWidgetResizable(true);
	layout->addWidget(controlArea);
	
	QWidget *controlBase = new QWidget(controlArea);
	
	m_layout = new QVBoxLayout(controlBase);
	m_layout->setContentsMargins(0,0,0,0);
	controlArea->setWidget(controlBase);
	
	setSourceWidget(0);
	
	connect(win, SIGNAL(subwindowActivated(QMdiSubWindow*)), this, SLOT(subwindowActivated(QMdiSubWindow*)));
}
	

void PropertyEditorWindow::setSourceWidget(DirectorSourceWidget* source)
{
	// Core - create the UI for this DirectorSourceWidget* scene() based on user controllable items
	// However, specialcase based on subclass:
	// If it's a CameraWidget then provide different controls than for other widgets
	
	while(m_layout->count() > 0)
	{
		QLayoutItem *item = m_layout->itemAt(m_layout->count() - 1);
		m_layout->removeItem(item);
		if(QWidget *widget = item->widget())
		{
			m_layout->removeWidget(widget);
			delete widget;
		}
			
		delete item;
		item = 0;
	}
	
	m_source = source;
	
	if(!source)
	{
		setWindowTitle(QString("Property Editor"));
		m_layout->addWidget(new QLabel("<i>Select a window to start editing properties.</i>"));
		m_layout->addStretch(1);
		return;
	}
	
	connect(source, SIGNAL(destroyed()), this, SLOT(sourceDestroyed()));
	
	VideoPlayerWidget *widgetVid = dynamic_cast<VideoPlayerWidget*>(source);
	CameraWidget *widgetCam = dynamic_cast<CameraWidget*>(source);
	if(widgetVid || widgetCam)
	{
		// croping
		// color adjustments
		// levels
		// flip h/v
		
		/*
		Q_PROPERTY(int brightness READ brightness WRITE setBrightness);
		Q_PROPERTY(int contrast READ contrast WRITE setContrast);
		Q_PROPERTY(int hue READ hue WRITE setHue);
		Q_PROPERTY(int saturation READ saturation WRITE setSaturation);
		
		Q_PROPERTY(bool flipHorizontal READ flipHorizontal WRITE setFlipHorizontal);
		Q_PROPERTY(bool flipVertical READ flipVertical WRITE setFlipVertical);
		
		Q_PROPERTY(QPointF cropTopLeft READ cropTopLeft WRITE setCropTopLeft);
		Q_PROPERTY(QPointF cropBottomRight READ cropBottomRight WRITE setCropBottomRight);
		*/
		
		setWindowTitle(QString("Properties - %1").arg(source->windowTitle()));
		
		#define NEW_SECTION(title) \
			ExpandableWidget *expand = new ExpandableWidget(title); \
			expand->setExpandedIfNoDefault(true); \
			QWidget *base = new QWidget(); \
			QFormLayout *form = new QFormLayout(base); \
			expand->setWidget(base); \
			m_layout->addWidget(expand); \
			PropertyEditorFactory::PropertyEditorOptions opts; \
			opts.reset();
		
		GLVideoDrawable *item = widgetVid ? widgetVid->drawable() : widgetCam->drawable();
		m_vid = item;
		
		// Histo
		{
			NEW_SECTION("Histogram");
			
			GLWidget *vid = new GLWidget();
			vid->setBackgroundColor(palette().color(QPalette::Window));
			vid->setViewport(QRectF(0,0,255*3,128*3)); // approx histogram output size for a 4:3 video frame
			
			HistogramFilter *filter = new HistogramFilter();
			
			GLVideoDrawable *gld = new GLVideoDrawable();
			gld->setVideoSource(filter);
			
			gld->setFilterType(GLVideoDrawable::Filter_Sharp);
			gld->setSharpAmount(1.25); // creates a nice relief/outline effect on the histogram
			
			vid->addDrawable(gld);
			gld->setRect(vid->viewport());
			
			form->addWidget(vid);
			
			filter->setHistoType(HistogramFilter::Gray);
			//filter->setVideoSource(item->videoSource());
			filter->setVideoSource(item->glWidget()->outputStream());
			filter->setIncludeOriginalImage(false);
			filter->setFpsLimit(10);
			filter->setDrawBorder(true);
		}
		
		// Profiles
		{
			NEW_SECTION("Store/Load Settings");
			
			m_settingsCombo = new QComboBox();
			m_settingsList.clear();
			
			QSettings s;
			QVariantList list = s.value(QString("vidopts/%1").arg(source->windowTitle())).toList();
			QStringList nameList = QStringList(); 
			foreach(QVariant data, list)
			{
				QVariantMap map = data.toMap();
				
				nameList << map["name"].toString();
				m_settingsList << map;
			}
			
			m_settingsCombo->addItems(nameList);
			//connect(combo, SIGNAL(activated(QString)), this, SLOT(loadVidOpts(QString)));
			form->addRow(tr("&Settings:"), m_settingsCombo);
			
			QHBoxLayout *hbox = new QHBoxLayout();
			
			QPushButton *btn1 = new QPushButton("Load");
			connect(btn1, SIGNAL(clicked()), this, SLOT(loadVidOpts()));
			hbox->addWidget(btn1);
			
			QPushButton *btn2 = new QPushButton("Save As...");
			connect(btn2, SIGNAL(clicked()), this, SLOT(saveVidOpts()));
			hbox->addWidget(btn2);
			
			QPushButton *btn3 = new QPushButton("Delete");
			connect(btn3, SIGNAL(clicked()), this, SLOT(deleteVidOpt()));
			hbox->addWidget(btn3);
			
			form->addRow("",hbox);
			
			
			QVariant var = source->property("_currentSetting");
			if(var.isValid())
			{
				int idx = var.toInt();
				m_settingsCombo->setCurrentIndex(idx);
				loadVidOpts(false); // false = dont call setSource(...);
			}
			
		}
		
		// Levels
		{
			NEW_SECTION("White/Black Levels");
			
			opts.min = 0;
			opts.max = 255;
			opts.step = 5;
			
			form->addRow(tr("&Black:"), PropertyEditorFactory::generatePropertyEditor(item, "blackLevel", SLOT(setBlackLevel(int)), opts));
			//form->addRow(tr("&Mid:"),   PropertyEditorFactory::generatePropertyEditor(item, "midLevel",   SLOT(setMidLevel(int)), opts));
			form->addRow(tr("&White:"), PropertyEditorFactory::generatePropertyEditor(item, "whiteLevel", SLOT(setWhiteLevel(int)), opts));
			
// 			opts.min = 0.1;
// 			opts.max = 3.0;
// 			form->addRow(tr("&Gamma:"), PropertyEditorFactory::generatePropertyEditor(item, "gamma", SLOT(setGamma(double)), opts));
// 			
			QPushButton *btn = new QPushButton("Apply to Player");
			connect(btn, SIGNAL(clicked()), this, SLOT(sendVidOpts()));
			form->addRow("",btn);
		}
		
		// Croping
		{
			NEW_SECTION("Color Adjustments");
			
			opts.min = -100;
			opts.max = +100;
			opts.step = 5;
			
			form->addRow(tr("&Brightness:"), 	PropertyEditorFactory::generatePropertyEditor(item, "brightness", SLOT(setBrightness(int)), opts));
			form->addRow(tr("&Contrast:"), 		PropertyEditorFactory::generatePropertyEditor(item, "contrast",   SLOT(setContrast(int)), opts));
			form->addRow(tr("&Saturation:"), 	PropertyEditorFactory::generatePropertyEditor(item, "saturation", SLOT(setSaturation(int)), opts));
			form->addRow(tr("&Hue:"), 		PropertyEditorFactory::generatePropertyEditor(item, "hue",        SLOT(setHue(int)), opts));
			
			QPushButton *btn = new QPushButton("Apply to Player");
			connect(btn, SIGNAL(clicked()), this, SLOT(sendVidOpts()));
			form->addRow("",btn);
		}
		
		// Advanced Filters
		{
			NEW_SECTION("Advanced Filters");
			
			QComboBox *combo = new QComboBox();
			QStringList filterNames = QStringList() 
				<< "(None)"
				<< "Sharp"
				<< "Blur"
				<< "Mean"
				<< "Bloom"
				<< "Emboss"
				<< "Edge";
			combo->addItems(filterNames);
			connect(combo, SIGNAL(activated(QString)), this, SLOT(setAdvancedFilter(QString)));
			form->addRow(tr("&Filter:"), combo);
			
			combo->setCurrentIndex(indexOfFilter((int)item->filterType()));
			
			opts.min = 0;
			opts.max = 2;
			opts.step = 1;//.25;
			
			m_sharpAmountWidget = PropertyEditorFactory::generatePropertyEditor(item, "sharpAmount", SLOT(setSharpAmount(double)), opts);
			m_sharpAmountWidget->setEnabled(item->filterType() == GLVideoDrawable::Filter_Sharp);
			
			form->addRow(tr("&Sharp Amount:"), m_sharpAmountWidget);
			
			QPushButton *btn = new QPushButton("Apply to Player");
			connect(btn, SIGNAL(clicked()), this, SLOT(sendVidOpts()));
			form->addRow("",btn);
		}
			
		// Croping
		{
			NEW_SECTION("Croping/Straightening");
			
			opts.doubleIsPercentage = true;
			opts.suffix = "%";
			opts.type = QVariant::Int;
			
			opts.min = 0;
			opts.max = 100;
			form->addRow(tr("Crop &Left:"),		PropertyEditorFactory::generatePropertyEditor(item, "cropLeft",   SLOT(setCropLeft(int)), opts));
			opts.min = -100;
			opts.max = 0;
			form->addRow(tr("Crop &Right:"),	PropertyEditorFactory::generatePropertyEditor(item, "cropRight",  SLOT(setCropRight(int)), opts));
			opts.min = 0;
			opts.max = 100;
			form->addRow(tr("Crop &Top:"),		PropertyEditorFactory::generatePropertyEditor(item, "cropTop",    SLOT(setCropTop(int)), opts));
			opts.min = -100;
			opts.max = 0;
			form->addRow(tr("Crop &Bottom:"),	PropertyEditorFactory::generatePropertyEditor(item, "cropBottom", SLOT(setCropBottom(int)), opts));
			
			opts.reset();
			//opts.suffix = " deg";
			opts.min = -360.0;
			opts.max =  360.0;
			opts.step = 1;
			opts.defaultValue = 0;
			//opts.type = QVariant::Int;
			opts.type = QVariant::Double;
			opts.value = item->rotation().z();
			form->addRow(tr("&Straighten:"), PropertyEditorFactory::generatePropertyEditor(this, "zRotation", SLOT(setStraightValue(double)), opts));

			
			QPushButton *btn = new QPushButton("Apply to Player");
			connect(btn, SIGNAL(clicked()), this, SLOT(sendVidOpts()));
			form->addRow("",btn);
		}
		
		// Flip
		{
			NEW_SECTION("Flip/Mirror");
			
			opts.text = "Flip Horizontal";
			form->addRow(PropertyEditorFactory::generatePropertyEditor(item, "flipHorizontal", SLOT(setFlipHorizontal(bool)), opts));
			
			opts.text = "Flip Vertical";
			form->addRow(PropertyEditorFactory::generatePropertyEditor(item, "flipVertical", SLOT(setFlipVertical(bool)), opts));
			
			QPushButton *btn = new QPushButton("Apply to Player");
			connect(btn, SIGNAL(clicked()), this, SLOT(sendVidOpts()));
			form->addRow("",btn);
		}
		
		
		m_layout->addStretch(1);
		
	}
	else
	{
		m_vid = 0;
		GLScene *scene = source->scene();
		if(scene)
		{
			bool foundWidget = false;
			GLDrawableList list = scene->drawableList();
			foreach(GLDrawable *gld, list)
			{
				if(gld->isUserControllable())
				{
					ExpandableWidget *widget = new ExpandableWidget(gld->itemName().isEmpty() ? "Item" : gld->itemName());
					
					DrawableDirectorWidget *editor = new DrawableDirectorWidget(gld, scene, m_dir);
					widget->setWidget(editor);
					widget->setExpandedIfNoDefault(true);
					
					m_layout->addWidget(widget);
					
					foundWidget = true;
				}
			}
			
			setWindowTitle(QString("Properties - %1").arg(scene->sceneName()));
			
			if(!foundWidget)
				m_layout->addWidget(new QLabel("<i>No modifiable items found in the active window.</i>"));
			
			m_layout->addStretch(1);
		}
		else
		{
			//setSourceWidget(0);
			m_layout->addWidget(new QLabel("<i>No active scene loaded in the selected window.</i>"));
			m_layout->addStretch(1);
			setWindowTitle(QString("Properties - %1").arg(source->windowTitle()));
		}
	}
	
	//adjustSize();
	
	
}

void PropertyEditorWindow::setAdvancedFilter(QString name)
{
	if(m_sharpAmountWidget)
		m_sharpAmountWidget->setEnabled(name == "Sharp");
	/*	
			QStringList filterNames = QStringList() 
				<< "(None)"
				<< "Sharp"
				<< "Blur"
				<< "Mean"
				<< "Bloom"
				<< "Emboss"
				<< "Edge";
	*/
	
	GLVideoDrawable::FilterType type = 
		name == "(None)" ?	GLVideoDrawable::Filter_None :
		name == "Sharp" ?	GLVideoDrawable::Filter_Sharp :
		name == "Blur" ?	GLVideoDrawable::Filter_Blur :
		name == "Mean" ?	GLVideoDrawable::Filter_Mean :
		name == "Bloom" ?	GLVideoDrawable::Filter_Bloom :
		name == "Emboss" ?	GLVideoDrawable::Filter_Emboss :
		name == "Edge" ?	GLVideoDrawable::Filter_Edge :
		GLVideoDrawable::Filter_None;
	
	m_vid->setFilterType(type);
}

int PropertyEditorWindow::indexOfFilter(int filter)
{
	GLVideoDrawable::FilterType type = (GLVideoDrawable::FilterType)filter;
	if(type == GLVideoDrawable::Filter_None)
		return 0;
		
	if(type == GLVideoDrawable::Filter_Sharp)
		return 1;
	if(type == GLVideoDrawable::Filter_Blur)
		return 2;
	if(type == GLVideoDrawable::Filter_Mean)
		return 3;
	if(type == GLVideoDrawable::Filter_Bloom)
		return 4;
	if(type == GLVideoDrawable::Filter_Emboss)
		return 5;
	if(type == GLVideoDrawable::Filter_Edge)
		return 6;
	
	return 0;
}	

void PropertyEditorWindow::sendVidOpts()
{
	if(!m_dir->players() || !m_vid)
		return;
		
	/*
		Q_PROPERTY(int brightness READ brightness WRITE setBrightness);
		Q_PROPERTY(int contrast READ contrast WRITE setContrast);
		Q_PROPERTY(int hue READ hue WRITE setHue);
		Q_PROPERTY(int saturation READ saturation WRITE setSaturation);
		
		Q_PROPERTY(bool flipHorizontal READ flipHorizontal WRITE setFlipHorizontal);
		Q_PROPERTY(bool flipVertical READ flipVertical WRITE setFlipVertical);
		
		Q_PROPERTY(QPointF cropTopLeft READ cropTopLeft WRITE setCropTopLeft);
		Q_PROPERTY(QPointF cropBottomRight READ cropBottomRight WRITE setCropBottomRight);
	*/
	
	QStringList props = QStringList() 
		<< "whiteLevel"
		<< "blackLevel"
// 		<< "midLevel"
// 		<< "gamma"
		<< "brightness"
		<< "contrast" 
		<< "hue"
		<< "saturation"
		<< "flipHorizontal"
		<< "flipVertical"
		<< "cropTop"
		<< "cropBottom"
		<< "cropLeft"
		<< "cropRight"
		<< "filterType"
		<< "sharpAmount"
		<< "rotation";
		
		
		
	foreach(PlayerConnection *player, m_dir->players()->players())
	{
		if(player->isConnected())
		{
			foreach(QString prop, props)
			{
				player->setUserProperty(m_vid, m_vid->property(qPrintable(prop)), prop);
			}
		}
	}
}

void PropertyEditorWindow::deleteVidOpt()
{
	if(!m_settingsCombo || !m_vid || !m_source)
		return;
	
	int idx = m_settingsCombo->currentIndex();
	if(idx < 0 || idx >= m_settingsList.size())
		return;
	qDebug() << "PropertyEditorWindow::deleteVidOpt(): idx:"<<idx;
	QVariantMap map = m_settingsList[idx];

	if(QMessageBox::question(this, "Really Delete?", QString("Are you sure you want to delete '%1'?").arg(map["name"].toString()),
		QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok)
	{
		QSettings s;
		
		QString key = QString("vidopts/%1").arg(m_source->windowTitle());
		QVariantList list = s.value(key).toList(), newList;
		QStringList nameList = QStringList();
		int counter = 0; 
		foreach(QVariant data, list)
		{
			if(counter != idx)
			{
				newList << data;
			}
			
			counter ++;
		}
		
		s.setValue(key, newList);
	}
	
	setSourceWidget(m_source);
}

void PropertyEditorWindow::loadVidOpts(bool setSource)
{
	if(!m_settingsCombo || !m_vid || !m_source)
		return;
	
	int idx = m_settingsCombo->currentIndex();
	if(idx < 0 || idx >= m_settingsList.size())
		return;
		 
	QVariantMap map = m_settingsList[idx];
	
	QList<QString> props = map.keys();
	
	foreach(QString prop, props)
	{
		m_vid->setProperty(qPrintable(prop), map[prop]);
	}
	
	foreach(PlayerConnection *player, m_dir->players()->players())
	{
		if(player->isConnected())
		{
			foreach(QString prop, props)
			{
				player->setUserProperty(m_vid, m_vid->property(qPrintable(prop)), prop);
			}
		}
	}
	
	m_source->setProperty("_currentSetting", idx);
	
	if(setSource)
		setSourceWidget(m_source);
}

void PropertyEditorWindow::saveVidOpts()
{
	if(!m_settingsCombo || !m_vid || !m_source)
		return;

	QStringList props = QStringList() 
		<< "whiteLevel"
		<< "blackLevel"
// 		<< "midLevel"
// 		<< "gamma"
		<< "brightness"
		<< "contrast" 
		<< "hue"
		<< "saturation"
		<< "flipHorizontal"
		<< "flipVertical"
		<< "cropTop"
		<< "cropBottom"
		<< "cropLeft"
		<< "cropRight"
		<< "filterType"
		<< "sharpAmount"
		<< "rotation";
		
	QVariantMap map;
	
	foreach(QString prop, props)
	{
		map[prop] = m_vid->property(qPrintable(prop));
	}
	
	int idx = m_settingsCombo->currentIndex();
	
	QString name = "";
	if(idx >= 0)
	{
		name = m_settingsCombo->itemText(idx);
	}
	else
	{
		QVariant var = m_source->property("_currentSetting");
		if(var.isValid())
		{
			name = m_settingsCombo->itemText(var.toInt());
		}
	}
	
	bool ok;
	QString text = QInputDialog::getText(this, tr("Setting Name"),
						tr("Setting name:"), QLineEdit::Normal,
						name, &ok);
	if (ok && !text.isEmpty())
	{
		name = text;
		map["name"] = name;
	
		QSettings s;
		
		bool found = false;
		QVariantList newList;
		
		QString key = QString("vidopts/%1").arg(m_source->windowTitle());
		QVariantList list = s.value(key).toList();
		QStringList nameList = QStringList(); 
		foreach(QVariant data, list)
		{
			QVariantMap existingMap = data.toMap();
			
			if(existingMap["name"].toString() == name)
			{
				newList << QVariant(map);
				found = true;
				//idx = newList.size()-1;
			}
			else
			{
				newList << data;
			}
		}
		
		if(!found)
		{
			newList << QVariant(map);
			idx = newList.size() - 1;
		}
		
		s.setValue(key, newList);
	}
	
	m_source->setProperty("_currentSetting", idx);
	setSourceWidget(m_source);
}

void PropertyEditorWindow::setStraightValue(double value)
{
	if(!m_vid || !m_source)
		return;
	
	// here's the easy part
	QVector3D rot = m_vid->rotation(); 
	rot.setZ(value); 
	m_vid->setProperty("rotation", rot);
	
	// now to calculate cropping
	
	QVector3D rotationPoint = m_vid->rotationPoint();
	//QRectF rect = m_vid->rect();
	
	// We're going to set the rect ourselves here
	QSizeF canvas = m_vid->canvasSize();
	QRectF rect(QPointF(0,0),canvas);

	qreal x = rect.width()  * rotationPoint.x() + rect.x();
	qreal y = rect.height() * rotationPoint.y() + rect.y();
	
	QTransform transform = QTransform()
		.translate(x,y)
		.rotate(rot.x(),Qt::XAxis)
		.rotate(rot.y(),Qt::YAxis)
		.rotate(rot.z(),Qt::ZAxis)
		.translate(-x,-y);
	
	QRectF rotRect = transform.mapRect(rect);
	
	//qDebug() << "PropertyEditorWindow::setStraightValue("<<value<<"): orig rect:"<<rect<<", rotated rect:"<<rotRect;
	
	QPointF tld = rotRect.topLeft()     - rect.topLeft();
	QPointF brd = rotRect.bottomRight() - rect.bottomRight();
// 	qDebug() << "\t topLeftDiff:		"<<tld;
// 	qDebug() << "\t bottomRightDiff:	"<<brd;
	 
// 	PropertyEditorWindow::setStraightValue( 19 ): orig rect: QRectF(0,0 1000x750) , rotated rect: QRectF(-94.8473,-142.354 1189.69x1034.71)
//          topLeftDiff:            QPointF(-94.8473, -142.354)
//          bottomRightDiff:        QPointF(94.8473, 142.354)

	// 2.2 is just a "magic number" found by guessing
	
	double rx = tld.x()*2.2;
	double ry = tld.y()*2.2;
	double rw = canvas.width() + fabs(rx*2);
	double rh = canvas.height() + fabs(ry*2);

	QRectF newRect = QRectF(rx,ry,rw,rh);
	m_vid->setProperty("rect", newRect);	
	
	//qDebug() << "\t newRect:        	"<<newRect;
}

void PropertyEditorWindow::sourceDestroyed() { setSourceWidget(0); }
	
void PropertyEditorWindow::subwindowActivated(QMdiSubWindow* win)
{
	if(!win)
		return;
		
	// This should just check for DirectorSourceWidget* subclass and call setSourceWidget()
	if(DirectorSourceWidget *src = dynamic_cast<DirectorSourceWidget*>(win->widget()))
	{
		setSourceWidget(src);
	}
	else
	{
		qDebug() << "Window activated: "<<win<<", widget:"<<win->widget()<<" - not a DirectorSourceWidget";
	}
}

////////////////////////////////////////////////////////

HistogramWindow::HistogramWindow(DirectorWindow *dir)
	: QWidget(dir)
	, m_dir(dir)
{
	setWindowTitle("Histogram");
	
	QVBoxLayout *vbox = new QVBoxLayout(this);
	//vbox->setContentsMargins(0,0,0,0);
	
	m_combo = new QComboBox();
	connect(m_combo, SIGNAL(activated(int)), this, SLOT(inputIdxChanged(int)));
	vbox->addWidget(m_combo);
	
	GLWidget *vid = new GLWidget();
	vid->setBackgroundColor(palette().color(QPalette::Window));
	vid->setViewport(QRectF(0,0,425,128)); // approx histogram output size for a 4:3 video frame
	vbox->addWidget(vid);

	m_filter = new HistogramFilter ();
	
	GLVideoDrawable *gld = new GLVideoDrawable();
	gld->setVideoSource(m_filter);
	vid->addDrawable(gld);
	gld->setRect(vid->viewport());
	
	setWindowTitle("Histogram");
	gld->setObjectName(qPrintable(windowTitle()));
	//resize(320,150);
	
	buildCombo();
}

bool HistogramWindow::setInput(QMdiSubWindow*)
{
	return false;
}
	
void HistogramWindow::inputIdxChanged(int x)
{
	if(x<0 || x>=m_sources.size())
		return;
		
	qDebug() << "HistogramWindow::inputIdxChanged: x:"<<x<<", new source:"<<m_sources[x];
	m_filter->setVideoSource(m_sources[x]);
}
	
void HistogramWindow::subwindowAdded(QMdiSubWindow*)
{
	buildCombo();
}
void HistogramWindow::windowClosed()
{
	buildCombo();
}

void HistogramWindow::buildCombo()
{
	QStringList names;
	QList<QMdiSubWindow*> windows = m_dir->subwindows();
	foreach(QMdiSubWindow *win, windows)
	{
		//if(DirectorSourceWidget *src = dynamic_cast<DirectorSourceWidget*>(win->widget()))
		
		if(CameraWidget *src = dynamic_cast<CameraWidget*>(win->widget()))
		{	
			connect(src, SIGNAL(destroyed()), this, SLOT(windowClosed()));
			
			names << src->windowTitle();
			m_sources << src->receiver();
		}
	}
	
	m_combo->clear();
	m_combo->addItems(names);
	if(!names.isEmpty())
	{
		m_combo->setCurrentIndex(0);
		inputIdxChanged(0);
	}
	
	//adjustSize();
}
	
/*private:
	class Source
	{
	public:
		QString title;
		VideoSource *source;
	};
	
	
	HistogramFilter *m_filter;
	QList<Source> m_sources;*/

////////////////////////////////////////////////////

InputBalanceWindow::InputBalanceWindow(DirectorWindow *dir)
	: QWidget(dir)
	, m_director(dir)
{
	setWindowTitle("Camera Color Balancer");
	
	QVBoxLayout *vbox = new QVBoxLayout(this);
	//vbox->setContentsMargins(0,0,0,0);
	
	QHBoxLayout *hbox = new QHBoxLayout();
	hbox->addWidget(new QLabel("Master:"));
	m_combo1 = new QComboBox();
	connect(m_combo1, SIGNAL(activated(int)), this, SLOT(masterChanged(int)));
	hbox->addWidget(m_combo1);
	hbox->addStretch(1);
	
	hbox->addWidget(new QLabel("Slave:"));
	m_combo2 = new QComboBox();
	connect(m_combo2, SIGNAL(activated(int)), this, SLOT(slaveChanged(int)));
	hbox->addWidget(m_combo2);
	
	vbox->addLayout(hbox);
	
	m_balancer = new VideoInputColorBalancer(this); 
	vbox->addWidget(m_balancer);
	
	//gld->setObjectName(qPrintable(windowTitle()));
	//resize(320,240);
	
	buildCombos();
}


void InputBalanceWindow::masterChanged(int x)
{
	if(x<0 || x>=m_sources.size())
		return;
		
	qDebug() << "InputBalanceWindow::masterChanged: x:"<<x<<", new source:"<<m_sources[x];
	m_balancer->setMasterSource(m_sources[x]);
}
	
void InputBalanceWindow::slaveChanged(int x)
{
	if(x<0 || x>=m_sources.size())
		return;
		
	qDebug() << "InputBalanceWindow::slaveChangedhanged: x:"<<x<<", new source:"<<m_sources[x];
	m_balancer->setSlaveSource(m_sources[x]);
}

void InputBalanceWindow::subwindowAdded(QMdiSubWindow*)
{
	buildCombos();
}
void InputBalanceWindow::windowClosed()
{
	buildCombos();
}

void InputBalanceWindow::buildCombos()
{
	m_sources.clear();
	QStringList names;
	QList<QMdiSubWindow*> windows = m_director->subwindows();
	foreach(QMdiSubWindow *win, windows)
	{
		//if(DirectorSourceWidget *src = dynamic_cast<DirectorSourceWidget*>(win->widget()))
		
		if(CameraWidget *src = dynamic_cast<CameraWidget*>(win->widget()))
		{	
			connect(src, SIGNAL(destroyed()), this, SLOT(windowClosed()));
			
			names << src->windowTitle();
			m_sources << src->receiver();
		}
	}
	
	m_combo1->clear();
	m_combo1->addItems(names);
	if(!names.isEmpty())
	{
		m_combo1->setCurrentIndex(0);
		masterChanged(0);
	}
	
	m_combo2->clear();
	m_combo2->addItems(names);
	if(names.size() > 1)
	{
		m_combo2->setCurrentIndex(1);
		slaveChanged(1);
	}
		
	//adjustSize();
}

	
// private:
// 	class Source
// 	{
// 	public:
// 		QString title;
// 		VideoSource *source;
// 	};
// 	
// 	VideoInputColorBalancer *m_balancer;
// 	QList<Source> m_sources;
// 
// 


////////////////////////////////////////////////////////

CameraMixerWidget::CameraMixerWidget(DirectorWindow *dir)
	: DirectorSourceWidget(dir)
{
	setWindowTitle("Camera Mixer");
	
	m_setGroup = new GLSceneGroup();
	m_scene = new GLScene();
	m_cam1  = new GLVideoInputDrawable();
	m_cam2  = new GLVideoInputDrawable();
	m_cam1->setZIndex(1);
	m_cam2->setZIndex(2);
	m_scene->addDrawable(m_cam1);
	m_scene->addDrawable(m_cam2);
	m_setGroup->addScene(m_scene);
	
	QVBoxLayout *vbox = new QVBoxLayout(this);
	//vbox->setContentsMargins(0,0,0,0);
	
	QHBoxLayout *hbox = new QHBoxLayout();
	m_combo1 = new QComboBox();
	connect(m_combo1, SIGNAL(activated(int)), this, SLOT(input1Changed(int)));
	hbox->addWidget(m_combo1);
	
	m_combo2 = new QComboBox();
	connect(m_combo2, SIGNAL(activated(int)), this, SLOT(input2Changed(int)));
	hbox->addWidget(m_combo2);
	
	vbox->addLayout(hbox);
	
	GLWidget *glw = new GLWidget();
	vbox->addWidget(glw);
	
	connect(glw, SIGNAL(clicked()), this, SLOT(switchTo()));
	m_scene->setGLWidget(glw);
	
	m_layoutCombo = new QComboBox();
	connect(m_layoutCombo, SIGNAL(activated(int)), this, SLOT(layoutChanged(int)));
	
	QStringList layouts;
	layouts << "Side-by-side" 
		<< "PiP - Top-Left"
		<< "PiP - Top-Right"
		<< "PiP - Bottom-Right"
		<< "PiP - Bottom-Left"
		<< "3D";
	m_layoutCombo->addItems(layouts);
	layoutChanged(0);
	
	vbox->addWidget(m_layoutCombo);
	
	//gld->setObjectName(qPrintable(windowTitle()));
	resize(320,240);
	
	buildCombos();
}

void CameraMixerWidget::layoutChanged(int idx)
{
	int pipW = 320;
	int pipH = 240;
	int pipXMargin = 10;
	int pipYMargin = 10;
	
	int sceneW = 1000;
	int sceneH = 750;
	
	m_cam1->setRotation(QVector3D(0,0,0));
	m_cam2->setRotation(QVector3D(0,0,0));
			
	switch(idx)
	{
		case 1: // Top-Left
			m_cam1->setRect(QRectF(0,0,sceneW,sceneH));
			m_cam2->setRect(QRectF(pipXMargin,pipYMargin,pipW,pipH));
		break;
		
		case 2: // Top-Right
			m_cam1->setRect(QRectF(0,0,sceneW,sceneH));
			m_cam2->setRect(QRectF(sceneW - pipW - pipXMargin,pipYMargin,pipW,pipH));
		break;
				
		case 3: // Bottom-Right
			m_cam1->setRect(QRectF(0,0,sceneW,sceneH));
			m_cam2->setRect(QRectF(sceneW - pipW - pipXMargin,sceneH - pipH - pipYMargin,pipW,pipH));
		break;
		
		case 4: // Bottom-Left
			m_cam1->setRect(QRectF(0,0,sceneW,sceneH));
			m_cam2->setRect(QRectF(pipXMargin,sceneH - pipH - pipYMargin,pipW,pipH));
		break;
		
		case 5: // 3d
		{
			int overlap = 100; //sceneW / 4;
			m_cam1->setRect(QRectF(0,0,sceneW/2 + overlap,sceneH));
			m_cam2->setRect(QRectF(sceneW/2 - overlap,0,sceneW/2 + overlap,sceneH));
			m_cam1->setRotation(QVector3D(0,-45,0));
			m_cam2->setRotation(QVector3D(0,45,0));
		}
		break;
		
		case 0:
		default:
			m_cam1->setRect(QRectF(0,0,sceneW/2,sceneH));
			m_cam2->setRect(QRectF(sceneW/2,0,sceneW/2,sceneH));
		break;
	};
}

// DirectorSourceWidget::	
bool CameraMixerWidget::switchTo()
{
	//int idx = m_combo->currentIndex();
	
// 	GLSceneGroup *group = m_collection->at(0); 
// 	GLScene *scene = group->at(idx);
	
	foreach(PlayerConnection *player, m_dir->players()->players())
 	{
 		if(player->isConnected())
 		{
 			//if(player->lastGroup() != m_setGroup)
 				player->setGroup(m_setGroup, m_scene);
 			//m_setGroup = group;
			//player->setUserProperty(gld, con, "videoConnection");
			player->setUserProperty(m_cam1, m_cam1->videoConnection(), "videoConnection");
			player->setUserProperty(m_cam2, m_cam2->videoConnection(), "videoConnection");
		}
	}
	
	if(m_switcher)
		m_switcher->notifyIsLive(this);
	
	return true;
}


void CameraMixerWidget::input1Changed(int x)
{
	if(x<0 || x>=m_conList.size())
		return;
		
	qDebug() << "CameraMixerWidget::input1changed: x:"<<x<<", new source:"<<m_conList[x];
	m_cam1->setVideoConnection(m_conList[x]);
	
	foreach(PlayerConnection *player, m_dir->players()->players())
 	{
 		if(player->isConnected())
 		{
 			if(player->lastGroup() == m_setGroup)
 				//player->setGroup(m_setGroup, m_scene);
 			//m_setGroup = group;
				player->setUserProperty(m_cam1, m_conList[x], "videoConnection");
		}
	}
}
	
void CameraMixerWidget::input2Changed(int x)
{
	if(x<0 || x>=m_conList.size())
		return;
		
	qDebug() << "CameraMixerWidget::input2changed: x:"<<x<<", new source:"<<m_conList[x];
	m_cam2->setVideoConnection(m_conList[x]);
	
	foreach(PlayerConnection *player, m_dir->players()->players())
 	{
 		if(player->isConnected())
 		{
 			if(player->lastGroup() == m_setGroup)
 				//player->setGroup(m_setGroup, m_scene);
 			//m_setGroup = group;
				player->setUserProperty(m_cam2, m_conList[x], "videoConnection");
		}
	}
}

void CameraMixerWidget::subwindowAdded(QMdiSubWindow*)
{
	buildCombos();
}
void CameraMixerWidget::windowClosed()
{
	buildCombos();
}

void CameraMixerWidget::buildCombos()
{
	QStringList names;
	QList<QMdiSubWindow*> windows = m_dir->subwindows();
	foreach(QMdiSubWindow *win, windows)
	{
		//if(DirectorSourceWidget *src = dynamic_cast<DirectorSourceWidget*>(win->widget()))
		
		if(CameraWidget *src = dynamic_cast<CameraWidget*>(win->widget()))
		{	
			connect(src, SIGNAL(destroyed()), this, SLOT(windowClosed()));
			
			names << src->windowTitle();
			m_conList << src->con();
		}
	}
	
	m_combo1->clear();
	m_combo1->addItems(names);
	if(!names.isEmpty())
	{
		m_combo1->setCurrentIndex(0);
		input1Changed(0);
	}
	
	m_combo2->clear();
	m_combo2->addItems(names);
	if(names.size() > 1)
	{
		m_combo2->setCurrentIndex(1);
		input2Changed(1);
	}
		
	//adjustSize();
}




//////////////////////////

VideoPlayerWidget::VideoPlayerWidget(DirectorWindow *d)
	: DirectorSourceWidget(d)
	, m_group(0)
	, m_scene(0)
	, m_collection(0)
	, m_director(d)
	, m_lockSetPosition(false)
	, m_isLocalPlayer(false)
	, m_pos(0.0)
	, m_muted(false)
	, m_volume(100)
	, m_dur(0.0)
	, m_isSliderDown(false) // used to stop updates from video object
{
	m_collection = new GLSceneGroupCollection();
	m_group = new GLSceneGroup();
	m_scene = new GLScene();
	m_scene->setSceneName("Video Player");
	m_video = new GLVideoFileDrawable();
	m_scene->addDrawable(m_video);
	m_group->addScene(m_scene);
	m_collection->addGroup(m_group);
	
	connect(m_video, SIGNAL(positionChanged(qint64)), this, SLOT(positionChanged(qint64)));
	connect(m_video, SIGNAL(durationChanged(double)), this, SLOT(durationChanged(double)));
	connect(m_video, SIGNAL(statusChanged(int)), this, SLOT(statusChanged(int)));
	
	QVBoxLayout *vbox = new QVBoxLayout(this);
	m_glw = new GLWidget(this);
	m_glw->setWindowTitle("VideoPlayerWidget");
	m_glw->addDrawable(m_video);
	vbox->addWidget(m_glw);
	
	QHBoxLayout *hbox = new QHBoxLayout();
	
	//QPushButton *hide = new QPushButton(QPixmap(":/data/stock-media-stop.png"), "");
	
	m_playPauseBtn	= new QPushButton(QPixmap(":/data/stock-media-play.png"), ""); // stock-media-stop.png when playing
	{
		connect(m_playPauseBtn, SIGNAL(clicked()), this, SLOT(togglePlay()));
		hbox->addWidget(m_playPauseBtn);
	}
	
	m_seekSlider	= new QSlider();
	{
		m_seekSlider->setMinimum(0);
		m_seekSlider->setMaximum(100);
		m_seekSlider->setOrientation(Qt::Horizontal);
		m_seekSlider->setSingleStep(1);
		m_seekSlider->setPageStep(30);
		connect(m_seekSlider, SIGNAL(valueChanged(int)), this, SLOT(setPosition(int)));
		connect(m_seekSlider, SIGNAL(sliderPressed()), this, SLOT(sliderPressed()));
		connect(m_seekSlider, SIGNAL(sliderReleased()), this, SLOT(sliderReleased()));
		hbox->addWidget(m_seekSlider);
	}
	
	m_timeLabel	= new QLabel("00:00/00:00");
	hbox->addWidget(m_timeLabel);
	
	m_muteButton	= new QPushButton(QPixmap("../data/stock-volume.png"),""); // stock-volume-muted when muited
	{
		m_muteButton->setCheckable(true);
		connect(m_muteButton, SIGNAL(toggled(bool)), this, SLOT(setMuted(bool)));
		hbox->addWidget(m_muteButton);
	}
	
	m_volumeSlider	= new QSlider();
	{
		m_volumeSlider->setMinimum(0);
		m_volumeSlider->setMaximum(100);
		m_volumeSlider->setOrientation(Qt::Horizontal);
		m_volumeSlider->setSingleStep(5);
		m_volumeSlider->setPageStep(10);
		connect(m_volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(setVolume(int)));
		hbox->addWidget(m_volumeSlider);
	}
	
	QPushButton *browse = new QPushButton(QPixmap(":/data/stock-open.png"), "");
	{
		connect(browse, SIGNAL(clicked()), this, SLOT(browse()));
		hbox->addWidget(browse);
	}
	
	QPushButton *props = new QPushButton(QPixmap(":/data/stock-properties.png"), "");
	{
		connect(browse, SIGNAL(clicked()), this, SLOT(showPropertyEditor()));
		hbox->addWidget(props);
	}
	
	vbox->addLayout(hbox);
	
	connect(m_glw, SIGNAL(clicked()), this, SLOT(switchTo()));
	//loadFile("hd_vid/Dolphins_720.wmv-xvid.avi");
	
	
	foreach(PlayerConnection *player, m_director->players()->players())
		if(player->isLocalPlayer())
		   m_isLocalPlayer = true;
}

void VideoPlayerWidget::showPropertyEditor()
{	
	if(m_dir)
		m_dir->showPropertyEditor(this);
}	

void VideoPlayerWidget::sliderPressed()
{
	m_isSliderDown = true;
}

void VideoPlayerWidget::sliderReleased()
{
	m_isSliderDown = false;
}


void VideoPlayerWidget::positionChanged(qint64 position)
{
	if(m_video->status() != 1)
		return;
		
	if(m_isSliderDown)
		return;
		
	position = position / 1000;
	
	m_lockSetPosition = true;
	
	qDebug() << "VideoPlayerWidget::positionChanged():"<<position;
	updatePositionUI(position);
	
	m_lockSetPosition = false;
}

void VideoPlayerWidget::updatePositionUI(int position)
{
	if(m_seekSlider->value() != (int)position)
		m_seekSlider->setValue((int)position);
		
	m_timeLabel->setText(QString().sprintf("%d/%d",(int)position,(int)m_dur));
}

void VideoPlayerWidget::durationChanged(double duration)
{
	qDebug() << "VideoPlayerWidget::durationChanged(): "<<duration;
	m_dur = duration;
	m_seekSlider->setMaximum((int)(duration+.5));
	positionChanged(m_video->position());
}

void VideoPlayerWidget::statusChanged(int status)
{
	qDebug() << "VideoPlayerWidget::statusChanged(): status:"<<status; 
	switch(status)
	{
		case 0:
			m_playPauseBtn->setIcon(QPixmap(":/data/stock-media-play.png"));
			break;
		case 1:
			m_playPauseBtn->setIcon(QPixmap(":/data/stock-media-pause.png"));
			break;
		case 2:
		default:
			m_playPauseBtn->setIcon(QPixmap(":/data/stock-media-play.png"));
			break;
	}
}
	
QVariantMap VideoPlayerWidget::saveToMap()
{
	QVariantMap map;
	map["file"] = file();
	map["pos"] = position();
	map["vol"] = volume();
	return map;
}

void VideoPlayerWidget::loadFromMap(const QVariantMap& map)
{
	loadFile(map["file"].toString());
	setPosition(map["pos"].toDouble());
	setVolume(map["vol"].toInt());
}

void VideoPlayerWidget::browse()
{
	QSettings settings;
	QString curFile = file();
	if(curFile.trimmed().isEmpty())
		curFile = settings.value("last-video-file").toString();
		
	QString fileName = QFileDialog::getOpenFileName(this, tr("Select Video File"), curFile, tr("Video Files (*.wmv *.mpeg *.mpg *.avi *.wmv *.flv *.mov *.mp4 *.m4a *.3gp *.3g2 *.mj2 *.mjpeg *.ipod *.m4v *.gsm *.swf *.dv *.dvd *.asf *.mtv *.roq *.aac *.ac3 *.aiff *.alaw *.iif);;Any File (*.*)"));
	if(fileName != "")
	{
		settings.setValue("last-video-file",fileName);
		if(!loadFile(fileName))
		{
			QMessageBox::critical(this,tr("File Does Not Exist"),tr("Sorry, but the file you chose does not exist. Please try again."));
		}
	}
}

bool VideoPlayerWidget::loadFile(QString fileName)
{
	if(m_video->setVideoFile(fileName))
	{
		m_filename = fileName;
		setWindowTitle(QString("Video - %1").arg(QFileInfo(fileName).fileName()));
		
		// If the player is "local" (running on same host as director)
		// we mute the director's video copy so it doesn't compete with the audio from the player
		if(m_isLocalPlayer)
		{
			m_video->setMuted(true);
			m_video->setVolume(0);
		}
		
		pause();
		
		foreach(PlayerConnection *player, m_director->players()->players())
		{
			if(player->isConnected())
			{
				//if(player->lastGroup() != group)
				player->setGroup(m_group, 0, true);
			}
		}
		
		return true;
	}
	return false;
}

// DirectorSourceWidget::	
bool VideoPlayerWidget::switchTo()
{
	qDebug() << "VideoPlayerWidget::switchTo()";
	foreach(PlayerConnection *player, m_director->players()->players())
 	{
 		if(player->isConnected())
 		{
 			//if(player->lastGroup() != group)
 			player->setGroup(m_group, m_scene);
 			player->setUserProperty(m_video, m_video->videoFile(), "videoFile");
 			player->setUserProperty(m_video, m_video->position(), "position");
 			player->setUserProperty(m_video, m_volume, "volume");
 			player->setUserProperty(m_video, m_muted, "muted");
 			//player->setUserProperty(m_video, m_video->status(), "status");
		}
	}
	
	play();
	
	
	if(m_switcher)
		m_switcher->notifyIsLive(this);
	
	return true;
}


VideoPlayerWidget::~VideoPlayerWidget()
{
	// TODO...?
}

void VideoPlayerWidget::setMuted(bool flag)
{
	qDebug() << "VideoPlayerWidget::setMuted(): "<<flag;
	
	// If the player is "local" (running on same host as director)
	// we don't unmute the director's video copy so it doesn't compete with the audio from the player
	if(!m_isLocalPlayer)
		m_video->setMuted(flag);
		
	m_muted = flag;
	syncProperty("muted", flag);
	m_muteButton->setIcon( flag ? QPixmap("../data/stock-volume-mute.png") : QPixmap("../data/stock-volume.png") );
}

void VideoPlayerWidget::setVolume(int v)
{
	//qDebug() << "VideoPlayerWidget::setVolume(): "<<v;
	
	// If the player is "local" (running on same host as director)
	// we don't set the volume (it's muted at the start)
	// the director's video copy so it doesn't compete with the audio from the player
	if(!m_isLocalPlayer)
		m_video->setVolume(v);
		
	m_volume = v;
	syncProperty("volume", v);
}

void VideoPlayerWidget::setPosition(double d)
{
	if(m_lockSetPosition)
		return;
	m_lockSetPosition = true;
	
	qDebug() << "VideoPlayerWidget::setPosition(): "<<d;
	
	int pos = (int)(d * 1000.0);
	
	m_video->setPosition(pos);
	syncProperty("position", pos);
	
	updatePositionUI((int)d);
	
	m_lockSetPosition = false;
}

void VideoPlayerWidget::play()
{
	m_video->setStatus(1);
	syncProperty("status", 1);
	m_playPauseBtn->setIcon(QPixmap(":/data/stock-media-pause.png"));
}

void VideoPlayerWidget::pause()
{
	m_video->setStatus(2);
	syncProperty("status", 2);
	m_playPauseBtn->setIcon(QPixmap(":/data/stock-media-play.png"));
}

void VideoPlayerWidget::togglePlay()
{
	if(m_video->status() == 1)
		pause();
	else
		play();
}

void VideoPlayerWidget::syncProperty(QString prop, QVariant value)
{
	foreach(PlayerConnection *player, m_director->players()->players())
		if(player->isConnected() &&
		   player->lastGroup() == m_group)
			player->setUserProperty(m_video, value, prop);
}

