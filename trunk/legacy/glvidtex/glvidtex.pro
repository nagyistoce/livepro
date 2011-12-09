
MOC_DIR = .build
RCC_DIR = .build
UI_DIR  = .build
OBJECTS_DIR = .build


HEADERS       = GLWidget.h \
		../livemix/VideoSource.h \
		../livemix/VideoThread.h \
		../livemix/VideoFrame.h \
		../livemix/CameraThread.h \
		GLDrawable.h \
		GLVideoDrawable.h \
		../ImageFilters.h \
		RichTextRenderer.h \
		VideoSender.h \
		VideoReceiver.h \
		GLImageDrawable.h \
		GLVideoLoopDrawable.h \
		GLVideoInputDrawable.h \
		GLVideoFileDrawable.h \
		GLVideoReceiverDrawable.h \
		GLTextDrawable.h \
		GLSceneGroup.h \
		../livemix/MjpegThread.h \
		GLVideoMjpegDrawable.h \
		CornerItem.h \
		GLEditorGraphicsScene.h \
		GLPlayerServer.h \
		GLPlayerClient.h \
		VideoEncoder.h \
		GLSvgDrawable.h \
		GLRectDrawable.h \
		VideoConsumer.h \
		VideoFilter.h \
		HistogramFilter.h \ 
		StaticVideoSource.h \
		VideoDifferenceFilter.h \
		#FaceDetectFilter.h \
		GLSceneGroupType.h \
		GLSceneTypeCurrentWeather.h \
		GLSpinnerDrawable.h \
		QtGetOpt.h \
		../livemix/DVizSharedMemoryThread.h \
		SharedMemorySender.h \
		GLSceneTypeNewsFeed.h \
		GLSceneTypeRandomImage.h \
		GLSceneTypeRandomVideo.h \
		EntityList.h \
		BMDOutput.h \
		RssParser.h \
		GLImageHttpDrawable.h
		
		
SOURCES       = GLWidget.cpp \
		../livemix/VideoSource.cpp \
		../livemix/VideoThread.cpp \
		../livemix/VideoFrame.cpp \
		../livemix/CameraThread.cpp \
		GLDrawable.cpp \
		GLVideoDrawable.cpp \
		../ImageFilters.cpp \
		RichTextRenderer.cpp \
		VideoSender.cpp \
		VideoReceiver.cpp \
		GLImageDrawable.cpp \
		GLVideoLoopDrawable.cpp \
		GLVideoInputDrawable.cpp \
		GLVideoFileDrawable.cpp \
		GLVideoReceiverDrawable.cpp \
		GLTextDrawable.cpp \
		GLSceneGroup.cpp \
		MetaObjectUtil.cpp \
		../livemix/MjpegThread.cpp \
		GLVideoMjpegDrawable.cpp \
		CornerItem.cpp \
		GLEditorGraphicsScene.cpp   \
		GLPlayerServer.cpp \
		GLPlayerClient.cpp \
		VideoEncoder.cpp \
		GLSvgDrawable.cpp \
		GLRectDrawable.cpp \
		VideoConsumer.cpp \
		VideoFilter.cpp \
		HistogramFilter.cpp \
		StaticVideoSource.cpp \
		VideoDifferenceFilter.cpp \
		#FaceDetectFilter.cpp \
		GLSceneGroupType.cpp \
		QtGetOpt.cpp  \
		GLSpinnerDrawable.cpp \
		../livemix/DVizSharedMemoryThread.cpp \
		SharedMemorySender.cpp \
		GLSceneTypeCurrentWeather.cpp \
		GLSceneTypeNewsFeed.cpp \
		GLSceneTypeRandomImage.cpp \
		GLSceneTypeRandomVideo.cpp \
		EntityList.cpp \
		BMDOutput.cpp \
		RssParser.cpp \
		GLImageHttpDrawable.cpp 
	
# MD5 is used for caching in GLImageDrawable
include(../3rdparty/md5/md5.pri)

# Generating QR Codes in GLSceneTypeNewsFeed
include(qrencode-3.1.0/qrencode-3.1.0.pri)
HEADERS += QRCodeQtUtil.h
SOURCES += QRCodeQtUtil.cpp

# Exiv is used for EXIF data extraction in GLImageDrawable to auto-rotate
# and in 'glslideshow' to caption images
include(../imgtool/exiv2-0.18.2-qtbuild/qt_build_root.pri)

# Include dviz resources to get the rich text editor icons
RESOURCES += ../dviz.qrc


# 'editor' compile target
editor: {
	TARGET = gleditor
	
	FORMS += ScenePropertiesDialog.ui
	
	HEADERS += ../livemix/EditorUtilityWidgets.h \
		../livemix/ExpandableWidget.h \
		EditorWindow.h \
		EditorGraphicsView.h \
		RtfEditorWindow.h \
		ScenePropertiesDialog.h
		
	SOURCES += editor-main.cpp \
		../livemix/EditorUtilityWidgets.cpp \
		../livemix/ExpandableWidget.cpp \
		EditorWindow.cpp \
		EditorGraphicsView.cpp \
		RtfEditorWindow.cpp \
		ScenePropertiesDialog.cpp
		
	include(../3rdparty/richtextedit/richtextedit.pri)
	include(../qtcolorpicker/qtcolorpicker.pri)
}

# 'gldirector' compile target
director: {
	TARGET = gldirector
	
	FORMS += DirectorWindow.ui \
		 PlayerSetupDialog.ui \
		 DrawableDirectorWidget.ui \
		 ScenePropertiesDialog.ui


	
	HEADERS += ../livemix/EditorUtilityWidgets.h \
		../livemix/ExpandableWidget.h \
		EditorWindow.h \
		EditorGraphicsView.h \
		DirectorWindow.h \
		PlayerConnection.h \
		RtfEditorWindow.h \
		PlayerSetupDialog.h \
		KeystonePointsEditor.h \
		FlowLayout.h \
		../livemix/VideoWidget.h \
		VideoInputSenderManager.h \
		DrawableDirectorWidget.h \
		ScenePropertiesDialog.h \
		VideoInputColorBalancer.h \
		DirectorMidiInputAdapter.h 
	
		
		
	SOURCES += director-main.cpp \
		../livemix/EditorUtilityWidgets.cpp \
		../livemix/ExpandableWidget.cpp \
		EditorWindow.cpp \
		EditorGraphicsView.cpp \
		DirectorWindow.cpp \
		PlayerConnection.cpp \
		RtfEditorWindow.cpp \
		PlayerSetupDialog.cpp \
		KeystonePointsEditor.cpp \
		FlowLayout.cpp \
		../livemix/VideoWidget.cpp \
		VideoInputSenderManager.cpp \
		DrawableDirectorWidget.cpp \
		ScenePropertiesDialog.cpp \
		VideoInputColorBalancer.cpp \
		DirectorMidiInputAdapter.cpp
		
	include(../3rdparty/richtextedit/richtextedit.pri)
	include(../qtcolorpicker/qtcolorpicker.pri)
	
	# MIDI Input for camera switching / xfade speed control 
	include(../miditcp/miditcp.pri)

}

# 'glplayer' ('glvidtex') compile target
player | glvidtex: {
	
	HEADERS += PlayerWindow.h \
		VideoInputSenderManager.h \
		../livemix/VideoWidget.h \
		../livemix/EditorUtilityWidgets.h
				
	SOURCES += player-main.cpp \
		PlayerWindow.cpp \
		VideoInputSenderManager.cpp \
		../livemix/VideoWidget.cpp \
		../livemix/EditorUtilityWidgets.cpp 
		
	HEADERS += ../http/HttpServer.h ../http/SimpleTemplate.h
	SOURCES += ../http/HttpServer.cpp ../http/SimpleTemplate.cpp
	
	include(../3rdparty/qjson/qjson.pri)

	win32 {
		CONFIG += console
	}
}

shadowtest: {
	TARGET = shadowtest
	#HEADERS += ShadowTestWindow.h
	SOURCES += shadowtest-main.cpp
		#ShadowTestWindow.cpp
}

glvidtex: {
	TARGET = glvidtex
}

player : {
	TARGET = glplayer
}

# 'glstreamenc' compile target - encodes output of glplayer from frames received via shared memory
encoder: {
	TARGET = glstreamenc
	HEADERS += StreamEncoderProcess.h
	SOURCES += StreamEncoderProcess.cpp \
		streamenc-main.cpp
}

# 'glplayercmd' compile target - Player 'remote control' from the command line
playercmd: {
	TARGET = glplayercmd
	HEADERS += PlayerCommandLineInterface.h \
		PlayerConnection.h 
	SOURCES += PlayerCommandLineInterface.cpp \
		PlayerConnection.cpp \
		playercmd-main.cpp
}

# 'glinputbalance' compile target - use to balance two video inputs to each other
inputbalance: {
	TARGET = glinputbalance
	
	HEADERS += VideoInputColorBalancer.h \
		../livemix/VideoWidget.h \
		../livemix/EditorUtilityWidgets.h
		
		
	SOURCES += inputbalance-main.cpp \
		VideoInputColorBalancer.cpp \
		../livemix/VideoWidget.cpp \
		../livemix/EditorUtilityWidgets.cpp 
}

# 'glslideshow' compile target - used for my art show in Union City
slideshow: {
	TARGET = glslideshow
	
	HEADERS += SlideShowWindow.h
		
	SOURCES += slideshow-main.cpp \
		SlideShowWindow.cpp
}

unix: {
	HEADERS += \
		../livemix/SimpleV4L2.h \
		V4LOutput.h
		
	SOURCES += \
		../livemix/SimpleV4L2.cpp \
		V4LOutput.cpp
		

	# CentOS fix - see http://theitdepartment.wordpress.com/2009/03/15/centos-qt-fcfreetypequeryface/
	LIBS += -L/opt/fontconfig-2.4.2/lib
}

# OpenCV not required - but it nice to have.
# It's used to calculate deformation matrices in GLWidget for keystoning.
# Without OpenCV, the keystone deformations are downright ugly.
# Specify via: qmake CONFIG+=opencv 
opencv: {
	DEFINES += OPENCV_ENABLED
	LIBS += -L/usr/local/lib -lcv -lcxcore
			
	HEADERS += EyeCounter.h
	SOURCES += EyeCounter.cpp

}

# Blackmagic DeckLink Capture Support
blackmagic: {

	BMD_SDK_HOME = /opt/DeckLink-SDK-7.9.5/Linux
	
	#message("Blackmagic DeckLink API enabled, using: $$BMD_SDK_HOME")
	DEFINES += ENABLE_DECKLINK_CAPTURE
		
	INCLUDEPATH += $$BMD_SDK_HOME/include
	SOURCES     += $$BMD_SDK_HOME/include/DeckLinkAPIDispatch.cpp
}


#RESOURCES     = glvidtex.qrc
QT           += opengl multimedia network svg


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
unix {
	LIBS += -lavdevice -lavformat -lavcodec -lavutil -lswscale -lbz2 
	INCLUDEPATH += /usr/include/ffmpeg
}

win32 {
	INCLUDEPATH += \
		../external/ffmpeg/include/msinttypes \
		../external/ffmpeg/include/libswscale \
		../external/ffmpeg/include/libavutil \
		../external/ffmpeg/include/libavdevice \
		../external/ffmpeg/include/libavformat \
		../external/ffmpeg/include/libavcodec \
		../external/ffmpeg/include
	
	LIBS += -L"../external/ffmpeg/lib" \
		-lavcodec-51 \
		-lavformat-52 \
		-lavutil-49 \
		-lavdevice-52 \
		-lswscale-0
}


# install
target.path = /opt/glvidtex/
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS glvidtex.pro
sources.path = /opt/glvidtex/src
INSTALLS += target sources
