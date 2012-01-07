/*
 * TestStabilizer: Test the Video Stabilizer Module (Digital Image Stabilization), by Shervin Emami (shervin.emami@gmail.com) on 17th April 2010.
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

#ifndef _strcmpi
#define _strcmpi strcasecmp
#endif

//BOOL cleanOutput = FALSE;			// Whether to show overlayed data on the video such as motion vectors.
BOOL pause = FALSE;				// Whether to wait for a keypress between each frame of video.
//BOOL getNewFrames = TRUE;			// Whether to process new video frames, or pause on the existing frame.
BOOL bumpImage = FALSE;				// Whether to bump the input frame, for testing
//int i;
int FPS;  // Usually 25 or 30 fps.		// Speed of saved video stream.
int movieWidth;
int movieHeight;


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
#define GET_AVERAGE_TIMING(s)		(double)(countTally_##s ? timeTally_##s / ((double)countTally_##s * cvGetTickFrequency()*1000.0) : 0)


CvCapture* capture = 0;
CvVideoWriter *videoWriter = 0;



int runTestStabilizer(int argc, char **argv)
{
	IplImage *image = 0;
	IplImage *frame;
	IplImage *bumpedFrame;
	int i;
	BOOL night_mode = FALSE;			// Whether to show a black background instead of the input image.
	double currentFrame_ms;
	int iterations = 0;

	StabilizerData stabilizer;

	// Default Stabilizer Settings:
	BOOL cleanOutput = FALSE;		// Whether to show overlayed data on the video such as motion vectors.
	BOOL showVectorsUsed = TRUE;	// Whether to display the Local Motion Vectors that were used for determining the Global Motion Vector.
	BOOL showCompensation = TRUE;	// Whether to show the compensated image or the original image.
	BOOL comparePrevFrame = TRUE;	// Whether to do Optical Flow compared to the previous frame or the first frame.
	BOOL elasticReset = FALSE;		// Whether the ROI should slowly move back towards the default position.

	DECLARE_TIMING(wholeFrame);
	DECLARE_TIMING(capture);
	DECLARE_TIMING(preprocess);
	DECLARE_TIMING(output);
	DECLARE_TIMING(wholeFrameInOut);
	
	// Print a welcome message, and the OpenCV version.
	printf ("Welcome to StabilizeLK, compiled with OpenCV version %s (%d.%d.%d)\n",
	    CV_VERSION, CV_MAJOR_VERSION, CV_MINOR_VERSION, CV_SUBMINOR_VERSION);
	printf("usage: StabilizeLK [input_video] [output_video] [--CLEAN] [--JUST_RECORD]\n");

	// Get the input video source, from a camera or a video file
	if( argc == 1 || (argc >= 2 && strlen(argv[1]) == 1 && isdigit(argv[1][0]))) {
        	capture = cvCaptureFromCAM( argc == 2 ? argv[1][0] - '0' : 0 );
	}
	else if( argc >= 2 && argv[1][0] != '-') {
        	capture = cvCaptureFromAVI( argv[1] );
	}
	if (!capture) {
		fprintf(stderr, "Could not open the camera or video file.\n");
		return -1;
	}

	// Set the desired video file speed.
	FPS = cvRound( cvGetCaptureProperty(capture, CV_CAP_PROP_FPS) );
	if (FPS <= 0 || FPS > 200) {
		FPS = 15;	// Speed of my webcam
	}

	// Get an initial frame so we know what size the image will be. 
	frame = cvQueryFrame( capture );
	if (!frame) {
		fprintf(stderr, "Could not access the camera or video file.\n");
		return -1;
	}
	movieWidth = frame->width;
	movieHeight = frame->height;
	printf("Got a video source with a resolution of %dx%d at %d fps.\n", movieWidth, movieHeight, FPS);

	// Create a video output file if desired
	if (argc >= 3 && argv[2][0] != '-') {
		int fourCC_code = CV_FOURCC('M','J','P','G');	// M-JPEG codec (apparently isn't very reliable)
		//int fourCC_code = CV_FOURCC('P','I','M','1');	// MPEG 1 codec
		//int fourCC_code = CV_FOURCC('D','I','V','3');	// MPEG 4.3 codec
		//int fourCC_code = CV_FOURCC('I','2','6','3');	// H263I codec
		//int fourCC_code = CV_FOURCC('F','L','V','1');	// FLV1 codec
		int isColor = 1;				
		printf("Storing stabilized video into '%f'\n", argv[2]);
		videoWriter = cvCreateVideoWriter(argv[2], fourCC_code, FPS, cvGetSize(frame), isColor);
	}

	// Check the flags on the command line.
	for (i=2; i<argc; i++) {
		if (_strcmpi(argv[i], "--CLEAN") == 0) {
			cleanOutput = TRUE;
		}
		if (_strcmpi(argv[i], "--JUST_RECORD") == 0) {
			showCompensation = FALSE;
		}
		if (_strcmpi(argv[i], "--PAUSE") == 0) {
			pause = TRUE;
		}
	}

    printf( "Hot keys: \n"
            "\t ESC - quit the program\n"
            "\t r - auto-initialize tracking\n"
            "\t n - switch the \"night\" mode on/off\n"
			"\t s - Show compensation\n"
			"\t c - Dont draw extra tracking info\n"
			"\t e - Slowly move the region back towards the start\n"
			"\t SPACE - pause between each frame for a key press\n"
			"\t ENTER - resume from pause mode\n"
			"\t p - Compare previous frame\n"
            );

    cvNamedWindow( "StabilizeLK", 1 );
	//cvNamedWindow( "imageOut", 1 );
	cvNamedWindow("input", 1);

	// Initialise the data and allocate all the buffers.
    image = cvCreateImage( cvGetSize(frame), 8, 3 );	// RGB image
    image->origin = frame->origin;
	bumpedFrame = cvCreateImage( cvGetSize(image), 8, 3);

	printf("Initializing the Stabilizer.\n");
	initStabilizer(&stabilizer, image, showVectorsUsed, showCompensation, comparePrevFrame, elasticReset, cleanOutput);

	printf("Processing the input stream ... \n");
    while (1)
    {
		int delay_ms;
		int c;
		IplImage *imageStabilized = 0;

		START_TIMING(wholeFrameInOut);	// Record the timing.

		// Capture the frame, either from the camera or video file
		if (iterations > 0) {	// Use the first frame that was already obtained during initialization.
			START_TIMING(capture);	// Record the timing.
			frame = cvQueryFrame( capture );
			if( !frame )
				break;
			STOP_TIMING(capture);	// Stop recording the timing.
		}

		START_TIMING(preprocess);	// Record the timing.

		// Possibly bump the image, for testing
		if (bumpImage) {
			bumpImage = FALSE;
			#define BUMP 20
			//frame = cvQueryFrame( capture );
			cvSetImageROI(frame, cvRect(0,BUMP, frame->width, frame->height-BUMP));
			cvSetImageROI(bumpedFrame, cvRect(0,0, frame->width, frame->height-BUMP));
			cvCopy(frame, bumpedFrame, NULL);
			cvResetImageROI(frame);
			cvResetImageROI(bumpedFrame);
			//cvCopy(bumpedFrame, frame, NULL);
			frame = bumpedFrame;
		}
		cvShowImage("input", frame);

		// Process the video frame.
		START_TIMING(wholeFrame);	// Record the timing.

        cvCopy( frame, image, 0 );	// get a copy of the input image.

        if ( night_mode )
            cvZero( image );	// set the image black instead of showing the input image.

		STOP_TIMING(preprocess);	// Stop recording the timing.

		// Perform Video Stabilization on the image compared to the previous image
		imageStabilized = stabilizeImage(&stabilizer, image);

		// Draw a rect to show the desired ROI
		if (!cleanOutput) {
			cvRectangle(imageStabilized, cvPoint(stabilizer.roi.x, stabilizer.roi.y), cvPoint(stabilizer.roi.x + stabilizer.roi.width-1, stabilizer.roi.y + stabilizer.roi.height-1), CV_RGB(255,255,0), 1, 8, 0);	// yellow border
		}

		STOP_TIMING(wholeFrame);	// Record the timing.

		START_TIMING(output);	// Record the timing.

		// Either save to a video file or display on the screen
		if (imageStabilized) {
			if (videoWriter) {
				// Save to the output video file
				cvWriteFrame(videoWriter, imageStabilized);      // stabilized image
			}
			//else {
				// Display an image on the GUI
				cvShowImage( "StabilizeLK", imageStabilized );	// stabilized image
			//}
		}

		STOP_TIMING(output);	// Record the timing.

		STOP_TIMING(wholeFrameInOut);	// Stop recording the timing for this entire frame.

		// Make sure the video runs at roughly the correct speed.
		// Add a delay that would result in roughly the desired frames per second.
		timeDiff_wholeFrameInOut = (double)cvGetTickCount() - timeStart_wholeFrameInOut;
		currentFrame_ms = (double)(timeDiff_wholeFrameInOut / (cvGetTickFrequency()*1000.0));
		delay_ms = cvRound((1000 / FPS) - currentFrame_ms);	// Factor in how much time was used to process this frame already.
		if (delay_ms < 1)
			delay_ms = 1;	// Make sure there is atleast some delay, to allow OpenCV to do its internal processing.
		if (pause)
			delay_ms = 0;
		c = cvWaitKey(delay_ms);	// Wait for a keypress, and let OpenCV display its GUI.

		if( (char)c == 27 )	// Check if the user hit the 'Escape' key
            break;	// Quit

		switch( (char) c )
        {
        case 'r':
            stabilizer.need_new_corners = 1;	// recalculate which corner points to track
			break;
		case 'R':
			cvSetCaptureProperty(capture, CV_CAP_PROP_POS_FRAMES, 0); //reset movie to beginning
			break;
		case 'p':
			stabilizer.comparePrevFrame = 1 - stabilizer.comparePrevFrame;
			break;
		case 'e':
			stabilizer.elasticReset = 1 - stabilizer.elasticReset;
			break;
        case 'n':
            night_mode = 1 - night_mode;
            break;
		case 's':
			stabilizer.showCompensation = 1 - stabilizer.showCompensation;
			break;
		case 'c':
			stabilizer.cleanOutput = 1 - stabilizer.cleanOutput;
			cleanOutput = stabilizer.cleanOutput;
			break;
		case ' ':	// Space key: Pause each frame
			pause = TRUE;
			break;
		case 13:	// Enter key: Resume from pause
			pause = FALSE;
			break;
		case 'b':
			bumpImage = TRUE;
			break;
		case 't':	// Test

			break;
        default:
            ;
        }

/*
//if (countTally_wholeFrame % 99 == 0) 
//	pause = TRUE;
if (countTally_wholeFrame == 1) 
{	// reset movie
	cvSetCaptureProperty(capture, CV_CAP_PROP_POS_FRAMES, 0*400); //reset movie to beginning
	printf("Reset Movie at frame %d.\n", countTally_wholeFrame);
}		
if (countTally_wholeFrame % (10*29) == 0) 
{	// reset movie
	cvSetCaptureProperty(capture, CV_CAP_PROP_POS_FRAMES, 0*400); //reset movie to beginning
	printf("Reset Movie at frame %d.\n", countTally_wholeFrame);
}		
//printf("T = %d\n", countTally_wholeFrame);
*/

		iterations++;
	}

	// Show the internal Video Stabilization timings.
	printStabilizerTimings(&stabilizer);
	printf("Average Frame Capture time = %f ms.\n", GET_AVERAGE_TIMING(capture) );
	printf("Average Image Preprocessing time = %f ms.\n", GET_AVERAGE_TIMING(preprocess) );
	printf("Average Video Stabilization time = %f ms.\n", GET_AVERAGE_TIMING(wholeFrame) );
	printf("Average Output Display time = %f ms.\n", GET_AVERAGE_TIMING(output) );
	printf("Average Total frame time (including input & output) = %f ms.\n", GET_AVERAGE_TIMING(wholeFrameInOut) );

	// Free the resources.
	finishStabilizer(&stabilizer);

	// Free the remaining resources used when this function returns.
	return 0;
}


int main( int argc, char** argv )
{
	runTestStabilizer(argc, argv);

	// Free the resources used.
    cvReleaseCapture( &capture );
    cvDestroyWindow("StabilizeLK");
	//cvDestroyWindow("imageOut");
	cvReleaseVideoWriter( &videoWriter );

    return 0;
}

