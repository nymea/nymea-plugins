include(../plugins.pri)

QT += network websockets

PKGCONFIG += nymea-mqtt

SOURCES += \
    integrationpluginshelly.cpp \
    shellyjsonrpcclient.cpp

HEADERS += \
    integrationpluginshelly.h \
    shellyjsonrpcclient.h
