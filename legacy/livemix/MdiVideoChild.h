
#ifndef MdiVideoChild_H
#define MdiVideoChild_H

#include <QWidget>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QMenu>

#include "MdiChild.h"

class VideoSource;
class VideoWidget;

class MdiVideoChild : public MdiChild
{
	Q_OBJECT

public:
	MdiVideoChild(QWidget *parent=0);
	
	VideoSource *videoSource() { return m_videoSource; }
	VideoWidget *videoWidget() { return m_videoWidget; }
	
	virtual void setVideoSource(VideoSource*);

signals:
	void clicked();
	
public slots:
	virtual void setRenderFps(bool);
	
protected:
	void setupDefaultGui();
	virtual void contextMenuEvent(QContextMenuEvent *);
	virtual void closeEvent(QCloseEvent*);
	virtual void showEvent(QShowEvent*);

	
	QVBoxLayout *m_layout;
	VideoSource *m_videoSource;
	VideoWidget *m_videoWidget;
	
	QMenu *m_configMenu;
	
	
};

#endif
