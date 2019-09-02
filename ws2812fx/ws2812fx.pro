include(../plugins.pri)

QT += serialport

TARGET = $$qtLibraryTarget(nymea_devicepluginws2812fx)

SOURCES += \
    devicepluginws2812fx.cpp \


HEADERS += \
    devicepluginws2812fx.h \
