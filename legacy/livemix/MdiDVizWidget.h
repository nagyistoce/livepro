#ifndef MdiDViz_H
#define MdiDViz_H

#include "MdiVideoChild.h"
class DVizSharedMemoryThread;

class MdiDVizWidget : public MdiVideoChild
{
	Q_OBJECT
public:
	MdiDVizWidget(QWidget*parent=0);
	
protected:
	DVizSharedMemoryThread *m_thread;
};

#endif
