#include "QRCodeQtUtil.h"

#include <QPainter>

QImage QRCodeQtUtil::encode(const QString& data, int size, int margin, QColor background, QColor foreground)
{
	QRcode* qrcode = QRcode_encodeString(qPrintable(data), 0, QR_ECLEVEL_L, QR_MODE_8,  1);
	if(!qrcode)
		return QImage();
	else
		return qrcodeToQImage(qrcode,size,margin,background,foreground);
}

QImage QRCodeQtUtil::qrcodeToQImage(QRcode *qrcode, int size, int margin, QColor background, QColor foreground)
{
	int datawidth = qrcode->width;
	int realwidth = datawidth * size + margin * 2;
	
	QImage image(realwidth,realwidth,QImage::Format_ARGB32);
	memset(image.scanLine(0),0,image.byteCount());
	
	QPainter painter(&image);
	painter.fillRect(image.rect(),background);
	for(int x=0;x<datawidth;x++)
		for(int y=0;y<datawidth;y++)
			if(1 & qrcode->data[y*datawidth+x])
				painter.fillRect(QRect(x*size + margin, y*size + margin, size, size), foreground);
	
	painter.end();
	
	return image;	
}
