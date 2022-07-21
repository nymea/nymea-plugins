include(../plugins.pri)

QT += network

PKGCONFIG += nymea-mqtt

SOURCES += \
    goediscovery.cpp \
    integrationplugingoecharger.cpp \

HEADERS += \
    goediscovery.h \
    integrationplugingoecharger.h \
