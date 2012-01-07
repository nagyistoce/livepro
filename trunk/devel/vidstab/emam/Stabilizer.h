/*
 * Video Stabilizer Module (Digital Image Stabilization), by Shervin Emami (shervin.emami@gmail.com) on 17th April 2010.
 *	Based loosely on the LKdemo example from OpenCV.
 *	Uses Harris corner detection for initial points, then tracks them using LK Pyramidal Optical Flow.
 *	Available in both C & Java at "http://www.shervinemami.co.cc/"
*/

#ifndef STABILIZER_H
#define STABILIZER_H

#if defined (__cplusplus)
extern "C"
{
#endif


#define MAX_POINTS 500				// Max number of corner points that will be found by LK. (Usually will be much less, like 60).
#define MAP_RESOLUTION 20			// Use a 2D histogram of +/-30 x +/-30 grid cells (from values between +/-30.0 x +/-30.0)
#define MAP_SIZE  MAP_RESOLUTION*2	// Allow both negative & positive values.


#ifndef BOOL
#define BOOL bool
#define TRUE true
#define FALSE false
#endif

// Internal variables used:
typedef struct _StabilizerData {
	int initialized;
	BOOL got_new_corners;		// Signals that the Stabilizer just found new corners.
	int countMap[MAP_SIZE][MAP_SIZE];
	CvPoint2D32f* pointsCurr;
	CvPoint2D32f* pointsPrev;
	CvPoint2D32f* swap_points;
	char* valid;
	char* status;
	IplImage *imageOut;
	IplImage *grey, *prev_grey, *pyramid, *prev_pyramid, *swap_temp;
	IplImage* temp;
	IplImage* eig;
	IplImage *imageRef;
	IplImage *imageTmp;
	int need_new_corners;
	int count;
	int valid_count;
	int detected_count;
	int flags;
	//CvPoint pt;
	CvRect roi;
	CvRect global_roi;
	float gmvX, gmvY;
	BOOL showVectorsUsed;		// Whether to display the Local Motion Vectors that were used for determining the Global Motion Vector.
	BOOL showCompensation;		// Whether to show the compensated image or the original image.
	BOOL comparePrevFrame;		// Whether to do Optical Flow compared to the previous frame or the first frame.
	BOOL elasticReset;			// Whether the ROI should slowly move back towards the default position.
	BOOL cleanOutput;			// Whether to show overlayed data on the video such as motion vectors.
} StabilizerData;



// Initialise the data and allocate all the buffers.
void initStabilizer(StabilizerData *s, const IplImage *image,
		BOOL showVectorsUsed,		// Whether to display the Local Motion Vectors that were used for determining the Global Motion Vector.
		BOOL showCompensation,		// Whether to show the compensated image or the original image.
		BOOL comparePrevFrame,		// Whether to do Optical Flow compared to the previous frame or the first frame.
		BOOL elasticReset,			// Whether the ROI should slowly move back towards the default position.
		BOOL cleanOutput			// Whether to show overlayed data on the video such as motion vectors.
		);

// Look for Harris Corner points in the image that should be good for tracking later using Optical Flow.
void findPointsToTrack(StabilizerData *s);

// Perform Video Stabilization (also called Digital Image Stabilization) on the given image
// compared to the previous image that was passed to this function.
// Do NOT free or resize the returned image, since it is used internally.
IplImage* stabilizeImage(StabilizerData *s, IplImage *image);

// Show the internal Video Stabilization timings.
void printStabilizerTimings(StabilizerData *s);

// Free the Stabilizer's used resources.
void finishStabilizer(StabilizerData *s);



#if defined (__cplusplus)
}
#endif

#endif	//STABILIZER_H
