#include "MainWindow.h"

#include "SwitchMonWidget.h"

#include <QApplication>
#include <QDesktopWidget>

MainWindow::MainWindow()
	: QWidget()
	, m_switchMon(0)
{
	m_switchMon = new SwitchMonWidget(this);
	m_switchMon->setGeometry(QApplication::desktop()->screenGeometry());

    //    m_layoutBase = this;
	setStyleSheet("background-color:rgb(50,50,50); color:#FFFFFF");

}

MainWindow::~MainWindow()
{
}

void MainWindow::resizeEvent(QResizeEvent*)
{
	//qDebug() << "Window Size: "<<width()<<" x "<<height();
	QRect rect = QApplication::desktop()->screenGeometry();
	m_switchMon->setGeometry(rect);
}
