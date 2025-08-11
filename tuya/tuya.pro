include(../plugins.pri)

QT *= network

PKGCONFIG += nymea-mqtt

SOURCES += \
    integrationplugintuya.cpp

HEADERS += \
    integrationplugintuya.h
