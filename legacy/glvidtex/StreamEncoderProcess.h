#ifndef StreamEncoderProcess_H
#define StreamEncoderProcess_H

#include <QtGui>

class DVizSharedMemoryThread;
class VideoEncoder;

class StreamEncoderProcess : public QObject
{
	Q_OBJECT
public:
	StreamEncoderProcess(QObject *parent=0);
	~StreamEncoderProcess();

private:
	DVizSharedMemoryThread *m_shMemRec;
	VideoEncoder *m_encoder;	

};

#endif
