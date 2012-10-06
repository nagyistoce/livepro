TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .

QT += network

#DEFINES += NO_OPENGL

include(../../gfxengine/gfxengine.pri)

#DEFINES += NO_OPENGL

MOC_DIR = ../.build
OBJECTS_DIR = ../.build
RCC_DIR = ../.build
UI_DIR = ../.build


# Input
HEADERS += \
	MainWindow.h
	
SOURCES += main.cpp \
	MainWindow.cpp
