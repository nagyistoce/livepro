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
	MouthyTest.h \
	FaceDetectFilter.h \
	EyeCounter.h  
	
	
SOURCES += \
	main.cpp \
	MouthyTest.cpp \
	FaceDetectFilter.cpp \
	EyeCounter.cpp
