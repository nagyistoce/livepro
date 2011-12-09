#include "ControlWindow.h"
#include "ui_ControlWindow.h"
#include <QApplication>
#include <QtGui>
#include "LiveScheduleEntry.h"
#include "LiveSchedule.h"

ControlWindow::ControlWindow(QWidget *parent)
	: QMainWindow(parent)
	, m_ui(new Ui::ControlWindow)
{
	m_ui->setupUi(this);
	
	connect(m_ui->actionAbout_LiveMix_Controller, SLOT(activated()),
		this, SLOT(showAboutWindow()));
	connect(m_ui->actionNew_Schedule, SLOT(activated()),
		this, SLOT(createNewSchedule()));
	connect(m_ui->actionOpen_Schedule, SLOT(activated()),
		this, SLOT(showOpenScheduleDialog()));
	connect(m_ui->actionSave_Schedule, SLOT(activated()),
		this, SLOT(saveSchedule()));
	connect(m_ui->actionSave_Schedule_As, SLOT(activated()),
		this, SLOT(showSaveScheduleAsDialog()));
	connect(m_ui->actionQuit, SLOT(activated()),
		qApp, SLOT(quit()));
	connect(m_ui->actionAdd_Layout, SLOT(activated()),
		this, SLOT(addNewLayout()));
	connect(m_ui->actionRemove_Layout, SLOT(activated()),
		SLOT(removeSelectedLayout()));
}

ControlWindow::~ControlWindow()
{
	delete m_ui;
}

void ControlWindow::showAboutWindow()
{
	QMessageBox::information(this, "About LiveMix Controller", "This is a controller application for controlling LiveMix player instances and/or outputting LiveMix layouts to the secondary video output of your computer.");
}

void ControlWindow::createNewSchedule()
{
	saveSchedule();
	loadSchedule(0);
	delete m_schedule;
	
// 	m_schedule = new LiveSchedule();
	loadSchedule(m_schedule);
}

void ControlWindow::showOpenScheduleDialog()
{
	QSettings s;
	QString curFile = m_schedule->filename();
	if(curFile.trimmed().isEmpty())
		curFile = s.value("last-lms-file").toString();

	QString fileName = QFileDialog::getOpenFileName(this, tr("Select LiveMix Schedule File (*.lms)"), curFile, tr("LiveMix Schedule File (*.lms *.liveschedule);;Any File (*.*)"));
	if(fileName != "")
	{
		s.setValue("last-lms-file",fileName);
		if(openSchedule(fileName))
		{
			return;
		}
		else
		{
			QMessageBox::critical(this,tr("File Does Not Exist"),tr("Sorry, but the file you chose does not exist. Please try again."));
		}
	}	
}

void ControlWindow::saveSchedule()
{
	if(!m_schedule)
		return;
	if(m_schedule->filename().isEmpty())
		showSaveScheduleAsDialog();
// 	else
// 		m_schedule->writeFile();
}

void ControlWindow::showSaveScheduleAsDialog()
{
	if(!m_schedule)
		return;
		
	QSettings s;
	QString curFile = m_schedule->filename();
	if(curFile.trimmed().isEmpty())
		curFile = s.value("last-lms-file").toString();

	QString fileName = QFileDialog::getSaveFileName(this, tr("Choose a Filename"), curFile, tr("LiveMix Schedule (*.lms);;Any File (*.*)"));
	if(fileName != "")
	{
		QFileInfo info(fileName);
		s.setValue("last-lms-file",fileName);
// 		m_schedule->writeFile(fileName);
	}
}

void ControlWindow::addNewLayout()
{
	QSettings s;
	QString lastLayoutFile = s.value("last-lmx-file").toString();

	QString fileName = QFileDialog::getOpenFileName(this, tr("Select LiveMix Layout File (*.lmx)"), lastLayoutFile, tr("LiveMix Layout File (*.lmx *.livemix);;Any File (*.*)"));
	if(fileName != "")
	{
		s.setValue("last-lmx-file",fileName);
		if(openLayout(fileName))
		{
			return;
		}
		else
		{
			QMessageBox::critical(this,tr("File Does Not Exist"),tr("Sorry, but the file you chose does not exist. Please try again."));
		}
	}
}

void ControlWindow::removeSelectedLayout()
{
	if(!m_schedule)
		return;
		
	QModelIndex idx = m_ui->layoutScheduleTable->currentIndex();
	if(!idx.isValid() || idx.row() < 0)
		return;
		
// 	m_schedule->removeEntry(idx.row());
	
	updateScheduleTable();
}
	
void ControlWindow::changeEvent(QEvent *e)
{
	QMainWindow::changeEvent(e);
	switch (e->type()) 
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);
			break;
		default:
			break;
	}
}

void ControlWindow::closeEvent(QCloseEvent *)
{
	saveSchedule();
}


void ControlWindow::updateScheduleTable()
{
	if(!m_schedule)
		return;
		
	QTableWidget *table = m_ui->layoutScheduleTable;
	table->clear();
	table->setRowCount(m_schedule->entries().size());
	
	QTableWidgetItem *prototype = new QTableWidgetItem();
	prototype->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

	int row=0;
	foreach(LiveScheduleEntry *entry, m_schedule->entries())
	{
		
		QTableWidgetItem *t = prototype->clone();
		if(!entry->pixmap().isNull())
			t->setIcon(QIcon(entry->pixmap()));
		//else
			//t->setText(QString::number(entry->id()));
		table->setItem(row,0,t);
		
		t = prototype->clone();
		t->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsEditable);
		t->setText(entry->title());
		table->setItem(row,1,t);
		
		t = prototype->clone();
		t->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsEditable);
		t->setText(entry->startTime().toString("hh:mm:ss"));
		t->setTextAlignment(Qt::AlignRight);
		table->setItem(row,2,t);
		
		t = prototype->clone();
		t->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsEditable);
		t->setText(QString::number(entry->duration()));
		t->setTextAlignment(Qt::AlignRight);
		table->setItem(row,3,t);
		
		row++;
	}
	
	table->sortByColumn(2,Qt::AscendingOrder);
	table->resizeColumnsToContents();
	table->resizeRowsToContents();
}


bool ControlWindow::openSchedule(const QString&)
{
	// TODO
}

void ControlWindow::loadSchedule(LiveSchedule *)
{
	// TODO
}

bool ControlWindow::openLayout(const QString& file)
{

}
