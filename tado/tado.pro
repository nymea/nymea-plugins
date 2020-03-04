include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationplugintado)

SOURCES += \
    integrationplugintado.cpp \
    tado.cpp

HEADERS += \
    integrationplugintado.h \
    tado.h

