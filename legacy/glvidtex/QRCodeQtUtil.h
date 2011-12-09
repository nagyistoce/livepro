#ifndef QRCodeQtUtil_H
#define QRCodeQtUtil_H

#include <QImage>
#include <QColor>
#include "qrencode-3.1.0/qrencode.h"



/** \class QRCodeQtUtil
	Designed to encode and convert data to a QImage from the qrencode library (3.1.0) by Kentaro Fukuchi <fukuchi@megaui.net>.
*/

class QRCodeQtUtil
{
public:
	/** Encode a QString to a QRCode and render the code to a QImage */
	static QImage encode(const QString&,  int size=3, int margin=4, QColor background=QColor(Qt::white), QColor foreground=QColor(Qt::black));
	
	/** Render the QRCode data to a QImage */ 
	static QImage qrcodeToQImage(QRcode* qrcode, int size=3, int margin=4, QColor background=QColor(Qt::white), QColor foreground=QColor(Qt::black));
};

#endif
