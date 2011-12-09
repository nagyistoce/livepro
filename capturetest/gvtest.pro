TEMPLATE = app
TARGET = gvtest

DEPENDPATH += .
INCLUDEPATH += .

MOC_DIR = .build
OBJECTS_DIR = .build
RCC_DIR = .build
UI_DIR = .build

QT += opengl network multimedia

INCLUDEPATH += ../gfxengine

# Input
HEADERS += ../gfxengine/VideoFrame.h \
	GVTestWindow.h
	
	
SOURCES += ../gfxengine/VideoFrame.cpp \
	GVTestWindow.cpp

unix {
	HEADERS += \
		../gfxengine/SimpleV4L2.h
	SOURCES += \
		../gfxengine/SimpleV4L2.cpp

	#LIBS += -lavdevice -lavformat -lavcodec -lavutil -lswscale -lbz2 
}
