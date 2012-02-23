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

# Client for the 'miditcp' server
include(/opt/miditcp/client/miditcp.pri)

# Qt Color Picker widget
include(../3rdparty/qtcolorpicker/qtcolorpicker.pri)

# Input

# Include dviz resources to get the rich text editor icons
RESOURCES += ../data/icons.qrc

FORMS += \
	DirectorWindow.ui  \
	DrawableDirectorWidget.ui \
	PlayerSetupDialog.ui \
	ScenePropertiesDialog.ui

HEADERS += \
	DirectorMidiInputAdapter.h  \
	DirectorWindow.h  \
	DrawableDirectorWidget.h  \
	EditorGraphicsView.h  \
	EditorWindow.h \
	../autodir/PlayerConnection.h \
	PlayerSetupDialog.h \
	ScenePropertiesDialog.h \
	KeystonePointsEditor.h
	
SOURCES += \
	main.cpp \
	DirectorMidiInputAdapter.cpp  \
	DirectorWindow.cpp  \
	DrawableDirectorWidget.cpp  \
	EditorGraphicsView.cpp  \
	EditorWindow.cpp \
	../autodir/PlayerConnection.cpp \
	PlayerSetupDialog.cpp \
	ScenePropertiesDialog.cpp \
	KeystonePointsEditor.cpp

#win32 {
#        CONFIG += console
#}
