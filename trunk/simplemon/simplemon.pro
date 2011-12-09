MOC_DIR = .build
OBJECTS_DIR = .build
RCC_DIR = .build
UI_DIR = .build

TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .

QT += network

# Input
HEADERS += \
	MainWindow.h
	
SOURCES += main.cpp \
	MainWindow.cpp
