TEMPLATE = app
TARGET = gvtest

DEPENDPATH += .
INCLUDEPATH += .

MOC_DIR = .build
OBJECTS_DIR = .build
RCC_DIR = .build
UI_DIR = .build

QT += opengl network multimedia


# Input
HEADERS += VideoFrame.h \
	GVTestWindow.h
	
	
SOURCES += VideoFrame.cpp \
	GVTestWindow.cpp

unix {
	HEADERS += \
		../livemix/SimpleV4L2.h
	SOURCES += \
		../livemix/SimpleV4L2.cpp

	#LIBS += -lavdevice -lavformat -lavcodec -lavutil -lswscale -lbz2 
}
