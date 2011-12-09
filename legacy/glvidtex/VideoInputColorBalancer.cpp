#include "VideoInputColorBalancer.h"

#include <QtGui>

#include "../livemix/VideoWidget.h"
#include "../livemix/EditorUtilityWidgets.h"
#include "HistogramFilter.h"
#include "VideoReceiver.h"

//#define VIDEO_HOST "192.168.0.18"
#define VIDEO_HOST "10.10.9.177"

#define VIDEO_FPS 5

VideoInputColorBalancer::VideoInputColorBalancer(QWidget *parent)
	: QWidget(parent)
	, m_histoMaster(new HistogramFilter())
	, m_histoSlave (new HistogramFilter())
	, m_adjustMaster(false)
	, m_autoAdjust(true)
	, m_autoAdjustThreshold(15)
	, m_hueDecay(0)
	, m_satDecay(0)
	, m_valDecay(0)
	, m_hasResetMaster(false)
	, m_hasResetSlave(false)
{
	connect(m_histoMaster, SIGNAL(hsvStatsUpdated(int , int , int , 
						      int , int , int ,
						      int , int , int )),

		         this,   SLOT(hsvStatsUpdated(int , int , int , 
						      int , int , int ,
						      int , int , int )));
	
	connect(m_histoSlave,  SIGNAL(hsvStatsUpdated(int , int , int , 
						      int , int , int ,
						      int , int , int )),

		         this,   SLOT(hsvStatsUpdated(int , int , int , 
						      int , int , int ,
						      int , int , int )));
	
	
	for(int i=0;i<9;i++)
		m_slaveStats[i]=0;
	
	for(int i=0;i<9;i++)
		m_masterStats[i]=0;
	
	for(int i=0;i<9;i++)
		m_deltas[i] = 0;
		
	// slave hsv
	m_vals[0]=50;
	m_vals[1]=50;
	m_vals[2]=50;
		
	// master hsv
	m_vals[3]=50;
	m_vals[4]=50;
	m_vals[5]=50;
	
	// contrast for slave/master
	m_vals[6]=50;
	m_vals[7]=50;
	
	
	QVBoxLayout *vbox = new QVBoxLayout(this);
	
	QHBoxLayout *hbox = new QHBoxLayout();
	hbox->addStretch(1);
	
	QCheckBox *enabFrameAccum = new QCheckBox("Average frames together");
	QSpinBox *frameAccumBox = new QSpinBox(this);
	frameAccumBox->setMinimum(1);
	frameAccumBox->setMaximum(100);
	frameAccumBox->setSuffix(" frames");
	
	hbox->addWidget(enabFrameAccum);
	hbox->addWidget(frameAccumBox);
	vbox->addLayout(hbox);
	
	// Setup video receiver and displays
	VideoWidget *vidMaster = new VideoWidget();
	m_sourceMaster = VideoReceiver::getReceiver(VIDEO_HOST,7755);
	
	m_sourceMaster->setFPS(VIDEO_FPS);
	vidMaster->setFps(VIDEO_FPS);
	vidMaster->setRenderFps(true);
	
// 	connect(m_sourceMaster, SIGNAL(connected()), this, SLOT(rxConnected()));
// 	if(m_sourceMaster->isConnected())
// 		rxConnected(m_sourceMaster);
	
	m_histoMaster->setVideoSource(m_sourceMaster);
	vidMaster->setVideoSource(m_histoMaster);
	vidMaster->setOverlayText("Master");
	
	vbox->addWidget(vidMaster);
	
	//m_sourceSlave = 0;
	
	m_sourceSlave = VideoReceiver::getReceiver(VIDEO_HOST,7756);
	m_sourceSlave->setFPS(VIDEO_FPS);
	
	VideoWidget *vidSlave = new VideoWidget();
	vidSlave->setFps(VIDEO_FPS);
	vidSlave->setRenderFps(true);
	
// 	connect(m_sourceSlave, SIGNAL(connected()), this, SLOT(rxConnected()));
// 	if(m_sourceSlave->isConnected())
// 		rxConnected(m_sourceSlave);
	
	m_histoSlave->setVideoSource(m_sourceSlave);
	vidSlave->setVideoSource(m_histoSlave);
	vidSlave->setOverlayText("Slave");
	
	vbox->addWidget(vidSlave);
	
	// Setup frame accum signal/slots
	enabFrameAccum->setChecked(m_histoMaster->frameAccumEnabled());
	frameAccumBox->setValue(m_histoMaster->frameAccumNum());
	
	connect(enabFrameAccum, SIGNAL(toggled(bool)), m_histoMaster, SLOT(setFrameAccumEnabled(bool)));
	connect(enabFrameAccum, SIGNAL(toggled(bool)), m_histoSlave, SLOT(setFrameAccumEnabled(bool)));
	connect(enabFrameAccum, SIGNAL(toggled(bool)), frameAccumBox, SLOT(setEnabled(bool)));
	
	connect(frameAccumBox, SIGNAL(valueChanged(int)), m_histoMaster, SLOT(setFrameAccumNum(int)));
	connect(frameAccumBox, SIGNAL(valueChanged(int)), m_histoSlave, SLOT(setFrameAccumNum(int)));
	
	
	// Setup adjustment controls
	hbox = new QHBoxLayout();
	QRadioButton *radioManual = new QRadioButton("&Manual Adjust",this);
	QRadioButton *radioAuto = new QRadioButton("&Auto Adjust",this);
	QSpinBox *thresholdSpin = new QSpinBox();
	thresholdSpin->setMinimum(1);
	thresholdSpin->setMaximum(255);
	thresholdSpin->setValue(m_autoAdjustThreshold);
	connect(thresholdSpin, SIGNAL(valueChanged(int)), this, SLOT(setAutoAdjustThreshold(int)));
	connect(radioAuto, SIGNAL(toggled(bool)), thresholdSpin, SLOT(setEnabled(bool)));
	
	hbox->addStretch(1);
	hbox->addWidget(radioManual);
	hbox->addWidget(radioAuto);
	hbox->addWidget(thresholdSpin);
	//hbox->addStretch(1);
	vbox->addLayout(hbox);
	
	connect(radioAuto, SIGNAL(toggled(bool)), this, SLOT(setAutoAdjust(bool)));
	
	m_manulAdjustments = new QGroupBox("Manual Adjustments");
	vbox->addWidget(m_manulAdjustments);
	
	hbox = new QHBoxLayout(m_manulAdjustments);
	
	QGridLayout *grid = new QGridLayout();
	int row = 0;
	
	/// Master/Slave Selection
	{
		QHBoxLayout *hbox = new QHBoxLayout();
		QRadioButton *radioSlave = new QRadioButton("&Adjust &Slave",this);
		QRadioButton *radioMaster = new QRadioButton("&Adjust Maste&r",this);
		hbox->addWidget(radioSlave);
		hbox->addWidget(radioMaster);
		hbox->addStretch(1);
		
		radioSlave->setChecked(true);
		m_radioMaster = radioMaster;
		m_radioSlave = radioSlave;
		
		connect(radioMaster, SIGNAL(toggled(bool)), this, SLOT(setAdjustMaster(bool)));
		
		grid->addLayout(hbox,row,0,1,4);
	}
	
	/// HUE Slider/Spinbox/Reset 
	{
		row ++;
		
		grid->addWidget(new QLabel("Hue:"),row,0);
		QSlider *slider = new QSlider();
		slider->setMinimum(0);
		slider->setMaximum(100);
		slider->setOrientation(Qt::Horizontal);
		
		QSpinBox *spin = new QSpinBox();
		spin->setMinimum(0);
		spin->setMaximum(100);
		spin->setSuffix("%");
		
		QPushButton *resetBtn = new QPushButton(QPixmap("../data/stock-close.png"), "");
		ObjectValueSetter *setter = new ObjectValueSetter(spin, SLOT(setValue(int)), 50);
		connect(resetBtn, SIGNAL(clicked()), setter, SLOT(executeSetValue()));
		
		connect(spin, SIGNAL(valueChanged(int)), slider, SLOT(setValue(int)));
		connect(slider, SIGNAL(valueChanged(int)), spin, SLOT(setValue(int)));
		
		connect(spin, SIGNAL(valueChanged(int)), this, SLOT(setHue(int)));
		
		m_hueBox = spin;
		grid->addWidget(slider,row,1);
		grid->addWidget(spin,row,2);
		grid->addWidget(resetBtn,row,3);
	}
	
	/// Saturation Slider/Spinbox/Reset 
	{
		row ++;
		
		grid->addWidget(new QLabel("Saturation:"),row,0);
		QSlider *slider = new QSlider();
		slider->setMinimum(0);
		slider->setMaximum(100);
		slider->setOrientation(Qt::Horizontal);
		
		QSpinBox *spin = new QSpinBox();
		spin->setMinimum(0);
		spin->setMaximum(100);
		spin->setSuffix("%");
		
		QPushButton *resetBtn = new QPushButton(QPixmap("../data/stock-close.png"), "");
		ObjectValueSetter *setter = new ObjectValueSetter(spin, SLOT(setValue(int)), 50);
		connect(resetBtn, SIGNAL(clicked()), setter, SLOT(executeSetValue()));
		
		connect(spin, SIGNAL(valueChanged(int)), slider, SLOT(setValue(int)));
		connect(slider, SIGNAL(valueChanged(int)), spin, SLOT(setValue(int)));
		
		connect(spin, SIGNAL(valueChanged(int)), this, SLOT(setSat(int)));
		
		m_satBox = spin;
		grid->addWidget(slider,row,1);
		grid->addWidget(spin,row,2);
		grid->addWidget(resetBtn,row,3);
	}
	
	/// Brightness Slider/Spinbox/Reset 
	{
		row ++;
		
		grid->addWidget(new QLabel("Brightness:"),row,0);
		QSlider *slider = new QSlider();
		slider->setMinimum(0);
		slider->setMaximum(100);
		slider->setOrientation(Qt::Horizontal);
		
		QSpinBox *spin = new QSpinBox();
		spin->setMinimum(0);
		spin->setMaximum(100);
		spin->setSuffix("%");
		
		QPushButton *resetBtn = new QPushButton(QPixmap("../data/stock-close.png"), "");
		ObjectValueSetter *setter = new ObjectValueSetter(spin, SLOT(setValue(int)), 50);
		connect(resetBtn, SIGNAL(clicked()), setter, SLOT(executeSetValue()));
		
		connect(spin, SIGNAL(valueChanged(int)), slider, SLOT(setValue(int)));
		connect(slider, SIGNAL(valueChanged(int)), spin, SLOT(setValue(int)));
		
		connect(spin, SIGNAL(valueChanged(int)), this, SLOT(setBright(int)));
		
		m_brightBox = spin;
		grid->addWidget(slider,row,1);
		grid->addWidget(spin,row,2);
		grid->addWidget(resetBtn,row,3);
	}
	
	/// Contrast Slider/Spinbox/Reset 
	{
		row ++;
		
		grid->addWidget(new QLabel("Contrast:"),row,0);
		QSlider *slider = new QSlider();
		slider->setMinimum(0);
		slider->setMaximum(100);
		slider->setOrientation(Qt::Horizontal);
		
		QSpinBox *spin = new QSpinBox();
		spin->setMinimum(0);
		spin->setMaximum(100);
		spin->setSuffix("%");
		
		QPushButton *resetBtn = new QPushButton(QPixmap("../data/stock-close.png"), "");
		ObjectValueSetter *setter = new ObjectValueSetter(spin, SLOT(setValue(int)), 50);
		connect(resetBtn, SIGNAL(clicked()), setter, SLOT(executeSetValue()));
		
		connect(spin, SIGNAL(valueChanged(int)), slider, SLOT(setValue(int)));
		connect(slider, SIGNAL(valueChanged(int)), spin, SLOT(setValue(int)));
		
		connect(spin, SIGNAL(valueChanged(int)), this, SLOT(setContrast(int)));
		
		m_contrastBox = spin;
		grid->addWidget(slider,row,1);
		grid->addWidget(spin,row,2);
		grid->addWidget(resetBtn,row,3);
	}
	
	
	hbox->addLayout(grid);
	
	QGroupBox *group = new QGroupBox("Master / Slave Deltas");
	vbox = new QVBoxLayout(group);
	QFormLayout *form = new QFormLayout();
	
	form->addRow("Hue:",m_hueDiff = new QLabel("-"));
	form->addRow("Sat:",m_satDiff = new QLabel("-"));
	form->addRow("Val:",m_valDiff = new QLabel("-"));
	
	vbox->addLayout(form);
	vbox->addStretch(1);
	
	hbox->addWidget(group);
	
	m_adjustMaster = false;
	m_autoAdjust = true;
	m_manulAdjustments->setEnabled(false);
	
	radioAuto->setChecked(false);
	radioManual->setChecked(true);
	
	setAutoAdjust(false);
	setAdjustMaster(false);
	
}

VideoInputColorBalancer::~VideoInputColorBalancer()
{}
	
	
void VideoInputColorBalancer::setMasterSource(VideoReceiver *rx)
{
	m_sourceMaster = rx;
	m_histoMaster->setVideoSource(m_sourceMaster);
}

void VideoInputColorBalancer::setSlaveSource(VideoReceiver *rx)
{
	m_sourceSlave = rx;
	m_histoSlave->setVideoSource(m_sourceSlave);
}

void VideoInputColorBalancer::hsvStatsUpdated(int hMin, int hMax, int hAvg, 
					      int sMin, int sMax, int sAvg,
					      int vMin, int vMax, int vAvg)
{
	//qDebug() << "hAvg:"<<hAvg<<", m_hasResetMaster:"<<m_hasResetMaster<<",  m_sourceMaster:"<<m_sourceMaster<<", isConnected:"<<m_sourceMaster->isConnected();
	if(hAvg > 0 && !m_hasResetMaster && m_sourceMaster && m_sourceMaster->isConnected())
	{
		m_hasResetMaster = true;
		rxConnected(m_sourceMaster);
	}
	
	if(hAvg > 0 && !m_hasResetSlave && m_sourceSlave && m_sourceSlave->isConnected())
	{
		m_hasResetSlave = true;
		rxConnected(m_sourceSlave);
	}
	
	HistogramFilter *filter = dynamic_cast<HistogramFilter*>(sender());
	
	if(filter == m_histoMaster)
	{
		m_masterStats[0] = hAvg;
		m_masterStats[1] = sAvg;
		m_masterStats[2] = vAvg;
	}
	else
	{
		m_slaveStats[0] = hAvg;
		m_slaveStats[1] = sAvg;
		m_slaveStats[2] = vAvg;
	}
	
	// convert deltas to percents
	m_deltas[0] = (m_masterStats[0] - m_slaveStats[0]) ;/// 255 * 100;
	m_deltas[1] = (m_masterStats[1] - m_slaveStats[1]) ;/// 255 * 100;
	m_deltas[2] = (m_masterStats[2] - m_slaveStats[2]) ;/// 255 * 100;
	
	m_hueDiff->setText(QString("<font color=red><b>%1</b></font")  .arg(m_deltas[0]));
	m_satDiff->setText(QString("<font color=green><b>%1</b></font").arg(m_deltas[1]));
	m_valDiff->setText(QString("<font color=blue><b>%1</b></font") .arg(m_deltas[2]));
	
	int decayReset = m_histoMaster->frameAccumNum();
	
	// The HSV 'decay' is to soften the effects of the requested change while the frames with the change accumlate in the histo buffer
	if(m_hueDecay >0)
		m_hueDecay --;
	if(m_hueDecay >0)
		m_deltas[0] /= m_hueDecay;
		
	if(m_satDecay >0)
		m_satDecay --;
	if(m_satDecay >0)
		m_deltas[1] /= m_satDecay;
	
	if(m_valDecay >0)
		m_valDecay --;
	if(m_valDecay >0)
		m_deltas[2] /= m_valDecay;
	
		
	
	if(m_autoAdjust)
	{
		int threshold = m_autoAdjustThreshold; //15; //(int) ( 255 * .075 );
		
		bool ht = abs(m_deltas[0]) > threshold;
		bool st = abs(m_deltas[1]) > threshold;
		bool vt = abs(m_deltas[2]) > threshold; 
		
		//qDebug() << "HSV Deltas: H:"<<m_deltas[0]<<"% - threshold?"<<ht<<", S:"<<m_deltas[1]<<"% - threshold?"<<st<<", V:"<<m_deltas[2]<<"% - threshold?"<<vt;
		
		if(ht)
		{
			int idx = 0;
			if(m_deltas[idx]>0)
			{
				int current = m_vals[idx];
				
				m_vals[idx]++;//=5;
				if(m_vals[idx]>100)
				{
					m_vals[idx] = 100;
					
					idx = 3;
					
					int c2 = m_vals[idx];
					m_vals[idx] --;
					if(m_vals[idx] < 0)
						m_vals[idx] = 0;
					
					if(m_vals[idx] != c2)
					{
						m_sourceMaster->setHue(m_vals[idx]);
						qDebug() << "Hue hit top threshold, but at 100% for slave, moving master down:" << m_vals[idx];
						
						m_hueDecay = decayReset; 
						
						setAdjustMaster(true);
						if(m_adjustMaster)
							m_hueBox->setValue(m_vals[idx]);
					}
				}
				else
				{
					if(m_vals[idx] != current)
					{
						m_sourceSlave->setHue(m_vals[idx]);
						qDebug() << "Hue hit top threshold, new hue:" << m_vals[idx];
						
						m_hueDecay = decayReset;
						
						setAdjustMaster(false);
						if(!m_adjustMaster)
							m_hueBox->setValue(m_vals[idx]);
					}
				}
			}
			else
			{
				int current = m_vals[idx];
				
				m_vals[idx]--;//=5;
				if(m_vals[idx]<0)
				{
					m_vals[idx] = 0;
					
					idx = 3;
					
					int c2 = m_vals[idx];
					m_vals[idx] ++;
					if(m_vals[idx] > 100)
						m_vals[idx] = 100;
					
					if(m_vals[idx] != c2)
					{
						m_sourceMaster->setHue(m_vals[idx]);
						qDebug() << "Hue hit bottom threshold, but slave 0% for slave, moving master up:" << m_vals[idx];
						
						m_hueDecay = decayReset;
						
						setAdjustMaster(true);
						if(m_adjustMaster)
							m_hueBox->setValue(m_vals[idx]);
					}
				}
				else
				{
					if(m_vals[idx] != current)
					{
						qDebug() << "Hue hit bottom threshold, new hue:" << m_vals[idx];
						m_sourceSlave->setHue(m_vals[idx]);
						
						m_hueDecay = decayReset;
						
						setAdjustMaster(false);
						if(!m_adjustMaster)
							m_hueBox->setValue(m_vals[idx]);
					}
				}
			}
		}
		
		if(st)
		{
			int idx = 1;
			if(m_deltas[idx]>0)
			{
				int current = m_vals[idx];
				
				m_vals[idx]++;//=5;
				if(m_vals[idx]>100)
				{
					m_vals[idx] = 100;
			
					idx = 4;
					
					int c2 = m_vals[idx];
					m_vals[idx] --;
					if(m_vals[idx] < 0)
						m_vals[idx] = 0;
					
					if(m_vals[idx] != c2)
					{
						m_sourceMaster->setSaturation(m_vals[idx]);
						qDebug() << "Saturation hit top threshold, but at 100% for slave, moving master down:" << m_vals[idx];
						
						m_satDecay = decayReset;
						
						setAdjustMaster(true);
						if(m_adjustMaster)
							m_satBox->setValue(m_vals[idx]);
					}
				}
				else
				{
					if(m_vals[idx] != current)
					{
						m_sourceSlave->setSaturation(m_vals[idx]);
						qDebug() << "Saturation hit top threshold, new sat:" << m_vals[idx];
						
						m_satDecay = decayReset;
						
						setAdjustMaster(false);
						if(!m_adjustMaster)
							m_satBox->setValue(m_vals[idx]);
					}
				}
			}
			else
			{
				int current = m_vals[idx];
				
				m_vals[idx]--;//=5;
				if(m_vals[idx]<0)
				{
					m_vals[idx] = 0;
					
					idx = 4;
					
					int c2 = m_vals[idx];
					m_vals[idx] ++;
					if(m_vals[idx] > 100)
						m_vals[idx] = 100;
					
					if(m_vals[idx] != c2)
					{
						m_sourceMaster->setSaturation(m_vals[idx]);
						qDebug() << "Saturation hit bottom threshold, but slave 0% for slave, moving master up:" << m_vals[idx];
						
						m_satDecay = decayReset;
						
						setAdjustMaster(true);
						if(m_adjustMaster)
							m_satBox->setValue(m_vals[idx]);
					}
				}
				else
				{
					if(m_vals[idx] != current)
					{
						qDebug() << "Saturation hit bottom threshold, new sat:" << m_vals[idx];
						m_sourceSlave->setSaturation(m_vals[idx]);
						
						m_satDecay = decayReset;
						
						setAdjustMaster(false);
						if(!m_adjustMaster)
							m_satBox->setValue(m_vals[idx]);
					}
				}
			}
		}
		
		if(vt)
		{
			int idx = 2;
			if(m_deltas[idx]>0)
			{
				int current = m_vals[idx];
				
				m_vals[idx]++;//=5;
				if(m_vals[idx]>100)
					m_vals[idx] = 100;
					
				if(m_vals[idx] != current)
				{
					m_sourceSlave->setBrightness(m_vals[idx]);
					qDebug() << "Value hit top threshold, new value:" << m_vals[idx];
					
					m_valDecay = decayReset;
					
					setAdjustMaster(true);
					if(!m_adjustMaster)
						m_brightBox->setValue(m_vals[idx]);
				}
			}
			else
			{
				int current = m_vals[idx];
				
				m_vals[idx]--;//=5;
				if(m_vals[idx]<0)
					m_vals[idx] = 0;
				
				if(m_vals[idx] != current)
				{
					qDebug() << "Value hit bottom threshold, new value:" << m_vals[idx];
					m_sourceSlave->setBrightness(m_vals[idx]);
					
					m_valDecay = decayReset;
					
					setAdjustMaster(false);
					if(!m_adjustMaster)
						m_brightBox->setValue(m_vals[idx]);
				}
			}
		}
	}
	
}

void VideoInputColorBalancer::setAutoAdjust(bool flag)
{
	m_autoAdjust = flag;
	m_manulAdjustments->setEnabled(!flag);
}

void VideoInputColorBalancer::setAdjustMaster(bool flag)
{
	m_adjustMaster = flag;
	if(m_radioMaster->isChecked() != flag)
		m_radioMaster->setChecked(flag);
	if(m_radioSlave->isChecked() == flag)
		m_radioSlave->setChecked(!flag);
	/// TODO load values to spinbox
	
	m_hueBox->setValue(flag ? m_vals[3] : m_vals[0]);
	m_satBox->setValue(flag ? m_vals[4] : m_vals[1]);
	m_brightBox->setValue(flag ? m_vals[5] : m_vals[2]);
	m_contrastBox->setValue(flag ? m_vals[7] : m_vals[6]);
	
}

void VideoInputColorBalancer::setHue(int val)
{
	int idx = m_adjustMaster ? 3 :0;
	if(m_vals[idx] != val)
	{
		m_vals[idx] = val;
		VideoReceiver *rx = m_adjustMaster ? m_sourceMaster : m_sourceSlave;
		if(rx)
			rx->setHue(val);
	}
}

void VideoInputColorBalancer::setSat(int val)
{
	int idx = m_adjustMaster ? 4 :1;
	if(m_vals[idx] != val)
	{
		m_vals[idx] = val;
		VideoReceiver *rx = m_adjustMaster ? m_sourceMaster : m_sourceSlave;
		if(rx)
			rx->setSaturation(val);
	}
}

void VideoInputColorBalancer::setBright(int val)
{
	int idx = m_adjustMaster ? 5 :2;
	//qDebug() << "VideoInputColorBalancer::setBright: "<<val<<", m_vals["<<idx<<"]:"<<m_vals[idx];
	if(m_vals[idx] != val)
	{
		m_vals[idx] = val;
		VideoReceiver *rx = m_adjustMaster ? m_sourceMaster : m_sourceSlave;
		if(rx)
			rx->setBrightness(val);
		
		//qDebug() << "VideoInputColorBalancer::setBright:  * setting *";
	}

}

void VideoInputColorBalancer::setContrast(int val)
{
	int idx = m_adjustMaster ? 7 :6;
	if(m_vals[idx] != val)
	{
		m_vals[idx] = val;
		VideoReceiver *rx = m_adjustMaster ? m_sourceMaster : m_sourceSlave;
		if(rx)
			rx->setContrast(val);
	}
}

void VideoInputColorBalancer::rxConnected(VideoReceiver *rx)
{
	if(!rx)
		rx = dynamic_cast<VideoReceiver*>(rx);
	if(!rx)
		return;
	rx->setHue(50);
	rx->setSaturation(50);
	rx->setBrightness(50);
	rx->setContrast(50);
}

void VideoInputColorBalancer::setAutoAdjustThreshold(int x)
{
	m_autoAdjustThreshold = x;
}
