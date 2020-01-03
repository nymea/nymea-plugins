include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginlifx)

QT += network

SOURCES += \
    devicepluginlifx.cpp \
    lifx.cpp \

HEADERS += \
    devicepluginlifx.h \
    lifx.h \

