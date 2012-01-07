#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <vector> // for std::Vector

int main(int argc, char **argv)
{
	char *file = argv[1];
	if(!file)
	{
		printf("Usage: %s filename\n", argv[0]);
		exit(-1);
	}
	
	IplImage *pImg = cvLoadImage(file);
	
	const int nSquaresAcross = 9; // known values from the calibration image
	const int nSquaresDown   = 7; // known values from the calibration image
	
	const int nCorners = (nSquaresAcross-1)*(nSquaresDown-1);
	
	CvSize szcorners = cvSize(nSquaresAcross-1,nSquaresDown-1);
	std::vector<CvPoint2D32f> vCornerList(nCorners);
	
	/* find corners to pixel accuracy */
	int cornerCount = 0;
	const int N = cvFindChessboardCorners(pImg, /* IplImage */
		szcorners,
		&vCornerList[0],
		&cornerCount);
	
	IplImage *pImgGray = cvCreateImage(cvGetSize(pImg), IPL_DEPTH_8U, 1);
	//CV_8UC1
	
	cvCvtColor(pImg, pImgGray, CV_BGR2GRAY); 
	
	/* should check that cornerCount==nCorners */
	/* sub-pixel refinement */
	cvFindCornerSubPix(pImg,
		&vCornerList[0],
		cornerCount,
		cvSize(5,5),
		cvSize(-1,-1),
		cvTermCriteria(CV_TERMCRIT_EPS,0,.001));
	
	/* iterate over vCornerList and print out */
	for(int i=0; i<nCorners; i++)
	{
		CvPoint2D32f p = vCornerList[i];
		printf("%f %f\n", p.x, p.y);
	}
	
}
