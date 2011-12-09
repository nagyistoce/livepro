#include <QApplication>

#include "GLWidget.h"
#include "../livemix/VideoThread.h"
#include "../livemix/CameraThread.h"
#include "../livemix/MjpegThread.h"

#include "GLVideoDrawable.h"
#include "StaticVideoSource.h"
#include "TextVideoSource.h"

#include "GLImageDrawable.h"
#include "GLVideoLoopDrawable.h"
#include "GLVideoInputDrawable.h"
#include "GLVideoFileDrawable.h"
#include "GLVideoReceiverDrawable.h"
#include "GLTextDrawable.h"
#include "GLVideoMjpegDrawable.h"

//#define HAS_QT_VIDEO_SOURCE

#include "VideoSender.h"
#include "VideoReceiver.h"

#include "GLSceneGroup.h"

#include "GLEditorGraphicsScene.h"

#ifdef HAS_QT_VIDEO_SOURCE
#include "QtVideoSource.h"
#endif

#include "MetaObjectUtil.h"

#include "EditorWindow.h"



int main(int argc, char *argv[])
{

	QApplication app(argc, argv);
	
	qApp->setApplicationName("GLEditor");
	qApp->setOrganizationName("Josiah Bryan");
	qApp->setOrganizationDomain("mybryanlife.com");
	
	MetaObjectUtil_Register(GLImageDrawable);
	MetaObjectUtil_Register(GLTextDrawable);
	MetaObjectUtil_Register(GLVideoFileDrawable);
	MetaObjectUtil_Register(GLVideoInputDrawable);
	MetaObjectUtil_Register(GLVideoLoopDrawable);
	MetaObjectUtil_Register(GLVideoReceiverDrawable);

	
	EditorWindow *editor = new EditorWindow();
// 	editor->setGroup(new GLSceneGroup());
// 	editor->addText("Welcome to GLEditor");
	editor->setIsStandalone(true);
	editor->show();
	//editor->move(800,0);
	
// 	EditorGraphicsView *graphicsView = new EditorGraphicsView();
// 	graphicsView->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
// 	//graphicsView->setRenderHint( QPainter::Antialiasing, true );
// 	
// 	GLEditorGraphicsScene * scene = new GLEditorGraphicsScene();
// 	
// 	graphicsView->setScene(scene);
// 	scene->setSceneRect(QRectF(0,0,800,600));
// 	graphicsView->resize(800,600);
// 	graphicsView->setWindowTitle("Test");
// 	
// 	//GLImageDrawable *drawable = new GLImageDrawable("me2.jpg");
// 	
// 	GLVideoDrawable *drawable = new GLVideoDrawable();
// 	if(0)
// 	{
// 		QString testFile = "/opt/qt-mobility-opensource-src-1.0.1/examples/player/dsc_7721.avi";
// 		#ifdef HAS_QT_VIDEO_SOURCE
// 		qDebug() << "addQtSource: using test file:"<<testFile;
// 		QtVideoSource *source = new QtVideoSource();
// 		source->setFile(testFile);
// 		source->start();
// 		
// 		drawable->setVideoSource(source);
// 		#else
// 		qDebug() << "QtVideoSource not compiled in, not showing any video";
// 		#endif
// 	
// 	}
// 	
// 	if(1)
// 	{
// 		//GLImageDrawable *drawable = new GLImageDrawable("me2.jpg");
// 		GLImageDrawable *drawable = new GLImageDrawable("Pm5544.jpg");
// 		//GLImageDrawable *drawable = new GLImageDrawable("grayscale3.jpg");
// 		//GLImageDrawable *drawable = new GLImageDrawable("/usr/share/backgrounds/images/default.jpg");
// 		
// 		//drawable->addShowAnimation(GLDrawable::AnimFade);
// 		//drawable->setRect(QRectF(0,0,1000,750));
// 		drawable->setRect(scene->sceneRect());
// 		scene->addItem(drawable);
// 		drawable->show();
// 		
// 		drawable->setSelected(true);
// 		drawable->setObjectName("image");
// 	}
// 	
// 	if(0)
// 	{
// 		//GLImageDrawable *drawable = new GLImageDrawable("me2.jpg");
// 		GLVideoMjpegDrawable *drawable = new GLVideoMjpegDrawable("http://cameras:8082");
// 		drawable->addShowAnimation(GLDrawable::AnimFade);
// 		//drawable->setRect(QRectF(0,0,1000,750));
// 		drawable->setRect(scene->sceneRect());
// 		scene->addItem(drawable);
// 		drawable->show();
// 		
// 	}
// 	
// 	
// 	if(0)
// 	{
// 		GLVideoLoopDrawable *drawable = new GLVideoLoopDrawable("../data/Seasons_Loop_3_SD.mpg");
// 		drawable->addShowAnimation(GLDrawable::AnimFade);
// 		drawable->setRect(QRectF(0,0,1600,900));
// 		//drawable->setAspectRatioMode(Qt::IgnoreAspectRatio);
// 		//scene->addItem(drawable);
// 		drawable->show();
// 		
// 		scene->addItem(drawable);
// 		
// // 		glw->setCanvasSize(QSizeF(1600.,900.));
// // 		glw->setViewport(QRectF(0.,0.,1600.,900.));
// // 		glw->setAspectRatioMode(Qt::IgnoreAspectRatio);
// 	}
// 	
//         if(0)
// 	{
//                 GLVideoInputDrawable *drawable = new GLVideoInputDrawable("/dev/video0");
//                 drawable->setCardInput("S-Video");
// 		drawable->addShowAnimation(GLDrawable::AnimFade);
// 		drawable->setRect(scene->sceneRect());
// 		scene->addItem(drawable);
// 		drawable->show();
// 	}
// 	
// 	if(0)
// 	{
// 		//QString testFile = "/root/Wildlife.wmv";
// 		QString testFile = "/opt/qt-mobility-opensource-src-1.0.1/examples/player/dsc_7721.avi";
// 		GLVideoFileDrawable *drawable = new GLVideoFileDrawable(testFile);
// 		drawable->addShowAnimation(GLDrawable::AnimFade);
// 		drawable->setRect(QRectF(0,0,1000,750));
// 		scene->addItem(drawable);
// 		drawable->show();
// 	}
// 	
// 	// NOT TESTED YET
// 	if(0)
// 	{
// 		GLVideoReceiverDrawable *drawable = new GLVideoReceiverDrawable("localhost",7755);
// 		drawable->addShowAnimation(GLDrawable::AnimFade);
// 		drawable->setRect(QRectF(0,0,1000,750));
// 		scene->addItem(drawable);
// 		drawable->show();
// 	}
// 	
// 	if(1)
// 	{
// 		GLTextDrawable *drawable = new GLTextDrawable("Hello, World!");
// 		drawable->addShowAnimation(GLDrawable::AnimFade);
// 		//drawable->setRect(QRectF(0,0,1000,750));
// 		drawable->setRect(scene->sceneRect());
// 		scene->addItem(drawable);
// 		drawable->show();
// 		drawable->setObjectName("text");
// 	}
// 	
// 	if(0)
// 	{
// 		GLVideoDrawable *drawable = new GLVideoDrawable();
// 		
// 		VideoThread * source = new VideoThread();
// 		source->setVideo("../data/Seasons_Loop_3_SD.mpg");
// 		//source->setVideo("../samples/BlueFish/EssentialsVol05_Abstract_Media/HD/Countdowns/Abstract_Countdown_3_HD.mp4");
// 		//source->setVideo("../samples/BlueFish/EssentialsVol05_Abstract_Media/SD/Countdowns/Abstract_Countdown_3_SD.mpg");
// 		
// 		source->start();
// 		
// 		drawable->setVideoSource(source);
// 		
// 		drawable->addShowAnimation(GLDrawable::AnimFade);
// 		drawable->setRect(QRectF(0,0,1000,750));
// 		scene->addItem(drawable);
// 		drawable->show();
// 	}
// 	
// 		
// 	
// 	
// 	graphicsView->show();
	
	return app.exec();
}



	
	
// 	if(!thread)
// 	{
// 		VideoThread * thread = new VideoThread();
// 		thread->setVideo("../data/Seasons_Loop_3_SD.mpg");
// 		thread->start();
// 		m_source = thread;
// 	}
 
