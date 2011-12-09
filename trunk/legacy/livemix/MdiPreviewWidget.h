#ifndef MdiPreviewWidget_H
#define MdiPreviewWidget_H

#include "MdiVideoChild.h"
#include "VideoSource.h"

#include <QComboBox>
#include <QList>
#include <QRect>
#include <QSlider>
#include <QDoubleSpinBox>

class MainWindow;
class VideoOutputWidget;
class GLWidget;
class MdiPreviewWidget : public MdiVideoChild
{
	Q_OBJECT
public:
	MdiPreviewWidget(QWidget*parent=0);
	
public slots:
	void takeSource(MdiVideoChild *);
	void takeSource(VideoSource *);
	void setRenderFps(bool flag);
	void setMainWindow(MainWindow*);
	
protected slots:
	void outputActionTriggered(QAction*);
	void textReturnPressed();
	void setFadeTimef(double);
	void setFadeTime(int);
	
	void videoChildAdded(MdiVideoChild*);
	void videoChildDestroyed();
	void overlayBoxIndexChanged(int);
	
protected:
	void setupOverlayBox();
	
	QComboBox *m_overlayBox;
	QList<QRect> m_screenList;
	
	QLineEdit *m_textInput;
	
	QSlider *m_fadeSlider;
	QDoubleSpinBox *m_timeBox;
	
	VideoWidget *m_outputWidget;
	//VideoOutputWidget * m_outputWidget;
	//GLWidget *m_outputWidget;
	
	MainWindow *m_mainWindow;
	
	QList<MdiVideoChild*> m_overlayChilds;
};

#endif
