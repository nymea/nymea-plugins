include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_devicepluginmqtt)

SOURCES += \
    devicepluginmqtt.cpp

HEADERS += \
    devicepluginmqtt.h


