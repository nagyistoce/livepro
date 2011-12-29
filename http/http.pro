MOC_DIR = .build
OBJECTS_DIR = .build
RCC_DIR = .build
UI_DIR = .build

TEMPLATE = app
TARGET = 
DEPENDPATH += . ../
INCLUDEPATH += . ../
	

QT += core \
	network 
	
# Input
HEADERS += HttpServer.h TestServer.h SimpleTemplate.h
SOURCES += HttpServer.cpp main.cpp TestServer.cpp SimpleTemplate.cpp
