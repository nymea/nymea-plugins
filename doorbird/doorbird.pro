include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_deviceplugindoorbird)

SOURCES += \
    deviceplugindoorbird.cpp \
    doorbird.cpp \

HEADERS += \
    deviceplugindoorbird.h \
    doorbird.h \
