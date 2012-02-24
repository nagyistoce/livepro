#ifndef ControlWindow_H
#define ControlWindow_H

#include <QtGui>
class OutputWindow;

class ControlWindow : public QWidget
{
	Q_OBJECT
public:
	ControlWindow();
	
	void setOutputWindow(OutputWindow *);
	
protected:
	void setupUi();
	
	OutputWindow *m_outputWin;
};

#endif
