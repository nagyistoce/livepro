#include <QtGui/QApplication>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QtOpenGL/QGLWidget>
#include <QDebug>
#include <QAbstractTextDocumentLayout>
#include <QTextDocument>
#include <QTextCursor>

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

//#include "CameraTest.h"
// #include "VideoWidget.h"
 #include "CameraThread.h"
// #include "VideoThread.h"
// #include "MjpegThread.h"

#include "LiveVideoInputLayer.h"
#include "LiveStaticSourceLayer.h"
#include "LiveTextLayer.h"
#include "LiveVideoFileLayer.h"
#include "LiveScene.h"
#include "../glvidtex/GLWidget.h"
#include "../glvidtex/GLVideoDrawable.h"
#include "../glvidtex/StaticVideoSource.h"

#include "MainWindow.h"

//#include "MdiCamera.h"

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

#include "ExpandableWidget.h"

QVariant LiveMix_Vector3dInterpolator(const QVector3D &start, const QVector3D &end, qreal progress)
{
	QVector3D val = (end - start) * progress + start;
	//qDebug() << "LiveMix_Vector3dInterpolator: end:"<<end<<", start:"<<start<<", progress:"<<progress<<", val:"<<val;
	return val;
	
}
 
int main(int argc, char **argv)
{
/*	#ifdef Q_WS_X11
		qDebug() << "Setting up multithreaded X11 library calls";
		XInitThreads();
	#endif*/

	qRegisterAnimationInterpolator<QVector3D>(LiveMix_Vector3dInterpolator);
	
	//QGL::setPreferredPaintEngine(QPaintEngine::OpenGL);
	QApplication app(argc, argv);
	qApp->setApplicationName("LiveMix");
	qApp->setOrganizationName("Josiah Bryan");
	qApp->setOrganizationDomain("mybryanlife.com");
	
	LiveScene_Register(LiveTextLayer);
	LiveScene_Register(LiveStaticSourceLayer);
	LiveScene_Register(LiveVideoInputLayer);
	LiveScene_Register(LiveVideoFileLayer);
	
//	QScrollArea *area = new QScrollArea;
// 	area->setWindowTitle("Expandable Widget Demo");
// 	area->setWidgetResizable(true);
// 	
// 	QWidget *containerWidget = new QWidget;
// 	QVBoxLayout *layout = new QVBoxLayout(containerWidget);
// 	
// 	ExpandableWidget *expand = new ExpandableWidget("Task 1");
// 	
// 	QLabel *label = new QLabel("Test");
// 	QPushButton *button = new QPushButton("Test Button");
// 	
// 	QWidget *base = new QWidget();
// 	QHBoxLayout *hbox = new QHBoxLayout(base);
// 	hbox->addWidget(label);
// 	hbox->addWidget(button);
// 	expand->setWidget(base);
// 	
// 	layout->addWidget(expand);
// 	layout->addStretch(1);
// 	
// 	area->setWidget(expand);
// 	area->show();
	

	MainWindow mw;
	mw.show();

// 	GLWidget *glOutputWin = new GLWidget();
// 	{
// 		glOutputWin->setWindowTitle("Live");
// 		glOutputWin->resize(1000,750);
// 		// debugging...
// 		//glOutputWin->setProperty("isEditorWidget",true);
// 	
// 	}
// 	
// 	GLWidget *glEditorWin = new GLWidget();
// 	{
// 		glEditorWin->setWindowTitle("Editor");
// 		glEditorWin->resize(400,300);
// 		glEditorWin->setProperty("isEditorWidget",true);
// 	}
// 	
// 	// Setup Editor Background
// 	if(true)
// 	{
// 		QSize size = glEditorWin->viewport().size().toSize();
// 		size /= 2.5;
// 		//qDebug() << "MainWindow::createLeftPanel(): size:"<<size;
// 		QImage bgImage(size, QImage::Format_ARGB32_Premultiplied);
// 		QBrush bgTexture(QPixmap("squares2.png"));
// 		QPainter bgPainter(&bgImage);
// 		bgPainter.fillRect(bgImage.rect(), bgTexture);
// 		bgPainter.end();
// 		
// 		StaticVideoSource *source = new StaticVideoSource();
// 		source->setImage(bgImage);
// 		//source->setImage(QImage("squares2.png"));
// 		source->start();
// 		
// 		GLVideoDrawable *drawable = new GLVideoDrawable(glEditorWin);
// 		drawable->setVideoSource(source);
// 		drawable->setRect(glEditorWin->viewport());
// 		drawable->setZIndex(-100);
// 		drawable->setObjectName("StaticBackground");
// 		drawable->show();
// 		
// 		glEditorWin->addDrawable(drawable);
// 	}
// 	
// 	
// 	
// 	// setup scene
// 	LiveScene *scene = new LiveScene();
// 	{
// 		//scene->addLayer(new LiveVideoInputLayer());
// 		scene->addLayer(new LiveStaticSourceLayer());
// 		scene->addLayer(new LiveTextLayer());
// 	}
// 	
// 	// add to live output
// 	{
// 		scene->attachGLWidget(glOutputWin);
// 		scene->layerList().at(0)->drawable(glOutputWin)->setObjectName("Output");
// 		//glOutputWin->addDrawable(scene->layerList().at(0)->drawable(glOutputWin));
// 		scene->layerList().at(0)->setVisible(true);
// 		scene->layerList().at(1)->setVisible(true);
// 	}
// 	
// 	// add to editor
// 	{
// 		scene->layerList().at(0)->drawable(glEditorWin)->setObjectName("Editor");
// 		//glEditorWin->addDrawable(scene->layerList().at(1)->drawable(glEditorWin));
// 		glEditorWin->addDrawable(scene->layerList().at(1)->drawable(glEditorWin));
// 	}
// 	
// 	// show windows
// 	{
// 		glOutputWin->show();
// 		glEditorWin->show();
// 	}
	
	

	return app.exec();
}

