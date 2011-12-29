#ifndef SimpleTemplate_H
#define SimpleTemplate_H

#include <QHash>
#include <QString>
#include <QVariant>

class SimpleTemplate
{
public:
	SimpleTemplate(const QString& file, bool isRawTemplateData = false);
	
	void param(const QString &param, const QString &value);
	void param(const QString &param, const QVariant &value);
	void param(const QString &param, const QVariantList &value);
	void params(const QVariantMap &);
	
	QString toString();
	
private:
	QVariantMap m_params;
	
	QString m_data;
	QString m_file;
};


#endif

