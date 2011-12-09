#ifndef DRAWABLEDIRECTORWIDGET_H
#define DRAWABLEDIRECTORWIDGET_H

#include <QWidget>

namespace Ui {
    class DrawableDirectorWidget;
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
class DirectorWindow;
#include "GLDrawable.h"


class DrawableDirectorWidget : public QWidget
{
	Q_OBJECT
public:
	DrawableDirectorWidget(GLDrawable *drawable, GLScene *scene, DirectorWindow *director);
	~DrawableDirectorWidget();

	DirectorWindow *director() { return m_directorWindow; }
	GLScene *scene() { return m_scene; }
	GLDrawable *drawable() { return m_drawable; }
	GLPlaylistItem *currentItem() { return m_currentItem; }
	
public slots:
	void setCurrentItem(GLPlaylistItem *);
	
protected slots:
	
	void itemSelected(const QModelIndex &);
	void currentItemChanged(const QModelIndex &idx,const QModelIndex &);
	
	void itemLengthChanged(double);
	void btnHideDrawable();
	void btnSendToPlayer();
	void btnAddToPlaylist();
	void btnRemoveFromPlaylist();
	void btnMoveUp();
	void btnMoveDown();

	void playlistTimeChanged(GLDrawable*, double);
	void playlistItemChanged(GLDrawable*, GLPlaylistItem *);
	void playlistItemDurationChanged();
	
	void pausePlaylist();
	void playPlaylist();
	
	void loadVideoInputList(int);
	void videoInputClicked();
	void activateVideoInput(VideoWidget*);
	
	void setDrawable(GLDrawable *gld=0);

protected:
	void keyPressEvent(QKeyEvent *event); 
	
	void changeEvent(QEvent *e);

	void setupUI();
	
	void setPlaylistEditingEnabled(bool);
	bool playlistEditingEnabled() { return m_playlistEditingEnabled; }
	
private:
	Ui::DrawableDirectorWidget *ui;
	
	DirectorWindow *m_directorWindow;
	GLScene *m_scene;
	GLDrawable *m_drawable;
	GLPlaylistItem * m_currentItem;
	
	FlowLayout *m_videoViewerLayout;
	
	QList<QPointer<VideoReceiver> > m_receivers;
	
	QList<QPointer<VideoWidget> > m_currentVideoWidgets;
	
	QList<PlayerConnection*> m_videoPlayerList;
	
	bool m_playlistEditingEnabled;
};

#endif // DRAWABLEDIRECTORWIDGET_H
