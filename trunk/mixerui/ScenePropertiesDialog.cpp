#include "ScenePropertiesDialog.h"
#include "ui_ScenePropertiesDialog.h"

#include "GLSceneGroupType.h"
#include "GLSceneTypes.h"

ScenePropertiesDialog::ScenePropertiesDialog(GLScene *scene, QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::ScenePropertiesDialog)
	, m_scene(scene)
{
	ui->setupUi(this);
	loadUI();
}

ScenePropertiesDialog::~ScenePropertiesDialog()
{
	delete ui;
}

void ScenePropertiesDialog::loadUI()
{
	connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(okClicked()));
	
	ui->sceneName->setText(m_scene->sceneName());
	ui->duration->setValue(m_scene->duration());
	ui->autoDuration->setChecked(m_scene->autoDuration());
	ui->dateTime->setDateTime(m_scene->scheduledTime());
	ui->autoDateTime->setChecked(m_scene->autoSchedule());
	
	QStringList names;
	names << "(No Type Assigned)";
	GLSceneTypeList list = GLSceneTypeFactory::list();
	int idx = -1;
	int count = 1;
	foreach(GLSceneType *type, list)
	{
		if(m_scene->sceneType() &&
		   m_scene->sceneType()->id() == type->id())
			idx = count;
		names << type->title();
		count ++;
	}
		
	ui->sceneTypeCombo->addItems(names);
	connect(ui->sceneTypeCombo, SIGNAL(activated(int)), this, SLOT(typeComboChanged(int)));
	if(idx > -1)
	{
		ui->sceneTypeCombo->setCurrentIndex(idx);
		typeComboChanged(idx);
	}
	else
	{
		ui->auditResults->setText("<i>Choose a scene type from the list above.</i>");
	}
}

void ScenePropertiesDialog::typeComboChanged(int idx)
{
	idx --;
	GLSceneTypeList list = GLSceneTypeFactory::list();
	if(idx < 0 || idx >= list.size())
		return;
	
	// Get a pointer to the scenetype
	GLSceneType *type = list.value(idx);
	
	qDebug() << "ScenePropertiesDialog::typeComboChanged: "<<idx<<", type:"<<type<<", m_scene:"<<m_scene;
	
	// If no scenetype on the scene OR
	// if the type on the scene is not this type,
	// then attach this type to the scene. 
	if(!m_scene->sceneType() || 
	    m_scene->sceneType()->id() != type->id())
	{
		type->attachToScene(m_scene); 
	}
	
	// Allow the scenetype to audit the scene to make sure the required fields are present
	// and display any audit errors to the user
	QStringList htmlBuffer;
	GLSceneType::AuditErrorList errors = type->auditTemplate(m_scene);
	if(!errors.isEmpty())
	{
		//qDebug() << "PlayerWindow: [DEBUG]: Errors found attaching weather type to scene:";
		bool allErrors = GLSceneType::AuditError::hasErrors(errors);
		QString color = allErrors ? "red" : "orange";
		htmlBuffer << QString("<font color='%1'><b>%2 found while attaching the scene type:</b></font><br>").arg(color).arg(allErrors ? "Errors" : "Warnings");
		foreach(GLSceneType::AuditError error, errors)
		{
			htmlBuffer << QString("<font color='%2'>%1</font><br>").arg(error.toString()).arg(error.isWarning ? "orange" : "red"); 
		}
	}
	
	ui->auditResults->setText(htmlBuffer.join(""));
	
	// Get a reference to the form layout
	QFormLayout *form = dynamic_cast<QFormLayout*>(ui->typeSettingsBox->layout());
	if(!form)
		return;
		
	// REmove any existing widgets from the form
	while(form->count() > 0)
	{
		QLayoutItem * item = form->itemAt(form->count() - 1);
		form->removeItem(item);
		if(QWidget *widget = item->widget())
		{
			//qDebug() << "DrawableDirectorWidget::setCurrentDrawable: Deleting "<<widget<<" from form, total:"<<form->count()<<" items remaining.";
			form->removeWidget(widget);
			if(QWidget *label = form->labelForField(widget))
			{
				form->removeWidget(label);
				delete label;
			}
			delete widget;
		}
// 		else
// 		if(QLayout *layout = item->layout())
// 		{
// 			form->removeItem(item);
// 		}
		delete item;
	}
	
	// Create editor widgets for the parameters of the new scenetype
	GLSceneType::ParamInfoList params = type->parameters();
	foreach(GLSceneType::ParameterInfo info, params)
	{
		QWidget * widget = PropertyEditorFactory::generatePropertyEditor(m_scene->sceneType(), qPrintable(info.name), info.slot, info.hints);
		if(dynamic_cast<QCheckBox*>(widget))
		{
			// checkbox already has text
			form->addRow("", widget);
			// Empty field to allign widget with other input items
		}
		else
		{
			form->addRow(info.title.isEmpty() ? 
				PropertyEditorFactory::guessTitle(info.name) : info.title, 
				widget);
		}
		form->addRow("", new QLabel(QString("<i><font size='small'>%1</font></i>"). arg(info.description)));
	}
	
}

void ScenePropertiesDialog::okClicked()
{
	m_scene->setSceneName(ui->sceneName->text());
	m_scene->setDuration(ui->duration->value());
	m_scene->setAutoDuration(ui->autoDuration->isChecked());
	m_scene->setScheduledTime(ui->dateTime->dateTime());
	m_scene->setAutoSchedule(ui->autoDateTime->isChecked());
}

void ScenePropertiesDialog::changeEvent(QEvent *e)
{
	QDialog::changeEvent(e);
	switch (e->type()) 
	{
		case QEvent::LanguageChange:
			ui->retranslateUi(this);
			break;
		default:
			break;
	}
}
