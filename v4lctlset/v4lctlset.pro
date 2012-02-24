TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .

QT += network

include(../gfxengine/gfxengine.pri)

MOC_DIR = ../.build
OBJECTS_DIR = ../.build
RCC_DIR = ../.build
UI_DIR = ../.build


# HEADERS += ../../gfxengine/VideoFrame.h
# SOURCES += ../../gfxengine/VideoFrame.cpp 
# 
# HEADERS += ../../gfxengine/SimpleV4L2.h
# SOURCES += ../../gfxengine/SimpleV4L2.cpp
	
SOURCES += main.cpp
