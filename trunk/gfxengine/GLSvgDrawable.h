#ifndef GLSvgDrawable_h
#define GLSvgDrawable_h

#include "GLImageDrawable.h"

class QSvgRenderer;

class GLSvgDrawable : public GLImageDrawable
{
	Q_OBJECT
	
public:
	GLSvgDrawable(QString file="", QObject *parent=0);
	virtual ~GLSvgDrawable();
	
	
// 	virtual void loadPropsFromMap(const QVariantMap&, bool onlyApplyIfChanged = false);
// 	virtual QVariantMap propsToMap();

public slots:
	virtual bool setImageFile(const QString&);
	
signals:
// 	void rendered();
	
protected:
	virtual void drawableResized(const QSizeF&);
/*	virtual void updateRects(bool secondSource=false);*/
	virtual bool canReleaseImage() { return false; }
	virtual void transformChanged();
	
private slots:
	void renderSvg();
	
private:
	QSvgRenderer *m_renderer;
	QTimer m_updateTimer;
};

#endif
