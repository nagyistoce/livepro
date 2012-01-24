MOC_DIR = .build
OBJECTS_DIR = .build
RCC_DIR = .build
UI_DIR = .build

TEMPLATE = app
TARGET = 

DEPENDPATH += . ../livemix ../autodir/
INCLUDEPATH += . ../livemix ../autodir/


# Core of the graphics engine
include(../gfxengine/gfxengine.pri)

# Generic QVariantMap-based server/client framework - used by the LiveDirector UI to control LiveMix
include(../varnet/varnet.pri)



# Input
HEADERS += \
	MainWindow.h \
	../livemix/ServerCommandNames.h \
	../autodir/PlayerConnection.h 
	
SOURCES += main.cpp \
	MainWindow.cpp \
	../autodir/PlayerConnection.cpp 