#ifndef MainWindow_H
#define MainWindow_H

#include <QtGui>

class PlayerConnection;
class GLVideoInputDrawable;
class GLSceneGroup;
class HistogramFilter;
class GLVideoDrawable;
class VideoReceiver;
class GLWidget;

class MainWindow : public QWidget
{
	Q_OBJECT
public:
	MainWindow();
	
protected slots:
	// Slot used to change text of m_connectBtn
	void textChanged(QString);
	
	// Connect to the server using the name in m_serverBox
	void connectToServer();

	// Set the filter type on the drawable based on the textual name
	void setAdvancedFilter(QString name);
	
	// Store the hints and send to player if connected
	void sendVidOpts();
	
	// Adjust the rect of the camera based on rotation
	void setStraightValue(double value);
	
	// Recreate UI
	void hintsReceived();
	
	// Extract list and update combo box
	void videoInputListReceived(const QStringList& list);
	
	// Combobox slot
	void videoSourceChanged(int idx);
	
	void hintWack();
	
	// Preset slots
	void loadPreset(bool rebuildUI=true);
	void savePreset();
	void deletePreset();
	
protected:
	// Convenience function to get the index in the box
	int indexOfFilter(int filter);
	
	// Create the controls, removing any existing controls
	void createControlUI();
	
	// Server name/IP
	QLineEdit *m_serverBox;
	
	// List of video inputs on the server
	QComboBox *m_inputBox;
	
	// List of video input connection strings
	QStringList m_videoInputs;
	
	// lock for videoInputListReceived()
	bool m_hasVideoInputsList;
	
	// store so we dont do unnecessary work
	int m_lastInputIdx;
	
	// flag for connection status
	bool m_isConnected;
	
	// Button to connect to server
	QPushButton *m_connectBtn;
	
	// Store ref to drawable so we can show changes on screen
	GLVideoInputDrawable *m_drw;
	
	// Used to send the camera to the player
	GLSceneGroup *m_group;
	
	// For adding/removing our scene
	GLWidget *m_glw;
	
	// Stored so we can enable/disable based on filter type
	QWidget *m_sharpAmountWidget;
	
	// Connection to the player, used to show the camera
	PlayerConnection *m_player;
	
	// Stores controls to tune the video source
	QVBoxLayout *m_layout;
	
	// Store ptrs so we can delete them when we change sources
	HistogramFilter *m_filter;
	GLVideoDrawable *m_filterDrawable;
	GLWidget *m_filterGlw;
	
	// Store ptrs to receivers so they dont get deleted when we change sources
	QList<VideoReceiver*> m_receivers;
	
	QTimer m_hintWackTimer;
	
	// Store con string to index presets
	QString m_currentConString;
	
	// Useful thigns for presets
	QComboBox *m_settingsCombo;
	QList<QVariantMap> m_settingsList;
};



#endif
