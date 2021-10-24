include(../plugins.pri)

QT += network

PKGCONFIG += nymea-mqtt

TARGET = $$qtLibraryTarget(nymea_integrationpluginmqttclient)

SOURCES += \
    integrationplugingaradget.cpp

HEADERS += \
    integrationplugingaradget.h

