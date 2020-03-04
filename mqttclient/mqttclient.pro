include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationpluginmqttclient)

SOURCES += \
    integrationpluginmqttclient.cpp

HEADERS += \
    integrationpluginmqttclient.h


