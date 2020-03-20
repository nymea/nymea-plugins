include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationplugindoorbird)

SOURCES += \
    integrationplugindoorbird.cpp \
    doorbird.cpp \

HEADERS += \
    integrationplugindoorbird.h \
    doorbird.h \
