TEMPLATE = app
TARGET = livemix

DEPENDPATH += .
INCLUDEPATH += .

MOC_DIR = .build
OBJECTS_DIR = .build
RCC_DIR = .build
UI_DIR = .build

#include(../qtcolorpicker/qtcolorpicker.pri)

QT += opengl network multimedia

win32 {
        # CONFIG += console
    #debug {
    #}
}

FORMS += ControlWindow.ui

# Input
HEADERS += CameraThread.h \
	CameraServer.h \
	VideoWidget.h \
	VideoSource.h \
	VideoThread.h \
	VideoFrame.h \
	MjpegThread.h \
	MainWindow.h \
	MdiChild.h \
	MdiVideoChild.h \
	MdiVideoWidget.h \
	MdiCameraWidget.h \
	MdiMjpegWidget.h \
#	MdiPreviewWidget.h \
	DVizSharedMemoryThread.h \
	MdiDVizWidget.h \
	../glvidtex/GLWidget.h \
	../glvidtex/GLDrawable.h \
	../glvidtex/GLVideoDrawable.h \
	../glvidtex/StaticVideoSource.h \
	../glvidtex/TextVideoSource.h \
	../glvidtex/RichTextRenderer.h \
	../ImageFilters.h \
	LiveScene.h \
	LiveLayer.h \
	LiveVideoInputLayer.h \
	LiveStaticSourceLayer.h \
	LiveTextLayer.h \
	ExpandableWidget.h \
	LayerControlWidget.h \
	LiveVideoLayer.h \
	LiveVideoFileLayer.h \
	LiveSceneListModel.h \
	EditorUtilityWidgets.h \
	LiveVideoLoopLayer.h \
	ControlWindow.h
	
	
SOURCES += main.cpp \
	CameraThread.cpp \
	CameraServer.cpp \
	VideoWidget.cpp \
	VideoSource.cpp \
	VideoThread.cpp \
	VideoFrame.cpp \
	MjpegThread.cpp \
	MainWindow.cpp \
	MdiChild.cpp \
	MdiVideoChild.cpp \
	MdiVideoWidget.cpp \
	MdiCameraWidget.cpp \
	MdiMjpegWidget.cpp \
#	MdiPreviewWidget.cpp \
	DVizSharedMemoryThread.cpp \
	MdiDVizWidget.cpp \
	../glvidtex/GLWidget.cpp \
	../glvidtex/GLDrawable.cpp \
	../glvidtex/GLVideoDrawable.cpp \
	../glvidtex/StaticVideoSource.cpp \
	../glvidtex/TextVideoSource.cpp \
	../glvidtex/RichTextRenderer.cpp \
	../ImageFilters.cpp \
	LiveScene.cpp \
	LiveLayer.cpp \
	LiveVideoInputLayer.cpp \
	LiveStaticSourceLayer.cpp \
	LiveTextLayer.cpp \
	ExpandableWidget.cpp \
	LayerControlWidget.cpp \ 
	LiveVideoLayer.cpp \
	LiveVideoFileLayer.cpp \
	LiveSceneListModel.cpp \
	EditorUtilityWidgets.cpp \
	LiveVideoLoopLayer.cpp \
	ControlWindow.cpp

unix {
	HEADERS += \
		../livemix/SimpleV4L2.h
	SOURCES += \
		../livemix/SimpleV4L2.cpp

	LIBS += -lavdevice -lavformat -lavcodec -lavutil -lswscale -lbz2 
}

win32 {	
        include(../external/ffmpeg/ffmpeg.pri)
}

win32 {
    QT_MOBILITY_HOME = C:/Qt/qt-mobility-opensource-src-1.0.2
}
unix {
    #QT_MOBILITY_HOME = /opt/qt-mobility-opensource-src-1.0.1
    QT_MOBILITY_HOME = /opt/qt-mobility-opensource-src-1.1.0-beta2
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
                ../glvidtex/QtVideoSource.h \
                PlaylistModel.h

        SOURCES += \
                ../glvidtex/QtVideoSource.cpp \
                PlaylistModel.cpp

        message("QtMobility enabled. Before running, ensure \$QT_PLUGIN_PATH contains $$QT_MOBILITY_HOME/plugins, otherwise media will not play.")
}
else: {
        message("QtMobility not enabled (use qmake CONFIG+=mobility and ensure $$QT_MOBILITY_HOME exists), QtVideoSource will not be built.")
        DEFINES -= QT_MOBILITY_ENABLED \
                   HAS_QT_VIDEO_SOURCE
}
