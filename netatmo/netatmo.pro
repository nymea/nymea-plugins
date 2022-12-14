include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationpluginnetatmo)

SOURCES += \
    integrationpluginnetatmo.cpp \
    netatmoconnection.cpp

HEADERS += \
    integrationpluginnetatmo.h \
    netatmoconnection.h


