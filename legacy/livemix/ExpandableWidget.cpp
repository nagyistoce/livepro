#include "ExpandableWidget.h"

#include <QToolButton>
#include <QVBoxLayout>
#include <QSettings>

ExpandableWidget::ExpandableWidget(const QString& title, QWidget *parent)
	: QWidget(parent)
	, m_title(title)
	, m_widget(0)
	, m_expanded(false)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0,0,0,0);

	m_button = new QToolButton(parent);
	m_button->setCheckable(true);
	m_button->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
	m_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	m_button->setArrowType(Qt::RightArrow);
	m_button->setIconSize(QSize(16, 16));
	m_button->setStyleSheet("border-top: 1px solid gray;background:rgb(200,200,200)");
	layout->addWidget(m_button);
	
	//setStyleSheet("ExpandableWidget {border-bottom: 1px solid gray}");
	
	connect(m_button, SIGNAL(toggled(bool)), this, SLOT(setExpanded(bool)));
	
	setTitle(title);
}

void ExpandableWidget::setWidget(QWidget *widget)
{
	bool wasExpanded = m_expanded;
	if(m_widget && m_expanded)
		setExpanded(false);
		
	m_widget = widget;
	
	if(wasExpanded)
		setExpanded(true);
}

void ExpandableWidget::setExpanded(bool expanded)
{
	m_expanded = expanded;
	m_button->setChecked(expanded);
	m_button->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
	
	if(m_widget)
	{
		if(expanded)
		{
			layout()->addWidget(m_widget);
			m_widget->show();
		}
		else
		{
			layout()->removeWidget(m_widget);
			m_widget->hide();
		}
	}
	
	QSettings settings;
	settings.setValue(QString("ExpandableWidget/state/%1").arg(m_title),expanded);
}

void ExpandableWidget::setTitle(const QString& title)
{
	m_title = title;
	m_button->setText(title);
	setObjectName(qPrintable(title));
	
	QSettings settings;
	setExpanded(settings.value(QString("ExpandableWidget/state/%1").arg(title),false).toBool());
}

void ExpandableWidget::setExpandedIfNoDefault(bool flag)
{
	QSettings settings;
	QVariant var = settings.value(QString("ExpandableWidget/state/%1").arg(m_title));
	if(!var.isValid())
		setExpanded(flag);
}
