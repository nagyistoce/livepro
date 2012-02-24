#include "ControlWindow.h"

ControlWindow::ControlWindow()
	: QWidget()
	, m_outputWin(0)
{
	setWindowTitle("StageDisplay Control");
	setupUi();
}

void ControlWindow::setupUi()
{
	QVBoxLayout *vbox = new QVBoxLayout();
	
	/// TODO build layout
}

void ControlWindow::setOutputWindow(OutputWindow *output)
{
	m_outputWin = output;
	/// TODO connect slots
}
