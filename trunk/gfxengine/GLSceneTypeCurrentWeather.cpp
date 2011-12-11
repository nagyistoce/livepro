#include "GLSceneTypeCurrentWeather.h"

#include "GLDrawable.h"

GLSceneTypeCurrentWeather::GLSceneTypeCurrentWeather(QObject *parent)
	: GLSceneType(parent)
{
	m_fieldInfoList
		<< FieldInfo("conditions",
			"Weather Conditions",
			"Text string describing the current weather conditions, such as 'Partly Cloudy.'",
			"Text",
			true)

		<< FieldInfo("temp",
			"Current Temperature",
			"A short numerical text giving the current temperature in farenheit, for example: 72*F",
			"Text",
			true)
			
		<< FieldInfo("high",
			"Today's Forecasted High Temperature",
			"A short numerical text giving the forecasted high temperature in farenheit, for example: 125*F",
			"Text",
			true)
			
		<< FieldInfo("temp",
			"Today's Forecasted Low Temperature",
			"A short numerical text giving the forecasted low temperature in farenheit, for example: -50*F",
			"Text",
			true)

		<< FieldInfo("winds",
			"Wind Conditions",
			"A short string giving the current wind conditions, such as 'Wind: SW at 9 mph'",
			"Text",
			true)

		<< FieldInfo("icon",
			"Weather Icon",
			"A scalable vector graphic (SVG) icon representing the current weather conditions.",
			"Svg",
			true)

		<< FieldInfo("location",
			"Location",
			"The normalized location description given by the server, such as 'Chicago, IL'",
			"Text",
			false)

		<< FieldInfo("background",
			"Background",
			"The background image, if included, can optionally be dimmed at night.",
			"Image",
			false)
		;

	m_paramInfoList
		<< ParameterInfo("location",
			"Location",
			"Can be a ZIP code or a location name (Chicago, IL)",
			QVariant::String,
			true,
			SLOT(setLocation(const QString&)))

		<< ParameterInfo("updateTime",
			"Update Time",
			"Time in minutes to wait between updates",
			QVariant::Int,
			true,
			SLOT(setUpdateTime(int)))

		<< ParameterInfo("setDimBackground",
			"Dim Background",
			"Dim background by 50% at night",
			QVariant::Bool,
			true,
			SLOT(setDimBackground(bool)));

	PropertyEditorFactory::PropertyEditorOptions opts;

	opts.reset();
	opts.maxLength = 9;
	m_paramInfoList[0].hints = opts;

	opts.reset();
	opts.min = 1;
	opts.max = 15;
	m_paramInfoList[1].hints = opts;

	opts.reset();
	opts.text = "Dim Background";
	m_paramInfoList[2].hints = opts;

	connect(&m_reloadTimer, SIGNAL(timeout()), this, SLOT(reloadData()));
	//m_reloadTimer.setInterval(1 * 60 * 1000); // every 1 minute
	//setParam
	setParam("updateTime", 1);

	setParam("dimBackground", true);

}

void GLSceneTypeCurrentWeather::setLiveStatus(bool flag)
{
	GLSceneType::setLiveStatus(flag);

	if(flag)
	{
		m_reloadTimer.start();
		applyFieldData();
	}
// 	else
// 	{
// 		m_reloadTimer.stop();
// 	}
}

void GLSceneTypeCurrentWeather::setParam(QString param, QVariant value)
{
	GLSceneType::setParam(param, value);

	if(param == "location")
		reloadData();
	else
	if(param == "updateTime")
		m_reloadTimer.setInterval(value.toInt() * 60 * 1000);
}

void GLSceneTypeCurrentWeather::reloadData()
{
	requestData(location());
}

void GLSceneTypeCurrentWeather::requestData(const QString &location)
{
	QUrl url("http://www.google.com/ig/api");
	url.addEncodedQueryItem("hl", "en");
	url.addEncodedQueryItem("weather", QUrl::toPercentEncoding(location));

	qDebug() << "GLSceneTypeCurrentWeather::requestData("<<location<<"): url:"<<url;

	QNetworkAccessManager *manager = new QNetworkAccessManager(this);
	connect(manager, SIGNAL(finished(QNetworkReply*)),
		this, SLOT(handleNetworkData(QNetworkReply*)));
	manager->get(QNetworkRequest(url));
}

void GLSceneTypeCurrentWeather::handleNetworkData(QNetworkReply *networkReply)
{
	QUrl url = networkReply->url();
	if (!networkReply->error())
		parseData(QString::fromUtf8(networkReply->readAll()));
	networkReply->deleteLater();
	networkReply->manager()->deleteLater();
}

QString GLSceneTypeCurrentWeather::extractIcon(const QString &data)
{
	if (m_icons.isEmpty())
	{
		m_icons["mostly_cloudy"]    = "weather-few-clouds";
		m_icons["cloudy"]           = "weather-overcast";
		m_icons["mostly_sunny"]     = "weather-sunny-very-few-clouds";
		m_icons["partly_cloudy"]    = "weather-sunny-very-few-clouds";
		m_icons["sunny"]            = "weather-sunny";
		m_icons["flurries"]         = "weather-snow";
		m_icons["fog"]              = "weather-fog";
		m_icons["haze"]             = "weather-haze";
		m_icons["icy"]              = "weather-icy";
		m_icons["sleet"]            = "weather-sleet";
		m_icons["chance_of_sleet"]  = "weather-sleet";
		m_icons["snow"]             = "weather-snow";
		m_icons["chance_of_snow"]   = "weather-snow";
		m_icons["mist"]             = "weather-showers";
		m_icons["rain"]             = "weather-showers";
		m_icons["chance_of_rain"]   = "weather-showers";
		m_icons["storm"]            = "weather-storm";
		m_icons["chance_of_storm"]  = "weather-storm";
		m_icons["thunderstorm"]     = "weather-thundershower";
		m_icons["chance_of_tstorm"] = "weather-thundershower";

		m_icons["night:sunny"]            = "weather-night-waxingcrescent-clear";
		m_icons["night:mostly_cloudy"]    = "weather-night-waxingcrescent-partially-cloudy";
		m_icons["night:cloudy"]           = "weather-night-waxingcrescent-cloudy";
		m_icons["night:mostly_sunny"]     = "weather-night-waxingcrescent-partially-cloudy";
		m_icons["night:partly_cloudy"]    = "weather-night-waxingcrescent-partially-cloudy";
		m_icons["night:flurries"]         = "weather-night-waxingcrescent-snow";
		m_icons["night:fog"]              = "weather-night-foggy";
// 		m_icons["haze"]             = "weather-haze";
// 		m_icons["icy"]              = "weather-icy";
// 		m_icons["sleet"]            = "weather-sleet";
// 		m_icons["chance_of_sleet"]  = "weather-sleet";
		m_icons["night:snow"]             = "weather-night-waxingcrescent-snow";
		m_icons["night:chance_of_snow"]   = "weather-night-waxingcrescent-snow";
// 		m_icons["mist"]             = "weather-showers";
// 		m_icons["rain"]             = "weather-showers";
// 		m_icons["chance_of_rain"]   = "weather-showers";
		m_icons["night:storm"]            = "weather-night-waxingcrescent-rain";
		m_icons["night:chance_of_storm"]  = "weather-night-waxingcrescent-rain";
		m_icons["night:thunderstorm"]     = "weather-night-waxingcrescent-thunderstorms";
		m_icons["night:chance_of_tstorm"] = "weather-night-waxingcrescent-thunderstorms";
	}
	QRegExp regex("([\\w]+).gif$");
	if (regex.indexIn(data) != -1)
	{
		QString i = regex.cap();
		i = i.left(i.length() - 4);

		bool night = isNight();

		QString name;
		if(night)
			name = m_icons.value("night:" + i);

		if(!night || name.isEmpty())
			name = m_icons.value(i);

		if (!name.isEmpty())
		{
			name.prepend("images/icons/");
			name.append(".svg");
			return name;
		}
	}
	return QString();
}

bool GLSceneTypeCurrentWeather::isNight()
{
	QTime time = QTime::currentTime();

	// The *right* way todo this in the future would be to check the LOCAL sunrise/sunset times
	// and then compare the current min/hour to the local sunrise/sunset!
	// For now, this is Good Enough.

	bool isNight = time.hour() <= 6 ||
		       time.hour() >= 6 + 12;
	
	if(m_fields["conditions"].toString() == "Sunny")
		isNight = false;

	return isNight;
}

// static QString toCelcius(QString t, QString unit)
// {
// 	bool ok = false;
// 	int degree = t.toInt(&ok);
// 	if (!ok)
// 		return QString();
// 	if (unit != "SI")
// 		degree = ((degree - 32) * 5 + 8)/ 9;
// 	return QString::number(degree) + QChar(176);
// }


#define GET_DATA_ATTR xml.attributes().value("data").toString()

void GLSceneTypeCurrentWeather::parseData(const QString &data)
{
	qDebug() << "GLSceneTypeCurrentWeather::parseData()";
	QString unitSystem;

	int temp = -1;
	QXmlStreamReader xml(data);
	while (!xml.atEnd())
	{
		xml.readNext();
		if (xml.tokenType() == QXmlStreamReader::StartElement)
		{
			if (xml.name() == "city")
			{
				QString city = GET_DATA_ATTR;
				setField("location", city);
			}
			if (xml.name() == "unit_system")
				unitSystem = xml.attributes().value("data").toString();

			// Parse current weather conditions
			if (xml.name() == "current_conditions")
			{
				while (!xml.atEnd())
				{
					xml.readNext();
					if (xml.name() == "current_conditions")
						break;

					if (xml.tokenType() == QXmlStreamReader::StartElement)
					{
						if (xml.name() == "condition")
						{
							setField("conditions", GET_DATA_ATTR);
						}
						if (xml.name() == "icon")
						{
							QString name = extractIcon(GET_DATA_ATTR);
							if (!name.isEmpty())
								setField("icon", name);
						}
						if (xml.name() == "temp_f")
						{
							QString tempStr = GET_DATA_ATTR;
							QString s = tempStr + QChar(176);
							temp = tempStr.toInt();
							setField("temp", s);
						}
						if (xml.name() == "wind_condition")
						{
							setField("winds", GET_DATA_ATTR);
						}
					}
				}
			}
			// Parse and collect the forecast conditions
			if (xml.name() == "forecast_conditions")
			{
				QString lowT, highT, day;
				while (!xml.atEnd())
				{
					xml.readNext();
					if (xml.name() == "forecast_conditions")
					{
						if (!day.isEmpty() && 
						    !lowT.isEmpty() &&
						    !highT.isEmpty())
						{
							QString today = QDate::shortDayName(QDate::currentDate().dayOfWeek());//, QDate::StandaloneFormat);
							if(day == today)
							{
								//qDebug() << "forecast_conditions: day:"<<day<<"/"<<today<<", lowT:"<<lowT<<", highT:"<<highT<<", temp:"<<temp;
								if(temp > highT.toInt())
									highT = QString("%1").arg(temp);
								if(temp < lowT.toInt())
									lowT = QString("%1").arg(temp);
								setField("high", highT + QChar(176));
								setField("low",  lowT + QChar(176));
							}
							
							
						}
						break;
					}
					if (xml.tokenType() == QXmlStreamReader::StartElement)
					{
						if (xml.name() == "day_of_week")
						{
							day = GET_DATA_ATTR;
						}
						if (xml.name() == "icon")
						{
							QString name = extractIcon(GET_DATA_ATTR);
							if (!name.isEmpty())
							{
								//
							}
						}
						if (xml.name() == "low")
							lowT = GET_DATA_ATTR;//toCelcius(GET_DATA_ATTR, unitSystem);
						if (xml.name() == "high")
							highT = GET_DATA_ATTR;//toCelcius(GET_DATA_ATTR, unitSystem);
					}
				}
			}
		}
	}

	if(dimBackground())
	{
		GLDrawable *background = lookupField("background");
		if(background)
			background->setOpacity(isNight() ? .5 : 1.);
	}
}
