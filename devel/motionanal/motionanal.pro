TEMPLATE = app
TARGET = mouthytest
DEPENDPATH += .
INCLUDEPATH += .

include(../../gfxengine/gfxengine.pri)

MOC_DIR = ../.build
OBJECTS_DIR = ../.build
RCC_DIR = ../.build
UI_DIR = ../.build


INCLUDEPATH += $$PWD/../../gfxengine
DEPENDPATH += $$PWD/../../gfxengine

INCLUDEPATH += /opt/OpenCV-2.1.0/src/cv/ /opt/OpenCV-2.1.0/include/opencv

# Input
HEADERS += \
	MotionAnal.h \
	TrackingFilter.h
	
SOURCES += \
	main.cpp \
	MotionAnal.cpp \
	TrackingFilter.cpp 
	