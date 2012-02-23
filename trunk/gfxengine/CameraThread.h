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

#ifdef HAS_LIBAV
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}
#endif

#include <QImage>
#include <QTimer>
#include <QMutex>

#include <QFile>
#include <QVariantMap>

#include "VideoSource.h"
class SimpleV4L2;

// For capturing from a Blackmagic DeckLink-compatible device (such as the Intensity Pro)
class BMDCaptureDelegate;

// For test signal generation
class TestSignalGenerator;

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
	
	// Must be called at start of app before enumerateDevices() has a chance to be called
	static void enableTestSignalGenerator(bool flag=false, int numChan=2);
	static int numTestChannels() { return m_numTestChan; }
	
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
	
	// relies on SimpleV4L2, therefore only works on linux with V4L2 devices currently (or BlackMagic too, since it is trivial to get signal flags from their SDK as well)
	bool hasSignal();
	
	QVariantMap videoHints();

public slots:
	void setDeinterlace(bool);
	void setFps(int fps=30);
	void enableRawFrames(bool enable=true);
	
	void setInput(int);
	bool setInput(const QString& name);
	
	void setVideoHints(QVariantMap);
	
	
signals:
	//void newImage(QImage);
	//void frameReady(int frameHoldTime);
	void signalLost();
	void signalFound();
	void signalStatusChanged(bool flag);

protected slots:
	void start(QThread::Priority);
	
	void run();
	void readFrame();

	void freeResources();
	int initCamera();
	
	void destroySource();
	
	// uses SimpleV4L2::hasSignal(), emits signalLost() (or signalFound() if appros), and signalStatusChanged(bool flag),
	void checkForSignal();
	
protected:
	friend class BMDCaptureDelegate;
	friend class TestSignalGenerator;
	void rawDataAvailable(uchar *bytes, int size, QSize pxSize, QTime captureTime = QTime());
	void imageDataAvailable(QImage img, QTime captureTime = QTime());
	void frameAvailable(VideoFramePtr ptr);
	
private:
	int m_fps;
	
	QTimer m_readTimer;

#ifdef HAS_LIBAV
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

        AVRational m_timebase;
#endif
        //QVideoBuffer * m_video_buffer;
	//QVideoBuffer * m_audio_buffer;

	QImage * m_frame;

// 	SDL_AudioSpec wanted_spec;
// 	SDL_AudioSpec spec;

	//QVideo * m_video;
	//QFFMpegVideoFrame m_current_frame;


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
	TestSignalGenerator *m_testGen;
	
	static bool m_enabTest;
	static int m_numTestChan;
	
	QMutex m_initMutex;
	
	QString m_inputName;
	
	bool m_error;
	
	QTimer m_checkSignalTimer;
	bool m_hasSignal;
	
	QVariantMap m_videoHints;
	
};


#endif //CameraThread_h

