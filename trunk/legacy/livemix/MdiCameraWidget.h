#ifndef MdiCameraWidget_H
#define MdiCameraWidget_H

#include "MdiVideoChild.h"
#include "CameraThread.h"

#include <QComboBox>
class QMenu;

class MdiCameraWidget : public MdiVideoChild
{
	Q_OBJECT
public:
	MdiCameraWidget(QWidget*parent=0);
	
	bool deinterlace() { return m_deinterlace; }
	
public slots:
	void setDeinterlace(bool flag);
	
protected slots:
	void deviceBoxChanged(int);	

protected:
	CameraThread *m_thread;
	QComboBox *m_deviceBox;
	QStringList m_cameras;
	bool m_deinterlace;
};

#endif
