#ifndef VideoInputColorBalancer_H
#define VideoInputColorBalancer_H

#include <QtGui>

class HistogramFilter;
class VideoReceiver;

class VideoInputColorBalancer : public QWidget
{
	Q_OBJECT
public:
	VideoInputColorBalancer(QWidget *parent=0);
	~VideoInputColorBalancer();
	
public slots:
	void setMasterSource(VideoReceiver *rx);
	void setSlaveSource(VideoReceiver *rx);
	
protected slots:
	void hsvStatsUpdated(int hMin, int hMax, int hAvg, 
	                     int sMin, int sMax, int sAvg,
	                     int vMin, int vMax, int vAvg);

	void setAutoAdjust(bool);
	void setAdjustMaster(bool);
	void setHue(int);
	void setSat(int);
	void setBright(int);
	void setContrast(int);
	
	void rxConnected(VideoReceiver *rx=0);
	
	void setAutoAdjustThreshold(int);
	
private:
	HistogramFilter *m_histoMaster;
	HistogramFilter *m_histoSlave;
	
	VideoReceiver *m_sourceMaster;
	VideoReceiver *m_sourceSlave;

	int m_masterStats[9];
	int m_slaveStats[9];
	
	int m_deltas[9];
	
	int m_vals[8];
	
	QSpinBox *m_hueBox;
	QSpinBox *m_satBox;
	QSpinBox *m_brightBox;
	QSpinBox *m_contrastBox;
	
	QLabel *m_hueDiff;
	QLabel *m_satDiff;
	QLabel *m_valDiff;
	
	int m_hueDecay;
	int m_satDecay;
	int m_valDecay;
	
	QRadioButton *m_radioMaster;
	QRadioButton *m_radioSlave;
	bool m_adjustMaster;
	bool m_autoAdjust;
	QGroupBox *m_manulAdjustments;
	
	QSpinBox *m_frameAccumNumBox;
	
	int m_autoAdjustThreshold;
	
	bool m_hasResetMaster;
	bool m_hasResetSlave;
};

#endif
