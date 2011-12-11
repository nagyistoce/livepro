#ifndef RichTextRenderer_H
#define RichTextRenderer_H

#include <QtGui>

#ifndef ITEM_PROPDEF
#define ITEM_PROPSET(className,setterName,typeName,memberName) \
	void className::set##setterName(typeName newValue) { \
		m_##memberName = newValue; \
		update(); \
	}
#define ITEM_PROPDEF(setterName,typeName,memberName) void set##setterName(typeName value); typeName memberName() const { return m_##memberName; }
#define V_ITEM_PROPDEF(setterName,typeName,memberName) virtual void set##setterName(typeName value); virtual typeName memberName() const { return m_##memberName; }
#endif

class RichTextRenderer : public QObject
{
	Q_OBJECT
	
	Q_PROPERTY(QString html READ html WRITE setHtml)
	
	Q_PROPERTY(int 		textWidth	READ textWidth		WRITE setTextWidth);
	
	Q_PROPERTY(bool	   	outlineEnabled 	READ outlineEnabled 	WRITE setOutlineEnabled);
	Q_PROPERTY(QPen    	outlinePen   	READ outlinePen 	WRITE setOutlinePen);
	
	Q_PROPERTY(bool		fillEnabled	READ fillEnabled	WRITE setFillEnabled);
	Q_PROPERTY(QBrush  	fillBrush    	READ fillBrush 		WRITE setFillBrush);
	
	Q_PROPERTY(bool		shadowEnabled	READ shadowEnabled	WRITE setShadowEnabled);
	Q_PROPERTY(double 	shadowBlurRadius READ shadowBlurRadius	WRITE setShadowBlurRadius);
	Q_PROPERTY(double	shadowOffsetX 	READ shadowOffsetX	WRITE setShadowOffsetX);
	Q_PROPERTY(double	shadowOffsetY 	READ shadowOffsetY	WRITE setShadowOffsetY);
	Q_PROPERTY(QBrush	shadowBrush 	READ shadowBrush	WRITE setShadowBrush);

public:
	RichTextRenderer(QObject *parent=0);
	virtual ~RichTextRenderer() {}

	const QString & html() { return m_html; }
	
	void changeFontSize(double); // force change the html font size
	void changeFontColor(QColor); // force change the html font color
	double findFontSize();
	int fitToSize(const QSize &, int minimumFontSize = 0, int maximumFontSize = 500);
	QSize findNaturalSize(int atWidth = -1);
	
	// not valid until setHtml called
	QSize size() { return m_image.size(); }
	
	ITEM_PROPDEF(TextWidth,		int,	textWidth);
	
	ITEM_PROPDEF(OutlineEnabled,	bool,	outlineEnabled);
	ITEM_PROPDEF(OutlinePen,	QPen,	outlinePen);
	
	ITEM_PROPDEF(FillEnabled,	bool,	fillEnabled);
	ITEM_PROPDEF(FillBrush,		QBrush,	fillBrush);
	
	ITEM_PROPDEF(ShadowEnabled,	bool,	shadowEnabled);
	ITEM_PROPDEF(ShadowBlurRadius,	double,	shadowBlurRadius);
	ITEM_PROPDEF(ShadowOffsetX,	double,	shadowOffsetX);
	ITEM_PROPDEF(ShadowOffsetY,	double,	shadowOffsetY);
	ITEM_PROPDEF(ShadowBrush,	QBrush,	shadowBrush);
	
	bool updatesLocked() { return m_updatesLocked; }
	bool lockUpdates(bool); // returns old status
	
	QPointF scaling() { return m_scaling; }
	
	QSizeF unscaledImageSize() { return m_rawSize; }
	
public slots:
	void setHtml(const QString&);
	void update();
	QImage renderText();
	
	void setShadowColor(const QColor& c) { setShadowBrush(c); }
	void setFillColor(const QColor& c) { setFillBrush(c); }
	void setOutlineColor(const QColor& c) { setOutlinePen(QPen(c, outlinePen().widthF())); }
	void setOutlineWidth(double w) { setOutlinePen(QPen(outlinePen().color(), w)); }
	
	void setScaling(double x, double y) {  setScaling(QPointF(x,y)); }
	void setScaling(QPointF);

signals:
	void textRendered(QImage img);

private:
	QString m_html;
	QImage m_image;
	
	// text document, layouting & rendering
	QTextDocument * m_text;
	QTextDocument * m_shadowText;
	
	// properties
	int m_textWidth;
	
	bool m_outlineEnabled;
	QPen m_outlinePen;
	
	bool m_fillEnabled;
	QBrush m_fillBrush;
	
	bool m_shadowEnabled;
	double m_shadowBlurRadius;
	double m_shadowOffsetX;
	double m_shadowOffsetY;
	QBrush m_shadowBrush;
	
	static QCache<QString,double> static_autoTextSizeCache;
	
	bool m_frameChanged;
	bool m_updatesLocked;
	
	QTimer m_updateTimer;
	
	QPointF m_scaling;
	QSizeF m_rawSize;
};

#endif
