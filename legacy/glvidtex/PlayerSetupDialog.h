#ifndef PLAYERSETUPDIALOG_H
#define PLAYERSETUPDIALOG_H

#include <QDialog>
#include <QtGui>

namespace Ui {
    class PlayerSetupDialog;
}

class GLWidgetSubview;
class PlayerConnection;
class PlayerSubviewsModel;
class PlayerConnectionList;

class PlayerSetupDialog : public QDialog
{
	Q_OBJECT

public:
	explicit PlayerSetupDialog(QWidget *parent = 0);
	~PlayerSetupDialog();

	PlayerConnectionList *playerList() { return m_playerList; }
	
public slots:
	void setPlayerList(PlayerConnectionList*);
	
private slots:
	void newPlayer();
	void removePlayer();
	
	void newSubview();
	void removeSubview();
	
	void testConnection();
	
	void playerSelected(const QModelIndex &);
	//void currentPlayerChanged(const QModelIndex &idx,const QModelIndex &);
	
	void subviewSelected(const QModelIndex &);
	//void currentSubviewChanged(const QModelIndex &idx,const QModelIndex &);
	
	void setCurrentPlayer(PlayerConnection*);
	void setCurrentSubview(GLWidgetSubview*);
	
	void alphamaskBrowse();
	
	void subviewTopLeftXChanged(int);
	void subviewTopLeftYChanged(int);
	
	void subviewTopRightXChanged(int);
	void subviewTopRightYChanged(int);
	
	void subviewBottomLeftXChanged(int);
	void subviewBottomLeftYChanged(int);
	
	void subviewBottomRightXChanged(int);
	void subviewBottomRightYChanged(int);
	
	void screenXChanged(int);
	void screenYChanged(int);
	void screenWChanged(int);
	void screenHChanged(int);
	
	void viewportXChanged(int);
	void viewportYChanged(int);
	void viewportWChanged(int);
	void viewportHChanged(int);
	
	void ignoreArBoxChanged(bool);
	
	void showAlphaMaskPreview(const QString&);
	
	void brightReset();
	void contReset();
	void hueReset();
	void satReset();
	
	void conConnected();
	void conDisconnected();
	void conLoginFailure();
	void conLoginSuccess();
	void conPlayerError(const QString&);
	void conPingResponseReceived(const QString&);
	void conTestStarted();
	void conTestEnded();
	void conTestResults(bool ok);
	
	void connectPlayer();
	void autoconBoxChanged(bool);
	
	
private:
	void setupUI();
	
	Ui::PlayerSetupDialog *ui;
	
	PlayerConnection *m_con;
	GLWidgetSubview *m_sub;
	PlayerSubviewsModel *m_subviewModel;
	PlayerConnectionList *m_playerList;
	
	bool m_stickyConnectionMessage;
};

#endif // PLAYERSETUPDIALOG_H
