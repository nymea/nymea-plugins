include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_integrationplugingoecharger)

QT += network

PKGCONFIG += nymea-mqtt

SOURCES += \
    integrationplugingoecharger.cpp \

HEADERS += \
    integrationplugingoecharger.h \
