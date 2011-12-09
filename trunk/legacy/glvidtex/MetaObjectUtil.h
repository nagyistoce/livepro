#ifndef MetaObjectUtil_H
#define MetaObjectUtil_H

#include <QMetaObject>
#include <QHash>

#define MetaObjectUtil_Register(a) MetaObjectUtil::registerClass(a::staticMetaObject.className(), &a::staticMetaObject);


class MetaObjectUtil
{
private:
	static QHash<QString, const QMetaObject*> s_metaObjects;	

public:
	static const QMetaObject* metaObjectForClass(const QString& name)       { return s_metaObjects[name]; }
	static void registerClass(const QString& name, const QMetaObject *meta) { s_metaObjects[name] = meta; }
};

	
#endif
