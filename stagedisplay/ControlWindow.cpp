#include "ControlWindow.h"
#include "ui_ControlWindow.h"

ControlWindow::ControlWindow(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::ControlWindow)
{
	ui->setupUi(this);
	setWindowTitle("StageDisplay Control");
}

ControlWindow::~ControlWindow()
{
	delete ui;
}

void ControlWindow::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
	switch (e->type())
	{
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}
