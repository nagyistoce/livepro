#ifndef EditorGraphicsView_H
#define EditorGraphicsView_H

#include <QtGui>

class EditorGraphicsView : public QGraphicsView
{
	public:
		EditorGraphicsView(QWidget * parent=0);
		bool canZoom() { return m_canZoom; }
		void setCanZoom(bool flag) { m_canZoom = flag; }

		qreal scaleFactor() { return m_scaleFactor; }
		void setScaleFactor(qreal);
		
		bool autoResize() { return m_autoResize; }
		void setAutoResize(bool flag);

	protected:
		void keyPressEvent(QKeyEvent *event);
		void wheelEvent(QWheelEvent *event);
		void scaleView(qreal scaleFactor);
		
		void resizeEvent(QResizeEvent *);
		void adjustViewScaling();
	private:
		bool m_canZoom;
		qreal m_scaleFactor;
		bool m_autoResize;
};



#endif
