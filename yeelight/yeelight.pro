include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_devicepluginyeelight)

SOURCES += \
    devicepluginyeelight.cpp \
    yeelight.cpp \
    ssdp.cpp \

HEADERS += \
    devicepluginyeelight.h \
    yeelight.h \
    ssdp.h \
