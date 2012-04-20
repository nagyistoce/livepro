//
// This code displays corners found by the opencv function
// GoodFeaturesToTrack (cvGoodFeaturesToTrack)
//
// Sample webcam code taken from
//   http://www.cs.iit.edu/~agam/cs512/lect-notes/
//   opencv-intro/opencv-intro.html#SECTION00070000000000000000
//
// compile with:
// gcc `pkg-config --cflags opencv` `pkg-config --libs opencv` 
// -o good_features good_features.cpp
//
// Kristi Tsukida  <kristi.tsukida@gmail.com> Aug 20, 2008
//
// Note: I commented out the corner detection using the Harris 
// algorithm because my computer isn't fast enough to process
// both the Harris and the eigenvalue corners in real time.
// You can uncomment it and test for yourself.
//

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <stdio.h>
#include <sys/time.h>

#include <math.h>

#define VIDEO_WINDOW   "Webcam"
#define CORNER_EIG     "Eigenvalue Corner Detection"
// Disable harris processing
//#define CORNER_HARRIS  "Corner Detection (Harris)"

#define USEC_PER_SEC 1000000L

#include <QImage>
#include <QDebug>
#include "../../gfxengine/SimpleV4L2.h"
#include "../../gfxengine/VideoFrame.h"
#include "../../gfxengine/VideoThread.h"

#include "emam/Stabilizer.h"

IplImage* QImage2IplImage(QImage qimg);
QImage IplImage2QImage(IplImage *iplImg);


#define SOURCE_CAP
//#define SOURCE_FILE

class CornerItem {
public:
	CornerItem(int _x, int _y)
		: x(_x)
		, y(_y)
		, dist((double)INT_MAX)
		, ya(-1)
		, xa(-1)
		{}
	int x;
	int y;
	double dist;
	int xa;
	int ya;
};

typedef QList<CornerItem> CornerList;

bool cornerCompPosition(const CornerItem &c1, const CornerItem &c2)
	{
		bool f1 = c1.x < c2.x;
		bool f2 = c1.y < c2.y;
		return f1 && f2; 
	}

bool cornerCompDist(const CornerItem &c1, const CornerItem &c2)
	{
		return c1.dist > c2.dist; 
	}


CvScalar target_color[4] = { // in BGR order 
		{{   0,   0, 255,   0 }},  // red
		{{   0, 255,   0,   0 }},  // green
		{{ 255,   0,   0,   0 }},  // blue
		{{   0, 255, 255,   0 }}   // yellow
};

// returns the number of usecs of (t2 - t1)
long time_elapsed (struct timeval &t1, struct timeval &t2) {

	long sec, usec;

	sec = t2.tv_sec - t1.tv_sec;
	usec = t2.tv_usec - t1.tv_usec;
	if (usec < 0) {
		--sec;
		usec = usec + USEC_PER_SEC;
	}
	return sec*USEC_PER_SEC + usec;
}

struct timeval start_time;
struct timeval end_time;
void start_timer() {
	struct timezone tz;
	gettimeofday (&start_time, &tz);
}
long end_timer() {
	struct timezone tz;
	gettimeofday (&end_time, &tz);
	return time_elapsed(start_time, end_time);
}



IplImage *image = 0, *grey = 0, *prev_grey = 0, *pyramid = 0, *prev_pyramid = 0, *swap_temp;

int win_size = 10;
const int MAX_COUNT = 500;
CvPoint2D32f* points[2] = {0,0}, *swap_points;
char* status = 0;
int count = 0;
int need_to_init = 1;
int night_mode = 0;
int flags = 0;
int add_remove_pt = 0;
CvPoint pt;

StabilizerData stabilizer;
bool init = false;

// A Simple Camera Capture Framework
int main(int argc, char *argv[]) {
	long harris_time;
	long eig_time;

	CvCapture* capture = 0;
	IplImage* curr_frame = 0; // current video frame
	IplImage* curr_frame2 = 0; // current video frame
	IplImage* gray_frame = 0; // grayscale version of current frame
	int w, h; // video frame size
	IplImage* eig_image = 0;
	IplImage* temp_image = 0;
	// Disable harris processing
	IplImage* harris_eig_image = 0;
	IplImage* harris_temp_image = 0;
	
	// Pick one of these capture methods:
	// You must have compiled opencv with ffmpeg enabled
	// to use a web stream!
	//capture = cvCaptureFromFile(
	//		"http://user:pw@192.168.1.101:81/img/video.mjpeg");
	//capture = cvCaptureFromAVI(
	//		"http://user:pw@192.168.1.101:81/img/video.mjpeg");
	
	// Capture from a webcam
	//capture = cvCaptureFromCAM(CV_CAP_ANY);
// 	capture = cvCaptureFromCAM(-1); // capture from video device #0
// 	if ( !capture) {
// 		fprintf(stderr, "ERROR: capture is NULL... Exiting\n");
// 		//getchar();
// 		return -1;
// 	}

	
	#ifdef SOURCE_CAP
	
	SimpleV4L2 *simpleCapture = new SimpleV4L2();
	QString cameraFile = "/dev/video0";
	if(simpleCapture->openDevice(qPrintable(cameraFile)))
	{
		simpleCapture->initDevice();
		simpleCapture->startCapturing();
	}
	else
	{
		qDebug() << "Error opening "<<cameraFile<<" for capture, exiting.";
		return -1;
	}
	
	VideoFrame *curFrame = 0;
	
	#else
	
	QString file = "/home/josiah/Videos/2011-12-16_15-40-38_557.3gp";
	//QString file = "/data/home/josiah/Download/from.web/photos/sp 2.9.04/cimg0016.avi";
	//QString file = "../data/20120115/sermon.wmv";
	//QString file = "../data/20120115/webcam3.mp4";
	VideoThread *videoThread = new VideoThread();
	videoThread->setVideo(file);
	videoThread->start(); // starts new thread and starts playing
	videoThread->seek(380 * 1000, 0);
	VideoFramePtr curFrame;
	
	
	#endif
	
	// Create a window in which the captured images will be presented
	//cvNamedWindow(VIDEO_WINDOW, 0); // allow the window to be resized
	
	cvNamedWindow(CORNER_EIG, 0); // allow the window to be resized
	//cvMoveWindow(CORNER_EIG, 330, 0);
	
	// Disable harris processing
	//cvNamedWindow(CORNER_HARRIS, 0); // allow the window to be resized
	//cvMoveWindow(CORNER_HARRIS, 660, 0);
	
	QImage imageTmp;
	
	const int MAX_CORNERS = 500;
	CvPoint2D32f lastCorners[MAX_CORNERS] = {0};
	bool hasLastCorners = false;
	
	CvPoint2D32f cornerTmp[MAX_CORNERS] = {0};
// 	
	CvMat* translate = cvCreateMat(3,3,CV_32FC1);
	
	
	QImage imageX;
	IplImage* frame = 0;
		
	// Show the image captured from the camera in the window and repeat
	while (true) {
		
		// Get one frame
// 		curr_frame = cvQueryFrame(capture);
// 		if ( !curr_frame) {
// 			fprintf(stderr, "ERROR: frame is null... Exiting\n");
// 			//getchar();
// 			break;
// 		}

		int i, k, c;
		
		#ifdef SOURCE_CAP
		VideoFrame *newFrame = simpleCapture->readFrame();
		#else
		VideoFramePtr newFrame = videoThread->frame();
		#endif
		
		if(newFrame->isValid())
		{
			#ifdef SOURCE_CAP
			// Only delete if SOURCE_CAP since VideoFramePtr's delete themselves automatically as needed
			if(curFrame)
				delete curFrame;
			#endif
			curFrame = newFrame;
			
			//qDebug() << "Got frame";
			
			if(!curFrame->image().isNull())
				imageTmp = curFrame->image();
			else
				imageTmp = QImage((const uchar*)curFrame->pointer(),curFrame->size().width(),curFrame->size().height(),QImage::Format_RGB32);
			
// 			if(!image.save("last_frame.png"))
// 			{
// 				qDebug() << "Error saving frame";
// 				continue;
// 			}
			
			#ifdef SOURCE_CAP
			// The test camera is hanging upside-down
			//image = image.mirrored(true,true);
			
			// Crop the image a little to eliminate the black capture border
			imageTmp = imageTmp.copy(imageTmp.rect().adjusted(10,10,-10,-10));
			#endif
			
			//qDebug() << "Setup: Mark1";
			
			if(imageTmp.format() != QImage::Format_ARGB32)
				imageTmp = imageTmp.convertToFormat(QImage::Format_ARGB32);
				
			//qDebug() << "Setup: Mark2";
			
// 			curr_frame = cvCreateImageHeader( cvSize(imageTmp.width(), imageTmp.height()), IPL_DEPTH_8U, 4);
// 			curr_frame->imageData = (char*) imageTmp.bits();

			frame = cvCreateImageHeader( cvSize(imageTmp.width(), imageTmp.height()), IPL_DEPTH_8U, 4);
			frame->imageData = (char*) imageTmp.bits();
			
			//qDebug() << "Setup: Mark3";
			
		}
		
		if(!curFrame)
		{
			//qDebug() << "No frame...";
			continue;
		}
			
		//curr_frame = QImage2IplImage(image);
		
		//qDebug() << "Setup: Mark4";

		#if 0
		// Get frame size
		w = curr_frame->width;
		h = curr_frame->height;

		// Convert the frame image to grayscale
		if( ! gray_frame ) {
			//fprintf(stderr, "Allocate gray_frame\n");
			int channels = 1;
			gray_frame = cvCreateImage(
					cvGetSize(curr_frame), 
					IPL_DEPTH_8U, channels);
		}
		
		//qDebug() << "Setup: Mark5";
		cvCvtColor(curr_frame, gray_frame, CV_BGR2GRAY);
		
		//qDebug() << "Setup: Mark6";
		// ==== Allocate memory for corner arrays ====
		if ( !eig_image) {
			//fprintf(stderr, "Allocate eig_image\n");
			eig_image = cvCreateImage(cvSize(w, h),
					IPL_DEPTH_32F, 1);
		}
		if ( !temp_image) {
			//fprintf(stderr, "Allocate temp_image\n");
			temp_image = cvCreateImage(cvSize(w, h),
					IPL_DEPTH_32F, 1);
		}
// Disable harris processing
		if ( !harris_eig_image) {
			//fprintf(stderr, "Allocate harris_eig_image\n");
			harris_eig_image = cvCreateImage(cvSize(w, h),
					IPL_DEPTH_32F, 1);
		}
		if ( !harris_temp_image) {
			//fprintf(stderr, "Allocate harris_temp_image\n");
			harris_temp_image = cvCreateImage(cvSize(w, h),
					IPL_DEPTH_32F, 1);
		}

		//qDebug() << "Setup: Mark7";
		
		// ==== Corner Detection: MinEigenVal method ====
		start_timer();
		
		#if 1
		CvPoint2D32f corners[MAX_CORNERS] = {0};
		int corner_count = MAX_CORNERS;
		double quality_level = 0.1;
		double min_distance = 10; //h/25;
		int eig_block_size = 3;
		int use_harris = false;
		
		cvGoodFeaturesToTrack(gray_frame,
				eig_image,                    // output
				temp_image,
				corners,
				&corner_count,
				quality_level,
				min_distance,
				NULL,
				eig_block_size,
				use_harris, 0.04);
		cvScale(eig_image, eig_image, 100, 0.00);
		eig_time = end_timer();
		//cvShowImage(CORNER_EIG, eig_image);
		
		#endif
		//qDebug() << "Setup: Mark8";
		
// Disable harris processing
//		// ==== Corner Detection: Harris method ====
//		start_timer();
//		const int MAX_CORNERS = 100;
		#if 0
		CvPoint2D32f corners[MAX_CORNERS] = {0};
		int corner_count = MAX_CORNERS;
		double harris_quality_level = 0.1;
		double harris_min_distance = 1;
		int harris_eig_block_size = 3;
		int harris_use_harris = true;
		
		cvGoodFeaturesToTrack(gray_frame,
				harris_eig_image,                    // output
				harris_temp_image,
				corners,
				&corner_count,
				harris_quality_level,
				harris_min_distance,
				NULL,
				harris_eig_block_size,
				harris_use_harris);
		cvScale(harris_eig_image, harris_eig_image, 200, 0.50);
		#endif
		
//		harris_time = end_timer();
//		cvShowImage(CORNER_HARRIS, harris_eig_image);
//		
//		fprintf(stderr, "harris time: %i  eig time: %i\n", harris_time, eig_time);
		
		
		
		//qDebug() << "Setup: Mark10";
		
		#endif
		
		if( !image )
		{
			/* allocate all the buffers */
			//image = cvCreateImage( cvGetSize(frame), 8, 3 );
			image = cvCloneImage( frame );
			image->origin = frame->origin;
			grey = cvCreateImage( cvGetSize(frame), 8, 1 );
			prev_grey = cvCreateImage( cvGetSize(frame), 8, 1 );
			pyramid = cvCreateImage( cvGetSize(frame), 8, 1 );
			prev_pyramid = cvCreateImage( cvGetSize(frame), 8, 1 );
			points[0] = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(points[0][0]));
			points[1] = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(points[0][0]));
			status = (char*)cvAlloc(MAX_COUNT);
			flags = 0;
		}
	
	
		cvCopy( frame, image, 0 );
		cvCvtColor( image, grey, CV_BGR2GRAY );
	
	
			
		// Default Stabilizer Settings:
		//BOOL cleanOutput = TRUE;		// Whether to show overlayed data on the video such as motion vectors.
		BOOL cleanOutput = FALSE;		// Whether to show overlayed data on the video such as motion vectors.
		BOOL showVectorsUsed = TRUE;		// Whether to display the Local Motion Vectors that were used for determining the Global Motion Vector.
		BOOL showCompensation = TRUE;		// Whether to show the compensated image or the original image.
		BOOL comparePrevFrame = TRUE;		// Whether to do Optical Flow compared to the previous frame or the first frame.
		BOOL elasticReset = FALSE;		// Whether the ROI should slowly move back towards the default position.

		
		if(!init)
		{
			init = true;
			printf("Initializing the Stabilizer.\n");
			initStabilizer(&stabilizer, image, showVectorsUsed, showCompensation, comparePrevFrame, elasticReset, cleanOutput);
		}
		//printf("Show video.\n");
		
		//cvShowImage(VIDEO_WINDOW, image);
		
		//printf("Stabilize.\n");
		
		IplImage *imageStabilized = stabilizeImage(&stabilizer, image);
		
		// Draw a rect to show the desired ROI
// 		if (!cleanOutput) 
// 		{
// 			//qDebug() << "ROI: "<<stabilizer.roi.x << stabilizer.roi.y <<  stabilizer.roi.width << stabilizer.roi.height;
// 			cvRectangle(imageStabilized, 
// 				cvPoint(stabilizer.roi.x, stabilizer.roi.y), 
// 				cvPoint(stabilizer.roi.x + stabilizer.roi.width-1, stabilizer.roi.y + stabilizer.roi.height-1), 
// 				CV_RGB(255,255,0), 1, 8, 0
// 			);	// yellow border
// 		}
		
		cvShowImage( CORNER_EIG, imageStabilized );
		
	
// 		if( night_mode )
// 			cvZero( image );
// 	
// 		if( need_to_init )
// 		{
// 			/* automatic initialization */
// 			IplImage* eig = cvCreateImage( cvGetSize(grey), 32, 1 );
// 			IplImage* temp = cvCreateImage( cvGetSize(grey), 32, 1 );
// 			double quality = 0.01;
// 			double min_distance = 10;
// 		
// 			count = MAX_COUNT;
// 			cvGoodFeaturesToTrack( grey, eig, temp, points[1], &count,
// 						quality, min_distance, 0, 3, 0, 0.04 );
// 			cvFindCornerSubPix( grey, points[1], count,
// 				cvSize(win_size,win_size), cvSize(-1,-1),
// 				cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03));
// 			cvReleaseImage( &eig );
// 			cvReleaseImage( &temp );
// 		
// 			add_remove_pt = 0;
// 		}
// 		else if( count > 0 )
// 		{
// 			cvCalcOpticalFlowPyrLK( prev_grey, grey, prev_pyramid, pyramid,
// 				points[0], points[1], count, cvSize(win_size,win_size), 3, status, 0,
// 				cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03), flags );
// 			flags |= CV_LKFLOW_PYR_A_READY;
// 			for( i = k = 0; i < count; i++ )
// 			{
// 				if( add_remove_pt )
// 				{
// 					double dx = pt.x - points[1][i].x;
// 					double dy = pt.y - points[1][i].y;
// 			
// 					if( dx*dx + dy*dy <= 25 )
// 					{
// 						add_remove_pt = 0;
// 						continue;
// 					}
// 				}
// 		
// 				if( !status[i] )
// 					continue;
// 		
// 				points[1][k++] = points[1][i];
// 				cvCircle( image, cvPointFrom32f(points[1][i]), 3, CV_RGB(0,255,0), -1, 8,0);
// 			}
// 			count = k;
// 		}
// 	
// 		if( add_remove_pt && count < MAX_COUNT )
// 		{
// 			points[1][count++] = cvPointTo32f(pt);
// 			cvFindCornerSubPix( grey, points[1] + count - 1, 1,
// 				cvSize(win_size,win_size), cvSize(-1,-1),
// 				cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03));
// 			add_remove_pt = 0;
// 		}
// 		
// 		
// 		
// 		
// 		if(hasLastCorners)
// 		{
// 			double delta = 0;
// 			for( i = 0; i < count; i++ )
// 			{
// 				int xa = lastCorners[i].x,
// 				    ya = lastCorners[i].y;
// 				
// 				double dist = sqrt( pow(xa-points[1][i].x, 2) + pow(ya-points[1][i].y,2) );
// 				
// 				delta += dist;
// 			}
// 			
// 			delta /= (double)count;
// 			//qDebug() << "Avg drift: "<<delta;
// 			fprintf(stderr,"Avg drift: %.01f\n",delta);
// 		}
// 		
// 		
		
		
		for( int i = 0; i < count; i++) {
			lastCorners[i].x = points[1][i].x;
			lastCorners[i].y = points[1][i].y;
			hasLastCorners = true;
		}
		
		
		
	
		CV_SWAP( prev_grey, grey, swap_temp );
		CV_SWAP( prev_pyramid, pyramid, swap_temp );
		CV_SWAP( points[0], points[1], swap_points );
		need_to_init = 0;
		//cvShowImage( CORNER_EIG, image );
	
		c = cvWaitKey(10);
		if( (char)c == 27 )
			break;
		if ( (c & 255) == 27)
			break;
			
		switch( (char) c )
		{
			case 'r':
				need_to_init = 1;
			break;
			case 'c':
				count = 0;
			break;
			case 'n':
				night_mode ^= 1;
			break;
			default:
			;
		}
		
		if(count <= 1)
		{
			//qDebug() << "Need to init!";
			need_to_init = 1;
		}
		else
		{
			//qDebug() << "Count:"<<count;
		}
		
		
		
        	#if 0
		if(hasLastCorners)
		{
			// Here we must:
			// Build up a list of Corners for this frame
			// (Struct of: x,y,dist,other, where other is the nearest corner from last frame)
			// Go over the list built and any corner that has dist of >2*avg, eliminate
			// Then create new list of corners for putting into the src and dest matrixes 
			
			
			
// 			class CornerItem {
// 				int x;
// 				int y;
// 				double dist;
// 				CornerItem other;
// 			};
// 			
// 			typedef QList<CornerItem> cornerList;

			// Build list and find closest corner from last fram distance
			CornerList cornerList;
			double distSum = 0;
			for(int i = 0; i < corner_count; i++) 
			{
				CornerItem corner(corners[i].x, corners[i].y);
				
				int n = -1;
				for(int k = 0; k < corner_count; k++) 
				{
					int xa = lastCorners[k].x,
					    ya = lastCorners[k].y;
					
					double dist = sqrt( pow(xa-corner.x, 2) + pow(ya-corner.y,2) );
					if(dist < corner.dist)
					{
						corner.dist = dist;
						corner.xa = xa;
						corner.ya = ya;
						n = k;
					}
				}
				
				if(n < 0)
				{
					qDebug() << "Odd - didn't find ANY corners in last frame less than INT_MAX distance from corner "<<i<<", skipping this frame";
					continue;
				}
				
				distSum += corner.dist;
				
				cornerList << corner;
			}
			
			double avgDist = distSum / ((double)cornerList.size());
			
			CornerList tmpList;
			
			double threshold = avgDist * 1000; // NB NOTE MAGIC NUMBER
			foreach(CornerItem item, cornerList)
				if(item.dist < threshold)
					tmpList << item;
				else
					qDebug() << "Skipping item, dist:"<<item.dist<<", thresh:"<<threshold;
			
			// Sort by distance and grab the top 10 biggest changes
			qSort(tmpList.begin(), tmpList.end(), cornerCompDist);
			cornerList = tmpList;
			tmpList.clear();
			int max = cornerList.size() > 20 ? 20: cornerList.size(); 
			for(int i=0; i<max; i++)
			{
				//qDebug() << "Dist: "<<cornerList[i].dist<<", avg:"<<avgDist;
				tmpList << cornerList[i];
			}
			
			// Now sort by position
			qSort(tmpList.begin(), tmpList.end(), cornerCompPosition);
			cornerList = tmpList;
			
			int numCorners = cornerList.size();
			if(cornerList.isEmpty())
			{
				qDebug() << " no corners found ";
				continue;
			}
			if(numCorners < 4)
			{
				qDebug() << "cvFindHomography requires 4 corners, only have:"<<numCorners;
				continue; 
			}
// 			else
// 				qDebug() << "corners: "<<numCorners;

			CvMat* src_mat = cvCreateMat( numCorners, 2, CV_32FC1 );
			CvMat* dst_mat = cvCreateMat( numCorners, 2, CV_32FC1 );
	
			//qDebug() << "mark1";
			int counter=0;
			foreach(CornerItem item, cornerList)
			{
				cornerTmp[counter].x = item.xa;
				cornerTmp[counter].y = item.ya;
				counter++;
			}
			cvSetData( src_mat, cornerTmp, sizeof(CvPoint2D32f));
			//qDebug() << "mark2";
			counter = 0;
			foreach(CornerItem item, cornerList)
			{
				cornerTmp[counter].x = item.x;
				cornerTmp[counter].y = item.y;
				counter++;
			}
			cvSetData( dst_mat, cornerTmp, sizeof(CvPoint2D32f));
			//qDebug() << "mark3";
		
			//figure out the warping!
			//warning - older versions of openCV had a bug
			//in this function.
			cvFindHomography(src_mat, dst_mat, translate);
		
			//qDebug() << "mark5";
			curr_frame2 = cvCloneImage( curr_frame );
			//qDebug() << "mark6";
			curr_frame2->origin = curr_frame2->origin;
			cvZero( curr_frame2 );

			//qDebug() << "mark7";
			cvWarpPerspective( curr_frame, curr_frame2, translate);
			//qDebug() << "mark8";
			
			cvShowImage(CORNER_EIG, curr_frame2);
			
			cvReleaseMat(&src_mat);
			cvReleaseMat(&dst_mat);
			
			// ==== Draw circles around detected corners in original image
			//fprintf(stderr, "corner[0] = (%f, %f)\n", corners[0].x, corners[0].y);
			int radius = h/25;
			foreach(CornerItem item, cornerList)
			{
				cvCircle(curr_frame,
						cvPoint((int)(item.x + 0.5f),(int)(item.y + 0.5f)),
						radius,
						target_color[0]);
					
				cvCircle(curr_frame,
						cvPoint((int)(item.xa + 0.5f),(int)(item.ya + 0.5f)),
						radius,
						target_color[1]);
			}
		}
		
		
		//qDebug() << "Setup: Mark9";
		cvShowImage(VIDEO_WINDOW, curr_frame);
		
		
		// Just for debugging - freeze the output :-)
		int delay = 10; //hasLastCorners ? 9999999 :10;
		
		// If ESC key pressed, Key=0x10001B under OpenCV 0.9.7(linux version),
		// remove higher bits using AND operator
		if ( (cvWaitKey(delay) & 255) == 27)
			break;
			
		#endif 
		
		
		
		

		//cvReleaseImage(&curr_frame);
	}

	cvReleaseMat(&translate);
	

	// Release the capture device housekeeping
	cvReleaseCapture( &capture);
	//cvDestroyWindow(VIDEO_WINDOW);
	cvDestroyWindow(CORNER_EIG);
// Disable harris processing
//	cvDestroyWindow(CORNER_HARRIS);
	return 0;
}




IplImage* QImage2IplImage(QImage qimg)
{
	if(qimg.format() != QImage::Format_ARGB32)
		qimg = qimg.convertToFormat(QImage::Format_ARGB32);
		
	IplImage *imgHeader = cvCreateImageHeader( cvSize(qimg.width(), qimg.width()), IPL_DEPTH_8U, 4);
	imgHeader->imageData = (char*) qimg.bits();
	
	uchar* newdata = (uchar*) malloc(sizeof(uchar) * qimg.byteCount());
	memcpy(newdata, qimg.bits(), qimg.byteCount());
	imgHeader->imageData = (char*)newdata;
	
	// NB: Caller is responsible for deleting imageData!
	return imgHeader;
}

QImage IplImage2QImage(IplImage *iplImg)
{
	int h        = iplImg->height;
	int w        = iplImg->width;
	int step     = iplImg->widthStep;
	char *data   = iplImg->imageData;
	int channels = iplImg->nChannels;
	
	QImage qimg(w, h, QImage::Format_ARGB32);
	for (int y = 0; y < h; y++, data += step)
	{
		for (int x = 0; x < w; x++)
		{
			char r=0, g=0, b=0, a=0;
			if (channels == 1)
			{
				r = data[x * channels];
				g = data[x * channels];
				b = data[x * channels];
			}
			else if (channels == 3 || channels == 4)
			{
				r = data[x * channels + 2];
				g = data[x * channels + 1];
				b = data[x * channels];
			}
			
			if (channels == 4)
			{
				a = data[x * channels + 3];
				qimg.setPixel(x, y, qRgba(r, g, b, a));
			}
			else
			{
				qimg.setPixel(x, y, qRgb(r, g, b));
			}
		}
	}
	return qimg;

}

