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
	include(../3rdparty/exiv2-0.18.2-qtbuild/qt_build_root.pri)
}

# qrencode used by the 'news' scene type to add a QR code to the scene.
# Not currently in subversion.
qrcode: {
	DEFINES += HAVE_QRCODE
	
	; Not in subversion yet ...
	include(../3rdparty/qrencode-3.1.0/qrencode-3.1.0.pri)
	
	HEADERS += QRCodeQtUtil.h
	SOURCES += QRCodeQtUtil.cpp
}



# Input
HEADERS += BMDOutput.h \		# Output to a DeckLink-compatible device or card. Currently a slow/low framerate process
           CameraThread.h \		# Primary capture interface (inherits VideoSource) - uses SimpleV4L2 internally on Linux, or libav on Windows/non-linux. Also supports BlackMagic (DeckLink) devices (see 'blackmagic' above)
           CornerItem.h \		# Creates a draggable corner for editing scenes
           EditorUtilityWidgets.h \	# Utility widgets for editing properties
           EntityList.h \		# List of HTML entities
           GLCommonShaders.h \		# GLSL code for shaders (used by GLWidget and GLVideoDrawable)
           GLDrawable.h \		# Common base class for drawables
           ExpandableWidget.h \		# A standalone utility widget which provides a header and content area, and click-header-to-expand/collapse-content functionality
           FlowLayout.h \		# FlowLayout, similar to java's flow layout
           GLDrawables.h \		# Generic header to include all drawables that inherit from GLDrawable
           GLEditorGraphicsScene.h \	# A QGraphicsScene-derivitive for editing GLSceneGroup
           GLImageDrawable.h \		# Draw a QImage 
           GLImageHttpDrawable.h \	# Draw a QImage received via a HTTP URL (supports polling and DViz) 
           GLRectDrawable.h \		# Draw a styled rectangle (border/fill/shadow/transparency)
           GLSceneGroup.h \		# Defines GLScene and GLSceneGroup classes - GLScene is a list of GLDrawables, and GLSceneGroup is a list of GLScenes
           GLSceneGroupType.h \		# Base class for custom GLSceneGroup types 
           GLSceneTypeCurrentWeather.h \# A simple current-day weather group
           GLSceneTypeNewsFeed.h \	# News feed based on Google News or an RSS feed
           GLSceneTypeRandomImage.h \	# Load a series of random images from a specified folder
           GLSceneTypeRandomVideo.h \	# Load a series of random videos from a specified folder
           GLSceneTypes.h \		# Includes all currently defined GLSceneTypes for easy access
           GLSpinnerDrawable.h \	# Draws an animated 'spinner'
           GLSvgDrawable.h \		# Draw a SVG loaded from a file
           GLTextDrawable.h \		# Draw rich text - also supports border, fill, shadow
           GLVideoDrawable.h \		# Base class to render a VideoSource using OpenGL or QGraphicsItem
           GLVideoFileDrawable.h \	# Play a video file using Qt's multimedia classes (with audio)
           GLVideoInputDrawable.h \	# Draw a video capture source (either from a remote source via VideoReceiver or locally captured using CameraThread)
           GLVideoLoopDrawable.h \	# Draw a video file using libav, no audio
           GLVideoMjpegDrawable.h \	# Draw a video feed from an MJPEG URL
           GLVideoReceiverDrawable.h \	# Draw a video feed from a VideoReceiver
           GLWidget.h \			# OpenGL widget for rendering GLScenes and GLDrawables
           HistogramFilter.h \		# A VideoFilter which converts a VideoSource into a histogram of that source
           ImageFilters.h \		# Blur filter
           MetaObjectUtil.h \		# Utilities for storing pointers to QMetaObjects and creating them
           MjpegThread.h \		# A VideoSource for MJPEG URLs
           RichTextRenderer.h \		# Rich text rendering engine
           RssParser.h \		# RSS feed parser
           RtfEditorWindow.h \		# Rich text editor
	   ScreenVideoSource.h \	# A VideoSource subclass which provides a video feed from a specified rectangle on the screen (screencap video feed)
           VideoConsumer.h \		# Base class for VideoConsumers such as VideoWidget and GLVideoDrawable
           VideoDifferenceFilter.h \	# A beta class for some background analysis research - defunct, not currently under development
           VideoEncoder.h \		# Development class for encoding video - not currently under development
           VideoFilter.h \		# Base class for VideoFilters such as HistogramFilter
           VideoFrame.h \		# VideoFrame is the core class for moving frames of video around this project
           VideoInputColorBalancer.h \	# A simple color blancer for two video sources - works, but needs revised
           VideoInputSenderManager.h \	# Manages setting up VideoSenders for all the enumerated video capture sources as reported by CameraThread
           VideoReceiver.h \		# Receive a video feed over the network from VideoSender, inherits VideoSource
           VideoSender.h \		# Send a feed over the network to a VideoRecever (functions as a server, listens on a given port), inherits VideoConsumer
           VideoSenderCommands.h \	# Common command string definition for VideoRecevier/VideoConsumer
           VideoSource.h \		# Base class definition of a VideoSource, used by GLVideoDrawable and VideoWidget
           VideoThread.h \		# Old interface for libav to place video loops, no audio - still used internally by GLVideoLoopDrawable
           VideoWidget.h		# Draw a single VideoSource in a QWidget (optionally uses QGLWidget)

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
