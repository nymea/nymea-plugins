include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_devicepluginmqttclient)

SOURCES += \
    devicepluginmqttclient.cpp

HEADERS += \
    devicepluginmqttclient.h


