#include "RtfEditorWindow.h"
#include <QtGui>
#include "../livemix/EditorUtilityWidgets.h"
#include "../3rdparty/richtextedit/richtexteditor_p.h"
#include "GLTextDrawable.h"

RtfEditorWindow::RtfEditorWindow(GLTextDrawable *gld, QWidget *parent)
	: QDialog(parent)
	, m_gld(gld)
{
	connect(gld, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged(const QString&)));
	connect(gld, SIGNAL(destroyed()), this, SLOT(deleteLater()));
	
	QVBoxLayout *layout = new QVBoxLayout();
		
	PropertyEditorFactory::PropertyEditorOptions opts;

	opts.reset();
	
	QHBoxLayout *controls1 = new QHBoxLayout();
	QWidget *boolEdit = PropertyEditorFactory::generatePropertyEditor(gld, "isCountdown", SLOT(setIsCountdown(bool)), opts);
	QWidget *dateEdit  = PropertyEditorFactory::generatePropertyEditor(gld, "targetDateTime", SLOT(setTargetDateTime(QDateTime)), opts);
	QCheckBox *box = dynamic_cast<QCheckBox*>(boolEdit);
	if(box)
	{
		connect(boolEdit, SIGNAL(toggled(bool)), dateEdit, SLOT(setEnabled(bool)));
		dateEdit->setEnabled(box->isChecked());
	}
	controls1->addWidget(boolEdit);
	controls1->addWidget(dateEdit);
	
	QWidget *boolEdit2 = PropertyEditorFactory::generatePropertyEditor(gld, "isClock", SLOT(setIsClock(bool)), opts);
	QWidget *stringEdit = PropertyEditorFactory::generatePropertyEditor(gld, "clockFormat", SLOT(setClockFormat(const QString&)), opts);
	box = dynamic_cast<QCheckBox*>(boolEdit2);
	if(box)
	{
		connect(boolEdit2, SIGNAL(toggled(bool)), stringEdit, SLOT(setEnabled(bool)));
		stringEdit->setEnabled(box->isChecked());
	}
		
	controls1->addWidget(boolEdit2);
	controls1->addWidget(stringEdit);
	
	controls1->addStretch(1);
	layout->addLayout(controls1);
	
	QFrame *frame1 = new QFrame(this);
	frame1->setFrameStyle(QFrame::HLine | QFrame::Sunken);
	frame1->setMidLineWidth(1);
	layout->addWidget(frame1);
	
	
	QHBoxLayout *controls2 = new QHBoxLayout();
	
	QWidget *boolEdit3 = PropertyEditorFactory::generatePropertyEditor(gld, "isScroller", SLOT(setIsScroller(bool)), opts);
	
	opts.reset();
	opts.type = QVariant::Int;
	opts.min = 1;
	opts.max = 100;
	opts.suffix = " px";
	opts.defaultValue = 10;
	QWidget *spinEdit = PropertyEditorFactory::generatePropertyEditor(gld, "scrollerSpeed", SLOT(setScrollerSpeed(int)), opts);
	
	opts.reset();
	opts.stringIsFile = true;
	QWidget *fileEdit  = PropertyEditorFactory::generatePropertyEditor(gld, "iconFile", SLOT(setIconFile(const QString&)), opts);
	
	box = dynamic_cast<QCheckBox*>(boolEdit3);
	if(box)
	{
		connect(boolEdit3, SIGNAL(toggled(bool)), spinEdit, SLOT(setEnabled(bool)));
		connect(boolEdit3, SIGNAL(toggled(bool)), fileEdit, SLOT(setEnabled(bool)));
		spinEdit->setEnabled(box->isChecked());
		fileEdit->setEnabled(box->isChecked());
	}
		
	controls2->addWidget(boolEdit3);
	controls2->addWidget(spinEdit);
	controls2->addWidget(fileEdit);
	
	controls2->addStretch(1);
	layout->addLayout(controls2);
	
	QFrame *frame2 = new QFrame(this);
	frame2->setFrameStyle(QFrame::HLine | QFrame::Sunken);
	frame2->setMidLineWidth(1);
	layout->addWidget(frame2);
	
	
	m_rtfEditor = new RichTextEditorDialog();
	m_rtfEditor->setText(gld->text());
	//m_editor->initFontSize(m_model->findFontSize());
	
	//m_rtfEditor->show();
	layout->addWidget(m_rtfEditor);
	//m_rtfEditor->adjustSize();
	
	QPushButton *okButton = new QPushButton(tr("&Save"));

	okButton->setDefault(true);
	connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
	
	QPushButton *closeButton = new QPushButton(tr("Cancel"));
	connect(closeButton, SIGNAL(clicked()), this, SLOT(reject()));
	
	setWindowTitle("Edit Text");
	
	QBoxLayout *controlLayout = new QHBoxLayout;
	controlLayout->addStretch(1);
	controlLayout->addWidget(closeButton);
	controlLayout->addWidget(okButton);
	layout->addLayout(controlLayout);
	
	m_rtfEditor->focusEditor();
	
	setLayout(layout);
	
	readSettings();
}

void RtfEditorWindow::textChanged(const QString& text)
{
	m_rtfEditor->setText(text);
}

void RtfEditorWindow::okClicked()
{
	m_gld->setText(m_rtfEditor->text(Qt::RichText));
	close();
}

void RtfEditorWindow::closeEvent(QCloseEvent */*event*/)
{
	writeSettings();
}

void RtfEditorWindow::readSettings()
{
	QSettings settings;
	QPoint pos = settings.value("RtfEditorWindow/pos", QPoint(10, 10)).toPoint();
	QSize size = settings.value("RtfEditorWindow/size", QSize(640,480)).toSize();
	move(pos);
	resize(size);
}

void RtfEditorWindow::writeSettings()
{
	QSettings settings;
	settings.setValue("RtfEditorWindow/pos", pos());
	settings.setValue("RtfEditorWindow/size", size());
}

