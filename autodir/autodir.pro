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

# Generic QVariantMap-based server/client framework - used by the LiveDirector UI to control LiveMix
include(../varnet/varnet.pri)

# JSON parser/encoder for control via a HTTP/JSON interface 
include(../3rdparty/qjson/qjson.pri)

# Generic HTTP server framework code for use in the HTTP/JSON interface
include(../http/http.pri)


# Input
HEADERS += \
	MainWindow.h \
	../livemix/ServerCommandNames.h \
	PlayerConnection.h \
	AnalysisFilter.h
	
SOURCES += main.cpp \
	MainWindow.cpp \
	PlayerConnection.cpp \
	AnalysisFilter.cpp
