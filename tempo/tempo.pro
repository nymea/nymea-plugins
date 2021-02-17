include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationplugintempo)

SOURCES += \
    integrationplugintempo.cpp \
    tempo.cpp

HEADERS += \
    integrationplugintempo.h \
    tempo.h

