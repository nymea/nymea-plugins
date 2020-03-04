include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_integrationpluginpushbullet)

QT+= network

SOURCES += \
    integrationpluginpushbullet.cpp

HEADERS += \
    integrationpluginpushbullet.h


