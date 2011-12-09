#include "DrawableDirectorWidget.h"
#include "ui_DrawableDirectorWidget.h"
#include "DirectorWindow.h"
#include "../livemix/EditorUtilityWidgets.h"
#include "RtfEditorWindow.h"
#include "../qtcolorpicker/qtcolorpicker.h"

#include "FlowLayout.h"

#include <QFileInfo>
#include <QFileDialog>
#include <QApplication>
#include <QGLWidget>

#include "GLDrawables.h"
#include "GLSceneGroup.h"

#include "../livemix/VideoWidget.h"
#include "VideoInputSenderManager.h"
#include "VideoReceiver.h"

#include "PlayerConnection.h"

DrawableDirectorWidget::DrawableDirectorWidget(GLDrawable *drawable, GLScene *scene, DirectorWindow *director)
	: QWidget(director)
	, ui(new Ui::DrawableDirectorWidget)
	, m_directorWindow(director)
	, m_scene(scene)
	, m_drawable(0)
{
	ui->setupUi(this);
	setupUI();	
	setDrawable(drawable);
}

DrawableDirectorWidget::~DrawableDirectorWidget()
{
	delete ui;
}

void DrawableDirectorWidget::setupUI()
{
	ui->playBtn->setText("");
	ui->playBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));

	ui->moveUpBtn->setText("");
	ui->moveUpBtn->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));

	ui->moveDownBtn->setText("");
	ui->moveDownBtn->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));

	ui->delBtn->setText("");
	ui->delBtn->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));

	connect(ui->playBtn,  SIGNAL(clicked()), this, SLOT(playPlaylist()));
	//connect(ui->pauseBtn, SIGNAL(clicked()), this, SLOT(pausePlaylist()));
	connect(ui->moveUpBtn,  SIGNAL(clicked()), this, SLOT(btnMoveUp()));
	connect(ui->moveDownBtn,  SIGNAL(clicked()), this, SLOT(btnMoveDown()));

	connect(ui->playlistView, SIGNAL(activated(const QModelIndex &)), this, SLOT(itemSelected(const QModelIndex &)));
	connect(ui->playlistView, SIGNAL(clicked(const QModelIndex &)),   this, SLOT(itemSelected(const QModelIndex &)));

	connect(ui->hideBtn, SIGNAL(clicked()), this, SLOT(btnHideDrawable()));
	connect(ui->sendToPlayerBtn, SIGNAL(clicked()), this, SLOT(btnSendToPlayer()));
	connect(ui->addToPlaylistBtn, SIGNAL(clicked()), this, SLOT(btnAddToPlaylist()));
	connect(ui->delBtn, SIGNAL(clicked()), this, SLOT(btnRemoveFromPlaylist()));

	ui->timeLabel->setText("");

}

void DrawableDirectorWidget::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
	switch (e->type())
	{
		case QEvent::LanguageChange:
			ui->retranslateUi(this);
			break;
		default:
			break;
	}
}



void DrawableDirectorWidget::itemSelected(const QModelIndex &idx)
{
	if(!m_drawable)
		return;
	if(idx.isValid())
	{
		GLPlaylistItem *item = m_drawable->playlist()->at(idx.row());
		setCurrentItem(item);

		foreach(PlayerConnection *con, m_directorWindow->players()->players())
			con->setPlaylistTime(m_drawable, m_drawable->playlist()->timeFor(item));
	}

}
void DrawableDirectorWidget::currentItemChanged(const QModelIndex &idx,const QModelIndex &)
{
	if(idx.isValid())
		itemSelected(idx);
}

void DrawableDirectorWidget::itemLengthChanged(double len)
{
	if(!m_currentItem)
		return;

	m_currentItem->setDuration(len);
}


void DrawableDirectorWidget::btnHideDrawable()
{
	if(!m_drawable)
		return;
	QPushButton *btn = dynamic_cast<QPushButton *>(sender());
	if(!btn)
		return;
	m_drawable->setHidden(btn->isChecked());

	foreach(PlayerConnection *con, m_directorWindow->players()->players())
		con->setVisibility(m_drawable, !btn->isChecked());
}
void DrawableDirectorWidget::btnSendToPlayer()
{
	const char *propName = m_drawable->metaObject()->userProperty().name();

	foreach(PlayerConnection *con, m_directorWindow->players()->players())
		con->setUserProperty(m_drawable, m_drawable->property(propName), propName);
}
void DrawableDirectorWidget::btnAddToPlaylist()
{
	if(!m_drawable)
		return;
	GLDrawablePlaylist *playlist = m_drawable->playlist();
	GLPlaylistItem * item = new GLPlaylistItem();

	QString userProp = QString(m_drawable->metaObject()->userProperty().name());
	QVariant prop = m_drawable->property(qPrintable(userProp));
	item->setValue(prop);

	GLDrawable *gld = m_drawable;
	if(GLTextDrawable *casted = dynamic_cast<GLTextDrawable*>(gld))
	{
		item->setTitle(casted->plainText());
		item->setDuration(5.0); /// just a guess
	}
	else
	if(GLVideoFileDrawable *casted = dynamic_cast<GLVideoFileDrawable*>(gld))
	{
		item->setTitle(QFileInfo(casted->videoFile()).fileName());
		item->setDuration(casted->videoLength());
	}
	else
	if(GLVideoLoopDrawable *casted = dynamic_cast<GLVideoLoopDrawable*>(gld))
	{
		item->setTitle(QFileInfo(casted->videoFile()).fileName());
		item->setDuration(casted->videoLength());
	}
	else
	if(GLImageDrawable *casted = dynamic_cast<GLImageDrawable*>(gld))
	{
		item->setTitle(QFileInfo(casted->imageFile()).fileName());
		item->setDuration(5.0); /// just a guess
	}
	else
	{
		item->setTitle(prop.toString());
		item->setDuration(5.0); /// just a guess
	}

	if(item->duration() <= 0.)
		item->setDuration(5.0);

	item->setAutoDuration(true);

	playlist->addItem(item);

	ui->playlistView->setModel(0);
	ui->playlistView->setModel(m_drawable->playlist());
	ui->playlistView->resizeRowsToContents();
	ui->playlistView->resizeColumnsToContents();
	//ui->playlistView->setCurrentIndex(playlist->indexOf(item));
	setCurrentItem(item);

	foreach(PlayerConnection *con, m_directorWindow->players()->players())
		con->updatePlaylist(m_drawable);
}

void DrawableDirectorWidget::btnRemoveFromPlaylist()
{
	if(!m_currentItem)
		return;
	if(!m_drawable)
		return;
	GLDrawablePlaylist *playlist = m_drawable->playlist();

	playlist->removeItem(m_currentItem);

	foreach(PlayerConnection *con, m_directorWindow->players()->players())
		con->updatePlaylist(m_drawable);

	// HACK-ish
	ui->playlistView->setModel(0);
	ui->playlistView->setModel(m_drawable->playlist());
	ui->playlistView->resizeRowsToContents();
	ui->playlistView->resizeColumnsToContents();

	GLPlaylistItem *newItem = playlist->at(0);
	if(newItem)
	{
		ui->playlistView->setCurrentIndex(playlist->indexOf(newItem));

		setCurrentItem(newItem);
	}
}

void DrawableDirectorWidget::btnMoveUp()
{
	if(!m_currentItem)
		return;
	if(!m_drawable)
		return;
	GLDrawablePlaylist *playlist = m_drawable->playlist();

	int row = playlist->rowOf(m_currentItem);
	if(row > 0)
	{
		playlist->removeItem(m_currentItem);

		GLPlaylistItem *prev = playlist->at(row-1);
		playlist->addItem(m_currentItem, prev);

		// HACK-ish
		ui->playlistView->setModel(0);
		ui->playlistView->setModel(m_drawable->playlist());
		ui->playlistView->resizeRowsToContents();
		ui->playlistView->resizeColumnsToContents();

		ui->playlistView->setCurrentIndex(playlist->indexOf(m_currentItem));

		foreach(PlayerConnection *con, m_directorWindow->players()->players())
			con->updatePlaylist(m_drawable);

	}
}

void DrawableDirectorWidget::btnMoveDown()
{
	if(!m_currentItem)
		return;
	if(!m_drawable)
		return;
	GLDrawablePlaylist *playlist = m_drawable->playlist();

	int row = playlist->rowOf(m_currentItem);
	if(row < playlist->size() - 1)
	{
		playlist->removeItem(m_currentItem);

		GLPlaylistItem *next = playlist->at(row+1);
		playlist->addItem(m_currentItem, next);

		// HACK-ish
		ui->playlistView->setModel(0);
		ui->playlistView->setModel(m_drawable->playlist());
		ui->playlistView->resizeRowsToContents();
		ui->playlistView->resizeColumnsToContents();

		ui->playlistView->setCurrentIndex(playlist->indexOf(m_currentItem));

		foreach(PlayerConnection *con, m_directorWindow->players()->players())
			con->updatePlaylist(m_drawable);

	}
}

void DrawableDirectorWidget::playlistTimeChanged(GLDrawable *gld, double value)
{
	if(m_drawable != gld)
		return;

	QString time = "";

	double min = value/60;
	double sec = (min - (int)(min)) * 60;
	//double ms  = (sec - (int)(sec)) * 60;
	time =  (min<10? "0":"") + QString::number((int)min) + ":" +
		(sec<10? "0":"") + QString::number((int)sec);/* + "." +
		(ms <10? "0":"") + QString::number((int)ms );*/

	ui->timeLabel->setText("<b><font style='font-family:Monospace'>"+ time +"</font></b>");
}

void DrawableDirectorWidget::playlistItemChanged(GLDrawable *gld, GLPlaylistItem *item)
{
	if(m_drawable != gld)
		return;

	if(m_currentItem != item)
		setCurrentItem(item);
}

void DrawableDirectorWidget::setPlaylistEditingEnabled(bool flag)
{
	m_playlistEditingEnabled = flag;
	ui->playlistSetupWidget->setVisible(flag);
}

void DrawableDirectorWidget::setDrawable(GLDrawable *gld)
{
	if(m_drawable == gld)
		return;
		
	if(m_drawable)
	{
		if(m_drawable->playlist()->isPlaying())
			m_drawable->playlist()->stop();
		
 		disconnect(m_drawable->playlist(), 0, this, 0);
// 		
// 		disconnect(ui->playBtn,  0, m_drawable->playlist(), 0);
// 		disconnect(ui->pauseBtn, 0, m_drawable->playlist(), 0);
	
		m_drawable = 0;
	}
	
	connect(gld, SIGNAL(destroyed()), this, SLOT(setDrawable()));
	
	m_drawable = gld;
	
	if(gld)
		ui->playlistView->setModel(gld->playlist());
	else
		ui->playlistView->setModel(0);
		
 	ui->playlistView->resizeColumnsToContents();
 	ui->playlistView->resizeRowsToContents();
	
	
	// Fun loop to clear out a QFormLayout - why doesn't QLayout just have a removeAll() or clear() method?
	QFormLayout *form = ui->itemPropLayout;
	while(form->count() > 0)
	{
		QLayoutItem * item = form->itemAt(form->count() - 1);
		form->removeItem(item);
		if(QWidget *widget = item->widget())
		{
			//qDebug() << "DrawableDirectorWidget::setCurrentDrawable: Deleting "<<widget<<" from form, total:"<<form->count()<<" items remaining.";
			form->removeWidget(widget);
			if(QWidget *label = form->labelForField(widget))
			{
				form->removeWidget(label);
				delete label;
			}
			delete widget;
		}
// 		else
// 		if(QLayout *layout = item->layout())
// 		{
// 			form->removeItem(item);
// 		}
		delete item;
	}
	
	if(m_drawable)
	{
		// Update the playlist on the player just to make sure its up-to-date with what we see
		foreach(PlayerConnection *con, m_directorWindow->players()->players())
			con->updatePlaylist(m_drawable);
	
		connect(gld->playlist(), SIGNAL(playlistItemChanged()), this, SLOT(playlistItemDurationChanged()));

		QString itemName = gld->itemName();
		QString typeName;
	
		PropertyEditorFactory::PropertyEditorOptions opts;
		
		setPlaylistEditingEnabled(true);
		
		if(GLVideoInputDrawable *item = dynamic_cast<GLVideoInputDrawable*>(gld))
		{
			// show device selection box
			// show deinterlace checkbox
			Q_UNUSED(item);
	
			typeName = "Video Input Item";
			
			setPlaylistEditingEnabled(false);
			
			QWidget *base = new QWidget();
			
			QVBoxLayout *vbox = new QVBoxLayout(base);
			
			QHBoxLayout *hbox = new QHBoxLayout();
			
			hbox->addWidget(new QLabel("Network Source:"));
			
			QComboBox *sourceBox = new QComboBox();
			QStringList itemList;
			//itemList << "(This Computer)";
			m_videoPlayerList.clear();
			foreach(PlayerConnection *con, m_directorWindow->players()->players())
				if(con->isConnected())
				{
					itemList << QString("Player: %1").arg(con->name());
					m_videoPlayerList << con;
				}
			
			sourceBox->addItems(itemList);
					
			hbox->addWidget(sourceBox);
			hbox->addStretch();
			
			vbox->addLayout(hbox);
			
			connect(sourceBox, SIGNAL(activated(int)), this, SLOT(loadVideoInputList(int)));
			
			m_videoViewerLayout = new FlowLayout();
			vbox->addLayout(m_videoViewerLayout);
			
			ui->itemPropLayout->addRow(base);
			
			if(itemList.size() > 0)
				loadVideoInputList(0);
			else
			{
				sourceBox->setDisabled(true);
				sourceBox->addItems(QStringList() << "(No Players Connected)");
				
				// Dont show the error box if the director window hasnt been shown yet
	// 			if(isVisible())
	// 				QMessageBox::warning(this, "No Players Connected","Sorry, no players are connected. You must connect to at least one video player before you can switch videos.");
			}
		}
		else
		if(GLVideoLoopDrawable *item = dynamic_cast<GLVideoLoopDrawable*>(gld))
		{
			opts.reset();
			opts.stringIsFile = true;
			opts.fileTypeFilter = tr("Video Files (*.wmv *.mpeg *.mpg *.avi *.wmv *.flv *.mov *.mp4 *.m4a *.3gp *.3g2 *.mj2 *.mjpeg *.ipod *.m4v *.gsm *.swf *.dv *.dvd *.asf *.mtv *.roq *.aac *.ac3 *.aiff *.alaw *.iif);;Any File (*.*)");
	
			ui->itemPropLayout->addRow(tr("&File:"), PropertyEditorFactory::generatePropertyEditor(item, "videoFile", SLOT(setVideoFile(const QString&)), opts, SIGNAL(videoFileChanged(const QString&))));
	
			typeName = "Video Loop Item";
		}
		else
		if(GLVideoFileDrawable *item = dynamic_cast<GLVideoFileDrawable*>(gld))
		{
			PropertyEditorFactory::PropertyEditorOptions opts;
			opts.stringIsFile = true;
			opts.fileTypeFilter = tr("Video Files (*.wmv *.mpeg *.mpg *.avi *.wmv *.flv *.mov *.mp4 *.m4a *.3gp *.3g2 *.mj2 *.mjpeg *.ipod *.m4v *.gsm *.swf *.dv *.dvd *.asf *.mtv *.roq *.aac *.ac3 *.aiff *.alaw *.iif);;Any File (*.*)");
			
			ui->itemPropLayout->addRow(tr("&File:"), PropertyEditorFactory::generatePropertyEditor(item, "videoFile", SLOT(setVideoFile(const QString&)), opts, SIGNAL(videoFileChanged(const QString&))));
	
			typeName = "Video Item";
		}
		else
		if(GLTextDrawable *item = dynamic_cast<GLTextDrawable*>(gld))
		{
			opts.reset();
			
			QWidget *edit = PropertyEditorFactory::generatePropertyEditor(gld, "plainText", SLOT(setPlainText(const QString&)), opts, SIGNAL(plainTextChanged(const QString&)));
			
	// 		QLineEdit *line = dynamic_cast<QLineEdit*>(edit);
	// 		if(line)
	// 			connect(gld, SIGNAL(plainTextChanged(const QString&)), line, SLOT(setText(const QString&)));
			
			QWidget *base = new QWidget();
			
			RtfEditorWindow *dlg = new RtfEditorWindow(item, base);
			QPushButton *btn = new QPushButton("&Advanced...");
			connect(btn, SIGNAL(clicked()), dlg, SLOT(show()));
			
			QHBoxLayout *hbox = new QHBoxLayout(base);
			hbox->setContentsMargins(0,0,0,0);
			hbox->addWidget(new QLabel("Text:"));
			hbox->addWidget(edit);
			hbox->addWidget(btn);
			
			ui->itemPropLayout->addRow(base); 
			
			QHBoxLayout *hbox2 = new QHBoxLayout();
			opts.text = "Load RSS Feed";
			QWidget *boolEdit = PropertyEditorFactory::generatePropertyEditor(gld, "isRssReader", SLOT(setIsRssReader(bool)), opts);
			
			opts.type = QVariant::String;
			QWidget *stringEdit = PropertyEditorFactory::generatePropertyEditor(gld, "rssUrl", SLOT(setRssUrl(const QString&)), opts);
			
			opts.reset();
			opts.value = item->rssRefreshTime() / 60 / 1000;
			opts.suffix = " min";
			opts.min = 1;
			opts.max = 60 * 3;
			QWidget *refreshEdit = PropertyEditorFactory::generatePropertyEditor(gld, "rssRefreshTime", SLOT(setRssRefreshTimeInMinutes(int)), opts);
			
			QCheckBox *box = dynamic_cast<QCheckBox*>(boolEdit);
			if(box)
			{
				connect(boolEdit, SIGNAL(toggled(bool)), stringEdit, SLOT(setEnabled(bool)));
				connect(boolEdit, SIGNAL(toggled(bool)), refreshEdit, SLOT(setEnabled(bool)));
				connect(boolEdit, SIGNAL(toggled(bool)), ui->playlistSetupWidget, SLOT(setDisabled(bool)));
				stringEdit->setEnabled(box->isChecked());
				refreshEdit->setEnabled(box->isChecked());
				ui->playlistSetupWidget->setDisabled(box->isChecked());
			}
			
			hbox2->addWidget(boolEdit);
			hbox2->addWidget(stringEdit);
			hbox2->addWidget(new QLabel("Reload After"));
			hbox2->addWidget(refreshEdit);
			ui->itemPropLayout->addRow(hbox2);	
			
			typeName = "Text Item";
		}
		else
		if(GLSvgDrawable *item = dynamic_cast<GLSvgDrawable*>(gld))
		{
			PropertyEditorFactory::PropertyEditorOptions opts;
			opts.stringIsFile = true;
			opts.fileTypeFilter = tr("SVG Files (*.svg);;Any File (*.*)");
			ui->itemPropLayout->addRow(tr("&SVG File:"), 
				PropertyEditorFactory::generatePropertyEditor(
					item,					// The QObject which contains the property to edit 
					"imageFile", 				// The property name on the QObject to edit
					SLOT(setImageFile(const QString&)), 	// The slot on the QObject which sets the property
					opts, 					// PropertyEditorOptions controlling the display of the editor
					SIGNAL(imageFileChanged(const QString&))	// An optional signal that is emitted by the QObject when the property is changed
				));
			
			typeName = "SVG Item";
		} 
		else
		if(GLRectDrawable *item = dynamic_cast<GLRectDrawable*>(gld))
		{
			QtColorPicker * fillColor = new QtColorPicker;
			fillColor->setStandardColors();
			fillColor->setCurrentColor(item->fillColor());
			connect(fillColor, SIGNAL(colorChanged(const QColor &)), item, SLOT(setFillColor(QColor)));
			
			ui->itemPropLayout->addRow(tr("Fill:"), fillColor);
			
			typeName = "Rectangle";
		}
		else
		if(GLImageDrawable *item = dynamic_cast<GLImageDrawable*>(gld))
		{
			PropertyEditorFactory::PropertyEditorOptions opts;
			opts.stringIsFile = true;
			opts.fileTypeFilter = tr("Image Files (*.png *.jpg *.bmp *.svg *.xpm *.gif);;Any File (*.*)");
			ui->itemPropLayout->addRow(tr("&Image:"), 
				PropertyEditorFactory::generatePropertyEditor(
					item,					// The QObject which contains the property to edit 
					"imageFile", 				// The property name on the QObject to edit
					SLOT(setImageFile(const QString&)), 	// The slot on the QObject which sets the property
					opts, 					// PropertyEditorOptions controlling the display of the editor
					SIGNAL(imageFileChanged(const QString&))	// An optional signal that is emitted by the QObject when the property is changed
				));
			
			typeName = "Image Item";
		} 
		
		
		//ui->drawableGroupBox->setTitle(itemName.isEmpty() ? typeName : QString("%1 (%2)").arg(itemName).arg(typeName));
		setWindowTitle(itemName.isEmpty() ? typeName : QString("%1 (%2)").arg(itemName).arg(typeName));
	}
}

void DrawableDirectorWidget::loadVideoInputList(int idx)
{
	//Q_UNUSED(idx);
	if(idx <0 || idx>=m_videoPlayerList.size())
		return;
	
	QStringList inputs;
	
// 	if(idx == 0)
// 	{
// 		inputs = m_vidSendMgr->videoConnections();;
// 	}
// 	else
// 	{
		PlayerConnection *con = m_videoPlayerList[idx]; //-1];
		inputs = con->videoInputs(); 
// 	}
	
	while(!m_receivers.isEmpty())
	{
		QPointer<VideoReceiver> rx = m_receivers.takeFirst();
		if(rx)
			rx->release(this);
	}
	
	while(m_videoViewerLayout->count() > 0)
	{
		QLayoutItem * item = m_videoViewerLayout->itemAt(m_videoViewerLayout->count() - 1);
		m_videoViewerLayout->removeItem(item);
		delete item;
	}
	
	m_currentVideoWidgets.clear();

	foreach(QString con, inputs)
	{
		QStringList opts = con.split(",");
		//qDebug() << "DrawableDirectorWidget::loadVideoInputList: Con string: "<<con;
		foreach(QString pair, opts)
		{
			QStringList values = pair.split("=");
			QString name = values[0].toLower();
			QString value = values[1];
			
			//qDebug() << "DrawableDirectorWidget::loadVideoInputList: Parsed name:"<<name<<", value:"<<value;
			if(name == "net")
			{
// 				QUrl url(value);
// 				
// 				QString host = url.host();
// 				int port = url.port();

				QStringList url = value.split(":");
				QString host = url[0];
				int port = url.size() > 1 ? url[1].toInt() : 7755;
				
				VideoReceiver *rx = VideoReceiver::getReceiver(host,port);
				
				if(!rx)
				{
					qDebug() << "DrawableDirectorWidget::loadVideoInputList: Unable to connect to "<<host<<":"<<port;
					QMessageBox::warning(this,"Video Connection",QString("Sorry, but we were unable to connect to the video transmitter at %1:%2 - not sure why.").arg(host).arg(port));
				}
				else
				{
					m_receivers << QPointer<VideoReceiver>(rx);
					
					qDebug() << "DrawableDirectorWidget::loadVideoInputList: Connected to "<<host<<":"<<port<<", creating widget...";
					
					VideoWidget *vid = new VideoWidget();
					m_currentVideoWidgets << vid;
					
					vid->setVideoSource(rx);
					vid->setOverlayText(QString("# %1").arg(m_currentVideoWidgets.size()));
					
					rx->setFPS(5);
					vid->setFps(5);
					vid->setRenderFps(true);
					
					vid->setProperty("-vid-con-string",con);
		
					m_videoViewerLayout->addWidget(vid);
					
					connect(vid, SIGNAL(clicked()), this, SLOT(videoInputClicked()));
				}
			}
		}
	}
}

void DrawableDirectorWidget::videoInputClicked()
{
	VideoWidget *vid = dynamic_cast<VideoWidget*>(sender());
	if(!vid)
	{
		qDebug() << "DrawableDirectorWidget::videoInputClicked: Sender is not a video widget, ignoring.";
		return;
	}
	activateVideoInput(vid);
}

void DrawableDirectorWidget::activateVideoInput(VideoWidget *vid)
{
	QString con = vid->property("-vid-con-string").toString();
	
	if(!m_drawable)
		return;
	
	qDebug() << "DrawableDirectorWidget::videoInputClicked: Using con string: "<<con;
	m_drawable->setProperty("videoConnection", con);
	
	foreach(PlayerConnection *player, m_directorWindow->players()->players())
		player->setUserProperty(m_drawable, con, "videoConnection");
}

void DrawableDirectorWidget::keyPressEvent(QKeyEvent *event)
{
	int idx = event->text().toInt();
	if(idx > 0 && idx <= m_currentVideoWidgets.size())
	{
		qDebug() << "DrawableDirectorWidget::keyPressEvent(): "<<event->text()<<", activating input #"<<idx;
		idx --; 
		VideoWidget *vid = m_currentVideoWidgets.at(idx);
		activateVideoInput(vid);
	}
}

void DrawableDirectorWidget::setCurrentItem(GLPlaylistItem *item)
{
	if(!item)
		return;
	
	m_currentItem = item;
	if(item)
	{
		//qDebug() << "DrawableDirectorWidget::setCurrentItem: title:"<<item->title()<<", duration: "<<item->duration();
		
		ui->playlistView->setCurrentIndex(m_drawable->playlist()->indexOf(item));
		
		const char *propName = m_drawable->metaObject()->userProperty().name();
		m_drawable->setProperty(propName, item->value());
	}
}


void DrawableDirectorWidget::pausePlaylist()
{
	if(!m_drawable)
		return;
	foreach(PlayerConnection *con, m_directorWindow->players()->players())
		con->setPlaylistPlaying(m_drawable, false);
}

void DrawableDirectorWidget::playPlaylist()
{
	if(!m_drawable)
		return;
	
	bool isPlaying = ui->playBtn->property("-playlist-isPlaying").toBool();
	if(!isPlaying)
	{
		ui->playBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
	}
	else
	{
		ui->playBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
	}
	 
	ui->playBtn->setProperty("-playlist-isPlaying", !isPlaying);
	
	foreach(PlayerConnection *con, m_directorWindow->players()->players())
		con->setPlaylistPlaying(m_drawable, !isPlaying);
}

void DrawableDirectorWidget::playlistItemDurationChanged()
{
	if(!m_drawable)
		return;
	foreach(PlayerConnection *con, m_directorWindow->players()->players())
		con->updatePlaylist(m_drawable);
}
