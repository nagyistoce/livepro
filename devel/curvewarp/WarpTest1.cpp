#include <QApplication>
#include <QtGui>
#include <opencv/cv.h>
#include <opencv/highgui.h>


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


int main(int argc, char **argv)
{
	QApplication app(argc,argv);
	
	// Load image
	QImage sourceImage("football_donut.jpg");
	
	// Source points
// 	QPolygonF sourcePoly = QPolygonF()
// 		<< QPointF(  0.0,  0.0)
// 		<< QPointF(100.0,  0.0)
// 		<< QPointF( 20.0, 20.0)
// 		<< QPointF( 10.0, 20.0);
	
	// 345,269 - 511,148
	// 563,237 - 461,366
	
	QPolygonF sourcePoly = QPolygonF() 
		<< QPointF(345.0,  269.0)
		<< QPointF(511.0,  148.0)
		<< QPointF(563.0,  237.0)
		<< QPointF(502.0,  280.0)
		<< QPointF(461.0,  366.0);

/*		<< QPointF(1000.0,  588.0)
		<< QPointF(1271.0,  603.0)
		<< QPointF(1231.0,  873.0)
		<< QPointF( 929.0,  835.0)*/
		;
		
	// Source points
	QPolygonF destPoly = QPolygonF() 
		<< QPointF(100.0,  100.0)
		<< QPointF(400.0,  100.0)
		<< QPointF(400.0,  250.0)
		<< QPointF(250.0,  250.0)
		<< QPointF(100.0,  250.0);
		
	// Setup an OpenCV output window
	#define CORNER_EIG     "WarpTetst1"
	cvNamedWindow(CORNER_EIG, 0); // allow the window to be resized
	
	// Convert source to openCv image
	IplImage *cvImg;
	if(sourceImage.format() != QImage::Format_ARGB32)
		sourceImage = sourceImage.convertToFormat(QImage::Format_ARGB32);
		
	cvImg = cvCreateImageHeader( cvSize(sourceImage.width(), sourceImage.height()), IPL_DEPTH_8U, 4);
	cvImg->imageData = (char*)sourceImage.bits();

// 	// Allocate openCv matrix
 	CvMat* src_mat = cvCreateMat( sourcePoly.size(), 2, CV_32FC1 );
 	CvMat* dst_mat = cvCreateMat( destPoly.size(),   2, CV_32FC1 );
// 
	CvPoint2D32f cvsrc[20];
 	CvPoint2D32f cvdst[20];

	// Copy source points
	int counter=0;
	//#define MAX_CORNERS 4
	//CvPoint2D32f cornerTmp[MAX_CORNERS] = {0};
	foreach(QPointF point, sourcePoly)
	{
		cvsrc[counter].x = point.x();
		cvsrc[counter].y = point.y();
		counter++;
	}
	//cvSetData( src_mat, cornerTmp, sizeof(CvPoint2D32f));
	
	// Copy destination points
	counter = 0;
	foreach(QPointF point, destPoly)
	{
		cvdst[counter].x = point.x();
		cvdst[counter].y = point.y();
		counter++;
	}
	//cvSetData( dst_mat, cornerTmp, sizeof(CvPoint2D32f));

// 	CvPoint2D32f cvsrc[4];
// 	CvPoint2D32f cvdst[4];
// 	
// 	QRectF target = QRectF(sourceImage.rect());
// 	
// 	// Warp source coordinates
// 	cvsrc[0].x = target.left();
// 	cvsrc[0].y = target.top();
// 	cvsrc[1].x = target.right();
// 	cvsrc[1].y = target.top();
// 	cvsrc[2].x = target.right();
// 	cvsrc[2].y = target.bottom();
// 	cvsrc[3].x = target.left();
// 	cvsrc[3].y = target.bottom();
// 	
// 	// Handle flipping the corner translation points so that the top left is flipped
// 	// without the user having think about flipping the point inputs.
// 	int     cbl = 2,
// 		cbr = 3,
// 		ctl = 0,
// 		ctr = 1;
// 		
// 	QPolygonF m_cornerTranslations;
// 	m_cornerTranslations
// 		<<  QPointF(500,0)
// 		<<  QPointF(-500,0)
// 		<<  QPointF(0,0)
// 		<<  QPointF(0,0);
// 
// 	// Warp destination coordinates
// 	cvdst[0].x = target.left()	+ m_cornerTranslations[ctl].x();
// 	cvdst[0].y = target.top()	+ m_cornerTranslations[ctl].y();
// 	cvdst[1].x = target.right()	- m_cornerTranslations[ctr].x();
// 	cvdst[1].y = target.top()	+ m_cornerTranslations[ctr].y();
// 	cvdst[2].x = target.right()	- m_cornerTranslations[cbr].x();
// 	cvdst[2].y = target.bottom()	- m_cornerTranslations[cbr].y();
// 	cvdst[3].x = target.left()	+ m_cornerTranslations[cbl].x();
// 	cvdst[3].y = target.bottom()	- m_cornerTranslations[cbl].y();
// 
// 
// 	//we create a matrix that will store the results
// 	//from openCV - this is a 3x3 2D matrix that is
// 	//row ordered
// 	//CvMat * translate = cvCreateMat(3,3,CV_32FC1);
// 
// 	//this is the slightly easier - but supposidly less
// 	//accurate warping method
// 	//cvWarpPerspectiveQMatrix(cvsrc, cvdst, translate);
// 
// 
// 	//for the more accurate method we need to create
// 	//a couple of matrixes that just act as containers
// 	//to store our points  - the nice thing with this
// 	//method is you can give it more than four points!
// 
// 	CvMat* src_mat = cvCreateMat( 4, 2, CV_32FC1 );
// 	CvMat* dst_mat = cvCreateMat( 4, 2, CV_32FC1 );

	//copy our points into the matrixes
	cvSetData( src_mat, cvsrc, sizeof(CvPoint2D32f));
	cvSetData( dst_mat, cvdst, sizeof(CvPoint2D32f));
	
	
	
	// Calculate convolution matrix
	CvMat* translate = cvCreateMat(3,3,CV_32FC1);
	cvFindHomography(src_mat, dst_mat, translate);

	// Prepare output image
	IplImage* outCvImg = 0;
	outCvImg = cvCloneImage( cvImg );
	outCvImg->origin = cvImg->origin;
// 	outCvImg = cvCreateImageHeader( cvSize(sourceImage.width(), sourceImage.height()), IPL_DEPTH_8U, 4);
// 	outCvImg->imageData = (char*)sourceImage.bits();
	cvZero( outCvImg );
	
	// Wrap perspective
	cvWarpPerspective( cvImg, outCvImg, translate );
	
	// display final image
	cvShowImage(CORNER_EIG, outCvImg);
	
	//we need to copy these values
	//from the 3x3 2D openCV matrix which is row ordered
	//
	// ie:   [0][1][2] x
	//       [3][4][5] y
	//       [6][7][8] w

	//to openGL's 4x4 3D column ordered matrix
	//        x  y  z  w
	// ie:   [0][3][ ][6]   [1-4]
	//       [1][4][ ][7]   [5-8]
	//	 [ ][ ][ ][ ]   [9-12]
	//       [2][5][ ][9]   [13-16]
	//
	//get the matrix as a list of floats
	float *matrix = translate->data.fl;
	
	qDebug() << "translate matrix: ";
	for(int i=0;i<9;i++)
	{
		printf("(%.03f) ",matrix[i]);
		if(i == 2 || i == 5 || i == 8)
			printf("\n");
	}


// 	m_warpMatrix[0][0] = matrix[0];
// 	m_warpMatrix[0][1] = matrix[3];
// 	m_warpMatrix[0][2] = 0.;
// 	m_warpMatrix[0][3] = matrix[6];
// 
// 	m_warpMatrix[1][0] = matrix[1];
// 	m_warpMatrix[1][1] = matrix[4];
// 	m_warpMatrix[1][2] = 0.;
// 	m_warpMatrix[1][3] = matrix[7];
// 
// 	m_warpMatrix[2][0] = 0.;
// 	m_warpMatrix[2][1] = 0.;
// 	m_warpMatrix[2][2] = 0.;
// 	m_warpMatrix[2][3] = 0.;
// 
// 	m_warpMatrix[3][0] = matrix[2];
// 	m_warpMatrix[3][1] = matrix[5];
// 	m_warpMatrix[3][2] = 0.;
// 	m_warpMatrix[3][3] = matrix[8];
	QImage outImage = IplImage2QImage(outCvImg);
	QMatrix m(matrix[0],matrix[1],matrix[3],matrix[4],matrix[6],matrix[7]);
	//QMatrix ( qreal m11, qreal m12, qreal m21, qreal m22, qreal dx, qreal dy )
	outImage = outImage.transformed(m, Qt::SmoothTransformation);
	
	QPainter p(&outImage);
	p.setPen(QPen(Qt::red,3.0));
	p.drawConvexPolygon(sourcePoly);
	
	p.setPen(QPen(Qt::green,3.0));
	p.drawConvexPolygon(destPoly);
	
	p.end();
	
	outImage = outImage.scaled(640,645);
	QLabel label;
	label.setPixmap(QPixmap::fromImage(outImage));
	label.show();
	label.raise();
	
	
	// wait for key press
	//cvWaitKey(99999);
	
	app.exec();
	
	// release matrix
	cvReleaseMat(&src_mat);
	cvReleaseMat(&dst_mat);
			
	cvDestroyWindow(CORNER_EIG);	
	
	
	return 0;
}
