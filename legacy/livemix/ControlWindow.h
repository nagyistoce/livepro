#ifndef CONTROLWINDOW_H
#define CONTROLWINDOW_H

#include <QMainWindow>

namespace Ui {
    class ControlWindow;
}

class LiveSchedule;

class ControlWindow : public QMainWindow
{
	Q_OBJECT
public:
	ControlWindow(QWidget *parent = 0);
	~ControlWindow();

public slots:
	bool openSchedule(const QString&);
	bool openLayout(const QString&);
	void loadSchedule(LiveSchedule *);

protected slots:
	void showAboutWindow();
	void createNewSchedule();
	void showOpenScheduleDialog();
	void saveSchedule();
	void showSaveScheduleAsDialog();
	void addNewLayout();
	void removeSelectedLayout();
		
	void updateScheduleTable();
	
protected:
	void changeEvent(QEvent *e);
	void closeEvent(QCloseEvent*);

private:
	Ui::ControlWindow *m_ui;
	LiveSchedule *m_schedule;
};

#endif // CONTROLWINDOW_H
