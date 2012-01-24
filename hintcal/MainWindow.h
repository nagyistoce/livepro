#ifndef MainWindow_H
#define MainWindow_H

#include <QtGui>

class PlayerConnection;
class GLVideoInputDrawable;
class GLSceneGroup;
class MainWindow : public QWidget
{
	Q_OBJECT
public:
	MainWindow();
	
protected slots:
	void textChanged(QString);
	void connectToServer();

	void setAdvancedFilter(QString name);
	void sendVidOpts();
	void setStraightValue(double value);
	
protected:
	int indexOfFilter(int filter);
	
	QLineEdit *m_serverBox;
	QPushButton *m_connectBtn;
	
	GLVideoInputDrawable *m_drw;
	GLSceneGroup *m_group;
	
	QWidget *m_sharpAmountWidget;
	
	PlayerConnection *m_player;
	
};



#endif
