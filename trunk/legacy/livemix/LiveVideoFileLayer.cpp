#include <QFileInfo>
#include "LiveVideoFileLayer.h"
#include "ExpandableWidget.h"
#include "EditorUtilityWidgets.h"
#include "../glvidtex/GLWidget.h"
#include "../glvidtex/GLVideoDrawable.h"

#include "PlaylistModel.h"

#ifdef HAS_QT_VIDEO_SOURCE
#include "../glvidtex/QtVideoSource.h"
#else
class QtVideoSource
{

};
#endif

LiveVideoFileLayer::LiveVideoFileLayer(QObject *parent)
	: LiveVideoLayer(parent)
	, m_video(0)
	, m_playlistModel(0)
{
}

LiveVideoFileLayer::~LiveVideoFileLayer()
{
	setVideoSource(0); // doesnt change m_video
	if(m_video)
	{
		delete m_video;
		m_video = 0;
	}
}

GLDrawable *LiveVideoFileLayer::createDrawable(GLWidget *widget, bool isSecondary)
{
	// We overrride createDrawable here just for future expansiosn sake
	return LiveVideoLayer::createDrawable(widget, isSecondary);
}

void LiveVideoFileLayer::initDrawable(GLDrawable *drawable, bool isFirst)
{
	//qDebug() << "LiveVideoFileLayer::setupDrawable: drawable:"<<drawable<<", copyFrom:"<<copyFrom;
	LiveVideoLayer::initDrawable(drawable, isFirst);

	if(isFirst)
	{
		// nothing to do 
		// Maybe in future set a static video source with a blue image?
	}
	
	if(m_video)
	{
		//qDebug() << "LiveVideoFileLayer::initDrawable: setting video:"<<m_video;
		setVideo(m_video);
	}
	else
	{
		qDebug() << "LiveVideoFileLayer::initDrawable: m_video is null";
	}
}

void LiveVideoFileLayer::setVideo(QtVideoSource *vid)
{
	//qDebug() << "LiveVideoFileLayer::setVideo: "<<vid;
	
// 	if(vid == m_video)
// 	{
// 		qDebug() << "LiveVideoFileLayer::setVideo: Not setting, vid == m_video";
// 		return;
// 	}
	
	#ifdef HAS_QT_VIDEO_SOURCE
	setVideoSource(vid);
	m_video = vid;
	//setInstanceName(guessTitle(QFileInfo(vid->file()).baseName()));
	#endif
}

QWidget * LiveVideoFileLayer::createLayerPropertyEditors()
{
	//qDebug() << "LiveVideoFileLayer::createLayerPropertyEditors: mark1";
	QWidget * base = new QWidget();
	QVBoxLayout *blay = new QVBoxLayout(base);
	blay->setContentsMargins(0,0,0,0);
#ifdef HAS_QT_VIDEO_SOURCE	
	ExpandableWidget *groupContent = new ExpandableWidget("Play List",base);
	blay->addWidget(groupContent);
	
	QWidget *groupContentContainer = new QWidget;
	QGridLayout *formLayout = new QGridLayout(groupContentContainer);
	formLayout->setContentsMargins(3,3,3,3);
	
	groupContent->setWidget(groupContentContainer);
	
	int row = 0;
	//m_listWidget = new QListWidget(groupContentContainer);
	m_playlistView = new QListView(groupContentContainer);
	
        if(!m_playlistModel)
	{
		m_playlistModel = new PlaylistModel(this);
		if(m_video)
		{
			m_playlistModel->setPlaylist(m_video->playlist());
			connect(m_video->playlist(), SIGNAL(currentIndexChanged(int)), this, SLOT(playlistPositionChanged(int)));
		}
	}

	
	m_playlistView->setModel(m_playlistModel);
	if(m_video)
        	m_playlistView->setCurrentIndex(m_playlistModel->index(m_video->playlist()->currentIndex(), 0));

	connect(m_playlistView, SIGNAL(activated(const QModelIndex&)), this, SLOT(playlistJump(const QModelIndex&)));

	formLayout->addWidget(m_playlistView, row, 0, 1, 2);
	
	row ++;
	QWidget *buttonBase = new QWidget(groupContentContainer);
	QHBoxLayout *hbox = new QHBoxLayout(buttonBase);
	formLayout->addWidget(buttonBase, row, 0, 1, 2);
	
	QPushButton *addItem = new QPushButton(QPixmap("../data/stock-open.png"),"Add File...");
	hbox->addWidget(addItem);
	
	BrowseDialogLauncher *setter = new BrowseDialogLauncher(this, SLOT(addFile(const QString&)), "");
	setter->setFilter(tr("Video Files (*.wmv *.mpeg *.mpg *.avi *.wmv *.flv *.mov *.mp4 *.m4a *.3gp *.3g2 *.mj2 *.mjpeg *.ipod *.m4v *.gsm *.swf *.dv *.dvd *.asf *.mtv *.roq *.aac *.ac3 *.aiff *.alaw *.iif);;Any File (*.*)"));
	connect(addItem, SIGNAL(clicked()), setter, SLOT(browse()));
	
	QPushButton *delItem = new QPushButton(QPixmap("../data/stock-delete.png"),"Remove");
	hbox->addWidget(delItem);
	connect(delItem, SIGNAL(clicked()), this, SLOT(btnDelItem()));
	
	QPushButton *moveItemUp = new QPushButton(QPixmap("../data/stock-go-up.png"),"");
	hbox->addWidget(moveItemUp);
	connect(moveItemUp, SIGNAL(clicked()), this, SLOT(btnMoveItemUp()));
	
	QPushButton *moveItemDown = new QPushButton(QPixmap("../data/stock-go-down.png"),"");
	hbox->addWidget(moveItemDown);
	connect(moveItemDown, SIGNAL(clicked()), this, SLOT(btnMoveItemDown()));
	
	if(m_video)
	{
		QPushButton *playBtn = new QPushButton(qApp->style()->standardIcon(QStyle::SP_MediaPlay),"");
		hbox->addWidget(playBtn);
		connect(playBtn, SIGNAL(clicked()), m_video->player(), SLOT(play()));
		
		QPushButton *pauseBtn = new QPushButton(qApp->style()->standardIcon(QStyle::SP_MediaPause),"");
		hbox->addWidget(pauseBtn);
		connect(pauseBtn, SIGNAL(clicked()), m_video->player(), SLOT(pause()));
	}

	
	
	formLayout->addWidget(buttonBase, row, 0, 1, 2);
	setupListWidget();
		
	groupContent->setExpandedIfNoDefault(true);
#endif	
	/////////////////////////////////////////
	
	QWidget *basics =  LiveVideoLayer::createLayerPropertyEditors();
	blay->addWidget(basics);
	
	return base;
}

void LiveVideoFileLayer::playlistJump(const QModelIndex &index)
{
#ifdef HAS_QT_VIDEO_SOURCE
	if(m_video && index.isValid()) 
		m_video->playlist()->setCurrentIndex(index.row());
#endif
}

void LiveVideoFileLayer::playlistPositionChanged(int currentItem)
{
#ifdef HAS_QT_VIDEO_SOURCE
	if(m_playlistView)
		m_playlistView->setCurrentIndex(m_playlistModel->index(currentItem, 0));
#endif
}



void LiveVideoFileLayer::setupListWidget()
{
// 	if(!m_listWidget)
// 		return;
// 	
// 	m_listWidget->clear();
// 	
// 	QStringList list = fileList();
// 	
// 	foreach(QString file, list)
// 	{
// 		QFileInfo info(file);
// 		QListWidgetItem *item = new QListWidgetItem(info.fileName());
// 		m_listWidget->addItem(item);
// 	}
}



void LiveVideoFileLayer::btnDelItem()
{
#ifdef HAS_QT_VIDEO_SOURCE		
	if(!m_video || !m_playlistView)
		return;
	QMediaPlaylist *list = m_video->playlist();
	QModelIndex idx = m_playlistView->currentIndex();
	if(!idx.isValid())
		return;
		
	//qDebug() << "LiveVideoFileLayer::btnDelItem(): idx: "<<idx.row();
	list->removeMedia(idx.row());
	
	QStringList files = fileList();
	files.removeAt(idx.row());
	m_props["fileList"] = files;
	setInstanceName(guessTitle(QString("%1 Video Files").arg(files.size())));
	
#endif	
}


void LiveVideoFileLayer::btnMoveItemUp()
{
#ifdef HAS_QT_VIDEO_SOURCE		
	if(!m_video || !m_playlistView)
		return;
	QMediaPlaylist *list = m_video->playlist();
	QModelIndex idx = m_playlistView->currentIndex();
	if(!idx.isValid())
		return;
	int row = idx.row();
		
	//qDebug() << "LiveVideoFileLayer::btnMoveItemUp(): row: "<<row;
	
	if(row < 1)
		return;
		
	QMediaContent content = list->media(row);
	list->removeMedia(row);
	list->insertMedia(row-1,content);
	
	m_playlistView->setCurrentIndex(m_playlistModel->index(row-1, 0));
	
	QStringList files = fileList();
	QString taken = files.takeAt(row);
	files.insert(row-1, taken);
	m_props["fileList"] = files;
	
	
#endif	
}

void LiveVideoFileLayer::btnMoveItemDown()
{
#ifdef HAS_QT_VIDEO_SOURCE		
	if(!m_video || !m_playlistView)
		return;
		
	QMediaPlaylist *list = m_video->playlist();
	QModelIndex idx = m_playlistView->currentIndex();
	if(!idx.isValid())
		return;
	int row = idx.row();
	
	//qDebug() << "LiveVideoFileLayer::btnMoveItemDown(): row: "<<row;
	
	if(row >= list->mediaCount())
		return;
		
	QMediaContent content = list->media(row);
	list->removeMedia(row);
	list->insertMedia(row+1,content);
	
	m_playlistView->setCurrentIndex(m_playlistModel->index(row+1, 0));
	
	QStringList files = fileList();
	QString taken = files.takeAt(row);
	files.insert(row+1, taken);
	m_props["fileList"] = files;
#endif	
}

void LiveVideoFileLayer::addFile(const QString& file)
{
#ifdef HAS_QT_VIDEO_SOURCE		

	QStringList list = fileList();
	list << file;
	m_props["fileList"] = list;
	setInstanceName(guessTitle(QString("%1 Video Files").arg(list.size())));
	
	if(!m_video)
	{
		setFileList(list);
		return;
	}
	
	QFileInfo info(file);
	if(!info.exists())
	{
		qDebug() << "LiveVideoFileLayer::addFile: Warning: File does not exist: "<<file;
		
	}
	else
	{
		QUrl url = QUrl::fromLocalFile(info.absoluteFilePath());
		qDebug() << "LiveVideoFileLayer::addFile: Adding media: "<<url;
		m_video->playlist()->addMedia(url);
		
		m_playlistView->setCurrentIndex(m_playlistModel->index(m_video->playlist()->mediaCount()-1, 0));
	}
#endif
}

void LiveVideoFileLayer::setFileList(const QStringList& list)
{
	qDebug() << "LiveVideoFileLayer::setFileList: list: "<<list;
	if(m_video && 
	   fileList() == list)
		return;
		
	if(!m_video)
	{
	
		#ifdef HAS_QT_VIDEO_SOURCE
		//qDebug() << "LiveVideoFileLayer::setFile: Creating video source";
		QtVideoSource *video = new QtVideoSource();
		setVideo(video);
		#else
		qDebug() << "LiveVideoFileLayer::setFile: Unable to play any videos - not compiled with QtVideoSource or QtMobility.";
		#endif
	}
	
	if(m_video)
	{
#ifdef HAS_QT_VIDEO_SOURCE		
		
		QMediaPlaylist *medialist = playlist();
		medialist->clear();
	
		foreach(QString file, list)
		{
			if(file.isEmpty())
				continue;
				
			QFileInfo info(file);
			
			if(!info.exists())
			{
				qDebug() << "LiveVideoFileLayer::setFileList: Warning: File does not exist: "<<file;
				
			}
			else
			{
				QUrl url = QUrl::fromLocalFile(info.absoluteFilePath());
				qDebug() << "LiveVideoFileLayer::setFileList: Adding media: "<<url;
				if (info.suffix().toLower() == QLatin1String("m3u"))
					medialist->load(url);
				else
					medialist->addMedia(url);
			}
		}
		
		m_video->playlist()->setCurrentIndex(0);
		m_video->player()->play();
#endif		
	}
	
	m_props["fileList"] = list;
	
	setupListWidget();

	setInstanceName(guessTitle(QString("%1 Files").arg(list.size())));
}

QMediaPlaylist * LiveVideoFileLayer::playlist()
{
	#ifdef HAS_QT_VIDEO_SOURCE
	if(m_video)
		return m_video->playlist();
	#endif
	
	return 0;
	
}

QMediaPlayer * LiveVideoFileLayer::player()
{
	#ifdef HAS_QT_VIDEO_SOURCE
	if(m_video)
		return m_video->player();
	#endif
	
	return 0;
	
}

void LiveVideoFileLayer::setLayerProperty(const QString& key, const QVariant& value)
{
	if(key == "fileList")
	{
		setFileList(value.toStringList());
	}
	else
	if(m_video)
	{
#ifdef HAS_QT_VIDEO_SOURCE		
		const char *keyStr = qPrintable(key);
		if(m_video->metaObject()->indexOfProperty(keyStr)>=0)
		{
			m_video->setProperty(keyStr, value);
		}
#endif		
	}
	
	LiveLayer::setLayerProperty(key,value);
}

