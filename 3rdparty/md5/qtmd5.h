#ifndef MD5_H
#define MD5_H

#include <QString>

// Originally from: http://www.owntech.org/doku.php?id=notes:qt:md5 - Accessed 20091015
// Integrated into DViz by Josiah Bryan 20091015

class MD5
{
public:
    MD5() {}
    static QString md5sum(QString val);
    static QString md5sum(QByteArray & ba);
};

#endif // MD5_H
