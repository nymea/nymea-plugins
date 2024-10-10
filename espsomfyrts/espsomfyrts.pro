include(../plugins.pri)

QT += network websockets

PKGCONFIG += nymea-mqtt

SOURCES += \
    espsomfyrts.cpp \
    espsomfyrtsdiscovery.cpp \
    integrationpluginespsomfyrts.cpp \

HEADERS += \
    espsomfyrts.h \
    espsomfyrtsdiscovery.h \
    integrationpluginespsomfyrts.h \
