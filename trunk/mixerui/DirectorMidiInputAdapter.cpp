
#include "DirectorMidiInputAdapter.h"

#include "DirectorWindow.h"

namespace DirectorMidiAction
{
	static DirectorWindow *mw;
	
	class BlackButton : public MidiInputAction
	{
		QString id() { return "64258ee6-f50e-4f94-ad6a-589a93ac2d40"; }
		QString name() { return "Black Output"; }
		void trigger(int)
		{
			mw->fadeBlack(!mw->isBlack());	
		}
	};
	
	class ActivateSourceButton : public MidiInputAction
	{
		public:
		ActivateSourceButton(int source=0) : MidiInputAction(), m_source(source) {}
		QString id() { return tr("ad44323f-22a2-43a5-9839-1d6476b5ca41:%1").arg(m_source); }
		QString name() { return tr("Activate Source %1").arg(m_source+1); }
		void trigger(int)
		{
			if(mw->switcher())
				mw->switcher()->activateSource(m_source);	
		}
		
		int m_source;
	};
	
	class FadeSpeedFader : public MidiInputAction
	{
		QString id() { return "1188e63f-4723-424c-b5fc-a1068da669c7"; }
		QString name() { return "Set Fade Speed"; }
		bool isFader() { return true; }
		void trigger(int value)
		{
			// 127 is max for my MIDI fader
			double percent = ((double)value) / 127.0;
			// setFadeSpeedPercent() expects an integer percent from 0-100 
			double speed = 100.0 * percent;
			
			mw->setFadeSpeedPercent((int)speed);
		}
	};
};

void DirectorMidiInputAdapter::setupActionList()
{
	m_actionList.clear();
	m_actionList
		<< new DirectorMidiAction::FadeSpeedFader()
		<< new DirectorMidiAction::BlackButton()
		<< new DirectorMidiAction::ActivateSourceButton(0)
		<< new DirectorMidiAction::ActivateSourceButton(1)
		<< new DirectorMidiAction::ActivateSourceButton(2)
		<< new DirectorMidiAction::ActivateSourceButton(3)
		<< new DirectorMidiAction::ActivateSourceButton(4)
		<< new DirectorMidiAction::ActivateSourceButton(5)
		<< new DirectorMidiAction::ActivateSourceButton(6)
		<< new DirectorMidiAction::ActivateSourceButton(7)
		<< new DirectorMidiAction::ActivateSourceButton(8)
		<< new DirectorMidiAction::ActivateSourceButton(9)
		;
	
	qDebug() << "DirectorMidiInputAdapter::setupActionList(): Added "<<m_actionList.size() <<" actions to the MIDI Action List";
}

DirectorMidiInputAdapter *DirectorMidiInputAdapter::m_inst = 0;

DirectorMidiInputAdapter::DirectorMidiInputAdapter()
	: MidiInputAdapter()
{
	setupActionList();
	loadSettings();
}

DirectorMidiInputAdapter *DirectorMidiInputAdapter::instance(DirectorWindow *win)
{
	if(!m_inst)
	{
		m_inst = new DirectorMidiInputAdapter();
		DirectorMidiAction::mw = win;
	}
	return m_inst;
}
