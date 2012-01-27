#include "V4LOutput.h"

#include "VideoSource.h"
	
// 	#define V4L_NATIVE_FORMAT VIDEO_PALETTE_RGB32
// 	#define V4L_QIMAGE_FORMAT QImage::Format_RGB32
	
	#define V4L_NATIVE_FORMAT VIDEO_PALETTE_RGB24
	#define V4L_QIMAGE_FORMAT QImage::Format_RGB888
	
// 	#define V4L_NATIVE_FORMAT VIDEO_PALETTE_RGB565
// 	#define V4L_QIMAGE_FORMAT QImage::Format_RGB16

// 	#define V4L_NATIVE_FORMAT VIDEO_PALETTE_RGB555
// 	#define V4L_QIMAGE_FORMAT QImage::Format_RGB555
	
	
extern "C" {
	#include <unistd.h>
	#include <stdlib.h>
	#include <stdio.h>
	#include <fcntl.h>
	#include <string.h>
	#include <errno.h>
	#include <sys/ioctl.h>
	#include <sys/mman.h>
	#include <signal.h>
	#include <sys/wait.h>
	#include <linux/videodev.h>
}

// // From: http://nehe.gamedev.net/tutorial/playing_avi_files_in_opengl/23001/
// Doesn't work yet under gcc - I don't know enough assembler to make it work!
// void flipRB640x480(void* buffer)                // Flips The Red And Blue Bytes (640x480)
// {
// 	void* b = buffer;                       // Pointer To The Buffer
// 	__asm__(                                // Assembler Code To Follow
// 		"mov %%ecx, 640*480       \n"     // Set Up A Counter (Dimensions Of Memory Block)
// 		"mov %%ebx, %0            \n"     // Points ebx To Our Data (b)
// 		"label:                 \n"     // Label Used For Looping
// 			"mov %%al,%%ebx+0 \n"     // Loads Value At ebx Into al
// 			"mov %%ah,%%ebx+2 \n"     // Loads Value At ebx+2 Into ah
// 			"mov %%ebx+2,%%al \n"     // Stores Value In al At ebx+2
// 			"mov %%ebx+0,%%ah \n"     // Stores Value In ah At ebx
// 			"add %%ebx,3      \n"     // Moves Through The Data By 3 Bytes
// 			"dec %%ecx        \n"     // Decreases Our Loop Counter
// 			"jnz label      \n"     // If Not Zero Jump Back To Label
// 		: "=r" (b)
// 		: "r" (b)
// 		: "%al", "%ah", "%ecx", "%ebx"
// 	);
// }

// The folllowing swap methods are from: http://stackoverflow.com/questions/6804101/fast-method-to-copy-memory-with-translation-argb-to-bgr
// Can't get them to work yet.

typedef unsigned char UInt8;

//typedef struct{ UInt8 Alpha; UInt8 Red; UInt8 Green; UInt8 Blue; } ARGB;
typedef struct{ UInt8 Red; UInt8 Green; UInt8 Blue; } RGB;
typedef struct{ UInt8 Blue; UInt8 Green; UInt8 Red; } BGR;

/*
void swap1(ARGB *orig, BGR *dest, unsigned imageSize) {
    unsigned x;
    for(x = 0; x < imageSize; x++) {
        *((uint32_t*)(((uint8_t*)dest)+x*3)) = OSSwapInt32(((uint32_t*)orig)[x]);
    }
}*/


void swap2(uchar *orig, uchar *dest, unsigned imageSize) {
    asm (
        "0:\n\t"
        "movl   (%1),%%eax\n\t"
        "bswapl %%eax\n\t"
        "movl   %%eax,(%0)\n\t"
        "addl   $4,%1\n\t"
        "addl   $3,%0\n\t"
        "decl   %2\n\t"
        "jnz    0b"
        :: "D" (dest), "S" (orig), "c" (imageSize)
        : "flags", "eax"
    );
}

/*
typedef uint8_t v16qi __attribute__ ((vector_size (16)));
void swap3(uint8_t *orig, uint8_t *dest, size_t imagesize) {
    v16qi mask = __builtin_ia32_lddqu((const char[]){3,2,1,7,6,5,11,10,9,15,14,13,0xFF,0xFF,0xFF,0XFF});
    uint8_t *end = orig + imagesize * 4;
    for (; orig != end; orig += 16, dest += 12) {
        __builtin_ia32_storedqu(dest,__builtin_ia32_pshufb128(__builtin_ia32_lddqu(orig),mask));
    }
}*/

// void swap2_2(uint8_t *orig, uint8_t *dest, size_t imagesize) {
//     int8_t mask[16] = {3,2,1,7,6,5,11,10,9,15,14,13,0xFF,0xFF,0xFF,0XFF};//{0xFF, 0xFF, 0xFF, 0xFF, 13, 14, 15, 9, 10, 11, 5, 6, 7, 1, 2, 3};
//     asm (
//         "lddqu  (%3),%%xmm1\n\t"
//         "0:\n\t"
//         "lddqu  (%1),%%xmm0\n\t"
//         "pshufb %%xmm1,%%xmm0\n\t"
//         "movdqu %%xmm0,(%0)\n\t"
//         "add    $16,%1\n\t"
//         "add    $12,%0\n\t"
//         "sub    $4,%2\n\t"
//         "jnz    0b"
//         :: "r" (dest), "r" (orig), "r" (imagesize), "r" (mask)
//         : "flags", "xmm0", "xmm1"
//     );
// }


int V4LOutput::startPipe (int dev, int width, int height)
{
	struct video_capability vid_caps;
	struct video_window vid_win;
	struct video_picture vid_pic;

	if (ioctl(dev, VIDIOCGCAP, &vid_caps) == -1) {
		printf ("ioctl (VIDIOCGCAP)\nError[%s]\n", strerror(errno));
		return (1);
	}

	if (ioctl(dev, VIDIOCGPICT, &vid_pic)== -1) {
		printf ("ioctl VIDIOCGPICT\nError[%s]\n", strerror(errno));
		return (1);
	}

	//vid_pic.palette = VIDEO_PALETTE_RGB555;
	//vid_pic.palette = VIDEO_PALETTE_RGB24;
	vid_pic.palette = V4L_NATIVE_FORMAT;
	
	
	if (ioctl(dev, VIDIOCSPICT, &vid_pic) == -1) {
		printf ("ioctl VIDIOCSPICT\nError[%s]\n", strerror(errno));
		return (1);
	}

	if (ioctl (dev, VIDIOCGWIN, &vid_win) == -1) {
		printf ("ioctl VIDIOCGWIN\nError[%s]\n", strerror(errno));
		return (1);
	}

	vid_win.width  = width;
	vid_win.height = height;

	if (ioctl(dev, VIDIOCSWIN, &vid_win)== -1) {
		printf ("ioctl VIDIOCSWIN\nError[%s]\n", strerror(errno));
		return (1);
	}
	return 0;
}


V4LOutput::V4LOutput(QString key, QObject *parent)
	: QThread(parent)
	, m_v4lOutputDev(-1)
	, m_v4lOutputName(key)
	, m_frameReady(false)
	, m_started(false)
{
	m_source = 0;
	start();
}


void V4LOutput::setupOutput()
{
	if(m_v4lOutputDev >= 0)
	{
		qDebug() << "V4LOutput: Closing existing output stream"<<m_v4lOutputDev<<" due to resize";
		close (m_v4lOutputDev);
	}
	
	m_v4lOutputDev = open (qPrintable(m_v4lOutputName), O_RDWR);
	if (m_v4lOutputDev < 0)
	{ 
		qDebug() << "V4LOutput: Failed to open output video device "<<m_v4lOutputName<<", error: "<<strerror(errno);
	}
	else
	{
		startPipe(m_v4lOutputDev, m_outputSize.width(), m_outputSize.height());
		qDebug() << "V4LOutput: Opened V4L output to filenum "<<m_v4lOutputDev<<", size:"<<m_outputSize; 
	}
}

V4LOutput::~V4LOutput()
{
	setVideoSource(0);
	if(!m_killed)
	{
		m_killed = true;
		quit();
		wait();
	}
}
	
void V4LOutput::setVideoSource(VideoSource *source)
{
	if(m_source == source)
		return;
		
	if(m_source)
		disconnectVideoSource();
	
	m_source = source;
	if(m_source)
	{	
		connect(m_source, SIGNAL(frameReady()), this, SLOT(frameReady()));
		connect(m_source, SIGNAL(destroyed()), this, SLOT(disconnectVideoSource()));
		
		//qDebug() << "GLVideoDrawable::setVideoSource(): "<<objectName()<<" m_source:"<<m_source;
		//setVideoFormat(m_source->videoFormat());
		m_source->registerConsumer(this);
		
		frameReady();
	}
	else
	{
		qDebug() << "V4LOutput::setVideoSource(): "<<this<<" Source is NULL";
	}

}

void V4LOutput::disconnectVideoSource()
{
	if(!m_source)
		return;
	disconnect(m_source, 0, this, 0);
	m_source->release(this);
	m_source = 0;
}


void V4LOutput::frameReady()
{
	if(!m_source)
		return;
		
	m_frameReady = true;
	
// 	if(!m_started)
// 		getFrame();
}

void V4LOutput::getFrame()
{
	VideoFramePtr frame = m_source->frame();
	if(!frame || !frame->isValid())
	{
		//qDebug() << "V4LOutput::frameReady(): Invalid frame or no frame";
		return;
	}
	
	m_frame = frame;
	
	if(m_frame->size() != m_outputSize)
	{
		m_outputSize = m_frame->size();
		setupOutput();
	}
	
	//if(m_transmitFps <= 0)
		processFrame();
}

void V4LOutput::run()
{
	m_started = true;
	while(!m_killed)
	{
		if(m_frameReady)
			getFrame();
		
		msleep(1);
	}
}



void V4LOutput::processFrame()
{
	//qDebug() << "V4LOutput::frameReady: Downscaling video for transmission to "<<m_transmitSize;
	// To scale the video frame, first we must convert it to a QImage if its not already an image.
	// If we're lucky, it already is. Otherwise, we have to jump thru hoops to convert the byte 
	// array to a QImage then scale it.
	QImage image;
	if(!m_frame->image().isNull())
	{
		image = m_frame->image();
	}
	else
	{
		const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(m_frame->pixelFormat());
		
		if(imageFormat != QImage::Format_Invalid)
		{
			image = QImage(m_frame->pointer(),
				m_frame->size().width(),
				m_frame->size().height(),
				m_frame->size().width() *
					(imageFormat == QImage::Format_RGB16  ||
					 imageFormat == QImage::Format_RGB555 ||
					 imageFormat == QImage::Format_RGB444 ||
					 imageFormat == QImage::Format_ARGB4444_Premultiplied ? 2 :
					 imageFormat == QImage::Format_RGB888 ||
					 imageFormat == QImage::Format_RGB666 ||
					 imageFormat == QImage::Format_ARGB6666_Premultiplied ? 3 :
					 4),
				imageFormat);
				
			image = image.copy();
			//qDebug() << "Downscaled image from "<<image.byteCount()<<"bytes to "<<scaledImage.byteCount()<<"bytes, orig ptr len:"<<m_frame->pointerLength()<<", orig ptr:"<<m_frame->pointer();
		}
		else
		{
			qDebug() << "VideoFilter::frameImage: Unable to convert pixel format to image format, cannot scale frame. Pixel Format:"<<m_frame->pixelFormat();
		}
	}
	
// 	if(image.width()  != FRAME_WIDTH ||
// 	   image.height() != FRAME_HEIGHT)
// 	{
// 		image = image.scaled(FRAME_WIDTH,FRAME_HEIGHT,Qt::IgnoreAspectRatio);
// 	}
	
	if(image.format() != V4L_QIMAGE_FORMAT)
		image = image.convertToFormat(V4L_QIMAGE_FORMAT);
		
	// QImage and V4L's def of RGB24 differ in that they have the R & B bytes swapped compared to each other 
	// - so we swap them here to appear correct in output. 
	if(V4L_NATIVE_FORMAT == VIDEO_PALETTE_RGB24 &&
	   V4L_QIMAGE_FORMAT == QImage::Format_RGB888)
	{
		
		
		uchar  tmp;
		uchar *bits = image.scanLine(0);
		for(int i=0; i<image.byteCount(); i+=3)
		{
			tmp       = bits[i];
			bits[i]   = bits[i+2];
			bits[i+2] = tmp;
		}
		
//		flipRB640x480(image.scanLine(0));
// 		QImage image2(image.size(), image.format());
// 		swap2(image.scanLine(0),image2.scanLine(0),image.byteCount());
// 		image = image2;
		
	}
	
	if(write(m_v4lOutputDev, (const uchar*)image.bits(), image.byteCount()) != image.byteCount()) 
	{
		qDebug() << "V4LOutput::readFrame(): Error writing to "<<m_v4lOutputName<<" (bytes written does not match bytes requested), V4L error: " << strerror(errno); 
	}
	else
	{
		//qDebug() << "DVizSharedMemoryThread::readFrame(): Wrote "<<outImg.byteCount()<<" bytes to "<<V4L_OUTPUT;
	} 
	
}

