#ifndef LiveSceneListModel_H
#define LiveSceneListModel_H

#include <QAbstractListModel>
#include <QList>
#include <QGraphicsView>
#include <QTimer>
#include <QRect>
#include <QSize>

#include <math.h>

#include <QStringList>
#include <QPointer>
#include <QHash>

class LiveLayer;
class LiveScene;

class LiveSceneListModel : public QAbstractListModel
{
Q_OBJECT

public:
	LiveSceneListModel(QObject *parent = 0);
	virtual ~LiveSceneListModel();
	
// 	void setSceneRect(QRect);
// 	QRect sceneRect(){ return m_sceneRect; }
// 	void setIconSize(QSize);
// 	QSize iconSize() { return m_iconSize; }
	
	void showOnlyUserControllable(bool flag=true);
	bool isOnlyUserControllableShown() { return m_showOnlyUserControllable; }
	
	void setLiveScene(LiveScene*);
	LiveLayer * itemFromIndex(const QModelIndex &index);
	LiveLayer * itemAt(int);
	QModelIndex indexForItem(LiveLayer *) const;
	QModelIndex indexForRow(int row) const;
	
	/* ::QAbstractListModel */
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &index, const QVariant & value, int role = Qt::EditRole);
	QVariant headerData(int section, Qt::Orientation orientation,
				int role = Qt::DisplayRole) const;

	/* Drag and Drop Support */
	Qt::ItemFlags flags(const QModelIndex &index) const;
	Qt::DropActions supportedDropActions() const { return Qt::MoveAction | Qt::CopyAction; }

	QStringList mimeTypes () const { QStringList x; x<<itemMimeType(); return x; }
 	QMimeData * mimeData(const QModelIndexList & indexes) const;
 	bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent);
 	
 	// Not from AbstractListModel, just for utility
	QString itemMimeType() const { return "application/x-livemix-livescene-listmodel-item"; }
 	

signals:
	void layersDropped(QList<LiveLayer*>);
	void repaintList();
	
public slots:	
	void releaseLiveScene();
	
private slots:
	void layerPropertyChanged(const QString& propertyId, const QVariant& value, const QVariant& oldValue);
	void layerAdded(LiveLayer*);
	void layerRemoved(LiveLayer*);
	
 	void modelDirtyTimeout();
	
protected:
	void markLayerDirty(LiveLayer*);
	
	void internalSetup();
	
	LiveScene * m_scene;
	QList<LiveLayer*> m_sortedLayers;
 	QList<LiveLayer*> m_dirtyLayers;
 	
	QTimer m_dirtyTimer;

	bool m_showOnlyUserControllable;

};

#endif
