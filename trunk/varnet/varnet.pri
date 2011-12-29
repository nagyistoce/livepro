
# A simple, generic TCP server/client that passes around messages made up of QVariantMaps

VPATH += $$PWD
DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD

QT += network 
	
HEADERS += \
	VariantMapServer.h \
	VariantMapClient.h

SOURCES += \
	VariantMapServer.cpp \
	VariantMapClient.cpp