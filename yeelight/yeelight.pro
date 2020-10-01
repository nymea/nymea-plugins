include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationpluginyeelight)

SOURCES += \
    integrationpluginyeelight.cpp \
    yeelight.cpp \
    ssdp.cpp \

HEADERS += \
    integrationpluginyeelight.h \
    yeelight.h \
    ssdp.h \
