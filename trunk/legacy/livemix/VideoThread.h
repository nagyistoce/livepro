#ifndef VideoThread_h
#define VideoThread_h


#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

#include <QThread>

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
#include <QTime>
#include <QMovie>
#include <QMutex>

#include "VideoSource.h"

class VideoWidget;
class VideoThread: public VideoSource
{
	Q_OBJECT

public:
	VideoThread(QObject *parent=0);
	~VideoThread();

	void setVideo(const QString&);

	enum Status {Running, NotRunning, Paused};
	Status status() { return m_status; }
	
	void registerConsumer(VideoWidget */*consumer*/) {}
	void release(VideoWidget */*consumer*/=0) {}
	
	//QImage frame();
	virtual VideoFormat videoFormat();
	
	QString videoFile() { return m_videoFile; }
	
	double duration() { return m_duration; }
	
	virtual void start(bool startPaused=false);
	
signals:
	//void newImage(QImage,QTime);
	//void frameReady(int frameHoldTime);
	void movieStateChanged(QMovie::MovieState);

public slots:
	void seek(int ms, int flags);
	void restart();
	void play();
	void pause();
	void stop();
	void setStatus(Status);

protected slots:
	void readFrame();
 	void releaseCurrentFrame();
 	void updateTimer();

protected:
	void run();

	void freeResources();
	int initVideo();


	void calculateVideoProperties();
	
	
private:
	QTimer *m_readTimer;

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

	QImage m_frame;

// 	SDL_AudioSpec wanted_spec;
// 	SDL_AudioSpec spec;

	//QVideo * m_video;
	//QFFMpegVideoFrame m_current_frame;

	AVRational m_timebase;

	quint64 m_start_timestamp;
	bool m_initial_decode;

	double m_previous_pts;

	int m_decode_timer;

	bool m_killed;

	double m_video_clock; ///<pts of last decoded frame / predicted pts of next decoded frame

	bool m_inited;

	QString m_videoFile;
	
	double m_previous_pts_delay;
	QSize m_frame_size;
	double m_duration;
	double m_fpms;
	double m_frame_rate;

	QTime m_run_time;
	int m_total_runtime;
	
	Status m_status;
	
	double m_frameTimer;
	int m_skipFrameCount;
	QTime m_time;
	
	QMutex m_bufferMutex;
	int m_nextDelay;
	bool m_frameConsumed;
	int m_frameLockCount;
	
	int m_frameSmoothCount;
	int m_frameSmoothAccum;
	
	bool m_startPaused;
};


#endif //VideoThread_h

