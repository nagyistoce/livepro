#if !defined(Q_OS_WIN)

extern "C" {

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>          /* for videodev2.h */

#include <linux/videodev2.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))
}

#include "VideoFrame.h"
#include "SimpleV4L2.h"


static void errno_exit(const char *s)
{
	fprintf (stderr, "%s error %d, %s\nExiting application.",
		s, errno, strerror (errno));

	exit (EXIT_FAILURE);
}


static void errno_print(const char *s)
{
	fprintf (stderr, "%s error %d, %s\n",
		s, errno, strerror (errno));

	//exit (EXIT_FAILURE);
}


static int xioctl(int    fd,
                  int    request,
                  void * arg)
{
	int r;

	do r = ioctl (fd, request, arg);
	while (-1 == r && EINTR == errno);

	return r;
}


SimpleV4L2::SimpleV4L2()
{
	m_fd = -1;
	m_startedCapturing = false;
	m_deviceInited = false;
}

SimpleV4L2::~SimpleV4L2()
{
	if(m_fd > -1)
	{
		if(m_startedCapturing)
			stopCapturing();
		if(m_deviceInited)
			uninitDevice();
		closeDevice();
	}
}


VideoFrame *SimpleV4L2::readFrame()
{
	struct v4l2_buffer buf;
	unsigned int i;
	
	VideoFrame *frame = new VideoFrame();
	frame->setPixelFormat(QVideoFrame::Format_RGB32);
	frame->setCaptureTime(QTime::currentTime());
	frame->setBufferType(VideoFrame::BUFFER_POINTER);
	frame->setHoldTime(1000/30);
	frame->setSize(m_imageSize);
		
	if(!m_startedCapturing)
		return frame;
	
/*	struct timeval tv_start, tv_end;
	
	if (gettimeofday(&tv_start, 0) < 0) {
		errno_exit("Error getting time");
	}*/
	
	//QByteArray array;
		
	switch (m_io) {
	
	case IO_METHOD_READ:
		if (-1 == read (m_fd, m_buffers[0].start, m_buffers[0].length)) {
			switch (errno) {
			case EAGAIN:
				return frame;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit ("read");
			}
		}

		//array.append((char*)m_buffers[0].start, m_buffers[0].length);
		//frame->setByteArray(array);
		
		//frame->pointer = (uchar*)malloc(sizeof(uchar) * m_buffers[0].length);
		memcpy(frame->allocPointer(m_buffers[0].length), m_buffers[0].start, m_buffers[0].length);
	
		break;

	case IO_METHOD_MMAP:
		CLEAR (buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		if (-1 == xioctl (m_fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return frame;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit ("VIDIOC_DQBUF");
			}
		}

		assert (buf.index < m_numBuffers);

// 		array.append((char*)m_buffers[buf.index].start, m_buffers[buf.index].length);
// 		frame->setByteArray(array);

		//frame->pointer = (uchar*)malloc(sizeof(uchar) * m_buffers[buf.index].length);
		{
			uchar *pointer = frame->allocPointer(m_buffers[buf.index].length);
			//qDebug() << "SimpleV4L2::readFrame: Read "<<m_buffers[buf.index].length<<" bytes into pointer "<<pointer;
			memcpy(pointer, m_buffers[buf.index].start, m_buffers[buf.index].length);
		}
		
		
		if (-1 == xioctl (m_fd, VIDIOC_QBUF, &buf))
			errno_exit ("VIDIOC_QBUF");

		break;

	case IO_METHOD_USERPTR:
		CLEAR (buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;

		if (-1 == xioctl (m_fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return frame;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit ("VIDIOC_DQBUF");
			}
		}

		for (i = 0; i < m_numBuffers; ++i)
			if (buf.m.userptr == (unsigned long) m_buffers[i].start
			&& buf.length == m_buffers[i].length)
				break;

		assert (i < m_numBuffers);

		//process_image ((void *) buf.m.userptr);
// 		array.append((char*)buf.m.userptr, buf.length);
// 		frame->setByteArray(array);
		
// 		frame->pointer = (uchar*)malloc(sizeof(uchar) * buf.length);
// 		memcpy(frame->pointer, buf.m.userptr, buf.length);
		
		if (-1 == xioctl (m_fd, VIDIOC_QBUF, &buf))
			errno_exit ("VIDIOC_QBUF");

		break;
	}
	/*
	if (gettimeofday(&tv_end, 0) < 0) {
		errno_exit("Error getting time");
	}
	
	long int elapsed = tv_end.tv_usec - tv_start.tv_usec;
	printf("Elapsed: %d usec\n",elapsed);*/

	//return elapsed;
	return frame;
}


void SimpleV4L2::stopCapturing()
{
	enum v4l2_buf_type type;

	switch (m_io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl (m_fd, VIDIOC_STREAMOFF, &type))
			errno_exit ("VIDIOC_STREAMOFF");

		break;
	}
}

bool SimpleV4L2::startCapturing()
{
	unsigned int i;
	enum v4l2_buf_type type;

	m_startedCapturing = false;
	switch (m_io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < m_numBuffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR (buf);

			buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory      = V4L2_MEMORY_MMAP;
			buf.index       = i;

			if (-1 == xioctl (m_fd, VIDIOC_QBUF, &buf))
				errno_exit ("VIDIOC_QBUF");
		}
		
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl (m_fd, VIDIOC_STREAMON, &type))
		{
			errno_print ("VIDIOC_STREAMON");
			return false;
		}

		break;

	case IO_METHOD_USERPTR:
		for (i = 0; i < m_numBuffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR (buf);

			buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory      = V4L2_MEMORY_USERPTR;
			buf.index       = i;
			buf.m.userptr	= (unsigned long) m_buffers[i].start;
			buf.length      = m_buffers[i].length;

			if (-1 == xioctl (m_fd, VIDIOC_QBUF, &buf))
				errno_exit ("VIDIOC_QBUF");
		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl (m_fd, VIDIOC_STREAMON, &type))
		{
			errno_print ("VIDIOC_STREAMON");
			return false;
		}

		break;
	}
	
	m_startedCapturing = true;
	return true;
}

void SimpleV4L2::uninitDevice()
{
	unsigned int i;

	switch (m_io) {
	case IO_METHOD_READ:
		free (m_buffers[0].start);
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < m_numBuffers; ++i)
			if (-1 == munmap (m_buffers[i].start, m_buffers[i].length))
				errno_exit ("munmap");
		break;

	case IO_METHOD_USERPTR:
		for (i = 0; i < m_numBuffers; ++i)
			free (m_buffers[i].start);
		break;
	}

	free (m_buffers);
}

void SimpleV4L2::io_init_read(unsigned int buffer_size)
{
	m_buffers = (v4l2_simple_buffer*)calloc (1, sizeof (*m_buffers));

	if (!m_buffers) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}

	m_buffers[0].length = buffer_size;
	m_buffers[0].start = malloc (buffer_size);

	if (!m_buffers[0].start) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}
}

void SimpleV4L2::io_init_mmap()
{
	struct v4l2_requestbuffers req;

	CLEAR (req);

	req.count               = 2;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_MMAP;

	if (-1 == xioctl (m_fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s does not support memory mapping\n", m_devName);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		fprintf (stderr, "Insufficient buffer memory on %s\n", m_devName);
		exit (EXIT_FAILURE);
	}

	m_buffers = (v4l2_simple_buffer*)calloc (req.count, sizeof (*m_buffers));

	if (!m_buffers) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}

	for (m_numBuffers = 0; m_numBuffers < req.count; ++m_numBuffers) {
		struct v4l2_buffer buf;

		CLEAR (buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = m_numBuffers;

		if (-1 == xioctl (m_fd, VIDIOC_QUERYBUF, &buf))
			errno_exit ("VIDIOC_QUERYBUF");

		m_buffers[m_numBuffers].length = buf.length;
		m_buffers[m_numBuffers].start =
			mmap (NULL /* start anywhere */,
			buf.length,
			PROT_READ | PROT_WRITE /* required */,
			MAP_SHARED /* recommended */,
			m_fd, buf.m.offset);

		if (MAP_FAILED == m_buffers[m_numBuffers].start)
			errno_exit ("mmap");
	}
}

void SimpleV4L2::io_init_userp(unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;
	unsigned int page_size;

	page_size = getpagesize ();
	buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

	CLEAR (req);

	req.count               = 4;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl (m_fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s does not support user pointer i/o\n", m_devName);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_REQBUFS");
		}
	}

	m_buffers = (v4l2_simple_buffer*)calloc (4, sizeof (*m_buffers));

	if (!m_buffers) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}

	for (m_numBuffers = 0; m_numBuffers < 4; ++m_numBuffers) {
		m_buffers[m_numBuffers].length = buffer_size;
		m_buffers[m_numBuffers].start = memalign (/* boundary */ page_size,
						buffer_size);

		if (!m_buffers[m_numBuffers].start) {
			fprintf (stderr, "Out of memory\n");
			exit (EXIT_FAILURE);
		}
	}
}

void SimpleV4L2::initDevice()
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	unsigned int min;

	if (-1 == xioctl (m_fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s is not a V4L2 device\n", m_devName);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf (stderr, "%s is not a video capture device\n", m_devName);
		exit (EXIT_FAILURE);
	}

	switch (m_io) {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
			fprintf (stderr, "%s does not support read i/o\n", m_devName);
			exit (EXIT_FAILURE);
		}

		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			fprintf (stderr, "%s does not support streaming i/o\n", m_devName);
			exit (EXIT_FAILURE);
		}

		break;
	}


	/* Select video input, video standard and tune here. */


	CLEAR (cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl (m_fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		if (-1 == xioctl (m_fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
			case EINVAL:
				/* Cropping not supported. */
				break;
			default:
				/* Errors ignored. */
				break;
			}
		}
	} else {	
		/* Errors ignored. */
	}


	CLEAR (fmt);

	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = 640; 
	fmt.fmt.pix.height      = 480;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR32; //V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field       = V4L2_FIELD_NONE; //V4L2_FIELD_INTERLACED;

	if (-1 == xioctl (m_fd, VIDIOC_S_FMT, &fmt))
	{
		if(22 == errno) // Invalid Argument
		{
			fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
			if (-1 == xioctl (m_fd, VIDIOC_S_FMT, &fmt))
				errno_exit ("VIDIOC_S_FMT");
		}
	}

	/* Note VIDIOC_S_FMT may change width and height. */

	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	switch (m_io) {
	case IO_METHOD_READ:
		io_init_read (fmt.fmt.pix.sizeimage);
		break;

	case IO_METHOD_MMAP:
		io_init_mmap ();
		break;

	case IO_METHOD_USERPTR:
		io_init_userp (fmt.fmt.pix.sizeimage);
		break;
	}
	
	#ifdef DEBUG
	printf("SimpleV4L2::initDevice(): image size: %d x %d, bytes: %d\n",
		fmt.fmt.pix.width,fmt.fmt.pix.height,
		fmt.fmt.pix.sizeimage);
	#endif
		
	m_imageSize = QSize(fmt.fmt.pix.width,fmt.fmt.pix.height);
	
	m_deviceInited = true;
}

void SimpleV4L2::closeDevice()
{
	if (-1 == close (m_fd))
		errno_exit ("close");

	m_fd = -1;
}


bool SimpleV4L2::openDevice(const char *dev_name)
{
	struct stat st; 
	
	m_devName = dev_name;
	m_io = IO_METHOD_MMAP;
	m_fd = -1;
	m_buffers = NULL;
	m_numBuffers = 0;
	
	if (-1 == stat (m_devName, &st)) {
		fprintf (stderr, "Cannot identify '%s': %d, %s\n",
			 m_devName, errno, strerror (errno));
		return false;
	}

	if (!S_ISCHR (st.st_mode)) {
		fprintf (stderr, "%s is not a character device\n", m_devName);
		return false;
	}

	m_fd = open (m_devName, O_RDWR /* required */ | O_NONBLOCK, 0);

	if (-1 == m_fd) {
		fprintf (stderr, "Cannot open '%s': %d, %s\n",
			 m_devName, errno, strerror (errno));
		return false;
	}
	
	return true;
}



QStringList SimpleV4L2::inputs()
{
	if(!m_inputs.isEmpty())
		return m_inputs;
		
	QStringList inputs;
	// xioctl call to enum inputs
	// Recreate the inputs vector
        for (unsigned int iNumber=0; iNumber < 99; ++iNumber) 
        {
		// get input description
		struct v4l2_input videoInput;
		CLEAR(videoInput);
		videoInput.index = iNumber;
		if (xioctl(m_fd, VIDIOC_ENUMINPUT, &videoInput))
			break;
	
		// append new VideoInput
		//VideoInput input;
		inputs << QString::fromLocal8Bit((const char*)videoInput.name);
		#ifdef DEBUG
		qDebug() << "SimpleV4L2::inputs: Input " << iNumber << ": " << inputs.last();
		#endif
	}
	m_inputs = inputs;
	return inputs;
}
	

int SimpleV4L2::input()
{
	// return current input	
	int index = 0;
	if (-1 == xioctl(m_fd, VIDIOC_G_INPUT, &index)) {
		perror("VIDIOC_G_INPUT");
		return -1;
	}
	
	return index;
}
	
void SimpleV4L2::setInput(int index)
{
	// xioctl call to set input
	if (-1 == xioctl(m_fd, VIDIOC_S_INPUT, &index)) {
		perror ("VIDIOC_S_INPUT");
		//return false;
	}
}

bool SimpleV4L2::setInput(const QString& name)
{
	// Must match exactly one of the names from inputs()
	int index = inputs().indexOf(name);
	if(index >= 0)
	{
// 		qDebug() << "SimpleV4l2::setInput: Found"<<name<<" at index"<<index;
		setInput(index);
		return true;
	}
	
	return false;
}

/*
	typedef struct {
		QString name;
		v4l2_std_id id;
	} StandardInfo;
*/
	
QList<SimpleV4L2::StandardInfo> SimpleV4L2::standards()
{
	if(!m_standards.isEmpty())
		return m_standards;
		
	//enum standards
	QList<SimpleV4L2::StandardInfo> list;
	
	int index = input();
	if(index < 0)
		return list;
		
	// describe the current input
	struct v4l2_input input;
	CLEAR(input);
	input.index = index;
	if (-1 == xioctl(m_fd, VIDIOC_ENUMINPUT, &input)) 
	{
		perror("VIDIOC_ENUM_INPUT");
		return list;
	}
	//qWarning("VideoDevice::detectSignalStandards: input %d is '%s' std: 0x%x", index, input.name, (int)input.std);

	// skip standard matching if reported is unknown
	if (input.std != V4L2_STD_UNKNOWN) 
	{
		// match the input.std to all the device's standards
		struct v4l2_standard standard;
		CLEAR(standard);
		standard.index = 0;
		while (-1 != xioctl(m_fd, VIDIOC_ENUMSTD, &standard)) 
		{
			if (standard.id & input.std)
			{
				SimpleV4L2::StandardInfo info;
				info.name = QString::fromLocal8Bit((const char*)standard.name);
				info.id = standard.id;
				
				#ifdef DEBUG
				qDebug() << "SimpleV4L2::standards: Standard "<<info.name<<" supported";
				#endif
				list << info;
			}
			standard.index++;
		}

		// EINVAL indicates the end of the enumeration, which cannot be empty unless this device falls under the USB exception.
		if (errno != EINVAL || standard.index == 0) 
		{
			perror("VIDIOC_ENUMSTD");
			return list;
		}
	}
	
	m_standards = list;
	
	return list;
}

SimpleV4L2::StandardInfo SimpleV4L2::standard()
{
	// return current standard
	
	// xioctl call to query current standard
	
	SimpleV4L2::StandardInfo info;
	v4l2_std_id stdId;
	CLEAR(stdId);
	CLEAR(info);
	if (-1 == xioctl(m_fd, VIDIOC_G_STD, &stdId)) {
		perror("VIDIOC_G_STD");
		return info;
	}
	
	QList<SimpleV4L2::StandardInfo> stdList = standards();
	foreach(SimpleV4L2::StandardInfo item, stdList)
		if(item.id == stdId)
			return item;
	
	info.name = "(Unknown)";
	info.id = stdId;
	return info;
}

void SimpleV4L2::setStandard(SimpleV4L2::StandardInfo info)
{
	// call to set with info.id
	if (-1 == xioctl(m_fd, VIDIOC_S_STD, &info.id)) {
		perror ("VIDIOC_S_STD");
		//return false;
	}
}

bool SimpleV4L2::setStandard(const QString& name)
{
	 // must be in the list return by standards()
	QList<SimpleV4L2::StandardInfo> stdList = standards();
	foreach(SimpleV4L2::StandardInfo item, stdList)
		if(item.name == name)
		{
			setStandard(item);
			return true;
		}
		
	return false;
}

#endif
