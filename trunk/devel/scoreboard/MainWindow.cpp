#include "MainWindow.h"

#include <QtCore>
#include <QtGui>

#include "GLWidget.h"
#include "GLSceneGroup.h"
#include "GLDrawables.h"

class ScaledGraphicsView : public QGraphicsView
{
public:
	ScaledGraphicsView(QWidget *parent=0) : QGraphicsView(parent)
	{
		setFrameStyle(QFrame::NoFrame);
		setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform );
		setCacheMode(QGraphicsView::CacheBackground);
		setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
		setOptimizationFlags(QGraphicsView::DontSavePainterState);
		setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
		setTransformationAnchor(AnchorUnderMouse);
		setResizeAnchor(AnchorViewCenter);
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	}

protected:
	void resizeEvent(QResizeEvent *)
	{
		adjustViewScaling();
	}

	void adjustViewScaling()
	{
		if(!scene())
			return;

		float sx = ((float)width()) / scene()->width();
		float sy = ((float)height()) / scene()->height();

		float scale = qMin(sx,sy);
		setTransform(QTransform().scale(scale,scale));
		//qDebug("Scaling: %.02f x %.02f = %.02f",sx,sy,scale);
		update();
		//m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatioByExpanding);
		//m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
	}

};


MainWindow::MainWindow()
	: QWidget()
{
	QStringList files = QStringList()
	<< "images/mimetypes/gnome-mime-x-font.png"
	<< "images/mimetypes/gnome-mime-application-x-executable.png"
	<< "images/mimetypes/gnome-mime-text.png"
	<< "images/mimetypes/gnome-mime-application.png"
	<< "images/mimetypes/gnome-mime-text-x-troff-man.png"
	<< "images/mimetypes/gnome-mime-image.png"
	<< "images/mimetypes/gnome-library.png"
	<< "images/mimetypes/gnome-compressed.png"
	<< "images/mimetypes/gnome-mime-audio.png"
	<< "images/mimetypes/gnome-mime-application-x-core-file.png"
	<< "images/mimetypes/gnome-mime-video.png"
	<< "images/devices/gnome-dev-printer.png"
	<< "images/devices/gnome-dev-keyboard.png"
	<< "images/devices/gnome-dev-printer-network.png"
	<< "images/devices/gnome-dev-floppy.png"
	<< "images/devices/gnome-dev-cdrom.png"
	<< "images/devices/gnome-dev-harddisk.png"
	<< "images/devices/gnome-dev-printer-new.png"
	<< "images/devices/gnome-dev-mouse-optical.png"
	<< "images/apps/charpick.png"
	<< "images/apps/gcalctool.png"
	<< "images/apps/gnome-searchtool-animation-rest.png"
	<< "images/apps/gnome-cdplayer-icon.png"
	<< "images/apps/gnome-help.png"
	<< "images/apps/gnome-mini-commander.png"
	<< "images/apps/gnome-settings-accessibility-keyboard.png"
	<< "images/apps/gnome-settings-keybindings.png"
	<< "images/apps/gnome-settings-theme.png"
	<< "images/apps/gnome-ccmime.png"
	<< "images/apps/gnome-starthere.png"
	<< "images/apps/gnome-logo-icon-transparent.png"
	<< "images/apps/gnome-screenshot.png"
	<< "images/apps/gnome-ccscreensaver.png"
	<< "images/apps/accessibility-directory.png"
	<< "images/apps/file-manager.png"
	<< "images/apps/gnome-workspace.png"
	<< "images/apps/gnome-system.png"
	<< "images/apps/gnome-application-generic.png"
	<< "images/apps/gnome-calc3.png"
	<< "images/apps/gucharmap.png"
	<< "images/apps/gnome-mixer.png"
	<< "images/apps/gnome-audio2.png"
	<< "images/apps/display-capplet.png"
	<< "images/apps/gdict.png"
	<< "images/apps/gnome-settings-background.png"
	<< "images/apps/gnome-joystick.png"
	<< "images/apps/gedit-icon.png"
	<< "images/apps/gnome-session.png"
	<< "images/apps/gnome-settings-ui-behavior.png"
	<< "images/apps/gnome-run.png"
	<< "images/apps/gnome-settings-accessibility-technologies.png"
	<< "images/apps/gnome-mixer-applet.png"
	<< "images/apps/gnome-ghex.png"
	<< "images/apps/perfmeter.png"
	<< "images/apps/gnome-multimedia.png"
	<< "images/apps/gnome-windows.png"
	<< "images/apps/gnome-cd.png"
	<< "images/apps/gnome-home.png"
	<< "images/apps/gnome-grecord.png"
	<< "images/apps/mozilla-icon.png"
	<< "images/apps/advanced-directory.png"
	<< "images/apps/gnome-panel.png"
	<< "images/apps/gnome-searchtool-animation.png"
	<< "images/apps/gnome-ghostview.png"
	<< "images/apps/gnome-settings-default-applications.png"
	<< "images/apps/gnome-main-menu.png"
	<< "images/apps/gnome-folder-generic.png"
	<< "images/apps/gnome-settings-sound.png"
	<< "images/apps/keyboard.png"
	<< "images/apps/gnome-graphics.png"
	<< "images/apps/gnome-log.png"
	<< "images/apps/clock.png"
	<< "images/apps/gnome-globe.png"
	<< "images/apps/gnome-eog.png"
	<< "images/apps/gnome-searchtool.png"
	<< "images/apps/gnome-window-manager.png"
	<< "images/apps/window-capplet.png"
	<< "images/apps/file-roller.png"
	<< "images/apps/gkb.png"
	<< "images/apps/gnome-terminal.png"
	<< "images/apps/gnome-util.png"
	<< "images/apps/launcher-program.png"
	<< "images/apps/star.png"
	<< "images/apps/evolution-1.4.png"
	<< "images/apps/gnome-character-map.png"
	<< "images/apps/gnome-devel.png"
	<< "images/apps/gnome-mailcheck.png"
	<< "images/apps/gnome-settings.png"
	<< "images/apps/gnome-settings-font.png"
	<< "images/filesystems/gnome-fs-network.png"
	<< "images/filesystems/gnome-fs-directory.png"
	<< "images/filesystems/gnome-fs-directory-accept.png"
	<< "images/filesystems/gnome-fs-share.png"
	<< "images/filesystems/gnome-fs-desktop.png"
	<< "images/filesystems/gnome-fs-regular.png"
	<< "images/filesystems/gnome-fs-trash-empty.png"
	<< "images/filesystems/gnome-fs-executable.png"
	<< "images/filesystems/gnome-fs-trash-full.png"
	<< "images/filesystems/gnome-fs-home.png"
	<< "images/filesystems/gnome-fs-client.png"
	<< "images/emblems/emblem-special.png"
	<< "images/emblems/emblem-personal.png"
	<< "images/emblems/emblem-noread.png"
	<< "images/emblems/emblem-important.png"
	;


	// Setup the layout of the window
	QVBoxLayout *vbox = new QVBoxLayout(this);
	//vbox->setContentsMargins(0,0,0,0);
	
		// Create the GLWidget to do the actual rendering
 		GLWidget *glw = new GLWidget(this);
 		vbox->addWidget(glw);
 		
 		QPolygonF cornerTranslations	= QPolygonF()
			<< QPointF(-250,500)
			<< QPointF(0,0)
			<< QPointF(250,-50)
			<< QPointF(0,0);

 		
 		glw->setCornerTranslations(cornerTranslations);

/*		ScaledGraphicsView * graphicsView = new ScaledGraphicsView();
		QGraphicsScene *graphicsScene = new QGraphicsScene();
		graphicsView->setScene(graphicsScene);
		graphicsView->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
		//graphicsScene->setSceneRect(QRectF(0,0,1000.,750.));
		graphicsView->setBackgroundBrush(Qt::black);
		vbox->addWidget(graphicsView);*/
		
		GLScene *scene = new GLScene();
		
		QList<GLDrawable*> items;
		for (int i = 0; i < 100 ; ++i) {
		//for (int i = 0; i < files.size(); ++i) {
			//GLImageDrawable *item = new GLImageDrawable(files[i % files.size()]);
			GLRectDrawable *item = new GLRectDrawable();
			item->setFillColor(Qt::white);
// 			item->setBorderColor(Qt::black);
// 			item->setBorderWidth(2.0);
			
			item->setRect(QRectF(0,0,16,16));
			//item->setOffset(-kineticPix.width()/2, -kineticPix.height()/2);
			item->setZIndex(i);
			items << item;
			scene->addDrawable(item);
			
			//graphicsScene->addItem(item);
			
// 			if(drawable->rect().isNull())
// 				//m_rect = QRectF(QPointF(0,0),canvasSize());
// 				drawable->setRect(m_graphicsScene->sceneRect());

		}
		m_items = items;

/*			m_image = QImage(640,480,QImage::Format_ARGB32_Premultiplied);
			m_image.fill(Qt::blue);
			
			GLImageDrawable *item = new GLImageDrawable(files[0]);
			item->setRect(QRectF(0,0,48,48));
			//item->setOffset(-kineticPix.width()/2, -kineticPix.height()/2);
			item->setZIndex(0);
			items << item;
			scene->addDrawable(item);
			
			graphicsScene->addItem(item);
			
 			m_imgDrw = item;
 			m_imgDrw->setRect(m_image.rect());*/
 			
//  			connect(&m_timer, SIGNAL(timeout()), this, SLOT(timeout()));
//  			timeout();
//  			m_timer.setInterval(50);
//  			m_timer.start();
//  			m_time.restart();
		
		// setGLWidget() adds all the drawables in the scene to the GLWidget
		scene->setGLWidget(glw);
		
		
		QSizeF avgSize(16.,16.);
		
		// States
		QState *rootState = new QState;
		QState *ellipseState = new QState(rootState);
		QState *figure8State = new QState(rootState);
		QState *randomState = new QState(rootState);
		QState *tiledState = new QState(rootState);
		QState *centeredState = new QState(rootState);
		
		// Values
		for (int i = 0; i < items.count(); ++i) {
			GLDrawable *item = items.at(i);
			// Ellipse
			ellipseState->assignProperty(item, "position",
							QPointF(cos((i / 63.0) * 6.28) * 250,
								sin((i / 63.0) * 6.28) * 250));
		
			// Figure 8
			figure8State->assignProperty(item, "position",
							QPointF(sin((i / 63.0) * 6.28) * 250,
								sin(((i * 2)/63.0) * 6.28) * 250));
		
			// Random
			randomState->assignProperty(item, "position",
							QPointF(-450 + qrand() % 800,
								-450 + qrand() % 800));
		
			// Tiled
			tiledState->assignProperty(item, "position",
						QPointF(((i % 40) - 20) * avgSize.width() + avgSize.width() / 2,
							((i / 40) - 20) * avgSize.height() + avgSize.height() / 2));
		
			// Centered
			centeredState->assignProperty(item, "position", QPointF());
		}

		QStateMachine *states = new QStateMachine();
		states->addState(rootState);
		states->setInitialState(rootState);
		rootState->setInitialState(centeredState);
		
		QParallelAnimationGroup *group = new QParallelAnimationGroup;
		for (int i = 0; i < items.count(); ++i) 
		{
			QPropertyAnimation *anim = new QPropertyAnimation(items[i], "position");
			//anim->setDuration(750);
			//anim->setDuration(750 + i * 25);
			//anim->setDuration(250 + (i % 250) * 25);
			//anim->setDuration(150 + (i % 250) * 7);
			anim->setDuration(250 + i * 10);
			anim->setEasingCurve(QEasingCurve::InOutBack);
			group->addAnimation(anim);
		}
		
		
		QHBoxLayout *buttons = new QHBoxLayout();
		vbox->addLayout(buttons);
		QPushButton *ellipseButton = new QPushButton(QPixmap("images/ellipse.png"), "");
		QPushButton *figure8Button = new QPushButton(QPixmap("images/figure8.png"), "");
		QPushButton *randomButton = new QPushButton(QPixmap("images/random.png"), "");
		QPushButton *tiledButton = new QPushButton(QPixmap("images/tile.png"), "");
		QPushButton *centeredButton = new QPushButton(QPixmap("images/centered.png"), "");
		
		buttons->addWidget(ellipseButton);
		buttons->addWidget(figure8Button);
		buttons->addWidget(randomButton);
		buttons->addWidget(tiledButton);
		buttons->addWidget(centeredButton);
		buttons->addStretch(1);
	
		QAbstractTransition *trans = rootState->addTransition(ellipseButton, SIGNAL(clicked()), ellipseState);
		trans->addAnimation(group);
		
		trans = rootState->addTransition(figure8Button, SIGNAL(clicked()), figure8State);
		trans->addAnimation(group);
		
		trans = rootState->addTransition(randomButton, SIGNAL(clicked()), randomState);
		trans->addAnimation(group);
		
		trans = rootState->addTransition(tiledButton, SIGNAL(clicked()), tiledState);
		trans->addAnimation(group);
		
		trans = rootState->addTransition(centeredButton, SIGNAL(clicked()), centeredState);
		trans->addAnimation(group);
		
		QTimer *timer = new QTimer();
		timer->start(125);
		timer->setSingleShot(true);
		
		trans = rootState->addTransition(timer, SIGNAL(timeout()), ellipseState);
		trans->addAnimation(group);
		
		states->start();
		
	
		glw->setViewport(QRect(-350, -350, 700, 700));
		//graphicsScene->setSceneRect(QRectF(-350, -350, 700, 700));
	
	
	// Adjust window for a 4:3 aspect ratio
	resize(700, 750);
}

void MainWindow::timeout()
{
// 	QPainterPath path;
// 	path.moveTo(controlPoints.at(index));
// 	path.cubicTo(controlPoints.at(index+1),
// 			controlPoints.at(index+2),
// 			controlPoints.at(index+3));
// 	

	QPainterPath path;
	//path.addRect(20, 20, 60, 60);
	
	int val = m_time.elapsed() % 3000;
	double progress = (double)val / 3000.;
	
	path.moveTo(-200, -200);
	path.cubicTo(200, 0,  100, 100,  200, 200);
	//path.cubicTo(0, 99,  50, 50,  0, 0);
	
	//qreal curLen = 1.0;
	double count = (double)m_items.count();
	
	for (int i = 0; i < m_items.count(); ++i)
	{
		GLDrawable *item = m_items[i];
		
		qreal curLen = ((double)i)/count + progress; //metrics.width(32); // width of the item
		if(curLen > 1.0)
			curLen -= 1.0;
		
		//qreal t = m_shapePath.percentAtLength(curLen);
		qreal t = curLen;
		QPointF pt = path.pointAtPercent(t);
		qreal angle = path.angleAtPercent(t);
		
		//qDebug() << "progress:"<<progress<<", item:"<<i<<", curLen:"<<curLen<<", pt:"<<pt<<", angle:"<<angle;
		
		
		// draw rotated letter
// 		painter->save();
		item->setPosition(pt);
		item->setRotation(QVector3D(angle,angle,angle));
		
		//curLen += interval;
	}
	
// 	QPainter painter(&m_image);
// 	//painter.fillRect(0, 0, 100, 100, Qt::white);
// 	painter.fillRect(m_image.rect(), Qt::transparent);
// 	painter.setPen(QPen(Qt::white, 1, Qt::SolidLine,
// 			Qt::FlatCap, Qt::MiterJoin));
// 	painter.setBrush(Qt::black);
// 	
// 	painter.drawPath(path);
// 	
// 	//QPainter painter(&m_image);
// 	//painter.strokePath(path, painter.pen());
// 	
// 	m_imgDrw->setImage(m_image);
// 	//m_imgDrw->setRect(
}
