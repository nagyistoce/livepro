#ifndef DirectorMidiInputAdapter_H
#define DirectorMidiInputAdapter_H

#include "MidiInputAdapter.h"

class DirectorWindow;
class DirectorMidiInputAdapter : public MidiInputAdapter
{
public:
	static DirectorMidiInputAdapter *instance(DirectorWindow *win =0);
	
protected:
	DirectorMidiInputAdapter();
	
	virtual void setupActionList();

private:
	static DirectorMidiInputAdapter *m_inst;
};

#endif
