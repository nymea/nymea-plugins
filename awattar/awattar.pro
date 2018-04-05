include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_devicepluginawattar)

SOURCES += \
    devicepluginawattar.cpp \
    heatpump.cpp

HEADERS += \
    devicepluginawattar.h \
    heatpump.h


