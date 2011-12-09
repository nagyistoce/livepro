#ifndef PlayerWindow_H
#define PlayerWindow_H

#include <QWidget>
#include <QVariant>
#include <QTimer>

class GLPlayerServer;

// // for testing
// class GLPlayerClient;

class GLSceneGroupCollection;
class GLSceneGroup;
class GLScene;
class GLWidget;
class QGraphicsView;
class QGraphicsScene;
class VideoInputSenderManager;
class VideoEncoder;
class GLPlaylistItem;
class GLRectDrawable;
class GLDrawable;
class SharedMemorySender;
class V4LOutput;
class GLWidgetSubview;

#include "../3rdparty/qjson/serializer.h"
#include "../livemix/VideoSource.h"

class PlayerWindow;
class PlayerCompatOutputStream : public VideoSource
{
	Q_OBJECT

protected:
	friend class PlayerWindow;
	PlayerCompatOutputStream(PlayerWindow *parent=0);

public:
	//virtual ~StaticVideoSource() {}

	VideoFormat videoFormat() { return VideoFormat(VideoFrame::BUFFER_IMAGE,QVideoFrame::Format_ARGB32); }
	//VideoFormat videoFormat() { return VideoFormat(VideoFrame::BUFFER_BYTEARRAY,QVideoFrame::Format_ARGB32); }

	const QImage & image() { return m_image; }
	int fps() { return m_fps; }

public slots:
	void setFps(int);

signals:
	void frameReady();

private slots:
	void renderScene();
	void setImage(QImage);

protected:
	virtual void consumerRegistered(QObject*);
	virtual void consumerReleased(QObject*);
	
private:
	PlayerWindow *m_win;
	QImage m_image;
	int m_fps;
	QTimer m_frameReadyTimer;
};


#include "../http/HttpServer.h"

class PlayerJsonServer : public HttpServer
{
	Q_OBJECT

protected:
	
	friend class PlayerWindow;
	PlayerJsonServer(quint16 port, PlayerWindow* parent = 0);
	
	void dispatch(QTcpSocket *socket, const QStringList &pathElements, const QStringMap &query);
	
private:
	void sendReply(QTcpSocket *socket, QVariantList list);
	QVariant stringToVariant(QString string, QString type);
	
	QVariantList examineScene(GLScene *scene=0);
	QVariantList examineGroup(GLSceneGroup *group=0);
	QVariantList examineDrawable(GLDrawable *gld=0);
	
	PlayerWindow * m_win;
	QJson::Serializer * m_jsonOut;

};

class PlayerWindow : public QWidget
{
	Q_OBJECT
public:
	PlayerWindow(QWidget *parent=0);

	GLSceneGroupCollection *collection() { return m_col; }
	GLSceneGroup *group() { return m_group; }
	QPointer<GLScene> scene() {  return m_scene; }
	GLSceneGroup *overlays() { return m_overlays; }

	void loadConfig(const QString& file="player.ini", bool verbose=false);
	
public slots:
	void setCollection(GLSceneGroupCollection*);
	bool setGroup(GLSceneGroup*);
	void setScene(GLScene*);
	
	void addOverlay(GLScene*);
	void removeOverlay(GLScene*);
	

private slots:
	void receivedMap(QVariantMap);

/*	// for testing
	void sendTestMap();
	void slotConnected();*/

	void currentPlaylistItemChanged(GLPlaylistItem*);
	void playlistTimeChanged(double);

	//void drawableIsVisible(bool);
	void opacityAnimationFinished(GLScene *scene =0);

protected:
	friend class PlayerCompatOutputStream;
	QGraphicsScene *graphicsScene() { return m_graphicsScene; }
	
	QRectF stringToRectF(const QString&);
	QPointF stringToPointF(const QString&);
	
	double xfadeSpeed() { return m_xfadeSpeed; }
	
protected slots:
	friend class PlayerJsonServer;
	
	void setBlack(bool black=true);
	void setCrossfadeSpeed(int ms);
	void setCanvasSize(const QSizeF&);
	void setScreen(QRect geom);
	void setIgnoreAR(bool);
	void setViewport(const QRectF&);
	
	QStringList videoInputs();
	void clearSubviews();
	void removeSubview(int id);
	void addSubview(GLWidgetSubview *view);
	
	void setTestScene();
	
private:
	void sendReply(QVariantList);
	
	void displayScene(GLScene*);
	void addScene(GLScene*, int layer=1, bool fadeInOpac=true);
	void removeScene(GLScene*);

	GLPlayerServer *m_server;

	bool m_loggedIn;

	QString m_validUser;
	QString m_validPass;

	QString m_playerVersionString;
	int m_playerVersion;

	GLSceneGroupCollection *m_col;
	GLSceneGroup *m_group;
	GLSceneGroup *m_oldGroup;
	GLSceneGroup *m_preloadGroup;
	QPointer<GLScene> m_scene;
	QPointer<GLScene> m_oldScene;

	QGraphicsView *m_graphicsView;
	QGraphicsScene *m_graphicsScene;

	GLWidget *m_glWidget;

	bool m_useGLWidget;

/*	// for testing
	GLPlayerClient *m_client;*/
	VideoInputSenderManager *m_vidSendMgr;

	VideoEncoder *m_outputEncoder;

	//SharedMemorySender *m_shMemSend;
	V4LOutput *m_v4lOutput;

	int m_xfadeSpeed;

	PlayerCompatOutputStream *m_compatStream;

	//GLRectDrawable *m_blackOverlay;
	bool m_isBlack;
	GLScene *m_blackScene;
	QPointer<GLScene> m_scenePreBlack;
	QList<GLDrawable*> m_oldDrawables;

	bool m_configLoaded;
	
	PlayerJsonServer *m_jsonServer;
	
	GLSceneGroup *m_overlays;
	
	GLScene *m_testScene;
	
	
};

#endif
