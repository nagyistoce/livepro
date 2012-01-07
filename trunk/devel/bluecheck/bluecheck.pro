TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .

QT += network

include(../../gfxengine/gfxengine.pri)

MOC_DIR = ../.build
OBJECTS_DIR = ../.build
RCC_DIR = ../.build
UI_DIR = ../.build


# Input
HEADERS += \
	MainWindow.h
	
SOURCES += main.cpp \
	MainWindow.cpp
