

#include "StreamEncoderProcess.h"
#include "../livemix/DVizSharedMemoryThread.h"
#include "VideoEncoder.h"

StreamEncoderProcess::StreamEncoderProcess(QObject *parent)
	: QObject(parent)
{
	m_shMemRec = DVizSharedMemoryThread::threadForKey("PlayerWindow-2");
	
	QString outputFile = "test-output/output.avi";
	
// 	QFileInfo info(outputFile);
// 	if(info.exists())
// 	{
// 		int counter = 1;
// 		QString newFile;
// 		while(QFileInfo(newFile = QString("%1-%2.%3")
// 			.arg(info.baseName())
// 			.arg(counter)
// 			.arg(info.completeSuffix()))
// 			.exists())
// 			
// 			counter++;
// 		
// 		qDebug() << "StreamEncoderProcess: Video output file"<<outputFile<<"exists, writing to"<<newFile<<"instead.";
// 		outputFile = newFile;
// 	}

	
	/*m_encoder = new VideoEncoder(outputFile);
	m_encoder->setVideoSource(m_shMemRec);
	m_encoder->startEncoder();*/ 
};

StreamEncoderProcess::~StreamEncoderProcess()
{
	if(m_encoder)
	{
		delete m_encoder;
		m_encoder = 0;
	}
}
