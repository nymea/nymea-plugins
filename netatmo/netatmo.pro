include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationpluginnetatmo)

SOURCES += \
    integrationpluginnetatmo.cpp \
    netatmobasestation.cpp \
    netatmooutdoormodule.cpp

HEADERS += \
    integrationpluginnetatmo.h \
    netatmobasestation.h \
    netatmooutdoormodule.h


