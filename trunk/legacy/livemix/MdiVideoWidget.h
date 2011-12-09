
#ifndef MdiVideoSource_h
#define MdiVideoSource_h

#include "MdiVideoChild.h"

class MdiVideoWidget : public MdiVideoChild
{
	Q_OBJECT
public:
	MdiVideoWidget(QWidget *parent=0);

	QString videoFile() { return m_videoFile; }

public slots:
	void setVideoFile(const QString& file);

protected slots:
	void browseButtonClicked();
	void returnPressed();
	
protected:
	QLineEdit * m_fileInput;
	QString m_videoFile;
	
};

#endif
