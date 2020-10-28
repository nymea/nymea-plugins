include(../plugins.pri)

QT += network

PKGCONFIG += nymea-mqtt

TARGET = $$qtLibraryTarget(nymea_integrationpluginmqttclient)

SOURCES += \
    integrationpluginmqttclient.cpp

HEADERS += \
    integrationpluginmqttclient.h


