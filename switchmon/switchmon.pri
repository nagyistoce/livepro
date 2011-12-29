VPATH += $$PWD
DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD

QT += network

# Can't just include the gfxengine.pri because it uses OpenGL and Android devices cant use openGL yet.
#include(../gfxengine/gfxengine.pri)
DEFINES += NO_OPENGL

INCLUDEPATH += $$PWD/../gfxengine
DEPENDPATH += $$PWD/../gfxengine

QT += multimedia

# Used for the HTTP interface to the host
include(../3rdparty/qjson/qjson.pri)

# Input
HEADERS += \
	SwitchMonWidget.h \
	VideoWidget.h \
	VideoReceiver.h \
	VideoFrame.h \
	VideoSource.h
	
SOURCES += \
	SwitchMonWidget.cpp \
	VideoWidget.cpp \
	VideoReceiver.cpp \
	VideoFrame.cpp \
	VideoSource.cpp 
