#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>


#ifndef AV_PKT_FLAG_KEY
# define AV_PKT_FLAG_KEY   0x0001
# if LIBAVCODEC_VERSION_MAJOR < 53
#   define PKT_FLAG_KEY AV_PKT_FLAG_KEY
# endif
#endif


#include "VideoEncoder.h"

/**************************************************************/
/* audio output */

/*
 * add an audio output stream
 */
AVStream *VideoEncoder::addAudioStream(AVFormatContext *oc, enum CodecID codec_id)
{
	AVCodecContext *c;
	AVStream *st;

	st = av_new_stream(oc, 1);
	if (!st) {
		fprintf(stderr, "Could not alloc stream\n");
		exit(1);
	}

	c = st->codec;
	c->codec_id = codec_id;
	//c->codec_type = AVMEDIA_TYPE_AUDIO;
	c->codec_type = CODEC_TYPE_AUDIO;

	/* put sample parameters */
	c->sample_fmt = SAMPLE_FMT_S16;
	c->bit_rate    = 64000;
	c->sample_rate = 44100;
	c->channels    = 2;

	// some formats want stream headers to be separate
	if(oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;

	return st;
}

void VideoEncoder::openAudio(AVFormatContext */*oc*/, AVStream *st)
{
	AVCodecContext *c;
	AVCodec *codec;

	c = st->codec;

	/* find the audio encoder */
	codec = avcodec_find_encoder(c->codec_id);
	if (!codec)
	{
		fprintf(stderr, "Audio codec not found\n");
		exit(1);
	}

	/* open it */
	if (avcodec_open(c, codec) < 0)
	{
		fprintf(stderr, "Could not open audio codec\n");
		exit(1);
	}

	/* init signal generator */
	m_audioData.t = 0;
	m_audioData.tincr  = 2 * M_PI * 110.0 / c->sample_rate;
	/* increment frequency by 110 Hz per second */
	m_audioData.tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

	m_audioData.audio_outbuf_size = 10000;
	m_audioData.audio_outbuf = (uint8_t*)av_malloc(m_audioData.audio_outbuf_size);

	/* ugly hack for PCM codecs (will be removed ASAP with new PCM
	support to compute the input frame size in samples */
	if (c->frame_size <= 1)
	{
		m_audioData.audio_input_frame_size = m_audioData.audio_outbuf_size / c->channels;
		switch(st->codec->codec_id)
		{
			case CODEC_ID_PCM_S16LE:
			case CODEC_ID_PCM_S16BE:
			case CODEC_ID_PCM_U16LE:
			case CODEC_ID_PCM_U16BE:
				m_audioData.audio_input_frame_size >>= 1;
				break;
			default:
				break;
		}
	}
	else
	{
		m_audioData.audio_input_frame_size = c->frame_size;
	}

	m_audioData.samples = (int16_t*)av_malloc(m_audioData.audio_input_frame_size * 2 * c->channels);
}

/* prepare a 16 bit dummy audio frame of 'frame_size' samples and
   'nb_channels' channels */
void VideoEncoder::getDummyAudioFrame(int16_t */*samples*/, int frame_size, int nb_channels)
{
	int j, i, v;
	int16_t *q;

	q = m_audioData.samples;
	for(j=0;j<frame_size;j++)
	{
		v = (int)(sin(m_audioData.t) * 10000);
		for(i = 0; i < nb_channels; i++)
			*q++ = v;

		m_audioData.t     += m_audioData.tincr;
		m_audioData.tincr += m_audioData.tincr2;
	}
}

void VideoEncoder::writeAudioFrame(AVFormatContext *oc, AVStream *st)
{
	AVCodecContext *c;
	AVPacket pkt;
	av_init_packet(&pkt);

	c = st->codec;

	getDummyAudioFrame(m_audioData.samples, m_audioData.audio_input_frame_size, c->channels);

	pkt.size = avcodec_encode_audio(c, m_audioData.audio_outbuf, m_audioData.audio_outbuf_size, m_audioData.samples);

	if (c->coded_frame &&
	    c->coded_frame->pts != (int)AV_NOPTS_VALUE)
		pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, st->time_base);

	pkt.flags |= AV_PKT_FLAG_KEY;
	pkt.stream_index = st->index;
	pkt.data = m_audioData.audio_outbuf;

	/* write the compressed frame in the media file */
	if (av_interleaved_write_frame(oc, &pkt) != 0)
	{
		fprintf(stderr, "Error while writing audio frame\n");
		exit(1);
	}
}

void VideoEncoder::closeAudio(AVFormatContext */*oc*/, AVStream *st)
{
	avcodec_close(st->codec);

	av_free(m_audioData.samples);
	av_free(m_audioData.audio_outbuf);
}

/**************************************************************/
/* video output */

/* add a video output stream */
AVStream *VideoEncoder::addVideoStream(AVFormatContext *oc, enum CodecID codec_id)
{
	AVCodecContext *c;
	AVStream *st;

	st = av_new_stream(oc, 0);
	if (!st)
	{
		fprintf(stderr, "Could not alloc stream\n");
		exit(1);
	}

	c = st->codec;
	c->codec_id = codec_id;
	//c->codec_type = AVMEDIA_TYPE_VIDEO;
	c->codec_type = CODEC_TYPE_VIDEO;

	/* put sample parameters */
	c->bit_rate = 2000000;
	//c->bit_rate = 2000000;

	/* resolution must be a multiple of two */
	c->width  = 1024;
	c->height = 768;

	/* time base: this is the fundamental unit of time (in seconds) in terms
	   of which frame timestamps are represented. for fixed-fps content,
	   timebase should be 1/framerate and timestamp increments should be
	   identically 1. */
	c->time_base.den = m_frameRate;
	c->time_base.num = 1;
	c->gop_size = 12; /* emit one intra frame every twelve frames at most */
	c->pix_fmt = STREAM_PIX_FMT;

	if (c->codec_id == CODEC_ID_MPEG2VIDEO)
	{
		/* just for testing, we also add B frames */
		c->max_b_frames = 2;
	}
	if (c->codec_id == CODEC_ID_MPEG1VIDEO)
	{
		/* Needed to avoid using macroblocks in which some coeffs overflow.
		   This does not happen with normal video, it just happens here as
		   the motion of the chroma plane does not match the luma plane. */
		c->mb_decision=2;
	}
	// some formats want stream headers to be separate
	if(oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;

	return st;
}

AVFrame *VideoEncoder::allocPicture(enum PixelFormat pix_fmt, int width, int height)
{
	AVFrame *picture;
	uint8_t *picture_buf;
	int size;

	picture = avcodec_alloc_frame();
	if (!picture)
		return NULL;
	size = avpicture_get_size(pix_fmt, width, height);
	picture_buf = (uint8_t*)av_malloc(size *  sizeof(uint8_t));
	if (!picture_buf)
	{
		av_free(picture);
		qDebug() << "VideoEncoder::allocPicture: Error allocating a new avpicture, size:"<<width<<"x"<<height<<", pix_fmt:"<<pix_fmt<<" - probably out of memory.";
		return NULL;
	}
	avpicture_fill((AVPicture *)picture, picture_buf, pix_fmt, width, height);
	return picture;
}

void VideoEncoder::openVideo(AVFormatContext *oc, AVStream *st)
{
	AVCodec *codec;
	AVCodecContext *c;

	m_videoData.frame_count = 0;

	c = st->codec;

	/* find the video encoder */
	codec = avcodec_find_encoder(c->codec_id);
	if (!codec) {
		fprintf(stderr, "VideoEncoder::openVideo: codec not found\n");
		exit(1);
	}

	/* open the codec */
	if (avcodec_open(c, codec) < 0)
	{
		fprintf(stderr, "VideoEncoder::openVideo: could not open codec\n");
		exit(1);
	}

	m_videoData.video_outbuf = NULL;
	if (!(oc->oformat->flags & AVFMT_RAWPICTURE))
	{
		/* allocate output buffer */
		/* XXX: API change will be done */
		/* buffers passed into lav* can be allocated any way you prefer,
		   as long as they're aligned enough for the architecture, and
		   they're freed appropriately (such as using av_free for buffers
		   allocated with av_malloc) */
		m_videoData.video_outbuf_size = 200000;
		m_videoData.video_outbuf = (uint8_t*)av_malloc(m_videoData.video_outbuf_size);
	}

	/* allocate the encoded raw picture */
	m_videoData.picture = allocPicture(c->pix_fmt, c->width, c->height);
	if (!m_videoData.picture)
	{
		fprintf(stderr, "VideoEncoder::openVideo: Could not allocate picture\n");
		exit(1);
	}

	/* if the output format is not YUV420P, then a temporary YUV420P
	   picture is needed too. It is then converted to the required
	   output format */
	m_videoData.tmp_picture = NULL;
	if (c->pix_fmt != NATIVE_PIX_FMT)
	{
		m_videoData.tmp_picture = allocPicture(NATIVE_PIX_FMT, c->width, c->height);
		if (!m_videoData.tmp_picture)
		{
			fprintf(stderr, "VideoEncoder::openVideo: Could not allocate temporary picture\n");
			exit(1);
		}
	}
}

/* prepare a dummy image */
void VideoEncoder::fillDummyYuvImage(AVFrame *pict, int frame_index, int width, int height)
{
	int x, y, i;

	i = frame_index;

	/* Y */
	for(y=0;y<height;y++)
	{
		for(x=0;x<width;x++)
		{
			pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;
		}
	}

	/* Cb and Cr */
	for(y=0;y<height/2;y++)
	{
		for(x=0;x<width/2;x++)
		{
			pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
			pict->data[2][y * pict->linesize[2] + x] =  64 + x + i * 5;
		}
	}
}

void VideoEncoder::fillAvPicture(AVFrame *pict, int frame_index, int width, int height)
{
	// If no data pending, pull the frame from the video source in order to compensate
	// for idiotic sources like DVizSharedMEmoryThread which sometimes doesnt emit frameReady() signals
	if(!m_dataPtr || m_byteCount<=0)
		frameReady();

	if(!m_dataPtr || m_byteCount<=0)
		return;

	m_dataLock.lock();
	memcpy(pict->data[0], m_dataPtr.data(), m_byteCount);
	m_dataLock.unlock();
	
	m_byteCount =0;
}

void VideoEncoder::writeVideoFrame(AVFormatContext *oc, AVStream *st)
{
	int out_size, ret = 0;
	AVCodecContext *c;

	c = st->codec;

// 	if(m_videoData.frame_count >= m_numFrames)
// 	{
// 		/* no more frame to compress. The codec has a latency of a few
// 		   frames if using B frames, so we get the last frames by
// 		   passing the same picture again */
// 	}
// 	else
// 	{
		if (c->pix_fmt != NATIVE_PIX_FMT)
		{
			/* as we only generate a YUV420P picture, we must convert it
			to the codec pixel format if needed */
			if (m_imgConvertCtx == NULL)
			{
				m_imgConvertCtx = sws_getContext(c->width, c->height,
								NATIVE_PIX_FMT,
								c->width, c->height,
								c->pix_fmt,
								SWS_BICUBIC, NULL, NULL, NULL);
				if (m_imgConvertCtx == NULL)
				{
					fprintf(stderr, "Cannot initialize the conversion context\n");
					exit(1);
				}
			}

			//fillDummyYuvImage(m_videoData.tmp_picture, m_videoData.frame_count, c->width, c->height);
			fillAvPicture(m_videoData.tmp_picture, m_videoData.frame_count, c->width, c->height);

			sws_scale(m_imgConvertCtx, m_videoData.tmp_picture->data, m_videoData.tmp_picture->linesize,
				0, c->height, m_videoData.picture->data, m_videoData.picture->linesize);
		}
		else
		{
			//fillDummyYuvImage(m_videoData.picture, m_videoData.frame_count, c->width, c->height);
			fillAvPicture(m_videoData.picture, m_videoData.frame_count, c->width, c->height);
		}
	//}


	if (oc->oformat->flags & AVFMT_RAWPICTURE)
	{
		/* raw video case. The API will change slightly in the near
		futur for that */
		AVPacket pkt;
		av_init_packet(&pkt);

		pkt.flags |= AV_PKT_FLAG_KEY;
		pkt.stream_index = st->index;
		pkt.data = (uint8_t *)m_videoData.picture;
		pkt.size = sizeof(AVPicture);

		ret = av_interleaved_write_frame(oc, &pkt);
	}
	else
	{
		/* encode the image */
		out_size = avcodec_encode_video(c, m_videoData.video_outbuf, m_videoData.video_outbuf_size, m_videoData.picture);
		/* if zero size, it means the image was buffered */
		if (out_size > 0)
		{
			AVPacket pkt;
			av_init_packet(&pkt);

			if (c->coded_frame->pts != (int)AV_NOPTS_VALUE)
			{
				if(m_starTime.isNull())
					m_starTime = m_captureTime;
				int timecode = m_starTime.msecsTo(m_captureTime);
				
				int my_pts = (int)(timecode / 1000. * c->time_base.den);
				if(m_lastPts < 0)
					m_lastPts = my_pts;
				if(my_pts == m_lastPts)
					my_pts ++;
				if(my_pts < m_lastPts)
					my_pts = m_lastPts+1; 
				
				//int my_pts = (int)(m_runtime.elapsed() / 1000. * 30);
				pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, st->time_base);
				
				qDebug() << "VideoEncoder::writeFrame: pkt.pts:"<<pkt.pts<<", c->coded_frame->pts:"<<c->coded_frame->pts
					//<<", c->time_base:"<<c->time_base.num<<"/"<<c->time_base.den
					//<<", st->time_base:"<<st->time_base.num<<"/"<<st->time_base.den
					<<", my_pts:"<<my_pts
					;//<<", m_lastPts:"<<m_lastPts
					//<<", m_captureTime:"<<m_captureTime.second() * 1000 + m_captureTime.msec();
					
				pkt.pts   = my_pts;
				m_lastPts = my_pts;
			}
				
			if(c->coded_frame->key_frame)
				pkt.flags |= AV_PKT_FLAG_KEY;
			pkt.stream_index = st->index;
			pkt.data = m_videoData.video_outbuf;
			pkt.size = out_size;

			/* write the compressed frame in the media file */
			ret = av_interleaved_write_frame(oc, &pkt);
		}
		else
		{
			ret = 0;
		}
	}
	if (ret != 0)
	{
		fprintf(stderr, "Error while writing video frame\n");
		exit(1);
	}
	m_videoData.frame_count++;
}

void VideoEncoder::closeVideo(AVFormatContext */*oc*/, AVStream *st)
{
	avcodec_close(st->codec);
	av_free(m_videoData.picture->data[0]);
	av_free(m_videoData.picture);
	if (m_videoData.tmp_picture)
	{
		av_free(m_videoData.tmp_picture->data[0]);
		av_free(m_videoData.tmp_picture);
	}
	av_free(m_videoData.video_outbuf);
}

/**************************************************************/
/* media file output */

VideoEncoder::VideoEncoder(const QString&file, QObject *parent)
	: QThread(parent)
	, m_filename(file)
	, m_imgConvertCtx(0)
	, m_encodingStarted(false)
	, m_encodingStopped(true)
	, m_source(0)
	, m_killed(false)
	, m_byteCount(0)
{
	//qDebug() << "VideoEncoder: Constructing with filename:"<<m_filename;
	/* initialize libavcodec, and register all codecs and formats */
	av_register_all();

	//setupEncoder();
}

VideoEncoder::~VideoEncoder()
{
	// Calls disconnectVideoSource(), which calls stopEncoder()
	setVideoSource(0);
}


void VideoEncoder::setVideoSource(VideoSource *source)
{
	if(m_source == source)
		return;

	if(m_source)
		disconnectVideoSource();

	m_source = source;

	//m_source->setIsBuffered(true);

	if(m_source)
	{
		connect(m_source, SIGNAL(frameReady()), this, SLOT(frameReady()));
		connect(m_source, SIGNAL(destroyed()), this, SLOT(disconnectVideoSource()));

		//qDebug() << "GLVideoDrawable::setVideoSource(): "<<objectName()<<" m_source:"<<m_source;
		//setVideoFormat(m_source->videoFormat());

		frameReady();
	}
	else
	{
		qDebug() << "VideoEncoder::setVideoSource(): "<<this<<" Source is NULL";
	}

}

void VideoEncoder::disconnectVideoSource()
{
	if(!m_source)
		return;
	disconnect(m_source, 0, this, 0);
	m_source = 0;

	stopEncoder();
}

bool VideoEncoder::startEncoder(const QString& filename, double duration, int frameRate)
{
	if(m_encodingStarted)
	{
		qDebug() << "VideoEncoder::startEncoder: Encoding already started, writing to:"<<filename<<". Call stopEncoder() before calling startEncoder() again.";
		return false;
	}
	m_encodingStarted = false;

	//qDebug() << "VideoEncoder::startEncoder: Given filename:"<<filename<<", have:"<<m_fileName;
	if(!filename.isEmpty())
		m_filename  = filename;

	if(m_filename.isEmpty())
	{
		qDebug() << "VideoEncoder::startEncoder: No filename given, cannot start encoder";
		return false;
	}

	frameRate = 30;

	m_duration  = duration;
	m_frameRate = frameRate;
	m_numFrames = ((int)(m_duration * m_frameRate));
	
	m_lastPts   = -1;

	start();

	// TODO Will this work? The idea is to move
	// the scaling done in frameReady() into the
	// encoder thread as opposed to the source thread.
	moveToThread(this);

	return true;
}



void VideoEncoder::frameReady()
{
	QTime t;
	t.start();	
	if(!m_source)
		return;

	if(!m_encodingStarted)
		return;

	VideoFramePtr frame = m_source->frame();
	if(!frame)
		return;

	QImage img;
	if(!frame->image().isNull())
	{
		img = frame->image();
		//qDebug() << "VideoEncoder::fillAvPicture: Located from VideoFrame/QImage data";
	}
	else
	{
		const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(frame->pixelFormat());
		if(imageFormat != QImage::Format_Invalid)
		{
			img = QImage(frame->pointer(),
				frame->size().width(),
				frame->size().height(),
				frame->size().width() *
					(imageFormat == QImage::Format_RGB16  ||
					 imageFormat == QImage::Format_RGB555 ||
					 imageFormat == QImage::Format_RGB444 ||
					 imageFormat == QImage::Format_ARGB4444_Premultiplied ? 2 :
					 imageFormat == QImage::Format_RGB888 ||
					 imageFormat == QImage::Format_RGB666 ||
					 imageFormat == QImage::Format_ARGB6666_Premultiplied ? 3 :
					 4),
				imageFormat);

			//qDebug() << "VideoEncoder::fillAvPicture: Loaded from raw VideoFrame data";
		}
		else
		{
			qDebug() << "VideoEncoder::frameReady: Unable to convert pixel format to a valid image format, cannot scale frame. Pixel Format:"<<frame->pixelFormat();
		}
	}


	if(img.format() != QImage::Format_ARGB32)
	{
		//qDebug() << "VideoEncoder::frameReady: Converting to QImage::Format_ARGB32";
		img = img.convertToFormat(QImage::Format_ARGB32);
	}

	AVCodecContext *c = m_videoStream->codec;

	if(img.size() != QSize(c->width,c->height))
	{
		//qDebug() << "VideoEncoder::frameReady: Scaling from:"<<img.size()<<" to "<<c->width<<"x"<<c->height;
		img = img.scaled(c->width,c->height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	}

	uchar *ptr = (uchar*)malloc(sizeof(uchar) * img.byteCount());
	const uchar *src = (const uchar*)img.bits();
	memcpy(ptr, src, img.byteCount());

	m_dataLock.lock();

	m_dataPtr = QSharedPointer<uchar>(ptr);
	m_byteCount = img.byteCount();
	m_captureTime = frame->captureTime();
	
	m_dataLock.unlock();
	
	//qDebug() << "VideoEncoder::frameReady(): Run time:"<<t.elapsed();

}

bool VideoEncoder::setupInternal()
{
	m_encodingStarted = true;
	m_audioPts = 0.0;
	m_videoPts = 0.0;

	/* auto detect the output format from the name. default is mpeg. */
	m_fmt = guess_format(NULL, qPrintable(m_filename), NULL);
	if (!m_fmt)
	{
		printf("Could not deduce output format from file extension: using MPEG.\n");
		m_fmt = guess_format("mpeg", NULL, NULL);
	}
	if (!m_fmt)
	{
		fprintf(stderr, "VideoEncoder::startEncoder: Could not find suitable output format\n");
		//xit(1);
		return false;
	}

	/* allocate the output media context */
	m_outputContext = 0;
	#ifndef Q_OS_WIN32
	// TODO right now, I can't find avformat_alloc_context on the ffmpeg copy I have in svn for building on windows, so VideoEncoder won't work on Win32 right now
		m_outputContext = avformat_alloc_context();
	#endif
	if (!m_outputContext)
	{
		fprintf(stderr, "VideoEncoder::startEncoder: Memory error allocating avformat context\n");
		//exit(1);
		return false;
	}
	m_outputContext->oformat = m_fmt;
	snprintf(m_outputContext->filename, sizeof(m_outputContext->filename), "%s", qPrintable(m_filename));

	/* add the audio and video streams using the default format codecs
	and initialize the codecs */
	m_videoStream = NULL;
	m_audioStream = NULL;
	if (m_fmt->video_codec != CODEC_ID_NONE)
	{
		m_videoStream = addVideoStream(m_outputContext, m_fmt->video_codec);
	}
	else
	{
		printf("VideoEncoder::startEncoder: Video NOT opened.\n");
		//exit(1);
		return false;
	}
	if (m_fmt->audio_codec != CODEC_ID_NONE)
	{
		m_audioStream = addAudioStream(m_outputContext, m_fmt->audio_codec);
	}
	else
	{
		printf("VideoEncoder::startEncoder: Audio NOT opened.\n");
		//exit(1);
		return false;
	}

	/* set the output parameters (must be done even if no
	   parameters). */
	if (av_set_parameters(m_outputContext, NULL) < 0)
	{
		fprintf(stderr, "VideoEncoder::startEncoder: Invalid output format parameters\n");
		//exit(1);
		return false;
	}

	dump_format(m_outputContext, 0, qPrintable(m_filename), 1);

	/* now that all the parameters are set, we can open the audio and
	video codecs and allocate the necessary encode buffers */
	if (m_videoStream)
		openVideo(m_outputContext, m_videoStream);
	if (m_audioStream)
		openAudio(m_outputContext, m_audioStream);

	/* open the output file, if needed */
	if (!(m_fmt->flags & AVFMT_NOFILE))
	{
		if (url_fopen(&m_outputContext->pb, qPrintable(m_filename), URL_WRONLY) < 0)
		{
			fprintf(stderr, "VideoEncoder::startEncoder: Could not open '%s' for writing\n", qPrintable(m_filename));
			//exit(1);
			return false;
		}
	}

	/* write the stream header, if any */
	av_write_header(m_outputContext);

	m_encodingStarted = true;
	m_encodingStopped = false;

	return true;
}

void VideoEncoder::run()
{
	if(!setupInternal())
	{
		qDebug() << "VideoEncoder::run(): Error doing internal setup. Not starting encoder.";
		return;
	}

	m_runtime.start();

	m_killed = false;
	while(!m_killed)
	{
		/* compute current audio and video time */
		if (m_audioStream)
			m_audioPts = (double)m_audioStream->pts.val * m_audioStream->time_base.num / m_audioStream->time_base.den;
		else
			m_audioPts = 0.0;

		if (m_videoStream)
			m_videoPts = (double)m_videoStream->pts.val * m_videoStream->time_base.num / m_videoStream->time_base.den;
		else
			m_videoPts = 0.0;

		if ((!m_audioStream) &&// || m_audioPts >= m_duration) &&
		    (!m_videoStream))// || m_videoPts >= m_duration))
		{
			qDebug() << "VideoEncoder::run(): Streams dont exist, stopping.";
			m_killed = true;
			//stopEncoder();
		}
		else
		{
			//printf("VideoEncoder::run(): Video PTS: %f | Audio PTS: %f | Runtime: %.02f\n",m_videoPts,m_audioPts,m_runtime.elapsed() / 1000.);

// 			/* write interleaved audio and video frames */
// 			if (!m_videoStream || (m_videoStream && m_audioStream && m_audioPts < m_videoPts))
// 			{
// 				writeAudioFrame(m_outputContext, m_audioStream);
// 				//printf(" + AudioFrame %f < %f\n",m_audioPts,m_videoPts);
// 			}
// 			else
// 			{
				writeVideoFrame(m_outputContext, m_videoStream);
				//printf(" + VideoFrame %f < %f\n",m_videoPts,m_audioPts);
//		 	}
		}

// 		double diff = m_videoPts - (m_runtime.elapsed() /1000.);
// 		if(diff > 0 && diff <= 30) // arbitrary max
// 		{
// 			//diff *= .75;
// 			diff *= 1000;
// 			qDebug() << "VideoEncoder::run(): Sleeping for "<<diff<<"ms";
// 			msleep((int)diff);
// 		}
// 		else
// 			qDebug() << "VideoEncoder::run(): Not sleeping, diff is "<<diff<<"ms";
// 			//msleep(1000 / m_frameRate * .75);
		msleep(40);
		//yieldCurrentThread();
	}

	teardownInternal();
}

void VideoEncoder::stopEncoder()
{
	if(m_encodingStopped)
		return;

	// Wait for the encoder to stop
	m_killed = true;
	quit();
	wait();
}

void VideoEncoder::teardownInternal()
{

	qDebug() << "VideoEncoder::teardownInternal(): Teardown starting...";

	/* write the trailer, if any.  the trailer must be written
	* before you close the CodecContexts open when you wrote the
	* header; otherwise write_trailer may try to use memory that
	* was freed on av_codec_close() */
	av_write_trailer(m_outputContext);

	qDebug() << "VideoEncoder::teardownInternal(): Closing streams...";

	/* close each codec */
	if (m_videoStream)
		closeVideo(m_outputContext, m_videoStream);
	if (m_audioStream)
		closeAudio(m_outputContext, m_audioStream);

	qDebug() << "VideoEncoder::teardownInternal(): Freeing streams...";

	/* free the streams */
	for(uint i = 0; i < m_outputContext->nb_streams; i++)
	{
		av_freep(&m_outputContext->streams[i]->codec);
		av_freep(&m_outputContext->streams[i]);
	}

	qDebug() << "VideoEncoder::teardownInternal(): Closing the output file...";

	if (!(m_fmt->flags & AVFMT_NOFILE))
	{
		/* close the output file */
		url_fclose(m_outputContext->pb);
	}

	qDebug() << "VideoEncoder::teardownInternal(): Closing the output context...";

	/* free the stream */
	av_free(m_outputContext);

	qDebug() << "VideoEncoder::teardownInternal(): Teardown complete";

	m_encodingStopped = true;
	m_encodingStarted = false;
}
