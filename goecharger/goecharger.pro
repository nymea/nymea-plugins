include(../plugins.pri)

QT += network

PKGCONFIG += nymea-mqtt

SOURCES += \
    goediscovery.cpp \
    integrationplugingoecharger.cpp \

HEADERS += \
    goestatus.h \
    goediscoveryresults.h \
    goediscovery.h \
    integrationplugingoecharger.h \
