#ifndef ExpandableWidget_H
#define ExpandableWidget_H

#include <QWidget>
class QToolButton;

class ExpandableWidget : public QWidget
{
	Q_OBJECT
	
	Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded);
	Q_PROPERTY(QString title READ title WRITE setTitle);
	
public:
	ExpandableWidget(const QString& title="", QWidget *parent=0);
	
	bool isExpanded() { return m_expanded; }
	QString title() { return m_title; }
	
	QWidget *widget() { return m_widget; }
	void setWidget(QWidget *widget);
	
	void setExpandedIfNoDefault(bool);
	
public slots:
	void setExpanded(bool flag=true);
	void setTitle(const QString&);
	
private:
	QString	     m_title;
	QToolButton *m_button;
	QWidget     *m_widget;
	bool	     m_expanded;
};


#endif
