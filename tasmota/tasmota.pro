include(../plugins.pri)

QT += network

PKGCONFIG += nymea-mqtt

SOURCES += \
    integrationplugintasmota.cpp

HEADERS += \
    integrationplugintasmota.h
