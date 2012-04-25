MOC_DIR = .build
OBJECTS_DIR = .build
RCC_DIR = .build
UI_DIR = .build

TEMPLATE = app
TARGET = 
DEPENDPATH += . ../livemix
INCLUDEPATH += . ../livemix

# Core of the graphics engine
include(../gfxengine/gfxengine.pri)

QT += gui network

HEADERS += PresetPlayer.h 
SOURCES += PresetPlayer.cpp 

HEADERS += PanTiltClient.h
SOURCES += PanTiltClient.cpp

win32 {
    include(../../miditcp/client/miditcp.pri)
}
unix {
    include(/opt/miditcp/client/miditcp.pri)
}
