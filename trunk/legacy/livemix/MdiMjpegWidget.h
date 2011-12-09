#ifndef MdiMjpeg_H
#define MdiMjpeg_H

#include "MdiVideoChild.h"
#include "MjpegThread.h"

class MdiMjpegWidget : public MdiVideoChild
{
	Q_OBJECT
public:
	MdiMjpegWidget(QWidget*parent=0);
	
protected slots:
	void urlReturnPressed();
	
protected:
	void setupDefaultGui();

	QLineEdit *m_urlInput;
	MjpegThread *m_thread;
};

#endif
