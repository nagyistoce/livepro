#include "GLSceneTypeRandomImage.h"
#include "../imgtool/exiv2-0.18.2-qtbuild/src/image.hpp"
#include "GLTextDrawable.h" // for htmlToPlainText

GLSceneTypeRandomImage::GLSceneTypeRandomImage(QObject *parent)
	: GLSceneType(parent)
	, m_currentIndex(0)
{
	m_fieldInfoList 
		<< FieldInfo("image", 
			"Image", 
			"The current image", 
			"Image", 
			true)
			
		<< FieldInfo("comments", 
			"Comments", 
			"The comments attached to the image via the EXIF data (if present.)", 
			"Text", 
			false)
			
		<< FieldInfo("filename", 
			"Filename", 
			"This class will attempt to parse the filename and extract a photograph number from it", 
			"Text", 
			false)
			
		<< FieldInfo("datetime", 
			"Date/Time", 
			"The date/time this file was taken (from the EXIF data.)", 
			"Text", 
			false);
		
		
	m_paramInfoList
		<< ParameterInfo("folder",
			"Folder",
			"Folder of images to display",
			QVariant::String,
			true,
			SLOT(setFolder(const QString&)))
			
		<< ParameterInfo("updateTime",
			"Update Time",
			"Time in minutes to wait between updates",
			QVariant::Int,
			true,
			SLOT(setUpdateTime(int)))
		
		<< ParameterInfo("changeTime",
			"Change Time",
			"Time in seconds to wait between changing images if needed",
			QVariant::Int,
			true,
			SLOT(setChangeTime(int)))
			
		<< ParameterInfo("notRandom",
			"Not Random",
			"Show images in order, not randomly",
			QVariant::Bool,
			true,
			SLOT(setNotRandom(bool)))
			
		<< ParameterInfo("autoRotate",
			"Auto Rotate",
			"Auto-rotate based on EXIF data",
			QVariant::Bool,
			true,
			SLOT(setAutoRotate(bool)))
		
		<< ParameterInfo("ignoreAR",
			"Ignore AR",
			"Stretch image to fill field provided",
			QVariant::Bool,
			true,
			SLOT(setIgnoreAR(bool)));
			
	PropertyEditorFactory::PropertyEditorOptions opts;
	
	opts.reset();
	opts.stringIsDir = true;
	m_paramInfoList[0].hints = opts;
	 
	opts.reset();
	opts.min = 1;
	opts.max = 15;
	m_paramInfoList[1].hints = opts;
	
	opts.reset();
	opts.min = 1;
	opts.max = 300;
	m_paramInfoList[2].hints = opts;
	
	opts.reset();
	opts.type = QVariant::Bool;
	opts.text = m_paramInfoList[3].title; 
	m_paramInfoList[3].hints = opts;
	
	opts.reset();
	opts.type = QVariant::Bool;
	opts.text = m_paramInfoList[4].title; 
	m_paramInfoList[4].hints = opts;
	
	opts.reset();
	opts.type = QVariant::Bool;
	opts.text = m_paramInfoList[5].title; 
	m_paramInfoList[5].hints = opts;
	
	connect(&m_reloadTimer, SIGNAL(timeout()), this, SLOT(reloadData()));
	connect(&m_changeTimer, SIGNAL(timeout()), this, SLOT(showNextImage()));
	//m_reloadTimer.setInterval(1 * 60 * 1000); // every 1 minute
	//setParam
	setParam("updateTime", 1);
	setParam("changeTime", 60);
	setParam("notRandom",  false);
	setParam("autoRotate", true);
	setParam("ignoreAR",   false);
}

void GLSceneTypeRandomImage::setLiveStatus(bool flag)
{
	//qDebug() << "GLSceneTypeRandomImage::setLiveStats(): "<<flag;
	GLSceneType::setLiveStatus(flag);
	
	if(flag)
	{
		m_reloadTimer.start();
		m_changeTimer.start();
		applyFieldData();
	}
	else
	{
		m_changeTimer.stop();
		QTimer::singleShot( 0, this, SLOT(showNextImage()) );
	}
}

void GLSceneTypeRandomImage::setParam(QString param, QVariant value)
{
	//qDebug() << "GLSceneTypeRandomImage::setParam(): "<<param<<": "<<value;
	GLSceneType::setParam(param, value);
	
	if(param == "folder")
		reloadData();
	else
	if(param == "updateTime")
		m_reloadTimer.setInterval(value.toInt() * 60 * 1000);
	else
	if(param == "changeTime")
		m_changeTimer.setInterval(value.toInt() * 1000);
}

void GLSceneTypeRandomImage::reloadData()
{
	//qDebug() << "GLSceneTypeRandomImage::reloadData()";
	readFolder(folder());
}

void GLSceneTypeRandomImage::readFolder(const QString &folder) 
{
	//qDebug() << "GLSceneTypeRandomImage::readFolder()";
	QDir dir(folder);
	dir.setNameFilters(QStringList() << "*.jpg" << "*.JPG" << "*.jpeg" << "*.png" << "*.PNG");
	QFileInfoList list = dir.entryInfoList(QDir::Files, QDir::Name);
	
	m_images.clear();
	
	//qDebug() << "GLSceneTypeRandomImage::readFolder(): Found "<<list.size()<<" images in "<<folder;
	
	foreach(QFileInfo info, list)
	{
		ImageItem item;
		QString fullFile = info.absoluteFilePath();
		//qDebug() << "GLSceneTypeRandomImage::readFolder(): Loading "<<fullFile;//<<" (ext:"<<ext<<")";
		
		QString comment = "";
		QString datetime = "";
		try
		{
			Exiv2::Image::AutoPtr exiv = Exiv2::ImageFactory::open(fullFile.toStdString()); 
			if(exiv.get() != 0)
			{
				exiv->readMetadata();
				Exiv2::ExifData& exifData = exiv->exifData();
				if (exifData.empty()) 
				{
					//qDebug() << fullFile << ": No Exif data found in the file";
				}
				else
				{
					comment = exifData["Exif.Image.ImageDescription"].toString().c_str();
					comment = GLTextDrawable::htmlToPlainText(comment);
					
					datetime = exifData["Exif.Photo.DateTimeOriginal"].toString().c_str();
				}
	
				if(comment.trimmed().isEmpty())
				{
					Exiv2::IptcData& iptcData = exiv->iptcData();
					comment = iptcData["Iptc.Application2.Caption"].toString().c_str();
					comment = GLTextDrawable::htmlToPlainText(comment);
					
					if (iptcData.empty()) 
					{
						//qDebug() << fullFile << ": No IPTC data found in the file";
					}
					else
					{
						//qDebug() << "GLSceneTypeRandomImage: IPTC Caption:"<<comment;
					}
				}
				else
				{
					//qDebug() << "GLSceneTypeRandomImage: EXIF Caption:"<<comment;
				}
				
				
			}
		}
		catch (Exiv2::AnyError& e) 
		{
			//std::cout << "Caught Exiv2 exception '" << e << "'\n";
			//return -1;
		}
		
		item.imageFile = fullFile;
		
		if(!comment.isEmpty())
			item.comments = comment;
		
		QFileInfo fileInfo(fullFile);
		QString fileName = fileInfo.baseName().toLower();
		fileName = fileName.replace(QRegExp("\\d{2,6}-\\{\\d\\}"),"");
		fileName = fileName.replace(QRegExp("(dsc_|sdc)"), "");
		
		if(!fileName.isEmpty())
		{
			item.parsedFilename = "Photograph # "+ fileName;
		}
		
		if(!datetime.isEmpty())
		{
			QDateTime parsedDate = QDateTime::fromString(datetime, "yyyy:MM:dd hh:mm:ss");
			item.datetime = "Photographed " + parsedDate.toString("dddd, MMMM d, yyyy");
		}
		
		
		m_images << item;
	}
	
	if(scene())
		showNextImage();
}

void GLSceneTypeRandomImage::showNextImage()
{
	//qDebug() << "GLSceneTypeRandomImage::showNextImage()";
	if(m_images.isEmpty())
	{
		qDebug() << "GLSceneTypeRandomImage::showNextImage(): No images loaded, nothing to show.";
		return;
	}
		
	if(!notRandom())
	{
		int high = m_images.size() - 1;
		if(high <= 0)
			high = 1;
// 		int low = 0;
		m_currentIndex = qrand() % (high + 1);
	}
	
	if(m_currentIndex < 0 || m_currentIndex >= m_images.size())
		m_currentIndex = 0;
	
	ImageItem item = m_images[m_currentIndex];
	
	setField("image", 	item.imageFile);
	setField("comments", 	item.comments);
	setField("filename",	item.parsedFilename);
	setField("datetime",	item.datetime);
	
	if(ignoreAR())
	{
		GLDrawable *gld = lookupField("image");
		
		if(GLImageDrawable *image = dynamic_cast<GLImageDrawable*>(gld))
			image->setAspectRatioMode(ignoreAR() ? 
				Qt::IgnoreAspectRatio :
				Qt::KeepAspectRatio   );
	}
	
	m_currentIndex++;
}

void GLSceneTypeRandomImage::sceneAttached(GLScene *)
{
	if(m_images.isEmpty() && !folder().isEmpty())
		reloadData();
		
	showNextImage();
}
