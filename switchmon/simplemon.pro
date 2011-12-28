MOC_DIR = .build
OBJECTS_DIR = .build
RCC_DIR = .build
UI_DIR = .build

TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .


QT += network

# Can't just include the gfxengine.pri because it uses OpenGL and Android devices cant use openGL yet.
#include(../gfxengine/gfxengine.pri)

# Used for the HTTP interface to the host
include(../3rdparty/qjson/qjson.pri)

QT += multimedia
DEFINES += NO_OPENGL

INCLUDEPATH += ../gfxengine
DEPENDPATH += ../gfxengine

# Input
HEADERS += \
	MainWindow.h \
	VideoWidget.h \
	VideoReceiver.h \
	VideoFrame.h \
	VideoSource.h
	
SOURCES += main.cpp \
	MainWindow.cpp \
	VideoWidget.cpp \
	VideoReceiver.cpp \
	VideoFrame.cpp \
	VideoSource.cpp 
