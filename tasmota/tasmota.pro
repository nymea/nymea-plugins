include(../plugins.pri)

QT += network

PKGCONFIG += nymea-mqtt

TARGET = $$qtLibraryTarget(nymea_integrationplugintasmota)

SOURCES += \
    integrationplugintasmota.cpp \

HEADERS += \
    integrationplugintasmota.h \
