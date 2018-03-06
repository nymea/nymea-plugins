include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginwakeonlan)

QT += network

SOURCES += \
    devicepluginwakeonlan.cpp

HEADERS += \
    devicepluginwakeonlan.h


