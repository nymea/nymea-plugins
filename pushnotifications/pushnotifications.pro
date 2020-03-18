include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_integrationpluginpushnotifications)

QT+= network

SOURCES += \
    integrationpluginpushnotifications.cpp

HEADERS += \
    integrationpluginpushnotifications.h


