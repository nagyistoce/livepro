#ifndef SharedMemorySender_H
#define SharedMemorySender_H

#include <QtGui>

#include "../livemix/VideoFrame.h"
#include "VideoConsumer.h"

#define FRAME_WIDTH  1024
#define FRAME_HEIGHT 768
#define FRAME_FORMAT QImage::Format_ARGB32

class SharedMemorySender : public QObject, 
			   public VideoConsumer
{
	Q_OBJECT
public:
	SharedMemorySender(QString key = "SharedMemorySender-1.0", QObject *parent=0);
	~SharedMemorySender();
	
	void setVideoSource(VideoSource *source);

public slots:
	void disconnectVideoSource();

private slots:
	void frameReady();
	void processFrame();
	
private:
	QSharedMemory m_sharedMemory;
	int m_memorySize;
	
	//QImage m_image;
	
	VideoFramePtr m_frame;
};


#endif
