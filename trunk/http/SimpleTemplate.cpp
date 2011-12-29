#include "SimpleTemplate.h"

#include <QTextStream>
#include <QStringList>

#include <QFile>
#include <QFileInfo>

#include <QDebug>

SimpleTemplate::SimpleTemplate(const QString& file, bool isRawTemplateData)
{
	if(isRawTemplateData)
	{
		m_data = file;
	}
	else
	{
		QFile fileHandle(file);
		if(!fileHandle.open(QIODevice::ReadOnly))
		{
			qWarning("SimpleTemplate: Unable to open '%s' for reading", qPrintable(file));
		}
		else
		{
			QTextStream os(&fileHandle);
			m_data = os.readAll();
			fileHandle.close();
		}
	}
}

void SimpleTemplate::param(const QString &param, const QString &value)
{
	m_params[param] = value;
}

void SimpleTemplate::param(const QString &param, const QVariant &value)
{
	m_params[param] = value;
}

void SimpleTemplate::param(const QString &param, const QVariantList &value)
{
	m_params[param] = value;
}

void SimpleTemplate::params(const QVariantMap &map)
{
	foreach(QString param, map.keys())
		m_params[param] = map.value(param);
}

QString SimpleTemplate::toString()
{
	QString data = m_data;
	
	QString incKey = "%%include:";
	QString endInc = "%%";
		
	int incIdx = data.indexOf(incKey,0,Qt::CaseInsensitive);
	if(incIdx > -1)
	{
		while(incIdx > -1)
		{
			int endIdx = data.indexOf(endInc,incIdx + incKey.length(),Qt::CaseInsensitive);
			if(endIdx < -1)
			{
				qDebug() << "SimpleTemplate: Unable to find end of include statement: "<<endInc;
				continue;
			}
				
			incIdx += incKey.length();
			int dataLen = qMax(0,endIdx - incIdx);
			QString incFile = data.mid(incIdx,dataLen);
			
			int blockIdx = incIdx - incKey.length();
			int blockLen = endIdx - incIdx + incKey.length() + endInc.length();
			
			//qDebug() << "incIdx:"<<incIdx<<", endIdx:"<<endIdx<<", dataLen:"<<dataLen<<", incKey:"<<incKey<<".length="<<incKey.length()<<", file:"<<incFile<<", blockIdx:"<<blockIdx<<", blockLen:"<<blockLen;//, data:"<<data;
			
			QString includeData = "";
			QFile file(incFile);
			if(!file.open(QIODevice::ReadOnly))
			{
				qWarning("SimpleTemplate: %%include%%: Unable to open '%s' for reading", qPrintable(incFile));
			}
			else
			{
				QTextStream os(&file);
				includeData = os.readAll();
				file.close();
			}
			
			data.replace(blockIdx,blockLen, includeData);
			
			incIdx = data.indexOf(incKey,incIdx,Qt::CaseInsensitive);
		}
	}
	
	foreach(QString key, m_params.keys())
	{
		QString valueKey = QString("%%%1%%").arg(key);
		QString loopKey  = QString("%%loop:%1%%").arg(key);
		QString ifKey    = QString("%%if:%1%%").arg(key);
		
		int ifIdx = data.indexOf(ifKey,0,Qt::CaseInsensitive);
		int loopIdx = data.indexOf(loopKey,0,Qt::CaseInsensitive);
		int valueIdx = data.indexOf(valueKey,0,Qt::CaseInsensitive);
		
		if(ifIdx > -1)
		{
			QString endIf = QString("%%/if:%1%%").arg(key);
			
			while(ifIdx > -1)
			{
				int endIdx = data.indexOf(endIf,ifIdx,Qt::CaseInsensitive);
				if(endIdx < -1)
				{
					qDebug() << "SimpleTemplate: Unable to find end of IF block: "<<endIf;
					continue;
				}
					
				ifIdx += ifKey.length();
				QString ifBlock = data.mid(ifIdx,endIdx - ifIdx);
				
				QVariant variant = m_params.value(key);
				bool flag; // leave un-init for now
				if(valueIdx > -1)
					flag = ! variant.toString().trimmed().isEmpty();
				else
				if(loopIdx > -1)
					flag = ! variant.toList().isEmpty();
				else
					flag = variant.toBool();
				
				QString elseKey = "%%else%%";
				int elseIdx = ifBlock.indexOf(elseKey,0,Qt::CaseInsensitive);
				
				int blockIdx = ifIdx  - ifKey.length();
				int blockLen = endIdx - ifIdx + ifKey.length() + endIf.length();
				if(elseIdx > -1)
				{
					QString blockPre  = ifBlock.left(elseIdx);
					QString blockPost = ifBlock.right(ifBlock.length() - elseIdx - elseKey.length());
					
					QString dataBlock = flag ? blockPre : blockPost;
					
					SimpleTemplate blockTmpl(dataBlock,true);
					blockTmpl.m_params = m_params;
					
					data.replace(blockIdx,blockLen, blockTmpl.toString());
					
				}
				else
				{
					if(flag)
					{
						// Save memory and time by only creating another template object
						// if this branch of the 'if' is true
						SimpleTemplate blockTmpl(ifBlock,true);
						blockTmpl.m_params = m_params;
						
						data.replace(blockIdx,blockLen, blockTmpl.toString());
					}
					else
					{
						data.replace(blockIdx,blockLen, "");
					}
				}
				
				ifIdx = data.indexOf(ifKey,ifIdx,Qt::CaseInsensitive);
			}
		}
		else
		if(loopIdx > -1)
		{
			QString endLoop = QString("%%/loop:%1%%").arg(key);
				
			while(loopIdx > -1)
			{
				int endIdx = data.indexOf(endLoop,loopIdx,Qt::CaseInsensitive);
				if(endIdx < -1)
				{
					qDebug() << "SimpleTemplate: Unable to find end of LOOP block: "<<endLoop;
					continue;
				}
					
				loopIdx += loopKey.length();
				QString loopBlock = data.mid(loopIdx,endIdx - loopIdx);
				
				SimpleTemplate blockTmpl(loopBlock,true);
				
				QStringList loopOutput;
				
				QVariantList dataList = m_params.value(key).toList();
				foreach(QVariant mapValue, dataList)
				{
					blockTmpl.m_params = mapValue.toMap();
					loopOutput << blockTmpl.toString();
				}
					
				int blockIdx = loopIdx - loopKey.length();
				int blockLen = endIdx  - loopIdx + loopKey.length() + endLoop.length();
				data.replace(blockIdx,blockLen, loopOutput.join(""));
				
				loopIdx = data.indexOf(loopKey,loopIdx,Qt::CaseInsensitive);
			}
		}
		else
		if(valueIdx > -1)
		{
			data.replace(valueKey, m_params.value(key).toString());
		}
	}
	return data;
}

