#include "PlayerSetupDialog.h"
#include "ui_PlayerSetupDialog.h"

#include "PlayerConnection.h"
#include "GLWidget.h"

#include <QtGui>

PlayerSetupDialog::PlayerSetupDialog(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::PlayerSetupDialog)
	, m_con(0)
	, m_sub(0)
	, m_subviewModel(0)
	, m_playerList(0)
	, m_stickyConnectionMessage(false)
{
	ui->setupUi(this);
	setupUI();
	setWindowTitle("Players Setup");
}

PlayerSetupDialog::~PlayerSetupDialog()
{
	delete ui;
}

void PlayerSetupDialog::setupUI()
{
	connect(ui->addPlayerBtn, SIGNAL(clicked()), this, SLOT(newPlayer()));
	connect(ui->removePlayerBtn, SIGNAL(clicked()), this, SLOT(removePlayer()));
	
	connect(ui->newSubviewBtn, SIGNAL(clicked()), this, SLOT(newSubview()));
	connect(ui->delSubviewBtn, SIGNAL(clicked()), this, SLOT(removeSubview()));

	connect(ui->testConnectionBtn, SIGNAL(clicked()), this, SLOT(testConnection()));
	connect(ui->connectBtn, SIGNAL(clicked()), this, SLOT(connectPlayer()));
	connect(ui->autoconnectBox, SIGNAL(toggled(bool)), this, SLOT(autoconBoxChanged(bool)));

	connect(ui->playerListview, SIGNAL(activated(const QModelIndex &)), this, SLOT(playerSelected(const QModelIndex &)));
	connect(ui->playerListview, SIGNAL(clicked(const QModelIndex &)),   this, SLOT(playerSelected(const QModelIndex &)));

	connect(ui->subviewListview, SIGNAL(activated(const QModelIndex &)), this, SLOT(subviewSelected(const QModelIndex &)));
	connect(ui->subviewListview, SIGNAL(clicked(const QModelIndex &)),   this, SLOT(subviewSelected(const QModelIndex &)));

	connect(ui->aphaMaskBrowseBtn, SIGNAL(clicked()), this, SLOT(alphamaskBrowse()));
	connect(ui->keyTLx, SIGNAL(valueChanged(int)), this, SLOT(subviewTopLeftXChanged(int)));
	connect(ui->keyTLy, SIGNAL(valueChanged(int)), this, SLOT(subviewTopLeftYChanged(int)));
	connect(ui->keyTRx, SIGNAL(valueChanged(int)), this, SLOT(subviewTopRightXChanged(int)));
	connect(ui->keyTRy, SIGNAL(valueChanged(int)), this, SLOT(subviewTopRightYChanged(int)));
	connect(ui->keyBLx, SIGNAL(valueChanged(int)), this, SLOT(subviewBottomLeftXChanged(int)));
	connect(ui->keyBLy, SIGNAL(valueChanged(int)), this, SLOT(subviewBottomLeftYChanged(int)));
	connect(ui->keyBRx, SIGNAL(valueChanged(int)), this, SLOT(subviewBottomRightXChanged(int)));
	connect(ui->keyBRy, SIGNAL(valueChanged(int)), this, SLOT(subviewBottomRightYChanged(int)));

	connect(ui->outputX, SIGNAL(valueChanged(int)), this, SLOT(screenXChanged(int)));
	connect(ui->outputY, SIGNAL(valueChanged(int)), this, SLOT(screenYChanged(int)));
	connect(ui->outputWidth, SIGNAL(valueChanged(int)), this, SLOT(screenWChanged(int)));
	connect(ui->outputHeight, SIGNAL(valueChanged(int)), this, SLOT(screenHChanged(int)));

	connect(ui->viewportX, SIGNAL(valueChanged(int)), this, SLOT(viewportXChanged(int)));
	connect(ui->viewportY, SIGNAL(valueChanged(int)), this, SLOT(viewportYChanged(int)));
	connect(ui->viewportWidth, SIGNAL(valueChanged(int)), this, SLOT(viewportWChanged(int)));
	connect(ui->viewportHeight, SIGNAL(valueChanged(int)), this, SLOT(viewportHChanged(int)));

	connect(ui->optIgnoreAR, SIGNAL(toggled(bool)), this, SLOT(ignoreArBoxChanged(bool)));

	connect(ui->aphaMaskBrowseBtn, SIGNAL(clicked()), this, SLOT(alphamaskBrowse()));
	connect(ui->alphaMaskFile, SIGNAL(textChanged(QString)), this, SLOT(showAlphaMaskPreview(QString)));
	
	connect(ui->brightnessResetBtn, SIGNAL(clicked()), this, SLOT(brightReset()));
	connect(ui->contrastResetBtn, SIGNAL(clicked()), this, SLOT(contReset()));
	connect(ui->hueResetBtn, SIGNAL(clicked()), this, SLOT(hueReset()));
	connect(ui->saturationResetBtn, SIGNAL(clicked()), this, SLOT(satReset()));
}

void PlayerSetupDialog::setPlayerList(PlayerConnectionList *list)
{
	m_playerList = list;
	ui->playerListview->setModel(list);
	
	if(list->size() > 0)
	{ 
		ui->playerListview->setCurrentIndex(list->index(0,0));
		setCurrentPlayer(list->at(0));
	}
	else
		newPlayer();
}

void PlayerSetupDialog::setCurrentPlayer(PlayerConnection* con)
{
	if(con == m_con)
		return;
		
	m_stickyConnectionMessage = false;
	if(m_con)
	{
		disconnect(ui->playerName, 0, m_con, 0);
		disconnect(ui->playerHost, 0, m_con, 0);
		disconnect(ui->playerUser, 0, m_con, 0);
		disconnect(ui->playerPass, 0, m_con, 0);
		disconnect(m_con, 0, this, 0);
	}
	
	if(m_subviewModel)
	{
		delete m_subviewModel;
		m_subviewModel = 0;
	}
	
	m_con = con;
	
	if(!con)
	{
		ui->boxConnection->setEnabled(false);
		ui->boxOutput->setEnabled(false);
		ui->boxSubviewOpts->setEnabled(false);
		ui->boxSubviews->setEnabled(false);
		ui->boxKeystone->setEnabled(true);
		return;
	}
	
	if(con->isConnected())
		conConnected();
	else
		conDisconnected();
	
	// Set up the UI with values from the player before connecting slots so we dont needlessly update the object
	ui->playerName->setText(con->name());
	ui->playerHost->setText(con->host());
	ui->playerUser->setText(con->user());
	ui->playerPass->setText(con->pass());
	ui->autoconnectBox->setChecked(con->autoconnect());
	ui->optIgnoreAR->setChecked(con->aspectRatioMode() == Qt::IgnoreAspectRatio);
	
	QRect screen = con->screenRect();
	if(screen.isEmpty())
		con->setScreenRect(screen = QRect(0,0,1024,768));
	ui->outputX->setValue(screen.x());
	ui->outputY->setValue(screen.y());
	ui->outputWidth->setValue(screen.width());
	ui->outputHeight->setValue(screen.height());
	
	QRect view = con->viewportRect();
	if(view.isEmpty())
		con->setViewportRect(view = QRect(0,0,1000,750));
	ui->viewportX->setValue(view.x());
	ui->viewportY->setValue(view.y());
	ui->viewportWidth->setValue(view.width());
	ui->viewportHeight->setValue(view.height());
	
	connect(ui->playerName, SIGNAL(textChanged(QString)), con, SLOT(setName(QString)));
	connect(ui->playerHost, SIGNAL(textChanged(QString)), con, SLOT(setHost(QString)));
	connect(ui->playerUser, SIGNAL(textChanged(QString)), con, SLOT(setUser(QString)));
	connect(ui->playerPass, SIGNAL(textChanged(QString)), con, SLOT(setPass(QString)));
	
	connect(con, SIGNAL(connected()), this, SLOT(conConnected()));
	connect(con, SIGNAL(disconnected()), this, SLOT(conDisconnected()));
	connect(con, SIGNAL(loginSuccess()), this, SLOT(conLoginSuccess()));
	connect(con, SIGNAL(loginFailure()), this, SLOT(conLoginFailure()));
	connect(con, SIGNAL(playerError(QString)), this, SLOT(conPlayerError(QString)));
	connect(con, SIGNAL(pingResponseReceived(QString)), this, SLOT(conPingResponseReceived(QString)));
	connect(con, SIGNAL(testStarted()), this, SLOT(conTestStarted()));
	connect(con, SIGNAL(testEnded()), this, SLOT(conTestEnded()));
	connect(con, SIGNAL(testResults(bool)), this, SLOT(conTestResults(bool)));
	
	ui->boxConnection->setEnabled(true);
	ui->boxOutput->setEnabled(true);

	m_subviewModel = new PlayerSubviewsModel(con);

	ui->subviewListview->setModel(m_subviewModel);
	ui->subviewListview->setCurrentIndex(m_subviewModel->index(0,0));
	
	GLWidgetSubview *sub = !con->subviews().isEmpty() ? con->subviews().at(0) : 0;
	if(!sub)
		con->addSubview(sub = new GLWidgetSubview());
	if(sub->title().isEmpty())
		sub->setTitle("Default Subview");
		
	setCurrentSubview(sub); 
}

void PlayerSetupDialog::setCurrentSubview(GLWidgetSubview * sub)
{
	if(sub == m_sub)
		return;
	if(m_sub)
	{
		disconnect(ui->subviewTitle, 0, m_sub, 0);
		disconnect(ui->subviewViewportX, 0, m_sub, 0);
		disconnect(ui->subviewViewportY, 0, m_sub, 0);
		disconnect(ui->subviewViewportX2, 0, m_sub, 0);
		disconnect(ui->subviewViewportY2, 0, m_sub, 0);
		disconnect(ui->alphaMaskFile, 0, m_sub, 0);
		disconnect(ui->subviewBrightnessSlider, 0, m_sub, 0);
		disconnect(ui->subviewContrastSlider, 0, m_sub, 0);
		disconnect(ui->subviewHueSlider, 0, m_sub, 0);
		disconnect(ui->subviewSaturationSlider, 0, m_sub, 0);
		disconnect(ui->optFlipH, 0, m_sub, 0);
		disconnect(ui->optFlipV, 0, m_sub, 0);
	}

	m_sub = sub;
	if(!sub)
	{
		ui->boxSubviewOpts->setEnabled(false);
		ui->boxSubviews->setEnabled(false);
		ui->boxKeystone->setEnabled(false);

		return;
	}
	
	ui->subviewTitle->setText(sub->title());
	ui->subviewViewportX->setValue(sub->left() * 100.);
	ui->subviewViewportY->setValue(sub->top()  * 100.);
	ui->subviewViewportX2->setValue(sub->right()  * 100.);
	ui->subviewViewportY2->setValue(sub->bottom() * 100.);
	
	ui->alphaMaskFile->setText(sub->alphaMaskFile());
	ui->subviewBrightnessSlider->setValue(sub->brightness());
	ui->subviewContrastSlider->setValue(sub->contrast());
	ui->subviewHueSlider->setValue(sub->hue());
	ui->subviewSaturationSlider->setValue(sub->saturation());
	
	ui->optFlipH->setChecked(sub->flipHorizontal());
	ui->optFlipV->setChecked(sub->flipVertical());
	
	QPolygonF poly = sub->cornerTranslations();
	ui->keyTLx->setValue((int)poly[0].x());
	ui->keyTLy->setValue((int)poly[0].y());
	ui->keyTRx->setValue((int)poly[1].x());
	ui->keyTRy->setValue((int)poly[1].y());
	ui->keyBLx->setValue((int)poly[3].x());
	ui->keyBLy->setValue((int)poly[3].y());
	ui->keyBRx->setValue((int)poly[2].x());
	ui->keyBRy->setValue((int)poly[2].y());


	connect(ui->subviewTitle, SIGNAL(textChanged(QString)), sub, SLOT(setTitle(QString)));
	connect(ui->subviewViewportX, SIGNAL(valueChanged(double)), sub, SLOT(setLeftPercent(double)));
	connect(ui->subviewViewportY, SIGNAL(valueChanged(double)), sub, SLOT(setTopPercent(double)));
	connect(ui->subviewViewportX2, SIGNAL(valueChanged(double)), sub, SLOT(setRightPercent(double)));
	connect(ui->subviewViewportY2, SIGNAL(valueChanged(double)), sub, SLOT(setBottomPercent(double)));

	connect(ui->alphaMaskFile, SIGNAL(textChanged(QString)), sub, SLOT(setAlphaMaskFile(QString)));
	connect(ui->subviewBrightnessSlider, SIGNAL(valueChanged(int)), sub, SLOT(setBrightness(int)) );
	connect(ui->subviewContrastSlider, SIGNAL(valueChanged(int)), sub, SLOT(setContrast(int)) );
	connect(ui->subviewHueSlider, SIGNAL(valueChanged(int)), sub, SLOT(setHue(int)) );
	connect(ui->subviewSaturationSlider, SIGNAL(valueChanged(int)), sub, SLOT(setSaturation(int)) );

	connect(ui->optFlipH, SIGNAL(toggled(bool)), sub, SLOT(setFlipHorizontal(bool)));
	connect(ui->optFlipV, SIGNAL(toggled(bool)), sub, SLOT(setFlipVertical(bool)));

	ui->boxSubviewOpts->setEnabled(true);
	ui->boxSubviews->setEnabled(true);
	ui->boxKeystone->setEnabled(true);
}

void PlayerSetupDialog::newPlayer()
{
	if(!m_playerList)
		return;
		
	PlayerConnection *con = new PlayerConnection();
	m_playerList->addPlayer(con);
	setCurrentPlayer(con);
	ui->playerListview->setCurrentIndex(m_playerList->index(m_playerList->size()-1,0));
}

void PlayerSetupDialog::removePlayer()
{
	if(!m_con)
		return;
	if(!m_playerList)
		return;
		
	/// TODO confirm deletion
	
	m_playerList->removePlayer(m_con);
	m_con->deleteLater();
	m_con = 0;
	
	if(m_playerList->size() > 0)
		setCurrentPlayer(m_playerList->at(0));
	else
		setCurrentPlayer(0);
}

void PlayerSetupDialog::newSubview()
{
	if(!m_con)
		return;
		
	GLWidgetSubview *sub = new GLWidgetSubview();
	sub->setTitle(QString("Subview %1").arg(m_con->subviews().size()+1));
	m_con->addSubview(sub);
	setCurrentSubview(sub);
	ui->subviewListview->setCurrentIndex(m_subviewModel->index(m_con->subviews().size()-1,0));
}

void PlayerSetupDialog::removeSubview()
{
	if(!m_sub)
		return;
	if(!m_con)
		return;
		
	/// TODO confirm deletion
	
	m_con->removeSubview(m_sub);
	m_sub->deleteLater();
	m_sub = 0;
	
	if(m_con->subviews().size() > 0)
		setCurrentSubview(m_con->subviews().at(0));
	else
		setCurrentSubview(0);
}

void PlayerSetupDialog::testConnection()
{
	if(!m_con) 
		return;
	
	m_con->testConnection();
	
	ui->testConnectionBtn->setEnabled(false);
	ui->connectBtn->setEnabled(false);
	ui->testConnectionBtn->setText("Testing...");
}

void PlayerSetupDialog::connectPlayer()
{
	if(!m_con)
		return;
	
	if(m_con->isConnected())
		m_con->disconnectPlayer();
	else
		m_con->connectPlayer();
		
	ui->testConnectionBtn->setEnabled(false);
	ui->connectBtn->setEnabled(false);
	ui->connectBtn->setText("Connecting...");
}

void PlayerSetupDialog::playerSelected(const QModelIndex & idx)
{
	if(!m_playerList)
		return;
	if(idx.isValid())
		setCurrentPlayer(m_playerList->at(idx.row()));
}
//void PlayerSetupDialog::currentPlayerChanged(const QModelIndex &idx,const QModelIndex &);

void PlayerSetupDialog::subviewSelected(const QModelIndex & idx)
{
	if(!m_subviewModel)
		return;
	if(!m_con)
		return;
	if(idx.isValid())
		setCurrentSubview(m_con->subviews().at(idx.row()));
}
//void PlayerSetupDialog::currentSubviewChanged(const QModelIndex &idx,const QModelIndex &);

void PlayerSetupDialog::alphamaskBrowse()
{
	/// TODO add file browser logic
}

void PlayerSetupDialog::subviewTopLeftXChanged(int value)
{
	if(!m_sub)
		return;
	QPolygonF poly = m_sub->cornerTranslations();
	poly[0].setX(value);
	m_sub->setCornerTranslations(poly);
}
void PlayerSetupDialog::subviewTopLeftYChanged(int value)
{
	if(!m_sub)
		return;
	QPolygonF poly = m_sub->cornerTranslations();
	poly[0].setY(value);
	m_sub->setCornerTranslations(poly);
}

void PlayerSetupDialog::subviewTopRightXChanged(int value)
{
	if(!m_sub)
		return;
	QPolygonF poly = m_sub->cornerTranslations();
	poly[1].setX(value);
	m_sub->setCornerTranslations(poly);
}

void PlayerSetupDialog::subviewTopRightYChanged(int value)
{
	if(!m_sub)
		return;
	QPolygonF poly = m_sub->cornerTranslations();
	poly[1].setY(value);
	m_sub->setCornerTranslations(poly);
}

void PlayerSetupDialog::subviewBottomLeftXChanged(int value)
{
	if(!m_sub)
		return;
	QPolygonF poly = m_sub->cornerTranslations();
	poly[3].setX(value);
	m_sub->setCornerTranslations(poly);
}

void PlayerSetupDialog::subviewBottomLeftYChanged(int value)
{
	if(!m_sub)
		return;
	QPolygonF poly = m_sub->cornerTranslations();
	poly[3].setY(value);
	m_sub->setCornerTranslations(poly);
}

void PlayerSetupDialog::subviewBottomRightXChanged(int value)
{
	if(!m_sub)
		return;
	QPolygonF poly = m_sub->cornerTranslations();
	poly[2].setX(value);
	m_sub->setCornerTranslations(poly);
}

void PlayerSetupDialog::subviewBottomRightYChanged(int value)
{
	if(!m_sub)
		return;
	QPolygonF poly = m_sub->cornerTranslations();
	poly[2].setY(value);
	m_sub->setCornerTranslations(poly);
}


void PlayerSetupDialog::screenXChanged(int value)
{
	if(!m_con)
		return;
	QRect rect = m_con->screenRect();
	rect = QRect(value, rect.y(), 
		     rect.width(), rect.height());
	m_con->setScreenRect(rect);
}

void PlayerSetupDialog::screenYChanged(int value)
{
	if(!m_con)
		return;
	QRect rect = m_con->screenRect();
	rect = QRect(rect.x(), value, 
		     rect.width(), rect.height());
	m_con->setScreenRect(rect);
}

void PlayerSetupDialog::screenWChanged(int value)
{
	if(!m_con)
		return;
	QRect rect = m_con->screenRect();
	rect = QRect(rect.x(), rect.y(), 
		     value, rect.height());
	m_con->setScreenRect(rect);
}

void PlayerSetupDialog::screenHChanged(int value)
{
	if(!m_con)
		return;
	QRect rect = m_con->screenRect();
	rect = QRect(rect.x(), rect.y(), 
		     rect.width(), value);
	m_con->setScreenRect(rect);
}


void PlayerSetupDialog::viewportXChanged(int value)
{
	if(!m_con)
		return;
	QRect rect = m_con->viewportRect();
	rect = QRect(value, rect.y(), 
		     rect.width(), rect.height());
	m_con->setViewportRect(rect);
}

void PlayerSetupDialog::viewportYChanged(int value)
{
	if(!m_con)
		return;
	QRect rect = m_con->viewportRect();
	rect = QRect(rect.x(), value, 
		     rect.width(), rect.height());
	m_con->setViewportRect(rect);
}

void PlayerSetupDialog::viewportWChanged(int value)
{
	if(!m_con)
		return;
	QRect rect = m_con->viewportRect();
	rect = QRect(rect.x(), rect.y(), 
		     value, rect.height());
	m_con->setViewportRect(rect);
}

void PlayerSetupDialog::viewportHChanged(int value)

{
	if(!m_con)
		return;
	QRect rect = m_con->viewportRect();
	rect = QRect(rect.x(), rect.y(), 
		     rect.width(), value);
	m_con->setViewportRect(rect);
}

void PlayerSetupDialog::ignoreArBoxChanged(bool value)
{
	if(!m_con)
		return;
	m_con->setAspectRatioMode(value ? Qt::IgnoreAspectRatio : Qt::KeepAspectRatio);
}

void PlayerSetupDialog::showAlphaMaskPreview(const QString& value)
{
	ui->alphaMaskPreview->setPixmap(QPixmap(value));
}

void PlayerSetupDialog::brightReset()
{
	ui->subviewBrightnessSlider->setValue(0);
}

void PlayerSetupDialog::contReset()
{
	ui->subviewContrastSlider->setValue(0);
}

void PlayerSetupDialog::hueReset()
{
	ui->subviewHueSlider->setValue(0);
}

void PlayerSetupDialog::satReset()
{
	ui->subviewSaturationSlider->setValue(0);
}

void PlayerSetupDialog::conConnected()
{
	ui->connectionTestResults->setText("<font color=green><b>Connected</b></font>");
	
	ui->connectBtn->setEnabled(true);
	ui->connectBtn->setText("&Disconnect");
	ui->testConnectionBtn->setEnabled(false);
}

void PlayerSetupDialog::conDisconnected()
{
	if(m_stickyConnectionMessage)
		return;	
	ui->connectionTestResults->setText("<font color=gray>Disconnected</font>");
	
	ui->connectBtn->setText("&Connect");
	ui->testConnectionBtn->setEnabled(true);
}

void PlayerSetupDialog::conLoginFailure()
{
	ui->connectionTestResults->setText("<font color=red><b>Login Failed</b></font>");
}

void PlayerSetupDialog::conLoginSuccess()
{
	ui->connectionTestResults->setText(QString("<font color=green><b>Logged In, %1</b></font>").arg(m_con->playerVersion()));
}

void PlayerSetupDialog::conPlayerError(const QString& err)
{
	ui->connectionTestResults->setText(QString("<font color=red><b>Error:</b> %1</font>").arg(err));
	m_stickyConnectionMessage = true;
}

void PlayerSetupDialog::conPingResponseReceived(const QString& str)
{
	ui->connectionTestResults->setText(QString("<font color=green><b>%1</b></font>").arg(str));
}

void PlayerSetupDialog::conTestStarted()
{
	ui->connectionTestResults->setText("<font color=black><b>Testing...</b></font>");
	ui->connectBtn->setEnabled(false);
	ui->testConnectionBtn->setEnabled(false);
}

void PlayerSetupDialog::conTestEnded()
{
	ui->connectBtn->setEnabled(true);
	ui->testConnectionBtn->setEnabled(true);
	ui->testConnectionBtn->setText("&Test");
}

void PlayerSetupDialog::autoconBoxChanged(bool flag)
{
	if(!m_con)
		return;
	m_con->setAutoconnect(flag);
}

void PlayerSetupDialog::conTestResults(bool flag)
{
	if(flag)
		ui->connectionTestResults->setText(QString("<font color=green><b>Player OK, %1</b></font>").arg(m_con->playerVersion()));
}
