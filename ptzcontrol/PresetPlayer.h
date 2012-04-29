#ifndef PresetPlayer_H
#define PresetPlayer_H

#include <QtGui>
#include "MidiInputAdapter.h"
//class QextSerialPort;
class PanTiltClient;
class PlayerZoomAdapter;
class VideoSource;
class VideoReceiver;

class Preset
{
public:
	Preset() : x(127),y(127),z(127),name("Unamed") {}
	QVariantMap toMap() {
		QVariantMap map;
		map["x"] = x;
		map["y"] = y;
		map["z"] = z;
		map["name"] = name;
		return map;
	}
	void loadMap(QVariantMap map) {
		x = map["x"].toInt();
		y = map["y"].toInt();
		z = map["z"].toInt();
		name = map["name"].toString();
	}
		
	int x;
	int y;
	int z;
	QString name;
};

class RelDragWidget : public QWidget
{
	Q_OBJECT
public:
	RelDragWidget();
	void setXBox(QSpinBox *box);
	void setYBox(QSpinBox *box);
	void setZBox(QSpinBox *box);
	
	QSize sizeHint() const;

protected:
	void mousePressEvent(QMouseEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void mouseReleasEvent(QMouseEvent *);
	void wheelEvent(QWheelEvent *);
		
	void paintEvent(QPaintEvent *);
	
private:
	QSpinBox *m_xBox;
	QSpinBox *m_yBox;
	QSpinBox *m_zBox;
	
	bool m_mouseIsDown;
	
	QPoint m_mouseDownAt;
	QPoint m_mouseDownValues;

};

class Pixmap;	

class PresetPlayer : public QWidget
{
	Q_OBJECT
public:
	PresetPlayer();
	~PresetPlayer();
	int presetIndex(Preset*);
	QList<Preset*> presets() { return m_presets; }
	
public slots:
	void sendPreset(Preset*);
	
private slots:
	void presetNameChanged(QString);
	void presetXChanged(int);
	void presetYChanged(int);
	void presetZChanged(int);
	void presetPlayBtn();
	void presetDeleteBtn();
	
	void xChanged(int);
	void yChanged(int);
	void zChanged(int);
	
	void setWinMaxW(int v) { m_winMax = QSize(v, m_winMax.height()); QSettings().setValue("winMaxW",v); }
	void setWinMaxH(int v) { m_winMax = QSize(m_winMax.width(), v);  QSettings().setValue("winMaxH",v); }
	void setWinMinW(int v) { m_winMin = QSize(v, m_winMin.height()); QSettings().setValue("winMinW",v); }
	void setWinMinH(int v) { m_winMin = QSize(m_winMin.width(), v);  QSettings().setValue("winMinH",v); }
	
	void setupMidi();
	
	void addPreset();
	void addPreset(Preset*);
	
	void pointClicked(QPoint);
	
	void updateLive();
	void sceneSnapshot();
	
protected:
	void setupGui();
	void setupClient();
	void rebuildPresetGui();
	
	void loadPresets();
	void savePresets();
	
	void sendServoValues(int x, int y, int z, bool zero=false);

	void closeEvent(QCloseEvent*);
	
	
private:
	QSplitter *m_split;
	QSplitter *m_masterSplitter;
	
	QString m_vidhost;
	int     m_vidport;
	QString m_ptzhost;
	int     m_ptzport;
	
	QSize m_winMax;
	QSize m_winMin;
	
	QPoint frameToPanTilt(QPoint);
	QSize m_frameSize;
	
	VideoReceiver *m_src;
	
	//QextSerialPort *m_port;
	PanTiltClient *m_client;
	PlayerZoomAdapter *m_zoom;	
	QList<Preset*> m_presets;
	QSpinBox *m_xBox;
	QSpinBox *m_yBox;
	QSpinBox *m_zBox;
	
	QGridLayout *m_presetLayout;
	
	bool m_lockSendValues;
	
	QGraphicsScene *m_scene;
	QGraphicsPixmapItem *m_live;
	QTimer m_updateLiveTimer;
	QTimer m_snapshotTimer;
	
	QHash<QString,Pixmap*> m_images;
};

class PlayerZoomAdapter : public QObject
{
	Q_OBJECT

public:
	PlayerZoomAdapter(PanTiltClient *client);
	
	int zoom() { return m_zoom; }
	void setZoom(int zoom, bool zero=true);
	
	int zoomMinusVal() { return m_zoomMinusVal; }
	int zoomPlusVal() { return m_zoomPlusVal; }
	int zoomMidVal() { return m_zoomMidVal; }
	
	
public slots:
	void setZoomMinusVal(int v);
	void setZoomPlusVal(int v);
	void setZoomMidVal(int v);
	void zero();

protected:
	int m_zeroTime;
	int m_pulseTime;
	int m_zoomMinusVal;
	int m_zoomPlusVal;
	int m_zoomMidVal;
	int m_zoom;
	PanTiltClient *m_client;
	bool m_hasZeroed;
};


class PresetMidiInputAdapter : public MidiInputAdapter
{
public:
	static PresetMidiInputAdapter *instance(PresetPlayer *player=0);
	
	void reloadActionList();
	
protected:
	PresetMidiInputAdapter(PresetPlayer*);
	
	virtual void setupActionList();

private:
	static PresetMidiInputAdapter *m_inst;
};

#endif
