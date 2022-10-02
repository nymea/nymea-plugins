include(../plugins.pri)

QT += network \
      websockets

PKGCONFIG += nymea-mqtt

SOURCES += \
    integrationpluginespuino.cpp

HEADERS += \
    integrationpluginespuino.h
