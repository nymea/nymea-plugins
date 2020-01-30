include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_deviceplugintado)

SOURCES += \
    deviceplugintado.cpp \
    tado.cpp

HEADERS += \
    deviceplugintado.h \
    tado.h

