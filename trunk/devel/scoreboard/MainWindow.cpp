#include "MainWindow.h"

#include <QtCore>
#include <QtGui>

#include "GLWidget.h"
#include "GLSceneGroup.h"
#include "GLDrawables.h"

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
		
		GLScene *scene = new GLScene();
		
		QList<GLImageDrawable*> items;
		//for (int i = 0; i < 64 ; ++i) {
		for (int i = 0; i < files.size(); ++i) {
			GLImageDrawable *item = new GLImageDrawable(files[i]);
			item->setRect(QRectF(0,0,48,48));
			//item->setOffset(-kineticPix.width()/2, -kineticPix.height()/2);
			item->setZIndex(i);
			items << item;
			scene->addDrawable(item);
		}
		
		// setGLWidget() adds all the drawables in the scene to the GLWidget
		scene->setGLWidget(glw);
		
		QSizeF avgSize(48.,48.);
		
		// States
		QState *rootState = new QState;
		QState *ellipseState = new QState(rootState);
		QState *figure8State = new QState(rootState);
		QState *randomState = new QState(rootState);
		QState *tiledState = new QState(rootState);
		QState *centeredState = new QState(rootState);
		
		// Values
		for (int i = 0; i < items.count(); ++i) {
			GLImageDrawable *item = items.at(i);
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
							QPointF(-250 + qrand() % 500,
								-250 + qrand() % 500));
		
			// Tiled
			tiledState->assignProperty(item, "position",
						QPointF(((i % 8) - 4) * avgSize.width() + avgSize.width() / 2,
							((i / 8) - 4) * avgSize.height() + avgSize.height() / 2));
		
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
			anim->setDuration(750 + i * 25);
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
	
	
	// Adjust window for a 4:3 aspect ratio
	resize(700, 750);
}

