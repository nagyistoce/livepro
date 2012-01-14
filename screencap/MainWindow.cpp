#include "MainWindow.h"

#include "VideoWidget.h"
#include "GLVideoInputDrawable.h"
#include "QtVideoSource.h"
#include "GLVideoFileDrawable.h"
#include "GLVideoDrawable.h"
#include "ScreenVideoSource.h"
#include "VideoSender.h"

#define SETTINGS_FILE "inputs.ini"
#define GetSettingsObject() QSettings settings(SETTINGS_FILE,QSettings::IniFormat);

#include <QApplication>

MainWindow::MainWindow()
	: QWidget()
{
	// Load the config from SETTINGS_FILE
	GetSettingsObject();

	// Grab number of inputs from config
	int numItems = settings.value("num-inputs",0).toInt();
	if(numItems <= 0)
	{
		qDebug() << "No inputs listed in "<<SETTINGS_FILE<<", sorry!";
		QTimer::singleShot(0,qApp,SLOT(quit()));
		return;
	}
		
	// Setup the layout of the window
	QHBoxLayout *hbox = new QHBoxLayout(this);
	hbox->setContentsMargins(0,0,0,0);
	
	// Finally, get down to actually creating the drawables 
	// and setting their positions
	for(int i=1; i<=numItems; i++)
	{
		// Tell QSettings to read from the current input
		QString group = tr("input%1").arg(i);
		qDebug() << "Reading input "<<group;
		settings.beginGroup(group);
		
		// Read capture rectangle
		QString rectString = settings.value("rect","").toString();
		if(rectString.isEmpty())
		{
			qDebug() << "Error: No rect found for input "<<i<<", assuming default rect of 0 0 320 240";
			rectString = "0 0 320 240";
		}
		
		// Create the QRect for capture
		QRect captureRect;
		QStringList points = rectString.split(" ");
		if(points.size() != 4)
		{
			qDebug() << "Error: Invalid rectangle definition for input "<<i<<" (not exactly four numbers seperated by three spaces): "<<rectString<<", assuming default rect";
			captureRect = QRect(0,0,320,240);
		}
		else
		{
			captureRect = QRect(
				points[0].toInt(),
				points[1].toInt(),
				points[2].toInt(),
				points[3].toInt()
			);
		}
		
		// Read transmit size
		QSize xmitSize = captureRect.size();
		QString xmitSizeString = settings.value("xmit-size","").toString();
		if(!xmitSizeString.isEmpty())
		{
			QStringList list = xmitSizeString.split(" ");
			if(list.size() != 2)
			{
				qDebug() << " Error: Invalid xmit-size string for input "<<i<<" (not exactly two numbers seperated by a space): "<<xmitSizeString<<", assuming xmit same size as captureRect: "<<captureRect.size();
			}
			else
			{
				xmitSize = QSize(list[0].toInt(), list[1].toInt());
			}
		}
		
		// Read capture fps
		int captureFps = settings.value("fps",-1).toInt();
		
		// Read transmit fps
		int xmitFps = settings.value("xmit-fps",10).toInt();
		
		// If captureFps not specified, default to same as xmit-fps
		if(captureFps < 0)
			captureFps = xmitFps;
		
		// Read transmit port
		int port = settings.value("port",-1).toInt(); // default of -1 means it will auto-allocate
		
		// Create the screen capture object
		ScreenVideoSource *src = new ScreenVideoSource();
		src->setCaptureRect(captureRect);
		src->setFps(captureFps);
		m_caps << src; // store for setting capture rect later
		
		// Create the video transmitter object
		VideoSender *sender = new VideoSender();
		sender->setVideoSource  (src);
		sender->setTransmitSize (xmitSize);
		sender->setTransmitFps  (xmitFps);
		sender->start           (port); // this actually starts the transmission
		m_senders << sender;
		
		// Output confirmation
		QString ipAddress = VideoSender::ipAddress();
		qDebug() << "Success: Input "<<i<<": Started video sender "<<tr("%1:%2").arg(ipAddress).arg(sender->serverPort()) << " for rect "<<src->captureRect();
			
		// Create the preview widget
		VideoWidget *drw = new VideoWidget();
		drw->setVideoSource(src);
		drw->setProperty("num",i-1);
		connect(drw, SIGNAL(clicked()), this, SLOT(changeRectSlot()));
		
		// Add widget to window
		hbox->addWidget(drw);
		
		// Return to top level of settings file for next group
		settings.endGroup();
	}
	
	// Adjust size of viewport and window to match number of items we have
	resize( 320 * numItems, 240 );
}

void MainWindow::resizeEvent(QResizeEvent*)
{
	//qDebug() << "Window Size: "<<width()<<" x "<<height(); 
}

class RectChangeWidget : public QLabel
{
public:
	RectChangeWidget(int num, MainWindow *ptr)
		: m_num(num), m_ptr(ptr)
	{
		QRect rect = ptr->currentCapRect(num);
		setWindowTitle(tr("Input %1").arg(num+1));
		show();
		move(rect.top(), rect.left());
		resize(rect.width(), rect.height());	
		
		setText("Select the area to capture by dragging this window and resizing it. Click inside the window when done. (Hint: On Linux, hold down ALT while using L or R mouse buttons to move or resize the window anywhere.)");
		setWordWrap(true);
		
	}

protected:
	void mousePressEvent( QMouseEvent */*event*/)
	{
		m_ptr->setCapRect( m_num, frameGeometry() );
		close();
		deleteLater();
	}
	
	

private:
	int m_num;
	MainWindow *m_ptr;
	QRect m_rect;

};

void MainWindow::changeRectSlot()
{
	QObject *send = sender();
	(void*)new RectChangeWidget(send->property("num").toInt(), this);
}

void MainWindow::setCapRect(int num, QRect rect)
{
	m_caps[num]->setCaptureRect(rect);
	qDebug() << "Changed input "<<(num+1)<<" capture rect to "<<rect;
	
	// Load the config from SETTINGS_FILE
	GetSettingsObject();
	
	// Tell QSettings to read from the current input
	QString group = tr("input%1").arg(num+1);
	settings.beginGroup(group);
	
	// If xmit size was auto, auto adjust xmit size based on current rect
	QString origXmitSize = settings.value("xmit-size","").toString();
	if(origXmitSize.isEmpty())
		m_senders[num]->setTransmitSize(rect.size());
	
}

QRect MainWindow::currentCapRect(int num)
{
	return m_caps[num]->captureRect();
}
