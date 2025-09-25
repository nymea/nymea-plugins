include(../plugins.pri)

QT += network

PKGCONFIG += nymea-mqtt

SOURCES += \
    integrationpluginmqttclient.cpp

HEADERS += \
    integrationpluginmqttclient.h


