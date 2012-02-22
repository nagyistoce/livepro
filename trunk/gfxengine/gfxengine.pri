MOC_DIR = .build
OBJECTS_DIR = .build
RCC_DIR = .build
UI_DIR = .build

VPATH += $$PWD
DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD

QT += opengl multimedia network svg


# Low-level V4L capture and output support for linux
unix: {
	HEADERS += \
		SimpleV4L2.h \
		V4LOutput.h
		
	SOURCES += \
		SimpleV4L2.cpp \
		V4LOutput.cpp
}

# CentOS fix - see http://theitdepartment.wordpress.com/2009/03/15/centos-qt-fcfreetypequeryface/
unix: {
	LIBS += -L/opt/fontconfig-2.4.2/lib
}

# OpenCV not required - but it nice to have.
# It's used to calculate deformation matrices in GLWidget for keystoning.
# Without OpenCV, the keystone deformations are downright ugly.
# Specify via: qmake CONFIG+=opencv 
opencv: {
	DEFINES += OPENCV_ENABLED
	LIBS += -L/usr/local/lib -lcv -lcxcore -L/opt/OpenCV-2.1.0/lib
	INCLUDEPATH += /opt/OpenCV-2.1.0/include
			
	HEADERS += \
		PointTrackingFilter.h
	SOURCES += \
		PointTrackingFilter.cpp

}

# Blackmagic DeckLink Capture Support
# Currently, only supported on linux - should be easily 
# adaptable to windows though.
blackmagic: {

	BMD_SDK_HOME = /opt/DeckLink-SDK-7.9.5/Linux
	
	#message("Blackmagic DeckLink API enabled, using: $$BMD_SDK_HOME")
	DEFINES += ENABLE_DECKLINK_CAPTURE
		
	INCLUDEPATH += $$BMD_SDK_HOME/include
	SOURCES     += $$BMD_SDK_HOME/include/DeckLinkAPIDispatch.cpp
}


# Set the mobility location on windows/linux
win32 {
    QT_MOBILITY_HOME = C:/Qt/qt-mobility-opensource-src-1.0.2
}
unix {
    #QT_MOBILITY_HOME = /opt/qt-mobility-opensource-src-1.0.1
     QT_MOBILITY_HOME = /opt/qt-mobility-opensource-src-1.1.0-beta2
    #QT_MOBILITY_HOME = /opt/qt-mobility-opensource-src-1.1.0-tp
}
	
# To enable, use: qmake CONFIG+=mobility, and make sure QT_MOBILITY_HOME is correct
# To run: Make sure QT_PLUGIN_PATH has $QT_MOBILITY_HOME/plugins added, else media will not play
# 	  ..and make sure $LD_LIBRARY_PATH has $QT_MOBILITY_HOME/lib - otherwise app will not start.
mobility: {
	isEmpty(QT_MOBILITY_SOURCE_TREE):QT_MOBILITY_SOURCE_TREE = $$QT_MOBILITY_HOME 
	isEmpty(QT_MOBILITY_BUILD_TREE):QT_MOBILITY_BUILD_TREE = $$QT_MOBILITY_HOME 
	
	#now include the dynamic config
	include($$QT_MOBILITY_BUILD_TREE/config.pri)
	
	CONFIG += mobility multimedia
	MOBILITY = multimedia
	#QT += multimediakit

	INCLUDEPATH += \
		$$QT_MOBILITY_HOME/src \
		$$QT_MOBILITY_HOME/src/global \
		$$QT_MOBILITY_HOME/src/multimedia \
		$$QT_MOBILITY_HOME/src/multimedia/audio \
		$$QT_MOBILITY_HOME/src/multimedia/video
		
	LIBS += -L$$QT_MOBILITY_BUILD_TREE/lib \
		-lQtMultimediaKit
	
	DEFINES += \
		QT_MOBILITY_ENABLED \
		HAS_QT_VIDEO_SOURCE
	
	HEADERS += \
		QtVideoSource.h
	SOURCES += \
		QtVideoSource.cpp
		
	#message("QtMobility enabled. Before running, ensure \$QT_PLUGIN_PATH contains $$QT_MOBILITY_HOME/plugins, otherwise media will not play.")
}
else: {
	message("QtMobility not enabled (use qmake CONFIG+=mobility and ensure $$QT_MOBILITY_HOME exists), QtVideoSource will not be built.")
    	DEFINES -= QT_MOBILITY_ENABLED
}

# FFMPEG is needed for looped videos and video input on Win32
include(../3rdparty/ffmpeg/ffmpeg.pri)

#unix {
#	LIBS += -lavdevice -lavformat -lavcodec -lavutil -lswscale -lbz2
#	INCLUDEPATH += /usr/include/ffmpeg
#}
#
#win32 {
#	INCLUDEPATH += \
#		../external/ffmpeg/include/msinttypes \
#		../external/ffmpeg/include/libswscale \
#		../external/ffmpeg/include/libavutil \
#		../external/ffmpeg/include/libavdevice \
#		../external/ffmpeg/include/libavformat \
#		../external/ffmpeg/include/libavcodec \
#		../external/ffmpeg/include
#
#	LIBS += -L"../external/ffmpeg/lib" \
#		-lavcodec-51 \
#		-lavformat-52 \
#		-lavutil-49 \
#		-lavdevice-52 \
#		-lswscale-0
#}


# MD5 is used to cache scaled images by GLImageDrawable
include(../3rdparty/md5/md5.pri)

# Rich text editor dialog used by RichTextEditorWindow.cpp
include(../3rdparty/richtextedit/richtextedit.pri)

# EXIV2 is optional and not currently in subversion.
# (but the code is in DViz if you *really* want to build with it.)
# Used by GLImageDrawable to extract rotation information. 
exiv2-qt: {
	DEFINES += HAVE_EXIV2_QT
	
	; Not in subversion yet ...
	include(3rdparty/exiv2-0.18.2-qtbuild/qt_build_root.pri)
}

# qrencode used by the 'news' scene type to add a QR code to the scene.
# Not currently in subversion.
qrcode: {
	DEFINES += HAVE_QRCODE
	
	; Not in subversion yet ...
	include(3rdparty/qrencode-3.1.0/qrencode-3.1.0.pri)
	
	HEADERS += QRCodeQtUtil.h
	SOURCES += QRCodeQtUtil.cpp
}



# Input
HEADERS += BMDOutput.h \
           CameraThread.h \
           CornerItem.h \
           EditorUtilityWidgets.h \
           EntityList.h \
           GLCommonShaders.h \
           GLDrawable.h \
           ExpandableWidget.h \
           FlowLayout.h \
           GLDrawables.h \
           GLEditorGraphicsScene.h \
           GLImageDrawable.h \
           GLImageHttpDrawable.h \
           GLRectDrawable.h \
           GLSceneGroup.h \
           GLSceneGroupType.h \
           GLSceneTypeCurrentWeather.h \
           GLSceneTypeNewsFeed.h \
           GLSceneTypeRandomImage.h \
           GLSceneTypeRandomVideo.h \
           GLSceneTypes.h \
           GLSpinnerDrawable.h \
           GLSvgDrawable.h \
           GLTextDrawable.h \
           GLVideoDrawable.h \
           GLVideoFileDrawable.h \
           GLVideoInputDrawable.h \
           GLVideoLoopDrawable.h \
           GLVideoMjpegDrawable.h \
           GLVideoReceiverDrawable.h \
           GLWidget.h \
           HistogramFilter.h \
           ImageFilters.h \
           MetaObjectUtil.h \
           MjpegThread.h \
           RichTextRenderer.h \
           RssParser.h \
           RtfEditorWindow.h \
	   ScreenVideoSource.h \
           VideoConsumer.h \
           VideoDifferenceFilter.h \
           VideoEncoder.h \
           VideoFilter.h \
           VideoFrame.h \
           VideoInputColorBalancer.h \
           VideoInputSenderManager.h \
           VideoReceiver.h \
           VideoSender.h \
           VideoSenderCommands.h \
           VideoSource.h \
           VideoThread.h \
           VideoWidget.h

SOURCES += BMDOutput.cpp \
           CameraThread.cpp \
           CornerItem.cpp \
           EditorUtilityWidgets.cpp \
           EntityList.cpp \
           ExpandableWidget.cpp \
           FlowLayout.cpp \
           GLDrawable.cpp \
           GLEditorGraphicsScene.cpp \
           GLImageDrawable.cpp \
           GLImageHttpDrawable.cpp \
           GLRectDrawable.cpp \
           GLSceneGroup.cpp \
           GLSceneGroupType.cpp \
           GLSceneTypeCurrentWeather.cpp \
           GLSceneTypeNewsFeed.cpp \
           GLSceneTypeRandomImage.cpp \
           GLSceneTypeRandomVideo.cpp \
           GLSpinnerDrawable.cpp \
           GLSvgDrawable.cpp \
           GLTextDrawable.cpp \
           GLVideoDrawable.cpp \
           GLVideoFileDrawable.cpp \
           GLVideoInputDrawable.cpp \
           GLVideoLoopDrawable.cpp \
           GLVideoMjpegDrawable.cpp \
           GLVideoReceiverDrawable.cpp \
           GLWidget.cpp \
           HistogramFilter.cpp \
           ImageFilters.cpp \
           MetaObjectUtil.cpp \
           MjpegThread.cpp \
           RichTextRenderer.cpp \
           RssParser.cpp \
           RtfEditorWindow.cpp \
           ScreenVideoSource.cpp \
           VideoConsumer.cpp \
           VideoDifferenceFilter.cpp \
           VideoEncoder.cpp \
           VideoFilter.cpp \
           VideoFrame.cpp \
           VideoInputColorBalancer.cpp \
           VideoInputSenderManager.cpp \
           VideoReceiver.cpp \
           VideoSender.cpp \
           VideoSource.cpp \
           VideoThread.cpp \
           VideoWidget.cpp \
