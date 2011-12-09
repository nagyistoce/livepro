#include <QApplication>
#include <QtGui>

#include "../ImageFilters.h"

// void showImage(QImage img, QString title="Image")
// {
// 	QLabel *label = new QLabel();
// 	label->setPixmap(QPixmap::fromImage(img));
// 	label->adjustSize();
// 	label->setWindowTitle(title);
// 	label->show();
// }

QVBoxLayout *vbox;
void showImage(QImage img,QString text)
{
	QPainter p(&img);
	p.setPen(Qt::red);
	p.drawRect(img.rect().adjusted(0,0,-1,-1));
	
	vbox->addWidget(new QLabel(text));
	
	QLabel *label = new QLabel();
 	Q_ASSERT(label!=NULL); 
 	label->setPixmap(QPixmap::fromImage(img));
 	vbox->addWidget(label);
 	//img.save(QString("debug/%1.png").arg(text));
}
	
int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	
	qApp->setApplicationName(argv[0]);
	qApp->setOrganizationName("Josiah Bryan");
	qApp->setOrganizationDomain("mybryanlife.com");
	
	/// Border attributes
	QColor	m_borderColor = Qt::blue;
	double	m_borderWidth = 3.0;

	/// Shadow attributes
	bool	m_shadowEnabled = true;
	double 	m_shadowBlurRadius = 3.0;
	QColor	m_shadowColor = Qt::black;
	double	m_shadowOpacity = 1.0;
	QPointF	m_shadowOffset = QPointF(3,3);
	
	QWidget *base = new QWidget();
	base->setWindowTitle("Shadow Test");
	/*QVBoxLayout **/vbox = new QVBoxLayout(base);
	
	//QLabel *label=0;
	
		
	int bw = (int)(m_borderWidth / 2);
		
	#define drawImageWithBorder(p, sourceImg)\
		p.drawImage(bw,bw,sourceImg); \
		p.setPen(QPen(m_borderColor,m_borderWidth));\
		p.drawRect(sourceImg.rect().adjusted(bw,bw,bw,bw)); 

		
	QImage sourceImg("../data/icon-next-large.png");
	//sourceImg = sourceImg.scaled(320,240);
	
	//if(!m_shadowEnabled && m_borderWidth < 0.001)
		//return sourceImg;
	QTime t;
	t.start();
		
	QSizeF originalSizeWithBorder = sourceImg.size();
	if(m_borderWidth > 0.001)
	{
		double x = m_borderWidth * 2;
		originalSizeWithBorder.rwidth() += x;
		originalSizeWithBorder.rheight() += x;
		
		if(!m_shadowEnabled)
		{
			QImage cache(originalSizeWithBorder.toSize(),QImage::Format_ARGB32_Premultiplied);
			memset(cache.scanLine(0),0,cache.byteCount());
			QPainter p(&cache);
			drawImageWithBorder(p,sourceImg);
			//return cache;
			return 1;
		}
	}
	
	
	double radius = m_shadowBlurRadius;
	
	// create temporary pixmap to hold a copy of the text
	QSizeF blurSize = ImageFilters::blurredSizeFor(originalSizeWithBorder, (int)radius);
	blurSize.rwidth() += fabs(m_shadowOffset.x()) + m_borderWidth;
	blurSize.rheight() += fabs(m_shadowOffset.y()) + m_borderWidth;
	//qDebug() << "Blur size:"<<blurSize<<", doc:"<<doc.size()<<", radius:"<<radius;
	QImage tmpImage(blurSize.toSize(),QImage::Format_ARGB32_Premultiplied);
	memset(tmpImage.scanLine(0),0,tmpImage.byteCount());
	
	// render the source image into a temporary buffer for bluring
	QPainter tmpPainter(&tmpImage);
	
	tmpPainter.save();
	double radiusSpacing = radius;// / 1.5;// * 1.5;
	QPointF translate1(radiusSpacing + (m_shadowOffset.x() > 0 ? m_shadowOffset.x() : 0), 
			   radiusSpacing + (m_shadowOffset.y() > 0 ? m_shadowOffset.y() : 0));
	//qDebug() << "stage1: radiusSpacing:"<<radiusSpacing<<", m_shadowOffset:"<<m_shadowOffset<<", translate1:"<<translate1; 
	
	tmpPainter.translate(translate1);
	
	if(m_borderWidth > 0.001)
	{
		drawImageWithBorder(tmpPainter, sourceImg);
	}
	else
		tmpPainter.drawImage(0,0,sourceImg);
	
	tmpPainter.restore();
	
	bool debug = true;
	
// 	if(debug)
// 		//tmpImage.save("debug/shadow-stage1.png");
// 		showImage(tmpImage,"stage1");
	
	// color the orignal image by applying a color to the copy using a QPainter::CompositionMode_DestinationIn operation. 
	// This produces a homogeneously-colored pixmap.
	QRect rect = tmpImage.rect();
	tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
	
	QColor color = m_shadowColor;
	color.setAlpha((int)(255.0 * m_shadowOpacity));
	tmpPainter.fillRect(rect, color);
	
	tmpPainter.end();
	
// 	if(debug)
// 		//tmpImage.save("debug/shadow-stage2.png");
// 		showImage(tmpImage,"stage2");

	// blur the colored text
	ImageFilters::blurImage(tmpImage, (int)radius);
	
// 	if(debug)
// 		//tmpImage.save("debug/shadow-stage3.png");
// 		showImage(tmpImage,"stage3");
	
	
	QPainter painter2(&tmpImage);
	
	painter2.save();
	QPointF translate2(radiusSpacing + (m_shadowOffset.x() < 0 ? m_shadowOffset.x() * -1 : 0), 
			   radiusSpacing + (m_shadowOffset.y() < 0 ? m_shadowOffset.y() * -1 : 0));
	//qDebug() << "stage12 radiusSpacing:"<<radiusSpacing<<", m_shadowOffset:"<<m_shadowOffset<<", translate2:"<<translate2;
	painter2.translate(translate2);
	
	// Render the original image (with or without the border) on top of the blurred copy
	if(m_borderWidth > 0.001)
	{
		drawImageWithBorder(painter2,sourceImg);
	}
	else
		painter2.drawImage(0,0,sourceImg);
	
	painter2.restore();
	
	qDebug() << "end: elapsed: "<<t.elapsed()<<" ms";
	
	if(debug)
		//cache.save("debug/shadow-stage5.png");
		showImage(tmpImage,"stage5");
	
	//return cache;

	base->adjustSize();
	base->show();

	return app.exec();
}

