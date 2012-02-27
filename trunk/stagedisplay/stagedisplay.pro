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

# # Tell our support code below that we won't be using OpenGL to render
# #DEFINES += NO_OPENGL
# 
# # Common support code
# INCLUDEPATH += $$PWD/../gfxengine
# DEPENDPATH += $$PWD/../gfxengine
# HEADERS += \
# 	GLDrawable.h \
# 	GLVideoDrawable.h \
# 	GLSceneGroup.h \
# 	GLWidget.h \
# 	CornerItem.h \
# 	CameraThread.h \
# 	VideoWidget.h \
# 	VideoReceiver.h \
# 	VideoFrame.h \
# 	VideoSource.h
# 	
# SOURCES += \
# 	GLDrawable.cpp \
# 	GLVideoDrawable.cpp \
# 	GLWidget.cpp \
# 	GLSceneGroup.cpp \
# 	CornerItem.cpp \
# 	CameraThread.cpp \
# 	VideoWidget.cpp \
# 	VideoReceiver.cpp \
# 	VideoFrame.cpp \
# 	VideoSource.cpp
 
# Core of the graphics engine
include(../gfxengine/gfxengine.pri)

# Input
HEADERS += \
	OutputWindow.h \
	ControlWindow.h 
SOURCES += main.cpp \
	OutputWindow.cpp \
	ControlWindow.cpp
