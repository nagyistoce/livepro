MOC_DIR = .build
OBJECTS_DIR = .build
RCC_DIR = .build
UI_DIR = .build

TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .

# Core of the graphics engine
include(../gfxengine/gfxengine.pri)

# JSON parser/encoder for control via a HTTP/JSON interface 
include(../3rdparty/qjson/qjson.pri)

# Generic HTTP server framework code for use in the HTTP/JSON interface
include(../http/http.pri)

# Generic QVariantMap-based server/client framework - used by the LiveDirector UI to control LiveMix
include(../varnet/varnet.pri)

# Parse command line options
include(../3rdparty/qtgetopt/getopt.pri)

# Input
HEADERS += \
	PlayerWindow.h

SOURCES += \
	main.cpp \
	PlayerWindow.cpp 

win32 {
        #CONFIG += console
        QT += multimediakit
}
