#ifndef GLEditorGraphicsScene_H
#define GLEditorGraphicsScene_H

#include <QGraphicsScene>
#include "GLDrawable.h"

class RectItem;

class GLEditorGraphicsScene : public QGraphicsScene
{
	Q_OBJECT
public:
	GLEditorGraphicsScene();
	
	QList<GLDrawable*> selectedDrawables();
	
	void clear();
	
	bool isEditingMode() { return m_editingMode; }
	
// signals:
// 	void selectionChanged();
// 	void drawableSelected(GLDrawable*);
	
public slots:
// 	void clearSelection(QList<GLDrawable*> ignoreList = QList<GLDrawable*>());
	void setSceneRect(const QRectF&);
	void removeDrawables();
	void deleteSelectedItems();
	
	void setEditingMode(bool flag=true);
	
protected:
// 	friend class GLDrawable;
// 	void itemSelected(GLDrawable*);
	
// 	virtual void mousePressEvent ( QGraphicsSceneMouseEvent * mouseEvent );
// 	virtual void mouseMoveEvent ( QGraphicsSceneMouseEvent * mouseEvent );
// 	virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * mouseEvent );
	virtual void keyPressEvent(QKeyEvent * event);

private:
	void addInternalItems();
	
	//QList<GLDrawable*> m_selection;
	RectItem * m_bgRect;
	//RectItem * m_dragRect;
	//bool m_lockClearSelection;	
	bool m_editingMode;
	//bool m_ctrlPressedWithMouse;
};

#endif
