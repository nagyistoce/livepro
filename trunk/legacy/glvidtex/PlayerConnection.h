#ifndef PlayerConnection_H
#define PlayerConnection_H

#include <QtGui>
#include <QAbstractSocket>

class GLPlayerClient;
class GLWidgetSubview;
class GLDrawable;
class GLPlaylistItem;
class GLSceneGroup;
class GLScene;

class PlayerConnection;
class PlayerSubviewsModel : public QAbstractListModel
{
	Q_OBJECT
public:
	PlayerSubviewsModel(PlayerConnection*);
	~PlayerSubviewsModel();
	
	int rowCount(const QModelIndex &/*parent*/) const;
	QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	bool setData(const QModelIndex &index, const QVariant & value, int role) ;
	
private slots:
	void subviewAdded(GLWidgetSubview*);
	void subviewRemoved(GLWidgetSubview*);
	void subviewChanged(GLWidgetSubview*);
	
private:
	PlayerConnection *m_conn;

};


class PlayerConnection : public QObject
{
	Q_OBJECT
	
	Q_PROPERTY(QString name		READ name WRITE setName);
	Q_PROPERTY(QString host		READ host WRITE setHost);
	Q_PROPERTY(QString user		READ user WRITE setUser);
	Q_PROPERTY(QString pass		READ pass WRITE setPass);
	Q_PROPERTY(QRect screenRect	READ screenRect WRITE setScreenRect);
	Q_PROPERTY(QRect viewportRect	READ viewportRect WRITE setViewportRect);
	Q_PROPERTY(QSizeF canvasSize READ canvasSize WRITE setCanvasSize);
	Q_PROPERTY(Qt::AspectRatioMode aspectRatioMode READ aspectRatioMode WRITE setAspectRatioMode);

	Q_PROPERTY(bool autoconnect READ autoconnect WRITE setAutoconnect);
	
	Q_PROPERTY(bool isConnected	READ isConnected);
	Q_PROPERTY(QString lastError	READ lastError);
	
	
	Q_PROPERTY(int useCount	READ useCount);
	
public:
	PlayerConnection(QObject *parent=0);
	PlayerConnection(QByteArray&, QObject *parent=0);
	~PlayerConnection();
	
	QByteArray toByteArray();
	void fromByteArray(QByteArray&);
	
	QList<GLWidgetSubview*> subviews() { return m_subviews; }
	
	QString name() { return m_name; }
	QString host() { return m_host; }
	QString user() { return m_user; }
	QString pass() { return m_pass; }
	QRect screenRect() { return m_screenRect; }
	QRect viewportRect() { return m_viewportRect; }
	QSizeF canvasSize() { return m_canvasSize; }
	Qt::AspectRatioMode aspectRatioMode() { return m_aspectRatioMode; }
	
	bool isLocalPlayer() { return m_host == "127.0.0.1" || m_host == "localhost"; }
	
	bool autoconnect() { return m_autoconnect; }
	bool autoReconnect() { return m_autoReconnect; }
	
	bool isConnected() { return m_isConnected; }
	QString lastError() { return m_lastError; }
	
	QString playerVersion() { return m_playerVersion; }
		
	bool videoIputsReceived() { return m_videoIputsReceived; }
	QStringList videoInputs(bool *hasReceivedResponse=0);
	
	bool waitForData(int msec=30000);
	bool waitForWrite(int msec=30000);

	int crossfadeSpeed() { return m_crossfadeSpeed; }
	
	int useCount() { return m_useCount; }
	
	GLSceneGroup *lastGroup() { return m_group; }
	GLScene *lastScene() { return m_scene; }
	
/* static: */
	/// Sort by the useCount() property in ascending order (Z-A). For use as a 'LessThan' argument to a qSort() call (See http://doc.qt.nokia.com/latest/qtalgorithms.html#qSort)
	static bool sortByUseCount(PlayerConnection *a, PlayerConnection *b);
	/// Sort by the useCount() property in descending order (A-Z). For use as a 'LessThan' argument to a qSort() call (See http://doc.qt.nokia.com/latest/qtalgorithms.html#qSort)
	static bool sortByUseCountDesc(PlayerConnection *a, PlayerConnection *b);

public slots:
	void addSubview(GLWidgetSubview*);
	void removeSubview(GLWidgetSubview*);
	
	void addOverlay(GLScene*);
	void removeOverlay(GLScene*);
	
	void connectPlayer(bool sendDefaults=true);
	void disconnectPlayer();
	
	void setName(const QString&);
	void setHost(const QString&);
	void setUser(const QString&);
	void setPass(const QString&);
	void setScreenRect(const QRect&);
	void setViewportRect(const QRect&);
	void setCanvasSize(const QSizeF&);
	void setAspectRatioMode(Qt::AspectRatioMode);
	
	void setAutoconnect(bool flag);
	void setAutoReconnect(bool flag);
	
	void testConnection();
	
	void setGroup(GLSceneGroup *group, GLScene *initialScene=0, bool preloadOnly=false);
	void setScene(GLScene*); // must be part of 'group'
	void setUserProperty(GLDrawable *, QVariant value, QString propertyName="");
	void setVisibility(GLDrawable *, bool isVisible=true);
	
	void queryProperty(GLDrawable *, QString propertyName="");
	
	void fadeBlack(bool flag);
	void setCrossfadeSpeed(int ms);
	
	void setPlaylistPlaying(GLDrawable *, bool play=true);
	void updatePlaylist(GLDrawable *);
	
	void setPlaylistTime(GLDrawable *, double time=0);
	
	void requestVideoInputList();
	
signals:
	void subviewAdded(GLWidgetSubview*);
	void subviewRemoved(GLWidgetSubview*);
	
	void connected();
	void disconnected();
	
	void loginSuccess();
	void loginFailure();
	
	void pingResponseReceived(const QString&);
	void testStarted();
	void testEnded();
	void testResults(bool);
	
	void playerError(const QString&);
	
	void propQueryResponse(GLDrawable *drawable, QString propertyName, const QVariant& value);
	
	void playerNameChanged(const QString&);
	
	void playlistTimeChanged(GLDrawable*, double time);
	void currentPlaylistItemChanged(GLDrawable*, GLPlaylistItem*);
	
	void dataReceived();
	
	void videoInputListReceived(const QStringList&);
	
protected:
	friend class PlayerSubviewsModel;
	
	QList<GLWidgetSubview*> m_subviews;
	
private slots:
	void subviewChanged(GLWidgetSubview*);
	void receivedMap(QVariantMap);
	void clientConnected();
	void clientDisconnected();
	void socketError(QAbstractSocket::SocketError);
	void lostConnection();
	void reconnect();
	
private:
	void setError(const QString&, const QString& cmd="");
	void sendCommand(QVariantList reply);
	
	QString m_name;
	QString m_host;
	QString m_user;
	QString m_pass;
	QRect m_screenRect;
	QRect m_viewportRect;
	QSizeF m_canvasSize;
	Qt::AspectRatioMode m_aspectRatioMode;
	
	bool m_autoconnect;
	bool m_autoReconnect;
	
	bool m_justTesting;
	
	QString m_playerVersion;
	
	GLPlayerClient *m_client;	
	
	GLSceneGroup *m_group;
	GLScene *m_scene;
	
	bool m_isConnected;
	QString m_lastError;
	
	QVariantList m_preconnectionCommandQueue;
	
	QStringList m_videoInputs;
	bool m_videoIputsReceived;

	int m_crossfadeSpeed;
	
	int m_useCount;
	
	QList<GLScene*> m_overlays;
};

class PlayerConnectionList : public QAbstractListModel
{
	Q_OBJECT
public:
	PlayerConnectionList(QObject *parent=0);
	~PlayerConnectionList();
	
	int rowCount(const QModelIndex &/*parent*/) const;
	QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	bool setData(const QModelIndex &index, const QVariant & value, int role) ;
	
	QByteArray toByteArray();
	void fromByteArray(QByteArray);
	
	QList<PlayerConnection*> players() { return m_players; }
	
	int size() { return m_players.size(); }
	PlayerConnection *at(int x) { return x>=0 && x<m_players.size()?m_players.at(x):0; }

public slots:
	void addPlayer(PlayerConnection *);
	void removePlayer(PlayerConnection *);
		
signals:
	void playerAdded(PlayerConnection *);
	void playerRemoved(PlayerConnection *);
	
private slots:
	void playerNameChanged(const QString&);
	
private:
	QList<PlayerConnection *> m_players;

};


#endif
