include(../plugins.pri)

QT += network websockets

SOURCES += \
    espsomfyrts.cpp \
    espsomfyrtsdiscovery.cpp \
    integrationpluginespsomfyrts.cpp \

HEADERS += \
    espsomfyrts.h \
    espsomfyrtsdiscovery.h \
    integrationpluginespsomfyrts.h \
