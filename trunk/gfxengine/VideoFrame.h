#ifndef VideoFrame_H
#define VideoFrame_H

#include <QImage>
#include <QTime>
#include <QDebug>
#include <QVideoFrame>
#include <QQueue>
#include <QMutex>
#include <QObject>
#include <QSharedPointer>

#ifndef GLuint
typedef unsigned int GLuint;  /* 4-byte unsigned */
#endif

//#define DEBUG_VIDEOFRAME_POINTERS

/// \class VideoFrame
/// A simple representation of a single video frame.
/// This class is used instead of QVideoFrame due to the simplicity of this class and the self-contained nature of the image data storage in comparrision to QVideoFrame's storage mechanism.
/// The main component of the VideoFrame is the image data: Image data is stored as either a raw buffer of bytes (uchar) or a QImage object.
/// The bufferType() method gives you information on where to look for the image data.
/// Regardless of bufferType(), pixelFormat() should return a a valid pixel format for this video frame.
class VideoFrame //: public QObject
{
// 	//Q_OBJECT
public:
	VideoFrame(); 
	
	VideoFrame(int holdTime, const QTime &captureTime = QTime());
	
	VideoFrame(const QImage &frame, int holdTime, const QTime &captureTime = QTime());
/*	
QVideoFrame::Format_ARGB32	1	The frame is stored using a 32-bit ARGB format (0xAARRGGBB). This is equivalent to QImage::Format_ARGB32.
QVideoFrame::Format_ARGB32_Premultiplied	2	The frame stored using a premultiplied 32-bit ARGB format (0xAARRGGBB). This is equivalent to QImage::Format_ARGB32_Premultiplied.
QVideoFrame::Format_RGB32	3	The frame stored using a 32-bit RGB format (0xffRRGGBB). This is equivalent to QImage::Format_RGB32
QVideoFrame::Format_RGB24	4	The frame is stored using a 24-bit RGB format (8-8-8). This is equivalent to QImage::Format_RGB888
QVideoFrame::Format_RGB565	5	The frame is stored using a 16-bit RGB format (5-6-5). This is equivalent to QImage::Format_RGB16.
QVideoFrame::Format_RGB555	6	The frame is stored using a 16-bit RGB format (5-5-5). This is equivalent to QImage::Format_RGB555.*/
	
	VideoFrame(VideoFrame* other);
	
	~VideoFrame();
	
	/// Returns true if bufferType() == BUFFER_INVALID
	bool isEmpty();
	/// Returns true if bufferType() != BUFFER_INVALID, AND: 
	/// - if the buffer is a QImage buffer, then QImage::isNull() must be false
	/// - if the buffer is a BUFFER_POINTER, then pointer() must not be NULL 
	bool isValid();
	
	/// Returns the approx memory consumption of this frame.
	/// 'approx' because it isn't perfect, especially when using
	/// BUFFER_POINTER bufferType's. 
	int byteSize() const;
	
	/// Returns the recommended 'hold time' for this frame.
	/// Hold time is given in milleseconds, and set by the object that creates this frame.
	/// It is purely advisory and currently not used by any known VideoConsumer except for VideoWidget. 
	int holdTime() { return m_holdTime; }
	/// Set the value for holdTime()
	void setHoldTime(int holdTime);
	
	/// Returns the time that this frame was created. Primarily relevant for latency-critical paths
	// such as live video capture, or for live feed monitoring such as in the Director live monitor.
	QTime captureTime() { return m_captureTime; }
	/// Sets the captureTime() of this frame - typically set using setCaptureTime(QTime::currentTime());
	void setCaptureTime(const QTime& time);
	
	/// Returns the QVideoFrame::PixelFormat for this frame. 
	QVideoFrame::PixelFormat pixelFormat() { return m_pixelFormat; }
	/// Set the pixelFormat() for this frame. Note that setImage() will attempt to set the format automatically based on the QImage::format()
	void setPixelFormat(QVideoFrame::PixelFormat format);
	
	/// Enum VideoFrameBufferType:
	//	- BUFFER_INVALID	- Frame is likely uninitalized
	//	- BUFFER_IMAGE		- The image data is in image() 
	//	- BUFFER_POINTER	- The image data is in pointer()
	enum BufferType
	{
		BUFFER_INVALID = 0,
		BUFFER_IMAGE,
		BUFFER_POINTER,
	};
	
	/// Returns the BufferType of this frame
	BufferType bufferType() { return m_bufferType; }
	/// Set the bufferType() to \a type. Note that setImage() and allocPointer() and setPointer() all set the buffer type correctly.
	void setBufferType(BufferType type);
	
	/// Returns the QImage assocated with this frame, if any.
	QImage image() { return m_image; }
	/// Sets the image() for this frame, updates bufferType(), calls setSize() and sets pixelFormat() accordingly. If the QImage does not match one of the
	/// supported Pixel Formats, then it is converted automatically to QImage::Format_ARGB32 internally. Note that this will significantly 
	/// slow down the system, so it is in your best interest to keep the images you provide within one of the following formats:
	/// - QImage::Format_ARGB32
	/// - QImage::Format_RGB32
	/// - QImage::Format_RGB888
	/// - QImage::Format_RGB16
	/// - QImage::Format_RGB555
	/// Any other image format will be converted to QImage::Format_ARGB32.
	/// Note that QVideoDrawable does infact support QImage::Format_RGB555 which reduces the memory consumption by half (thats 50%) giving you a huge gain in bandwidth
	/// if you can provide RGB555 images right at the start, such as for video capture, etc.
	void setImage(QImage image);
	
	/// Convenience function - returns \em true if bufferType() is BUFFER_POINTER
	bool isRaw() { return bufferType() == BUFFER_POINTER; }
	 
	/// Returns a pointer to the stored image data, if any. Note that there is no detach or copy operation performed - this is a direct pointer to the raw data. 
	uchar *pointer() { return m_pointer; }
	/// Returns the given pointer length in bytes set either by setPointer() or allocPointer()
	int pointerLength() { return m_pointerLength; }
	/// Give a pointer to a block of memory to this VideoFrame. Note that VideoFrame takes ownership of the pointer and will call free() on the pointer when the VideoFrame is deleted.
	void setPointer(uchar *pointer, int length);
	/// Allocate a pointer of the given number of \a bytes - sets bufferType() and pointerLength() accordingly.
	uchar *allocPointer(int bytes);
	
	/// Returns the size in pixels of the video frame.
	QSize size() { return m_size; }
	/// Set the size in pixels of the video frame. Note that this is set correctly by setImage(). Also sets the rect() for this frame.
	void setSize(QSize size);
	
	/// Returns the QRect for this frame. Set by setRect() or setSize() 
	QRect rect() { return m_rect; }
	/// Set the QRect of this frame. Set by setSize() or setRect()
	void setRect(QRect rect);
	
	/// Returns true if pointer() debug statements are being printed to STDERR.
	bool debugPtr() { return m_debugPtr; }
	/// Enables/disables debug statement output to STDERR in allocPointer() and ~VideoFrame() with regards to the pointer() allocation/deletion.
	void setDebugPtr(bool flag) { m_debugPtr = flag; }	
	
	/// Returns true if sectTextureId() has been called.
	bool hasTextureId() { return m_hasTextureId; }
	/// Returns the OpenGL textureId for this frame. The caller is responsible for ensuring that this ID is valid in the current context.
	GLuint textureId() { return m_textureId; }
	/// Set the texture ID for this frame so that other VideoConsumer objects don't have to re-upload this frame to the GPU.
	void setTextureId(GLuint id);

	/// Convenience function to convert this VideoFrame to a QImage if isRaw(). If bufferType() is BUFFER_IMAGE, then this is equivelant to calling image(). 
	/// The resulting QImage is cached in m_image and the cached QImage is returned on subsequent calls. Calls to image() after this will return the cached image,
	/// even if the VideoFrame still isRaw().
	/// If \a detachImage is true, then QImage::copy() is called before returning the final image.
	QImage toImage(bool detachImage=true);
	
protected:
	
	/// Time in milliseconds to 'hold' this frame on screen before displaying the next frame.
	int m_holdTime;
	
	/// Used for measuring latency - holds the time the frame is created.
	/// The VideoWidget can then compare it to the time the frame is displayed
	/// to determine how much latency there is in the pipeline.
	QTime m_captureTime;
	
	/// Holds the underlying format of the data in either the image, pointers, or byte arrayf
	QVideoFrame::PixelFormat m_pixelFormat;
	
	/// Type of data this frame holds
	BufferType m_bufferType;
	
	/// The frame itself - usually RGB32_Premul.
	QImage m_image;
	
	/// If bufferTYpe is POINTER, then, of course, data is expected to be in this pointer
	uchar *m_pointer;
	int m_pointerLength;
	
	/// Regardless of the buffer type, these members are expecte to contain the size and rect of the image, can be set both with setSize(), below
	QSize m_size;
	QRect m_rect;
	
	/// Set to true to print m_pointer allocation/deletion debug statements.
	bool m_debugPtr;
	
	/// Lock mutes for setting m_textureId
	QMutex m_textureIdMutex;
	/// True if setTextureId() has been called
	bool m_hasTextureId;
	/// The textureid itself.
	GLuint m_textureId;
};

typedef QSharedPointer<VideoFrame> VideoFramePtr;

class VideoFrameQueue : public QQueue<VideoFramePtr>
{
public:
	VideoFrameQueue(int maxBytes=0)
		: QQueue<VideoFramePtr>()
		, m_maxBytes(maxBytes)
		{}
		
	int byteSize() const
	{
		int sum = 0;
		for(int i=0; i<size(); i++)
		{
			sum += at(i)->byteSize();
		}
		return sum;
	}
	
	void setMaxByteSize(int bytes)
	{
		m_maxBytes = bytes;
	}
	
	int maxByteSize() { return m_maxBytes; }
	
	void enqueue(VideoFrame *frame)
	{
// 		if(m_maxBytes > 0)
// 			while(byteSize() > m_maxBytes)
// 				dequeue();	
		
		QQueue<VideoFramePtr>::enqueue(VideoFramePtr(frame));
		//enqueue(VideoFramePtr(frame));
	};
	
	void enqueue(VideoFramePtr frame)
	{
// 		if(m_maxBytes > 0)
// 			while(byteSize() > m_maxBytes)
// 				dequeue();	
		
		QQueue<VideoFramePtr>::enqueue(frame);
		//enqueue(VideoFramePtr(frame));
	};
	
protected:
	int m_maxBytes;
};



#endif
