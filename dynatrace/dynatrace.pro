include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_deviceplugindynatrace)

SOURCES += \
    deviceplugindynatrace.cpp \
    ufo.cpp

HEADERS += \
    deviceplugindynatrace.h \
    ufo.h
