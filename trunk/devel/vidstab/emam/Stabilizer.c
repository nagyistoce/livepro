/*
 * Video Stabilizer Module (Digital Image Stabilization), by Shervin Emami (shervin.emami@gmail.com) on 17th April 2010.
 *	Based loosely on the LKdemo example from OpenCV.
 *	Uses Harris corner detection for initial points, then tracks them using LK Pyramidal Optical Flow.
 *	Available in both C & Java at "http://www.shervinemami.co.cc/"
*/

#define CV_NO_BACKWARD_COMPATIBILITY

#include "cv.h"
#include "highgui.h"
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "ImageUtils.h"		// ImageUtils by Shervin Emami on 20th Feb 2010.
#include "Stabilizer.h"		// Video Stabilizer Module by Shervin Emami on 17th Apr 2010.

// Internal Video Stabilization parameters:
double RESET_RATIO = 0.9;//0.95;//0.6;			// Ratio of points that should be remaining before finding new tracking points again.
float MOTION_COMPENSATION_STRENGTH = 1.0f;//0.6;//4.5f;	// Strength of the motion compensation
float MAX_GLOBAL_MOVEMENT = 200.0f;//30;	// Limit the Global Motion Vector
float MAX_LOCAL_MOVEMENT = 30.0f;//30;	// Limit the Local Motion Vector
BOOL USE_SUBPIXEL_ACCURACY = FALSE;	// Whether to find sub-pixel tracking points (very slow) or just pixel points (faster).
BOOL PRINT_VALID_POINTS = FALSE;	// Whether to print to the console showing which points are valid or invalid.
double CORNER_QUALITY = 0.01;		// Controls roughly how many points are generated. 0.001 creates hundreds of points, 0.1 makes a few.
double CORNER_MIN_DISTANCE = 15.0;	// Desired minimum distance between points.
int CORNER_BLOCK_SIZE = 3;		// Block size when scanning the image for corners. 1 is basic, 3 is good.
int CORNER_MAX_ITERATIONS = 20;		// When the Sub-pixel corner search should stop.
double CORNER_EPSILON = 0.03;		// When the Sub-pixel corner search should stop.
int OPFLOW_MAX_ITERATIONS = 20;		// When the Optical Flow search should stop.
double OPFLOW_EPSILON = 0.03;		// When the Optical Flow search should stop.
int OPFLOW_PYR_LEVEL = 4;		// How many pyramid levels should the Optical Flow use, to detect faster movements.
int CORNER_WIN_SIZE = 10;		// Size of the window for Sub-pixel corner search.
int OPFLOW_WIN_SIZE = 10;		// Size of the window for Optical Flow search.
BOOL REPLICATE_BORDERS = FALSE;		// Whether the border pixels should be replicated, or just set to a default color.



// Record the execution time of some code. By Shervin Emami.
// eg:
//	DECLARE_TIMING(myTimer);
//	START_TIMING(myTimer);
//	printf("Hello!\n");
//	STOP_TIMING(myTimer);
//	printf("Time: %f ms\n", GET_TIMING(myTimer) );
#define DECLARE_TIMING(s)		double timeStart_##s; double timeDiff_##s; double timeTally_##s = 0; int countTally_##s = 0
#define START_TIMING(s)			timeStart_##s = (double)cvGetTickCount()
#define STOP_TIMING(s)			timeDiff_##s = (double)cvGetTickCount() - timeStart_##s; timeTally_##s += timeDiff_##s; countTally_##s++
#define GET_TIMING(s)			(double)(timeDiff_##s / (cvGetTickFrequency()*1000.0))
#define GET_AVERAGE_TIMING(s)	(double)(countTally_##s ? timeTally_##s / ((double)countTally_##s * cvGetTickFrequency()*1000.0) : 0)


// Allow to record timings
DECLARE_TIMING(corners);
DECLARE_TIMING(cornerProcess);
DECLARE_TIMING(cornersSubPixel);
DECLARE_TIMING(opticalFlow);
DECLARE_TIMING(gmv);
DECLARE_TIMING(gmvProcess);
DECLARE_TIMING(compensate);


// Initialise the data and allocate all the buffers.
void initStabilizer(StabilizerData *s, const IplImage *image,
		BOOL showVectorsUsed,		// Whether to display the Local Motion Vectors that were used for determining the Global Motion Vector.
		BOOL showCompensation,		// Whether to show the compensated image or the original image.
		BOOL comparePrevFrame,		// Whether to do Optical Flow compared to the previous frame or the first frame.
		BOOL elasticReset,			// Whether the ROI should slowly move back towards the default position.
		BOOL cleanOutput			// Whether to show overlayed data on the video such as motion vectors.
		)
{
	CvSize pyramid_size;
	CvSize size = cvGetSize(image);

	if (!s) {
		printf("ERROR in initStabilizer(): No StabilizerData was given!\n");
		exit(1);
	}

	// Initialize all the internal data to 0.
	memset(s, 0, sizeof(StabilizerData));

	// Get the settings
	s->cleanOutput = cleanOutput;
	s->showVectorsUsed = showVectorsUsed;
	s->showCompensation = showCompensation;
	s->comparePrevFrame = comparePrevFrame;
	s->elasticReset = elasticReset;
	
	s->grey = cvCreateImage( size, 8, 1 );		// Greyscale image
	s->prev_grey = cvCreateImage( size, 8, 1 );
	//s->imageOut = cvCreateImage( size, 8, 3);	// RGB image
	s->imageOut = cvCloneImage(image);		// RGB image
	s->eig = cvCreateImage( size, 32, 1 );		// 32-bit Greyscale image, used for scratch space
	s->temp = cvCreateImage( size, 32, 1 );		//		"		"		"		"		"		"
	
	// Temporary scratch space for OpticalFlowPyrLK, should be atleast (w+8)*h/3 bytes.
	pyramid_size = cvSize( image->width+8, image->height/3 );
	s->pyramid = cvCreateImage( pyramid_size, 8, 1 );
	s->prev_pyramid = cvCreateImage( pyramid_size, 8, 1 );
	
	// Allocate the points array for the current and previous frame.
	s->pointsPrev = (CvPoint2D32f*)cvAlloc(MAX_POINTS*sizeof(s->pointsPrev[0]));
	s->pointsCurr = (CvPoint2D32f*)cvAlloc(MAX_POINTS*sizeof(s->pointsCurr[0]));
	s->status = (char*)cvAlloc(MAX_POINTS);
	s->valid = (char*)cvAlloc(MAX_POINTS);
	s->flags = 0;
	s->need_new_corners = 1;	// Start tracking from the first video frame.
	s->initialized = 1;	// Show that the module has been initialized.
}


// Look for Harris Corner points in the image that should be good for tracking later using Optical Flow.
void findPointsToTrack(StabilizerData *s)
{
	if (!s) {
		printf("Stabilizer ERROR in findPointsToTrack(): No StabilizerData was given!\n");
		exit(1);
	}
	if (!s->initialized) {
		printf("Stabilizer ERROR in findPointsToTrack(): StabilizerData was not initialized!\n");
		exit(1);
	}

	// Keep a copy of this reference image, so that the following frames
	// will find their movements relative to this reference frame.
	s->imageRef = cvCloneImage(s->grey);		// Greyscale Ref image

	// Look for corners that seem good for tracking, storing them into 'pointsCurr'.
	s->count = MAX_POINTS;
	START_TIMING(corners);	// Record the timing.
	cvGoodFeaturesToTrack( s->grey, s->eig, s->temp, s->pointsCurr, &s->count,
                           CORNER_QUALITY, CORNER_MIN_DISTANCE, NULL, CORNER_BLOCK_SIZE, 0, 99999.0 );
	STOP_TIMING(corners);	// Stop recording the timing.
	if (!s->cleanOutput)
		printf("Stabilizer Corner Detection time = %f ms.\n", GET_TIMING(corners) );

	// Get sub-pixel accuracy for the points given.
	if (USE_SUBPIXEL_ACCURACY) {
		START_TIMING(cornersSubPixel);	// Record the timing
		cvFindCornerSubPix( s->grey, s->pointsCurr, s->count,
			cvSize(CORNER_WIN_SIZE, CORNER_WIN_SIZE), cvSize(-1,-1),
			cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, CORNER_MAX_ITERATIONS, CORNER_EPSILON));
		STOP_TIMING(cornersSubPixel);	// Stop recording the timing.
		if (!s->cleanOutput)
			printf("Stabizer Corner Sub-Pixel time = %f ms.\n", GET_TIMING(cornersSubPixel) );
	}

	s->detected_count = s->count;
	if (!s->cleanOutput)
		printf("Stabilizer Points obtained from corner detection: %d\n", s->detected_count);

	// Set all points as valid since new points have just been generated.
	memset(s->valid, TRUE, s->count);
	//memset(s->status, TRUE, s->count);

	s->valid_count = s->count;

	s->flags = 0;	// Calculate the data again since everything has changed.
	s->global_roi = cvRect(0,0,0,0);	// clear the Global ROI
	s->gmvX = 0.0f;	// Reset the Global Motion Vector
	s->gmvY = 0.0f;
}


// Perform Video Stabilization (also called Digital Image Stabilization) on the given image
// compared to the previous image that was passed to this function.
// Do NOT free or resize the returned image, since it is used internally.
IplImage* stabilizeImage(StabilizerData *s, IplImage *image)
{
    int i, k;
	float f;
	IplImage *imageCurrentReference = 0;
	float aveX, aveY;
	int indexXused, indexYused;
	int aveCount;
	int maxCount;
	float invDiv;

	if (!s) {
		printf("Stabilizer ERROR in stabilizeImage(): No StabilizerData was given!\n");
		exit(1);
	}
	if (!s->initialized) {
		printf("Stabilizer ERROR in stabilizeImage(): StabilizerData was not initialized!\n");
		exit(1);
	}

	// Convert the current frame to Greyscale
    	cvCvtColor( image, s->grey, CV_BGR2GRAY );

	// At first, find corner points in the image to initialize the tracking.
	// If a lot of the points are later detected (because of movement), then add new ones.
	if( s->need_new_corners || (s->valid_count < cvRound(s->detected_count * RESET_RATIO)))
    	{
		START_TIMING(cornerProcess);	// Record the timing.

		findPointsToTrack(s);

		// Show the new LK points on the screen as orange.
		if (!s->cleanOutput) {
			for( i = 0; i < s->valid_count; i++ ) {
				cvCircle( image, cvPointFrom32f(s->pointsCurr[i]), 2, CV_RGB(255,200,50), -1, 8,0);
			}
		}

		STOP_TIMING(cornerProcess);	// Stop the timing.

		s->got_new_corners = TRUE;		// Signal that new corners were obtained.
	}
	else if( s->count > 0 )
	{
		s->got_new_corners = FALSE;		// Signal that previous corners were used.

		// Find the optical flow using Pyramid levels to handle sudden movements, and LK for good reliability.
		START_TIMING(opticalFlow);	// Record the timing.

		if (s->comparePrevFrame)
			imageCurrentReference = s->prev_grey;	// Compare the current frame with the previous frame.
		else
			imageCurrentReference = s->imageRef;	// Compare the current frame with the first (or ref) frame.
			
		// Compare imageCurrentReference & pointsPrev with grey, generating pointsCurr & status arrays.
		cvCalcOpticalFlowPyrLK( imageCurrentReference, s->grey, s->prev_pyramid, s->pyramid,
			s->pointsPrev, s->pointsCurr, s->count, cvSize(OPFLOW_WIN_SIZE,OPFLOW_WIN_SIZE), OPFLOW_PYR_LEVEL, s->status, 0,
			cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, OPFLOW_MAX_ITERATIONS, OPFLOW_EPSILON), s->flags );

		// Now that atleast the first call to cvCalcOpticalFlowPyrLK() has occured, reuse the PyrA data.
		if (s->comparePrevFrame)
			s->flags |= CV_LKFLOW_PYR_A_READY;

			// Process each point
		for( i = k = 0; i < s->count; i++ )
		{
			// Delete the points where the optical flow wasn't found for some reason, due to wierd movement or offscreen.
			if( !s->status[i] ) 
			{
				s->valid[i] = FALSE;	// mark it as an invalid point from now on.
				//printf("Stabilizer: deleting bad point %d at (%f,%f).\n", i, pointsCurr[i].x, pointsCurr[i].y);
				if (s->pointsCurr[i].x > 0.0 && s->pointsCurr[i].y > 0.0 && !s->cleanOutput) 
				{
					cvCircle( image, cvPointFrom32f(s->pointsCurr[i]), 5, CV_RGB(255,0,0), -1, 8,0);	// show bad points as Red
					cvCircle( image, cvPointFrom32f(s->pointsCurr[i]), 15, CV_RGB(255,0,0), 1, 8,0);	// show bad points as Red
				}
                		continue;
			}
			
			// Ignore points that were previously marked invalid.
			if ( !s->valid[i] )
				continue;

			// Count how many valid points are available.
			k++;
        	}
		STOP_TIMING(opticalFlow);	// Stop the timing.

		// Delete the points that had a bad status.
		s->valid_count = k;
		if (!s->cleanOutput)
			printf("Stabilizer LK point count: %d (i=%d, k=%d)\n", s->count, i, k);

		// If most of the image has been lost (due to lots of movement, or an extreme change in image),
		// then skip this video frame and on the next frame, recalculate the corner points to track.
		if( s->valid_count < cvRound(s->detected_count * RESET_RATIO)) {
			printf("Stabilizer recalculating the corner points in the middle of the iteration.\n");
			return stabilizeImage(s, image);	// Process the frame again.
			//return NULL;		// Wait for the next frame before doing anything.
		}

		// Find the Global Motion Vector from all the Local Motion Vectors.
		START_TIMING(gmvProcess);
		START_TIMING(gmv);

		// Create a 2D histogram to find the most common Local Motion Vector, and use that as the Global Motion Vector.
		// Note that this will allow moving objects, as long as most of the image is a static background.

		// For each valid corner point, increment the counter for that Local Motion Vector (dx,dy).
		// For efficiency, a 2D map of integer counts of pixels is used.
		// eg:	If a valid point has moved +6.8 pixels right and +7.9 pixels down, then increment the countMap value for (+6.8, +7.9).
		//		Since we want to allow negatives, and to fit the approximate range of -30 to +30 pixels into about 20 grid cells (3x3 pixels per grid cell),
		//		this is accessed at countMap[+6.8 * 10/30 + 10, +7.9 * 10/30 + 10].
		memset(s->countMap, 0, MAP_SIZE * MAP_SIZE * sizeof(int));
		maxCount = 0;
		aveX = 0.0f;
		aveY = 0.0f;
		aveCount = 0;
		for (i=0; i<s->count; i++) {
			float dX, dY;
			float distSquared;
			int indexX, indexY;
			int newCount;
			CvScalar lineColor;
			if (s->status[i] && s->valid[i]) {	// Make sure its an existing vector
				dX = s->pointsCurr[i].x - s->pointsPrev[i].x;
				dY = s->pointsCurr[i].y - s->pointsPrev[i].y;
				//printf("Stabilizer: curr=%f,%f, prev=%f,%f\n", pointsCurr[i].x, pointsCurr[i].y, pointsPrev[i].x, pointsPrev[i].y);
				lineColor = CV_RGB(255,255,255);	// good lines are shown as white

				// Make sure its not massive
				distSquared = dX*dX + dY*dY;
				if (distSquared < MAX_LOCAL_MOVEMENT*MAX_LOCAL_MOVEMENT) {
					// Add the Local Motion Vector to the entries for the Global Motion Vector
					indexX = cvRound(dX * (float)(MAP_RESOLUTION-1) / (float)MAX_LOCAL_MOVEMENT) + MAP_RESOLUTION;
					indexY = cvRound(dY * (float)(MAP_RESOLUTION-1) / (float)MAX_LOCAL_MOVEMENT) + MAP_RESOLUTION;
					if (indexX < 0 || indexX >= MAP_SIZE || indexY < 0 || indexY >= MAP_SIZE) {
						printf("Stabilizer ERROR! countMap %f,%f => %d,%d!\n", dX,dY, indexX,indexY);
						return NULL;
					}

					// Increment the counter (histogram bin count).
					newCount = (s->countMap[indexY][indexX]) + 1;
					s->countMap[indexY][indexX] = newCount;

					// Find the most popular cell bin
					if (newCount > maxCount) {
						maxCount = newCount;
						indexXused = indexX;
						indexYused = indexY;
						// Reset the bin's average vector, since the max bin has just changed
						aveX = 0.0f;
						aveY = 0.0f;
						aveCount = 0;
					}
					// Get the average Local Motion Vector value for the max cell bin (for better accuracy than just using the cell bin's center value)
					if (newCount == maxCount) {
						aveX += dX;
						aveY += dY;
						aveCount++;
					}							
				}
				else {
					dX = 0;
					dY = 0;
					lineColor = CV_RGB(0,0,0);		// bad (large) lines are shown as black
				}

				if (!s->showVectorsUsed && !s->cleanOutput) {
					// Show the LK points on the screen as green.
					cvCircle( image, cvPointFrom32f(s->pointsCurr[i]), 2, CV_RGB(0,255,0), -1, 8,0);
					// Show the movement of the points
					cvLine( image, cvPointFrom32f(s->pointsCurr[i]), cvPointFrom32f(s->pointsPrev[i]), lineColor, 1, 8, 0);
				}

			}
		}

		// Get the average value for the max histogram bin
		if (aveCount > 0) {
			invDiv = 1.0f / (float)aveCount;
			aveX = aveX * invDiv;	// divide by aveCount
			aveY = aveY * invDiv;	// divide by aveCount
		}
		// Make sure there were atleast a few consistent vectors. If all the vectors were randomly distributed, then assume its just noise and dont move.
		if (maxCount < 5) {
			aveX = 0.0f;
			aveY = 0.0f;
		}
		STOP_TIMING(gmv);	// Stop recording the timing.

		// Set the Global Motion Vector to the average of the most common Local Motion Vector.
		s->gmvX = aveX;
		s->gmvY = aveY;
		if (!s->cleanOutput)
			printf("Stabilizer's Global Motion Vector using histogram: (%.2f,%.2f) (maxCount %d)\n", s->gmvX, s->gmvY, maxCount);

		// Show the Local Motion Vectors that were used for the Global Motion Vector.
		if (s->showVectorsUsed && !s->cleanOutput) {
			for (i=0; i<s->count; i++) {
				float dX, dY;
				float distSquared;
				int indexX, indexY;
				CvScalar lineColor;
				CvScalar pointColor;
				if (s->status[i] && s->valid[i]) {	// Make sure its an existing vector
					dX = s->pointsCurr[i].x - s->pointsPrev[i].x;
					dY = s->pointsCurr[i].y - s->pointsPrev[i].y;
					//printf("Stabilizer: curr=%f,%f, prev=%f,%f\n", pointsCurr[i].x, pointsCurr[i].y, pointsPrev[i].x, pointsPrev[i].y);
					lineColor = CV_RGB(255,0,255);	// unused lines are shown as purple
					pointColor = CV_RGB(0,150,0);	// unused points are shown as dark green

					// Make sure its not massive
					distSquared = dX*dX + dY*dY;
					if (distSquared < MAX_LOCAL_MOVEMENT*MAX_LOCAL_MOVEMENT) {
						// Add the Local Motion Vector to the entries for the Global Motion Vector
						indexX = cvRound(dX * (float)(MAP_RESOLUTION-1) / (float)MAX_LOCAL_MOVEMENT) + MAP_RESOLUTION;
						indexY = cvRound(dY * (float)(MAP_RESOLUTION-1) / (float)MAX_LOCAL_MOVEMENT) + MAP_RESOLUTION;

						// If this Local Motion Vector is in the most popular cell bin (used for the Global Motion Vector),
						// then draw it differently.
						if (indexX == indexXused && indexY == indexYused) {
							lineColor = CV_RGB(0,0,255);	// Local Motion Vectors that are used for the GMV are shown as /*white*/ blue.
							pointColor = CV_RGB(0,255,0);		// draw the used points as bright green.
						}
					}
					else {
						dX = 0;
						dY = 0;
						lineColor = CV_RGB(0,0,0);		// bad (large) lines are shown as black
						pointColor = CV_RGB(140,140,140);	// draw the bad points as grey
					}

					// Show the LK points on the screen.
					cvCircle( image, cvPointFrom32f(s->pointsCurr[i]), 2, pointColor, -1, 8,0);
					// Show the movement of the points
					cvLine( image, cvPointFrom32f(s->pointsCurr[i]), cvPointFrom32f(s->pointsPrev[i]), lineColor, 1, 8, 0);
				}
			}

		} // end if showVectorsUsed

		// Display the Global Motion Vector as blue
		if (!s->cleanOutput) {
			drawCross(image, cvPoint(image->width/2, image->height/2), 6, CV_RGB(0,0,255));
			cvLine( image, cvPoint(image->width/2, image->height/2), cvPoint(image->width/2 - cvRound(s->gmvX * 4.0f), image->height/2 - cvRound(s->gmvY * 4.0f)), CV_RGB(0,0,255), 2, 8, 0);
		}
		STOP_TIMING(gmvProcess);

	} // end if (count > 0)

	// Do motion compensation by translating the image by the global motion vector.
	START_TIMING(compensate);

	// Limit the GMV
	if (s->gmvX * s->gmvX + s->gmvY * s->gmvY > MAX_GLOBAL_MOVEMENT*MAX_GLOBAL_MOVEMENT) {
		s->gmvX = 0;
		s->gmvY = 0;
	}

	// Translate the image using the Global Motion Vector
	f = MOTION_COMPENSATION_STRENGTH;	// Strength of the motion compensation
	// Combine this frame's transformation with the previous transformations
	if (s->comparePrevFrame) {
		// Keep track of the global ROI without cropping to within the image.
		s->global_roi.x = s->global_roi.x + cvRound(s->gmvX * f);
		s->global_roi.y = s->global_roi.y + cvRound(s->gmvY * f);
	}
	else {
		s->global_roi.x = cvRound(s->gmvX * f);
		s->global_roi.y = cvRound(s->gmvY * f);
	}
	// Allow the region to slowly move back towards the default position.
	if (s->elasticReset) {
		s->global_roi.x = cvRound(0.9f * s->global_roi.x);
		s->global_roi.y = cvRound(0.9f * s->global_roi.y);
	}

	s->roi.x = s->global_roi.x;	// use the global roi
	s->roi.y = s->global_roi.y;	//		"		"
	s->roi.width = image->width + s->roi.x;
	s->roi.height = image->height + s->roi.y;

	// Make sure the displayed image is within the viewing dimensions
	s->roi = cropRect(s->roi, image->width, image->height);

	if ( (s->roi.x < 0) || (s->roi.y < 0) || (s->roi.x + s->roi.width-1 >= image->width) || (s->roi.y + s->roi.height-1 >= image->height) || (s->roi.width < 0) || (s->roi.height < 0) ) {
		printf("Stabilizer ERROR!! ROI=(%d,%d) %dx%d\n", s->roi.x, s->roi.y, s->roi.width, s->roi.height);
		return NULL;
	}

	// Free the previous resources, since the images are about to change dimensions
	if (s->imageTmp)
		cvReleaseImage(&s->imageTmp);

	if (s->roi.width > 0 && s->roi.height > 0) {
		static BOOL imgOutInit = FALSE;
		if(!imgOutInit)
		{
			//cvSet(s->imageOut, CV_RGB(200,255,200), 0);	// Set a default background color
			imgOutInit = TRUE;
		}

		s->imageTmp = cropImage(image, s->roi);
		if (!s->imageTmp) {
			printf("Stabilizer ERROR: Couldnt create imageTmp!\n");
			return NULL;
		}

		// Set the desired position in the output image to paste the input image

		// Make sure the output image stays in the same position
		// If camera is moving right, then move the image to the left of the view.
		if (s->roi.x + s->roi.width-1 >= image->width-1)
			s->roi.x = 0;
		else	// If camera is moving left, then move the image to the right of the view.
			s->roi.x = image->width - s->roi.width;
			
		// If camera is moving down, then move the image to the top of the view.
		if (s->roi.y + s->roi.height-1 >= image->height-1)
			s->roi.y = 0;
		else	// If camera is moving up, then move the image to the bottom of the view.
			s->roi.y = image->height - s->roi.height;
			
		// Make sure the displayed image is within the viewing dimensions
		s->roi = cropRect(s->roi, image->width, image->height);

		if (s->roi.width > 0 && s->roi.height > 0) {
			//printRect(roi, "ROI");
			// Use the ROI as the output image
			if (REPLICATE_BORDERS) {
				// Use the ROI as the output image, but replicate its border pixels across the remaining image.
				cvCopyMakeBorder(s->imageTmp, s->imageOut, cvPoint(s->roi.x, s->roi.y), IPL_BORDER_REPLICATE, cvScalarAll(0));
			}
			else {
				// Use the ROI as the output image
				cvSetImageROI(s->imageOut, s->roi);
				cvCopy(s->imageTmp, s->imageOut, NULL);
				cvResetImageROI(s->imageOut);
			}
		}
		else {
			printf("Stabilizer: Also an empty ROI=(%d,%d) %dx%d\n", s->roi.x, s->roi.y, s->roi.width, s->roi.height);
		}
	}
	else {
		printf("Stabilizer: Empty ROI=(%d,%d) %dx%d\n", s->roi.x, s->roi.y, s->roi.width, s->roi.height);
	}

	// If no output has been generated, then show the input image
	if (!s->showCompensation)
	{
		cvCopy(image, s->imageOut, NULL);
		//printf("No compensation shown\n");
	}

	// Use the current data as the "previous frame" on the next iteration, and re-use the "previous frame" array as the current frame next time (double-buffering).
	if (s->need_new_corners || s->comparePrevFrame ) {
		CV_SWAP( s->prev_grey, s->grey, s->swap_temp );
		CV_SWAP( s->prev_pyramid, s->pyramid, s->swap_temp );
		CV_SWAP( s->pointsPrev, s->pointsCurr, s->swap_points );
	}

	STOP_TIMING(compensate);

	// Show which points are valid or invalid / bad.
	if (PRINT_VALID_POINTS) {
		for (i=0; i<s->count; i++) {
			if (s->status[i] && s->valid[i])
				printf(".");
			else if (s->valid[i])
				printf("_");
			else if (s->status[i])
				printf("|");
			else
				printf("X");
		}
		if (s->count > 0)
			printf("\n");
	}

	// Now that the initial frame has been processed, compare it to the next frames.
	s->need_new_corners = 0;

	return s->imageOut;
}


// Show the internal Video Stabilization timings.
void printStabilizerTimings(StabilizerData *s)
{
	if (!s) {
		printf("Stabilizer ERROR in printStabilizerTimings(): No StabilizerData was given!\n");
		exit(1);
	}
	if (!s->initialized) {
		printf("Stabilizer ERROR in printStabilizerTimings(): StabilizerData was not initialized!\n");
		exit(1);
	}

	printf("Average Corner Detection time = %f ms.\n", GET_AVERAGE_TIMING(corners) );
	printf("Average Corner Sub-Pixel time = %f ms.\n", GET_AVERAGE_TIMING(cornersSubPixel) );
	printf("Average Corner Total time = %f ms.\n", GET_AVERAGE_TIMING(cornerProcess) );
	printf("Average Optical Flow time = %f ms.\n", GET_AVERAGE_TIMING(opticalFlow) );
	printf("Average GMV Calculation time = %f ms.\n", GET_AVERAGE_TIMING(gmv) );
	printf("Average GMV Total time = %f ms.\n", GET_AVERAGE_TIMING(gmvProcess) );
	printf("Average Motion Compensation time = %f ms.\n", GET_AVERAGE_TIMING(compensate) );
}


// Free the Stabilizer's used resources.
void finishStabilizer(StabilizerData *s)
{
	if (!s) {
		printf("Stabilizer ERROR in finishStabilizer(): No StabilizerData was given!\n");
		exit(1);
	}
	if (!s->initialized) {
		// Dont free anything
		return;
	}

	// Free the images
	if (s->grey)
		cvReleaseImage( &s->grey );
	if ( s->prev_grey)
		cvReleaseImage( & s->prev_grey );
	if (s->imageOut)
		cvReleaseImage( &s->imageOut );
	if (s->eig)
		cvReleaseImage( &s->eig );
	if (s->temp)
		cvReleaseImage( &s->temp );
	if (s->pyramid)
		cvReleaseImage( &s->pyramid );
	if (s->prev_pyramid)
		cvReleaseImage( &s->prev_pyramid );
		
	// Free the arrays
	if (s->pointsCurr)
		cvFree( &s->pointsCurr );
	if (s->pointsPrev)
		cvFree( &s->pointsPrev );
	if (s->status)
		cvFree( &s->status );
	if (s->valid)
		cvFree( &s->valid );

}
