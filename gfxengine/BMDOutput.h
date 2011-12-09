#ifndef BMDOutput_H
#define BMDOutput_H

#include <QtGui>

#include "../livemix/VideoFrame.h"
#include "VideoConsumer.h"

class BMDOutputDelegate;

class BMDOutput : public QThread, 
		  public VideoConsumer
{
	Q_OBJECT
public:
	BMDOutput(QString output = "bmd:0", QObject *parent=0);
	~BMDOutput();
	
	void setVideoSource(VideoSource *source);

public slots:
	void disconnectVideoSource();

private slots:
	void frameReady();
	
protected:
	void run();
	
private:
	void getFrame();
	void setupOutput();
	
	QString m_bmdOutputName;
	
	bool m_frameReady;
	bool m_killed;
	bool m_started;
	
	BMDOutputDelegate *m_bmd;
};


#endif
