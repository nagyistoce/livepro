MOC_DIR = .build
OBJECTS_DIR = .build
RCC_DIR = .build
UI_DIR = .build

TEMPLATE = app
TARGET = switchmon
DEPENDPATH += .
INCLUDEPATH += .

include(switchmon.pri)

SOURCES += main.cpp

