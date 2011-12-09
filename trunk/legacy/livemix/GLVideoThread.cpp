#include "GLVideoThread.h"
#include <QTime>

#include <QtGui>
#include <QtOpenGL>
#include <QColor>

#include "VideoOutputWidget.h"

#include "../livemix/VideoThread.h"
#include "../livemix/CameraThread.h"

GLVideoThread::GLVideoThread(VideoOutputWidget *gl) 
	: QThread(), glw(gl)
{
	doRendering = true;
	doResize = false;
	newFrame = false;
	lastFrameTime = 0;
	time.start();
	
	frames = 0;
	
	frame.image = QImage( 16, 16, QImage::Format_RGB32 );
	frame.image.fill( Qt::green );
		
// 	#ifdef Q_OS_WIN
// 		QString defaultCamera = "vfwcap://0";
// 	#else
// 		QString defaultCamera = "/dev/video0";
// 	#endif
// 	
// 	videoSource = CameraThread::threadForCamera(defaultCamera);
// 	
// 	if(videoSource)
// 	{
// 		CameraThread *camera = dynamic_cast<CameraThread*>(videoSource);
// 		camera->setFps(30);
// 	}
// 	else
// 	{
// 	 	videoSource = new VideoThread();
// 		dynamic_cast<VideoThread*>(videoSource)->setVideo("../data/Seasons_Loop_3_SD.mpg");
// 	}
// 	
// 	videoSource->start();
// 	setVideoSource(videoSource);
	
}

void GLVideoThread::setVideoSource(VideoSource *source)
{
	QMutexLocker lock(&videoSourceMutex);
	if(videoSource)
		disconnect(this,0,videoSource,0);
	
	videoSource = source;
	connect(videoSource, SIGNAL(frameReady()), this, SLOT(frameReady()), Qt::QueuedConnection);
}

void GLVideoThread::stop()
{
	doRendering = false;
}

void GLVideoThread::resizeViewport(const QSize &size)
{
	QMutexLocker lock(&resizeMutex);
	w = size.width();
	h = size.height();
	doResize = true;
}    

void GLVideoThread::frameReady()
{
	//QMutexLocker lock(&videoMutex);
	QMutexLocker lock(&videoSourceMutex);
	frame = videoSource->frame();
	newFrame = true;
}


void GLVideoThread::run()
{
	srand(QTime::currentTime().msec());
	xrot = 0; //rand() % 360;
	yrot = 0; //rand() % 360;
	zrot = 0; //rand() % 360;
	
	xscale = 1;
	yscale = 1;
	zscale = 1;
	
	xscaleInc = 0.1;
	yscaleInc = 0.2;
	zscaleInc = 0.4;
	
	glw->makeCurrent();
	
// 	glMatrixMode(GL_PROJECTION);
// 	glLoadIdentity();
// 	glOrtho(-5.0, 5.0, -5.0, 5.0, 1.0, 100.0);
// 	glMatrixMode(GL_MODELVIEW);
// 	glViewport(0, 0, 100, 100);
// 	glClearColor(0.0, 0.0, 0.0, 1.0);
// 	glShadeModel(GL_SMOOTH);
// 	glEnable(GL_DEPTH_TEST);
// 	glEnable(GL_TEXTURE_2D);
	
	QImage texOrig = frame.image, 
		texGL;
// 	if ( !texOrig.load( "me2.jpg" ) )
// 	{
// 		texOrig = QImage( 16, 16, QImage::Format_RGB32 );
// 		texOrig.fill( Qt::green );
// 	}
	
// 	if(texOrig.format() != QImage::Format_RGB32)
// 		texOrig = texOrig.convertToFormat(QImage::Format_RGB32);
	
	texGL = QGLWidget::convertToGLFormat( texOrig );
	glGenTextures( 1, &texture[0] );
	glBindTexture( GL_TEXTURE_2D, texture[0] );
	glTexImage2D( GL_TEXTURE_2D, 0, 3, texGL.width(), texGL.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, texGL.bits() );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	
	QSize textureSize = texGL.size();
	
	
	glEnable(GL_TEXTURE_2D);					// Enable Texture Mapping ( NEW )
	glShadeModel(GL_SMOOTH);					// Enable Smooth Shading
	//glClearColor(0.0f, 0.0f, 1.0f, 1.0f);				// Black Background
	glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
	glClearDepth(1.0f);						// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);					// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);						// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);		// Really Nice Perspective Calculations
	glViewport(0, 0, 320,240);
	
	float opacity = 0.90;
	glColor4f(opacity,opacity,opacity,opacity);			// Full Brightness, 50% Alpha ( NEW )
	//glBlendFunc(GL_SRC_ALPHA,GL_ONE);				// Blending Function For Translucency Based On Source Alpha Value ( NEW )
	glEnable (GL_BLEND); 
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glBlendFunc(GL_ONE, GL_ZERO);
	//glDisable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);


	int sleep = 1000/60;
	while (doRendering) 
	{
		
		if (doResize) 
		{
			resizeMutex.lock();
			glViewport(0, 0, w, h);
			doResize = false;
			
			if(h == 0)
				h = 1;
			
			glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
			glLoadIdentity();							// Reset The Projection Matrix
		
			// Calculate The Aspect Ratio Of The Window
			gluPerspective(45.0f,(GLfloat)w/(GLfloat)h,0.1f,100.0f);
		
			glMatrixMode(GL_MODELVIEW);						// Select The Modelview Matrix
			glLoadIdentity();							// Reset The Modelview Matrix
			qDebug() << "Resized: "<<w<<"x"<<h;
			resizeMutex.unlock();
		}
		
		
		
		// Rendering code goes here
		
		//qDebug() << "rot:"<<xrot<<yrot<<zrot;
	
 		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear The Screen And The Depth Buffer
		glLoadIdentity();									// Reset The View
		glTranslatef(0.0f,0.0f,-5.0f);
	
 		glRotatef(xrot,1.0f,0.0f,0.0f);
 		glRotatef(yrot,0.0f,1.0f,0.0f);
 		glRotatef(zrot,0.0f,0.0f,1.0f);
	
		//glScalef(xscale, yscale, zscale);
	
		
		
		
		if(newFrame) // && (time.elapsed() - lastFrameTime) >= frame.holdTime)
		{
			lastFrameTime = time.elapsed();
			//QMutexLocker lock(&resizeMutex);
			sleep = qMax(1000/60, frame.holdTime);
			
			
			//texture[0] = glw->bindTexture( frame.image );
			
			texGL = QGLWidget::convertToGLFormat( frame.image );
			if(textureSize != texGL.size())
			{
				glTexImage2D( GL_TEXTURE_2D, 0, 3, texGL.width(), texGL.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, texGL.bits() );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
				qDebug() << "Texutre resized from "<<textureSize<<" to "<<texGL.size();
				
				textureSize = texGL.size();
			}
			else
			{
				glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, texGL.width(), texGL.height(), GL_RGBA, GL_UNSIGNED_BYTE, texGL.bits());
				
			}
			
			// update texture
		}
		
		glBindTexture(GL_TEXTURE_2D, texture[0]);
		
		glBegin(GL_QUADS);
			// Front Face
			glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
			// Back Face
			glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
			// Top Face
			glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
			// Bottom Face
			glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
			// Right face
			glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
			// Left Face
			glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
		glEnd();
	
		xrot+=0.3f;
		yrot+=0.2f;
		zrot+=0.4f;
		
// 		xscale += xscaleInc;
// 		yscale += yscaleInc;
// 		zscale += zscaleInc;
// 		
// 		if(xscale < 0.1 || xscale >= 10);
// 			xscaleInc *= -1;
// 			
// 		if(yscale < 0.1 || yscale >= 10);
// 			yscaleInc *= -1;
// 			
// 		if(zscale < 0.1 || zscale >= 10);
// 			zscaleInc *= -1;
		
		//glw->renderText(10, 10, qPrintable(QString("%1 fps").arg(framesPerSecond)));
    		
    		if (!(frames % 100)) 
    		{
			QString framesPerSecond;
			framesPerSecond.setNum(frames /(time.elapsed() / 1000.0), 'f', 2);

			qDebug() << this << "FPS: "<<framesPerSecond;

			time.start();
			frames = 0;
			
			lastFrameTime = time.elapsed();
		}
		
		frames ++;

		glw->swapBuffers();
		//msleep(1000/80);
		//msleep(sleep);
	}
}


