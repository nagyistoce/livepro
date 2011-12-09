#include <QtGui>

#include "MdiChild.h"

MdiChild::MdiChild(QWidget *parent)
	: QWidget(parent)
{
	//setAttribute(Qt::WA_DeleteOnClose);
	setWindowFlags(Qt::FramelessWindowHint);
}
