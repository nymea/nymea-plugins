include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_devicepluginatag)

SOURCES += \
    devicepluginatag.cpp \
    atag.cpp \

HEADERS += \
    devicepluginatag.h \
    atag.h \
