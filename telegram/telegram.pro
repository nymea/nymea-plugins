include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_integrationplugintelegram)

QT+= network

SOURCES += \
    integrationplugintelegram.cpp

HEADERS += \
    integrationplugintelegram.h


