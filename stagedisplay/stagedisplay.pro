# Qt modules required
QT += gui network opengl multimedia

# Target setup and depends
TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .

# Build output location
MOC_DIR = .build
OBJECTS_DIR = .build
RCC_DIR = .build
UI_DIR = .build

 
# Core of the graphics engine
include(../gfxengine/gfxengine.pri)

FORMS += ControlWindow.ui

# Input
HEADERS += \
	OutputWindow.h \
	ControlWindow.h 
SOURCES += main.cpp \
	OutputWindow.cpp \
	ControlWindow.cpp
