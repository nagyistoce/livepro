#ifndef CameraThread_h
#define CameraThread_h


#ifndef UINT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

// Defn from http://gcc.gnu.org/ml/gcc-bugs/2002-10/msg00259.html
#ifndef INT64_C
# define INT64_C(c) c ## LL
#endif

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

#include <QImage>
#include <QTimer>
#include <QMutex>

#include <QFile>

#include "VideoSource.h"
class SimpleV4L2;

// For capturing from a Blackmagic DeckLink-compatible device (such as the Intensity PRo)
class BMDCaptureDelegate;

class CameraThread: public VideoSource
{
	Q_OBJECT

private:
	CameraThread(const QString& device, QObject *parent=0);
	static QMap<QString,CameraThread *> m_threadMap;
	static QStringList m_enumeratedDevices;
	static bool m_devicesEnumerated;
	
public:
	~CameraThread();

	static CameraThread * threadForCamera(const QString& camera);
	
	static QStringList enumerateDevices(bool forceReenum=false);
	
	QStringList inputs();
	int input();
	
	
	//QImage frame();
	
	int fps() { return m_fps; }
	
	bool deinterlace() { return m_deinterlace; }
	
	bool rawFramesEnabled() { return m_rawFrames; }
	
	virtual VideoFormat videoFormat();
	
	const QString & inputName() { return m_cameraFile; }
	
	bool inputInitalized() { return m_inited; }
	bool hasError() { return m_error; }

public slots:
	void setDeinterlace(bool);
	void setFps(int fps=30);
	void enableRawFrames(bool enable=true);
	
	void setInput(int);
	bool setInput(const QString& name);
	
	
signals:
	//void newImage(QImage);
	//void frameReady(int frameHoldTime);

protected slots:
	void start(QThread::Priority);
	
	void run();
	void readFrame();

	void freeResources();
	int initCamera();
	
	void destroySource();
	
protected:
	friend class BMDCaptureDelegate;
	void rawDataAvailable(uchar *bytes, int size, QSize pxSize, QTime captureTime = QTime());
	void imageDataAvailable(QImage img, QTime captureTime = QTime());
	
private:
	int m_fps;
	
	QTimer m_readTimer;

	AVFormatContext * m_av_format_context;
	AVCodecContext * m_video_codec_context;
	AVCodecContext * m_audio_codec_context;
	AVCodec * m_video_codec;
	AVCodec * m_audio_codec;
	AVFrame * m_av_frame;
	AVFrame * m_av_rgb_frame;
	SwsContext * m_sws_context;
	AVRational m_time_base_rational;
	uint8_t * m_buffer;
	int m_video_stream;
	int m_audio_stream;

	//QVideoBuffer * m_video_buffer;
	//QVideoBuffer * m_audio_buffer;

	QImage * m_frame;

// 	SDL_AudioSpec wanted_spec;
// 	SDL_AudioSpec spec;

	//QVideo * m_video;
	//QFFMpegVideoFrame m_current_frame;

	AVRational m_timebase;

	quint64 m_start_timestamp;
	bool m_initial_decode;

	double m_fpms;

	double m_previous_pts;

	int m_decode_timer;

	double m_video_clock; ///<pts of last decoded frame / predicted pts of next decoded frame

	bool m_inited;

	QString m_cameraFile;
	
	int m_refCount;	

	int m_frameCount;

	QMutex m_readMutex;
	
	bool m_deinterlace;
	
	static QMutex threadCacheMutex;
	
	bool m_rawFrames;
	
	QFile m_videoDev;
	QByteArray m_frameData;
	
	SimpleV4L2 * m_v4l2;
	BMDCaptureDelegate *m_bmd;
	
	QMutex m_initMutex;
	
	QString m_inputName;
	
	bool m_error;
	
};


#endif //CameraThread_h

