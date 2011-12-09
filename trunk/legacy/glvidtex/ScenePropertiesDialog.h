#ifndef SCENEPROPERTIESDIALOG_H
#define SCENEPROPERTIESDIALOG_H

#include <QDialog>
#include <QPointer>

class GLScene;
namespace Ui {
	class ScenePropertiesDialog;
}

/** \class ScenePropertiesDialog
	Used internally by EditorWindow to setup the GLScene properties and choose a GLSceneType for the scene.
*/
class ScenePropertiesDialog : public QDialog {
	Q_OBJECT
public:
	ScenePropertiesDialog(GLScene *scene, QWidget *parent = 0);
	~ScenePropertiesDialog();

protected slots:
	void typeComboChanged(int);
	
	void okClicked();

protected:
	void changeEvent(QEvent *e);
	void loadUI();
	
private:
	Ui::ScenePropertiesDialog *ui;
	
	QPointer<GLScene> m_scene;
};

#endif // SCENEPROPERTIESDIALOG_H
