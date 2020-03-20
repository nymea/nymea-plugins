include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationplugindynatrace)

SOURCES += \
    integrationplugindynatrace.cpp \
    ufo.cpp

HEADERS += \
    integrationplugindynatrace.h \
    ufo.h
