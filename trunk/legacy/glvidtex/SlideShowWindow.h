#ifndef SlideShowWindow_H
#define SlideShowWindow_H

#include <QWidget>
#include <QVariant>
#include <QTimer>

class GLSceneGroup;
class GLScene;
class GLWidget;
class QGraphicsView;
class QGraphicsScene;
class GLPlaylistItem;
class GLRectDrawable;
class GLDrawable;

#include "../livemix/VideoSource.h"

class SlideShowWindow : public QWidget
{
	Q_OBJECT
public:
	SlideShowWindow(QWidget *parent=0);
	
	GLSceneGroup *group() { return m_group; }
	//GLScene *scene() {  return m_scene; }
	
public slots:
	void setGroup(GLSceneGroup*);
	void setScene(GLScene*);

private slots:
	//void drawableIsVisible(bool);
	void opacityAnimationFinished();
	
	void timerTick();
	
	void setSceneNum(int);

private:
	GLSceneGroup *m_group;
	QPointer<GLScene> m_scene;
	QPointer<GLScene> m_oldScene;
	
	QGraphicsView *m_graphicsView;
	QGraphicsScene *m_graphicsScene;
	
	GLWidget *m_glWidget;
	
	bool m_useGLWidget;
	
	int m_xfadeSpeed;
	
	QList<GLDrawable*> m_oldDrawables;
	
	QList<GLScene*> m_scenes;
	int m_currentIdx;
	QTimer m_sceneTimer;	
};

#endif
