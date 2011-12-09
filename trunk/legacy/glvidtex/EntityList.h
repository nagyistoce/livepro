/* From the http://sourceforge.net/projects/kyndig/ project */

#ifndef ENTITYLIST_H
#define ENTITYLIST_H

#include <QString>
#include <QHash>

class EntityList
{
public:
	static QString encodeEntities(QString);
	static QString decodeEntities(QString);
private:
	static bool m_init;
	static void init();
	static QHash<QString,QString> m_dec;
	static QHash<QString,QString> m_enc;
};

#endif
