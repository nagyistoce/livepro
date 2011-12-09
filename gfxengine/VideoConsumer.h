#ifndef VideoConsumer_H
#define VideoConsumer_H

class VideoSource;

class VideoConsumer
{
public:
	VideoConsumer();
	virtual ~VideoConsumer() {};
	
	virtual void setVideoSource(VideoSource*) = 0;
	virtual void disconnectVideoSource() = 0;
	
	VideoSource *videoSource() { return m_source; }	

protected:
	VideoSource *m_source;

};
	
#endif
