VPATH += $$PWD
DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD

QT += network 
	
HEADERS += \
	HttpServer.h \
	SimpleTemplate.h

SOURCES += \
	HttpServer.cpp \
	SimpleTemplate.cpp