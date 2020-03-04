include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_integrationpluginwakeonlan)

QT += network

SOURCES += \
    integrationpluginwakeonlan.cpp

HEADERS += \
    integrationpluginwakeonlan.h


