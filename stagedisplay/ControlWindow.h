#ifndef CONTROLWINDOW_H
#define CONTROLWINDOW_H

#include <QWidget>

namespace Ui {
    class ControlWindow;
}

class ControlWindow : public QWidget 
{
	Q_OBJECT

public:
	ControlWindow(QWidget *parent = 0);
	~ControlWindow();

protected slots:
	void setVideoSource(QString);
	void connectSource();
	
	void setOutputX(int);
	void setOutputY(int);
	void setOutputW(int);
	void setOutputH(int);
	
	void setTimerEnabled(bool);
	void setStartTime(int);
	void currentTimeChanged(int);
	void timerBtn();
	void resetTimerBtn();
	void timerFontSizeChanged(int);
	void timerDrawBgChanged(bool);
	void timerPositionChanged(int);
	
	void messageChanged(QString);
	void showMsgBtn();
	void hideMsgBtn();
	void flashMsgBtnToggled(bool);
	void flashSpeedChanged(int);
	void msgFontSizeChanged(int);
	void msgDrawBgChanged(bool);
	void msgPositionChanged(int);
	
protected:
	void changeEvent(QEvent *e);

private:
	Ui::ControlWindow *ui;
};

#endif // CONTROLWINDOW_H
