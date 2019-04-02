include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginpushbullet)

QT+= network

SOURCES += \
    devicepluginpushbullet.cpp

HEADERS += \
    devicepluginpushbullet.h


