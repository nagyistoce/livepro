#include "MdiDVizWidget.h"

#include "VideoWidget.h"
#include "DVizSharedMemoryThread.h"

MdiDVizWidget::MdiDVizWidget(QWidget *parent)
	: MdiVideoChild(parent)
{
	m_thread = DVizSharedMemoryThread::threadForKey("dviz/live");
	
	setVideoSource(m_thread);
	
	setWindowTitle("DViz Feed");
	
	videoWidget()->setFps(1);
}
