#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <stdio.h>
#include <math.h>

//# ./undistort 0_0000.jpg 1367.451167 1367.451167 0 0 -0.246065 0.193617 -0.002004 -0.002056

int main(int argc, char **argv)
{


	//----------------------------------------
	/* Panoramic unwarp stuff */

//	ofVideoPlayer player;
//	ofxXmlSettings XML;
	//ofxQtVideoSaver *videoRecorder;
	int	currentCodecId;
	//string outputFileName;
	char *handyString;

/*	void computePanoramaProperties();
	void computeInversePolarTransform();
		
	void drawTexturedCylinder();
	void drawPlayer();
	void drawUnwarpedVideo();*/
		
//	bool testMouseInPlayer();
	bool bMousePressed;
	bool bMousePressedInPlayer;
	bool bMousepressedInUnwarped;
	bool bAngularOffsetChanged;
	bool bPlayerPaused;
	bool bCenterChanged;
	bool bSavingOutVideo;
	bool bSaveAudioToo;
	int  nWrittenFrames;
	int  codecQuality;
	
	//ofImage unwarpedImage;
	//ofxCvColorImage	warpedImageOpenCV;
	//ofxCvColorImage unwarpedImageOpenCV;
	//ofxCvFloatImage srcxArrayOpenCV; 
	//ofxCvFloatImage srcyArrayOpenCV; 

	unsigned char *warpedPixels;
	unsigned char *unwarpedPixels;
	
	int   warpedW;
	int   warpedH;
	float unwarpedW;
	float unwarpedH;
	float warpedCx;
	float warpedCy;
	float savedWarpedCx;
	float savedWarpedCy;
	float savedAngularOffset;
	float angularOffset;

	float maxR;
	float minR;
	float maxR_factor;
	float minR_factor;
	int   interpMethod; 
	float playerScaleFactor;

	unsigned char *blackColor;
	CvScalar	blackOpenCV;
	IplImage	*warpedIplImage;
	IplImage	*unwarpedIplImage;

	float *xocvdata;
	float *yocvdata;

	float yWarpA; // for parabolic fit for Y unwarping
	float yWarpB;
	float yWarpC;

	//-----------------------------------
	/* For the texture-mapped cylinder */
	//ofTexture unwarpedTexture;
	int   cylinderRes;
	float *cylinderX;
	float *cylinderY;
	float cylinderWedgeAngle;
	float blurredMouseX;
	float blurredMouseY;
	
	//-----------------------------------
	//-----------------------------------
	
	bMousePressed   = false;
	bCenterChanged  = false;
	bPlayerPaused   = false;
	bAngularOffsetChanged = false;
	bMousePressedInPlayer = false;
	bMousepressedInUnwarped = false;
	bSavingOutVideo = false;
	bSaveAudioToo   = false;
	nWrittenFrames  = 0;
	handyString = new char[128];
	//outputFileName = "output.mov";

	maxR_factor   = 0.96; //XML.getValue("MAXR_FACTOR", 0.96);
	minR_factor   = 0.16; //XML.getValue("MINR_FACTOR", 0.16);
	angularOffset = 0.0; //XML.getValue("ROTATION_DEGREES", 0.0);

	int loadedQuality  = 3; //XML.getValue("CODEC_QUALITY", 3);
	//loadedQuality = MIN(5, MAX(0, loadedQuality));
/*	int codecQualities[] = {
		OF_QT_SAVER_CODEC_QUALITY_MIN,
		OF_QT_SAVER_CODEC_QUALITY_LOW,
		OF_QT_SAVER_CODEC_QUALITY_NORMAL,
		OF_QT_SAVER_CODEC_QUALITY_HIGH,
		OF_QT_SAVER_CODEC_QUALITY_MAX,             
		OF_QT_SAVER_CODEC_QUALITY_LOSSLESS
	};*/
	codecQuality = 0; //codecQualities[loadedQuality];

// 	player.loadMovie(XML.getValue("INPUT_FILENAME", "input.mov"));
// 	player.getDuration();

		
	unwarpedW = 1280; //(int) XML.getValue("OUTPUT_W", 1280);
	unwarpedH = 256; //(int) XML.getValue("OUTPUT_H", 256);

	// Interpolation method: 
	// 0 = CV_INTER_NN, 1 = CV_INTER_LINEAR, 2 = CV_INTER_CUBIC.
	interpMethod = 1; //(int) XML.getValue("INTERP_METHOD", 1); 
	//XML.setValue("INTERP_METHOD", (int) interpMethod);
	
	int bSaveAud = 0; //(int) XML.getValue("INCLUDE_AUDIO", 0); 
	bSaveAudioToo = (bSaveAud != 0);

	yWarpA =   0.1850;
	yWarpB =   0.8184;
	yWarpC =  -0.0028;


	//======================================
	// create data structures for unwarping
	blackOpenCV = cvScalarAll(0);

	char *sourceFile = argv[1];
	char *outputFile = argv[2];

	//IplImage *src = cvLoadImage(sourceFile, 1);
	warpedIplImage = cvLoadImage(sourceFile);
	
	CvSize warpedSize = cvGetSize(warpedIplImage);
	warpedW = warpedSize.width;
	warpedH = warpedSize.height;
	
	// The "warped" original source video produced by the Bloggie.
	//warpedW = 1280; //player.width;
	//warpedH =  720; //player.height;
	int nWarpedBytes = warpedW * warpedH * 3;
	printf("warpedW = %d, warpedH = %d\n", warpedW, warpedH);
	
	//warpedImageOpenCV.allocate(warpedW, warpedH);
	//warpedIplImage = cvCreateImage(CvSize(warpedW, warpedH), src->depth, src->nChannels);
	warpedPixels = new unsigned char[nWarpedBytes];	
	//warpedIplImage = warpedImageOpenCV.getCvImage();
	cvSetImageROI(warpedIplImage, cvRect(0, 0, warpedW, warpedH));

	int nUnwarpedPixels = (int)(unwarpedW * unwarpedH);
	int nUnwarpedBytes  = (int)(unwarpedW * unwarpedH * 3);
	//unwarpedImage.allocate(unwarpedW, unwarpedH, OF_IMAGE_COLOR);
	unwarpedPixels = new unsigned char[nUnwarpedBytes];
	//unwarpedTexture.allocate(unwarpedW, unwarpedH,GL_RGB);
	IplImage *unwarpedImage = cvCreateImage(cvGetSize(warpedIplImage), warpedIplImage->depth, warpedIplImage->nChannels);
	unwarpedIplImage = unwarpedImage;
	
// 	srcxArrayOpenCV.allocate(unwarpedW, unwarpedH);
// 	srcyArrayOpenCV.allocate(unwarpedW, unwarpedH);
	IplImage *srcxArrayOpenCV = cvCreateImage(cvGetSize(warpedIplImage), IPL_DEPTH_32F, 1);
	IplImage *srcyArrayOpenCV = cvCreateImage(cvGetSize(warpedIplImage), IPL_DEPTH_32F, 1);
	//cvSetROI(srcxArrayOpenCV, 0, 0, unwarpedW, unwarpedH);
	//cvSetROI(srcyArrayOpenCV, 0, 0, unwarpedW, unwarpedH);
	
	xocvdata = (float*) srcxArrayOpenCV->imageData;
	yocvdata = (float*) srcyArrayOpenCV->imageData;

	playerScaleFactor = 1.0; //(float)(ofGetHeight() - unwarpedH)/(float)warpedH;
	savedWarpedCx = warpedCx = 2.0; //XML.getValue("CENTERX", warpedW / 2.0);
	savedWarpedCy = warpedCy = 2.0; //XML.getValue("CENTERY", warpedH / 2.0);
	savedAngularOffset = angularOffset;
	
	//---------------------------
	// cylinder vizualization properties
	cylinderRes = 90;
	cylinderWedgeAngle = 360.0 / (cylinderRes-1);
	cylinderX = new float[cylinderRes];
	cylinderY = new float[cylinderRes];
	// We're not doing opengl rendering right now, so we dont need this loop
/*	for (int i = 0; i < cylinderRes; i++) {
		cylinderX[i] = cos(ofDegToRad((float)i * cylinderWedgeAngle));
		cylinderY[i] = sin(ofDegToRad((float)i * cylinderWedgeAngle));
	}*/
	blurredMouseY = 0;
	blurredMouseX = 0;
	
	//videoRecorder = new ofxQtVideoSaver();
	currentCodecId = 16;

	//---------------------------
	// start it up
	maxR  = warpedH * maxR_factor / 2;
	minR  = warpedH * minR_factor / 2;

	//Used for the by hand portion and OpenCV parts of the shootout. 
	//For the by hand, use the normal unwarpedW width instead of the step
	//For the OpenCV, get the widthStep from the CvImage and use that for quarterstep calculation
	// we assert that the two arrays have equal dimensions, srcxArray = srcyArray
	float radius, angle;
	float circFactor = 0 - (M_PI*2)/(float)unwarpedW;
	float difR = maxR-minR;
	int   dstRow, dstIndex;
	
	// already got these pointers
	/*
	xocvdata = (float*) srcxArrayOpenCV.getCvImage()->imageData; 
	yocvdata = (float*) srcyArrayOpenCV.getCvImage()->imageData;
	*/ 
	
	#define DEG_TO_RAD 0.01745329238
	
	for (int dsty=0; dsty<unwarpedH; dsty++)
	{
		float y = ((float)dsty/(float)unwarpedH);
		float yfrac = yWarpA*y*y + yWarpB*y + yWarpC;
		yfrac = MIN(1.0, MAX(0.0, yfrac)); 

		radius = (yfrac * difR) + minR;
		dstRow = (int)(dsty * unwarpedW); 	
		
		for (int dstx=0; dstx<unwarpedW; dstx++)
		{
			dstIndex = dstRow + dstx;
			angle    = ((float)dstx * circFactor) + (DEG_TO_RAD * angularOffset);
			
			xocvdata[dstRow + dstx] = warpedCx + radius*cosf(angle);
			yocvdata[dstRow + dstx] = warpedCy + radius*sinf(angle);
		}
	}
	
	// Pointers point to the memory already, n oneed to 'set'
	//srcxArrayOpenCV.setFromPixels(xocvdata, unwarpedW, unwarpedH);
	//srcyArrayOpenCV.setFromPixels(yocvdata, unwarpedW, unwarpedH);
	
	// warpedIplImage already has the image since we loaded it with cvLoadImage
	//memcpy(warpedPixels, player.getPixels(), warpedW*warpedH*3);
	//warpedIplImage->imageData = (char*) warpedPixels; 
	
	cvRemap(warpedIplImage,  unwarpedIplImage, 
			srcxArrayOpenCV, //.getCvImage(), 
			srcyArrayOpenCV, //.getCvImage(), 
			interpMethod | CV_WARP_FILL_OUTLIERS, blackOpenCV );
		
	//unwarpedPixels = (unsigned char*) unwarpedIplImage->imageData;
	//unwarpedImage.setFromPixels(unwarpedPixels, unwarpedW, unwarpedH, OF_IMAGE_COLOR, true);
	//unwarpedTexture.loadData(unwarpedPixels, unwarpedW, unwarpedH, GL_RGB);
	
	
	#define CORNER_EIG     "WarpTetst1"
	cvNamedWindow(CORNER_EIG, 0); // allow the window to be resized
	
	/*
	char *sourceFile = argv[1];
	char *outputFile = argv[2];
	float 
// 		fx = 500,
// 		fy = 334,
// 		cx = 500/2,
// 		cy = 334/2,
// 		k1 = -1.05,
// 		k2 =  0.95,
// 		p1 =  0,
// 		p2 =  0;
		fx = 1367.451167,
		fy =  1367.451167,
		cx = 0,
		cy = 0,
		k1 =  -0.246065 ,
		k2 =  0.193617 ,
		p1 =  -0.002004 ,
		p2 =  -0.002056;
	
	CvMat* intrinsics = cvCreateMat(3, 3, CV_64FC1);
	cvZero(intrinsics);
	cvmSet(intrinsics, 0, 0, fx);
	cvmSet(intrinsics, 1, 1, fy);
	cvmSet(intrinsics, 2, 2, 0.0);
	cvmSet(intrinsics, 0, 2, cx);
	cvmSet(intrinsics, 1, 2, cy);
	
	CvMat* dist_coeffs = cvCreateMat(1, 4, CV_64FC1);
	cvZero(dist_coeffs);
	cvmSet(dist_coeffs, 0, 0, k1);
	cvmSet(dist_coeffs, 0, 1, k2);
	cvmSet(dist_coeffs, 0, 2, p1);
	cvmSet(dist_coeffs, 0, 3, p2);
	
	IplImage *src = cvLoadImage(sourceFile, 1);
	
	IplImage *dst = cvCreateImage(cvGetSize(src), src->depth, src->nChannels);
	IplImage *mapx = cvCreateImage(cvGetSize(src), IPL_DEPTH_32F, 1);
	IplImage *mapy = cvCreateImage(cvGetSize(src), IPL_DEPTH_32F, 1);
	//cvInitUndistortMap(intrinsics, dist_coeffs, mapx, mapy);
	
	cvRemap(src, dst, mapx, mapy, CV_INTER_LINEAR + CV_WARP_FILL_OUTLIERS,  cvScalarAll(0));
	//cvUndistort2(src, dst, intrinsics, dist_coeffs, 0);
	
	cvShowImage(CORNER_EIG, dst);
	*/
	cvShowImage(CORNER_EIG, unwarpedImage);	
	
	cvSaveImage(outputFile, unwarpedImage); //, 0);
	
	cvWaitKey(99999);
	//while(1){ } 
	
	cvDestroyWindow(CORNER_EIG);
}
