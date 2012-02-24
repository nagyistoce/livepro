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

# Tell our support code below that we won't be using OpenGL to render
#DEFINES += NO_OPENGL

# Common support code
INCLUDEPATH += $$PWD/../gfxengine
DEPENDPATH += $$PWD/../gfxengine
HEADERS += \
	GLDrawable.h \
	GLVideoDrawable.h \
	VideoWidget.h \
	VideoReceiver.h \
	VideoFrame.h \
	VideoSource.h
	
SOURCES += \
	GLDrawable.cpp \
	GLVideoDrawable.cpp \
	VideoWidget.cpp \
	VideoReceiver.cpp \
	VideoFrame.cpp \
	VideoSource.cpp 

# Input
HEADERS += \
	OutputWindow.h \
	ControlWindow.h 
SOURCES += main.cpp \
	OutputWindow.cpp \
	ControlWindow.cpp
