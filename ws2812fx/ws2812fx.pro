include(../plugins.pri)

QT += serialport

TARGET = $$qtLibraryTarget(nymea_integrationpluginws2812fx)

SOURCES += \
    integrationpluginws2812fx.cpp \
    nymealight.cpp \
    nymealightserialinterface.cpp


HEADERS += \
    integrationpluginws2812fx.h \
    nymealight.h \
    nymealightinterface.h \
    nymealightserialinterface.h
