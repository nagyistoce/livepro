#ifndef RtfEditorWindow_h
#define RtfEditorWindow_h

#include <QtGui>

class RichTextEditorDialog;
class GLTextDrawable;

class RtfEditorWindow : public QDialog
{
	Q_OBJECT
public:
	RtfEditorWindow(GLTextDrawable *drawable, QWidget *parent=0);
	 
protected:
	void closeEvent(QCloseEvent */*event*/);
	
private slots:
	void textChanged(const QString&);
	void okClicked();
	
private:
	void readSettings();
	void writeSettings();
	
	GLTextDrawable *m_gld;
	RichTextEditorDialog *m_rtfEditor;
};

#endif
