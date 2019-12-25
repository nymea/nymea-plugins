include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_devicepluginlifx)

SOURCES += \
    devicepluginlifx.cpp \
    lifx.cpp \

HEADERS += \
    devicepluginlifx.h \
    lifx.h \

