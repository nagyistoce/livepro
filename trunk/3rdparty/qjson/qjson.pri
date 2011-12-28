
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

# Input
HEADERS += json_parser.hh \
           json_scanner.h \
           location.hh \
           parser.h \
           parser_p.h \
           parserrunnable.h \
           position.hh \
           qjson_debug.h \
           qjson_export.h \
           serializer.h \
           serializerrunnable.h \
           stack.hh
SOURCES += json_parser.cc \
           json_scanner.cpp \
           parser.cpp \
           parserrunnable.cpp \
           serializer.cpp \
           serializerrunnable.cpp
