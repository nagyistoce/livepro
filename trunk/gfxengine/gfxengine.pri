MOC_DIR = .build
OBJECTS_DIR = .build
RCC_DIR = .build
UI_DIR = .build

VPATH += $$PWD
DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD

# Input
HEADERS += BMDOutput.h \
           CameraThread.h \
           GLCommonShaders.h \
           HistogramFilter.h \
           MjpegThread.h \
           SimpleV4L2.h \
           VideoConsumer.h \
           VideoDifferenceFilter.h \
           VideoEncoder.h \
           VideoFilter.h \
           VideoFrame.h \
           VideoInputColorBalancer.h \
           VideoInputSenderManager.h \
           VideoReceiver.h \
           VideoSender.h \
           VideoSenderCommands.h \
           VideoSource.h

SOURCES += BMDOutput.cpp \
           CameraThread.cpp \
           HistogramFilter.cpp \
           MjpegThread.cpp \
           SimpleV4L2.cpp \
           VideoConsumer.cpp \
           VideoDifferenceFilter.cpp \
           VideoEncoder.cpp \
           VideoFilter.cpp \
           VideoFrame.cpp \
           VideoInputColorBalancer.cpp \
           VideoInputSenderManager.cpp \
           VideoReceiver.cpp \
           VideoSender.cpp \
           VideoSource.cpp
