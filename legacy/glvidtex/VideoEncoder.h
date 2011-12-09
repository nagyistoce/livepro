#ifndef VideoEncoder_H
#define VideoEncoder_H

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

#undef exit

#define STREAM_PIX_FMT PIX_FMT_YUV420P
#define NATIVE_PIX_FMT PIX_FMT_RGB32
//PIX_FMT_YUV420P /* default pix_fmt */

#include "../livemix/VideoSource.h"

class VideoEncoderData
{
public:
	AVFrame *picture, *tmp_picture;
	uint8_t *video_outbuf;
	int frame_count, video_outbuf_size;
};

class AudioEncoderData
{
public:

	float t, 
		tincr, 
		tincr2;
	int16_t *samples;
	uint8_t *audio_outbuf;
	int audio_outbuf_size;
	int audio_input_frame_size;
	
};

class VideoEncoder : public QThread
{
	Q_OBJECT
public:
	VideoEncoder(const QString &file="", QObject *parent=0);
	~VideoEncoder();
	
	// Must call startEncoder() before anything is actually written to disk
	// startEncoder() calls QThread::start() to start pumping frames to disk
	bool startEncoder(const QString& filename="", double duration = 10.0, int frameRate = 25);
	
	bool encodingStarted() { return m_encodingStarted; }
	
public slots:
	void setVideoSource(VideoSource *);
	void disconnectVideoSource();
	
	// stopEncoder() called automatically from destructor if not called before destroying encoder
	// stopENcoder() is also called automatically from disconnectVideoSource()
	void stopEncoder();


protected slots:
	void frameReady();

protected:
	bool setupInternal();
	void run();
	void teardownInternal();

	AVStream *addAudioStream(AVFormatContext *oc, enum CodecID codec_id);
	void openAudio(AVFormatContext *oc, AVStream *st);
	void getDummyAudioFrame(int16_t *samples, int frame_size, int nb_channels);
	void writeAudioFrame(AVFormatContext *oc, AVStream *st);
	void closeAudio(AVFormatContext *oc, AVStream *st);
	
	AVStream *addVideoStream(AVFormatContext *oc, enum CodecID codec_id);
	AVFrame *allocPicture(enum PixelFormat pix_fmt, int width, int height);
	void openVideo(AVFormatContext *oc, AVStream *st);
	void fillDummyYuvImage(AVFrame *pict, int frame_index, int width, int height);
	void fillAvPicture(AVFrame *pict, int frame_index, int width, int height);
	void writeVideoFrame(AVFormatContext *oc, AVStream *st);
	void closeVideo(AVFormatContext *oc, AVStream *st);
	
private:
	AudioEncoderData m_audioData;
	VideoEncoderData m_videoData;
	
	AVOutputFormat  *m_fmt;
	AVFormatContext *m_outputContext;
	AVStream *m_audioStream,
		 *m_videoStream;
	double  m_audioPts,
		m_videoPts;
		
	QString m_filename;
	
	double m_duration;
	int m_frameRate;
	int m_numFrames;
	
	SwsContext *m_imgConvertCtx;
	
	bool m_encodingStarted;
	bool m_encodingStopped;
	
	VideoSource *m_source;
	
	bool m_killed;
	
	QTime m_runtime;
	
	QSharedPointer<uchar> m_dataPtr;
	int m_byteCount;
	QTime m_captureTime;
	QTime m_starTime;
	int m_lastPts;
	QMutex m_dataLock;
};

#endif 
