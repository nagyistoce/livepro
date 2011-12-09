#ifndef LayerControlWidget_H
#define LayerControlWidget_H

#include <QFrame>

class QLabel;
class QSlider;
class QPushButton;

class LiveLayer;
class LayerControlWidget : public QFrame 
{
        Q_OBJECT
public:
	LayerControlWidget(LiveLayer*);
	virtual ~LayerControlWidget();
	
	LiveLayer *layer() { return m_layer; }
	bool isCurrentWidget() { return m_isCurrentWidget; }

signals:
	void clicked();

public slots:
	void setIsCurrentWidget(bool);

protected slots:
	void instanceNameChanged(const QString&);
	void opacitySliderChanged(int);
	void drawableVisibilityChanged(bool);

protected:
	virtual void setupUI();
	void mouseReleaseEvent(QMouseEvent*);

private:
	LiveLayer *m_layer;
	QLabel *m_nameLabel;
	QSlider *m_opacitySlider;
	QPushButton *m_liveButton;
	QPushButton *m_editButton;
	bool m_isCurrentWidget;
};


#endif
