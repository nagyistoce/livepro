#include <QtGui/QApplication>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QtOpenGL/QGLWidget>

#include <QtGui>
#include <QtOpenGL>


#include "GVTestWindow.h"
#include "SimpleV4L2.h"


BoxItem::BoxItem(QGraphicsScene * scene, QGraphicsItem * parent, QString camera) : QGraphicsItem(parent, scene),
		m_contentsRect(0,0,640,480), brush(QColor(255,0,0,255))
		, m_v4l2(0)
		, m_frameCount(0)
		, m_latencyAccum(0)
		, m_debugFps(true)
		, m_isVisible(true)
		
	{
		setFlags(QGraphicsItem::ItemIsMovable); 
		pen.setWidthF(3);
		pen.setColor(QColor(0,0,0,255));
		
		m_v4l2 = new SimpleV4L2();
		QString cameraFile = camera; //"/dev/video0";
		if(m_v4l2->openDevice(qPrintable(cameraFile)))
		{
			setObjectName(qPrintable(cameraFile));
// 			// Do the code here so that we dont have to open the device twice
// 			if(!setInput("Composite"))
// 				if(!setInput("Composite1"))
// 					setInput(0);

			m_v4l2->initDevice();
			m_v4l2->startCapturing();
// 			m_inited = true;
			
// 			qDebug() << "CameraThread::initCamera(): finish2";

// 			return 1;
		}
		readFrame();
		connect(&m_readTimer, SIGNAL(timeout()), this, SLOT(readFrame()));
		m_readTimer.setInterval(1000/35);
		m_readTimer.start();
		m_time.start();
		
	}
BoxItem::~BoxItem()
{
	if(m_v4l2)
	{
		delete m_v4l2;
		m_v4l2 = 0;
	};
}

void BoxItem::show()
{
	QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
	animation->setDuration(700);
	animation->setEndValue(1.);
	
	animation->start();
	
	m_isVisible = true;
	emit isVisible(true);
}
	
void BoxItem::hide()
{
	QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
	animation->setDuration(700);
	animation->setEndValue(0.);
	
	animation->start();
	
	m_isVisible = false;
	emit isVisible(false);
}


QRectF BoxItem::boundingRect() const
	{
		return m_contentsRect;
	}
	
void BoxItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
	{
		//painter->setPen(pen);
		//painter->setBrush(brush);
		//painter->drawRect(m_contentsRect);
		
		//qDebug() << "painter opac:"<<painter->opacity()<<", item opac:"<<opacity();
		VideoFrame f = m_v4l2->readFrame();
		if(f.isValid())
			m_frame = f;
		
		//qglClearColor(Qt::black);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
		QImage image((const uchar*)m_frame.byteArray.constData(),m_frame.size.width(),m_frame.size.height(),QImage::Format_RGB32);
		
// 		painter->save();
// 		painter->scale(-1, 1);
// 		painter->translate(-m_contentsRect.width(), 0);
		//painter->setOpacity(0.5);
		painter->drawImage(m_contentsRect, image); //,image.convertToFormat(QImage::Format_ARGB32));
		//painter->restore();
		
		
		if(!m_frame.captureTime.isNull())
		{
			int msecLatency = m_frame.captureTime.msecsTo(QTime::currentTime());
			
			m_latencyAccum += msecLatency;
		}
		
		if (!(m_frameCount % 100)) 
		{
			QString framesPerSecond;
			framesPerSecond.setNum(m_frameCount /(m_time.elapsed() / 1000.0), 'f', 2);
			
			QString latencyPerFrame;
			latencyPerFrame.setNum((((double)m_latencyAccum) / ((double)m_frameCount)), 'f', 3);
			
			if(m_debugFps && framesPerSecond!="0.00")
				qDebug() << "BoxItem::paint: "<<objectName()<<" FPS: " << qPrintable(framesPerSecond) << (m_frame.captureTime.isNull() ? "" : qPrintable(QString(", Latency: %1 ms").arg(latencyPerFrame)));
	
			m_time.start();
			m_frameCount = 0;
			m_latencyAccum = 0;
			
			//lastFrameTime = time.elapsed();
		}
		m_frameCount++;
	}

void BoxItem::readFrame()
	{
		
		update();
	}



QFormLayout * createToggleBox()
{
	QWidget *tb = new QWidget();
	tb->setWindowTitle("Toggle Box");
	QFormLayout *layout = new QFormLayout;
	tb->setLayout(layout);
	tb->show();
	return layout;
}

void addButtons(QFormLayout *layout, BoxItem *glw)
{
        //return;
	QHBoxLayout *box = new QHBoxLayout;
	
	QPushButton *show = new QPushButton("Show");
	QObject::connect(show, SIGNAL(clicked()), glw, SLOT(show()));
	QObject::connect(glw, SIGNAL(isVisible(bool)), show, SLOT(setDisabled(bool)));
	show->setDisabled(glw->isVisible());
	
	QPushButton *hide = new QPushButton("Hide");
	QObject::connect(hide, SIGNAL(clicked()), glw, SLOT(hide()));
	QObject::connect(glw, SIGNAL(isVisible(bool)), hide, SLOT(setEnabled(bool)));
	hide->setEnabled(glw->isVisible());
	
	box->addWidget(show);
	box->addWidget(hide);
	
	layout->addRow(glw->objectName(), box); 
}


int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	
	QGraphicsView *graphicsView = new QGraphicsView();
 	graphicsView->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
 	graphicsView->setRenderHint( QPainter::Antialiasing, true );
	
	QGraphicsScene * scene = new QGraphicsScene();
	
	QFormLayout * tb = createToggleBox();
		
	graphicsView->setScene(scene);
	scene->setSceneRect(0,0,800,600);
	graphicsView->resize(800,600);
	graphicsView->setWindowTitle("Test");
	
// 	for(int i=0;i<100;i++)
// 	{
		BoxItem *x = new BoxItem(scene,0, "/dev/video0");
		addButtons(tb,x);
		//x->setPos(qrand() % 800, qrand() % 600);
		x->setPos(0,0);
		
		x = new BoxItem(scene,0,"/dev/video1");
		addButtons(tb,x);
		//x->setPos(qrand() % 800, qrand() % 600);
		x->setPos(0,0);
// 	}
	
	graphicsView->show();
	
	return app.exec();
}

